"""Flatpak backend using libflatpak through GObject introspection."""

from __future__ import annotations

import logging

from nxpm.errors import NxpmError
from nxpm.model import Package, parse_package_id

log = logging.getLogger("nxpm.flatpak")
_RESULT_LIMIT = 80


def _gi_flatpak():
    import gi

    gi.require_version("Flatpak", "1.0")
    from gi.repository import Flatpak

    return Flatpak


def _title(ref) -> str:
    for getter in ("get_appdata_name",):
        method = getattr(ref, getter, None)
        if method is None:
            continue
        try:
            value = method()
        except Exception:
            value = None
        if value:
            return str(value)
    return str(ref.get_name())


def _summary(ref) -> str:
    method = getattr(ref, "get_appdata_summary", None)
    if method is None:
        return ""
    try:
        value = method()
    except Exception:
        return ""
    return str(value or "")


def _version(ref) -> str:
    method = getattr(ref, "get_appdata_version", None)
    if method is not None:
        try:
            value = method()
        except Exception:
            value = None
        if value:
            return str(value)
    commit = getattr(ref, "get_commit", None)
    if commit is not None:
        try:
            value = commit()
        except Exception:
            value = None
        if value:
            return str(value)[:12]
    branch = getattr(ref, "get_branch", None)
    if branch is not None:
        try:
            return str(branch() or "")
        except Exception:
            return ""
    return ""


def package_from_ref(scope: str, ref, *, installed: bool, update: bool = False) -> Package:
    formatted = str(ref.format_ref())
    name = str(ref.get_name())
    kind = "runtime" if formatted.startswith("runtime/") else "app"
    return Package(
        id=f"flatpak:{scope}:{formatted}",
        name=_title(ref),
        summary=_summary(ref),
        version=_version(ref),
        source="flatpak",
        kind=kind,
        installed=installed,
        update=update,
        appstream_id=name,
        icon_name=name,
        risk="low",
    )


class FlatpakBackend:
    source = "flatpak"

    def __init__(self, installations: dict[str, object] | None = None) -> None:
        self._installations = installations

    def installations(self) -> dict[str, object]:
        if self._installations is not None:
            return self._installations
        Flatpak = _gi_flatpak()
        found = {}
        try:
            found["user"] = Flatpak.Installation.new_user()
        except Exception as exc:
            log.warning("User Flatpak installation is unavailable: %s", exc)
        try:
            found["system"] = Flatpak.Installation.new_system()
        except Exception as exc:
            log.warning("System Flatpak installation is unavailable: %s", exc)
        self._installations = found
        return found

    def search(self, query: str) -> list[Package]:
        needle = query.casefold()
        matches: list[Package] = []
        seen: set[str] = set()
        for scope, installation in self.installations().items():
            for ref in self._installed_refs(installation):
                package = package_from_ref(scope, ref, installed=True)
                if needle in package.name.casefold() or needle in package.appstream_id.casefold():
                    matches.append(package)
                    seen.add(package.id)
            for ref, remote in self._remote_refs(installation):
                ref_name = str(ref.get_name())
                if needle not in ref_name.casefold() and needle not in remote.casefold():
                    continue
                package = package_from_ref(scope, ref, installed=False)
                package.summary = package.summary or remote
                if package.id in seen:
                    continue
                matches.append(package)
                seen.add(package.id)
                if len(matches) >= _RESULT_LIMIT:
                    return matches
        return matches

    def list_installed(self) -> list[Package]:
        packages = []
        for scope, installation in self.installations().items():
            for ref in self._installed_refs(installation):
                packages.append(package_from_ref(scope, ref, installed=True))
        return packages

    def list_updates(self) -> list[Package]:
        packages = []
        for scope, installation in self.installations().items():
            method = getattr(installation, "list_installed_refs_for_update", None)
            if method is None:
                continue
            try:
                refs = method() or []
            except Exception as exc:
                log.warning("Could not check Flatpak updates for %s: %s", scope, exc)
                continue
            for ref in refs:
                packages.append(package_from_ref(scope, ref, installed=True, update=True))
        return packages

    def details(self, package_id: str) -> Package:
        source, payload = parse_package_id(package_id)
        if source != "flatpak":
            raise NxpmError("invalid-input", "That identifier is not a Flatpak package.")
        scope, ref_text = payload.split(":", 1)
        installation = self.installations().get(scope)
        if installation is None:
            raise NxpmError("not-found", f"The {scope} Flatpak installation is not available.")
        for ref in self._installed_refs(installation):
            if str(ref.format_ref()) == ref_text:
                return package_from_ref(scope, ref, installed=True)
        for ref, _remote in self._remote_refs(installation):
            if str(ref.format_ref()) == ref_text:
                return package_from_ref(scope, ref, installed=False)
        raise NxpmError("not-found", "Flatpak does not know that application.")

    def apply(self, package_id: str, operation: str, progress) -> None:
        source, payload = parse_package_id(package_id)
        if source != "flatpak":
            raise NxpmError("invalid-input", "That identifier is not a Flatpak package.")
        scope, ref_text = payload.split(":", 1)
        installation = self.installations().get(scope)
        if installation is None:
            raise NxpmError("not-found", f"The {scope} Flatpak installation is not available.")
        if scope == "system":
            raise NxpmError(
                "authorization-required",
                "System-wide Flatpak changes are not applied by the user service.",
            )
        Flatpak = _gi_flatpak()
        transaction = Flatpak.Transaction.new_for_installation(installation)
        transaction.set_no_interaction(True)
        if operation == "remove":
            transaction.add_uninstall(ref_text)
        elif operation == "update":
            transaction.add_update(ref_text, None, None)
        else:
            remote = self._remote_for_ref(installation, ref_text)
            transaction.add_install(remote, ref_text, None)
        self._connect_progress(transaction, progress)
        progress(0.05, "Contacting Flatpak")
        transaction.run()

    def _installed_refs(self, installation):
        try:
            return list(installation.list_installed_refs() or [])
        except Exception as exc:
            log.warning("Could not list installed Flatpak refs: %s", exc)
            return []

    def _remote_refs(self, installation):
        results = []
        try:
            remotes = installation.list_remotes() or []
        except Exception as exc:
            log.warning("Could not list Flatpak remotes: %s", exc)
            return results
        for remote in remotes:
            name = str(remote.get_name())
            try:
                refs = installation.list_remote_refs_sync(name, None) or []
            except Exception as exc:
                log.info("Skipping Flatpak remote %s: %s", name, exc)
                continue
            for ref in refs:
                formatted = str(ref.format_ref())
                if formatted.startswith("app/"):
                    results.append((ref, name))
        return results

    def _remote_for_ref(self, installation, ref_text: str) -> str:
        for ref, remote in self._remote_refs(installation):
            if str(ref.format_ref()) == ref_text:
                return remote
        raise NxpmError("not-found", "No configured Flatpak remote contains that application.")

    def _connect_progress(self, transaction, progress) -> None:
        def changed(progress_object):
            try:
                value = float(progress_object.get_progress()) / 100.0
            except Exception:
                value = -1
            progress(value, "Flatpak is working")

        def on_new(_transaction, _operation, progress_object):
            if progress_object is not None:
                progress_object.connect("changed", changed)

        if hasattr(transaction, "connect"):
            transaction.connect("new-operation", on_new)
