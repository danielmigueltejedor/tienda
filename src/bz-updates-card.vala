/* bz-updates-card.vala
 *
 * Copyright 2026 Alexander Vanhee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

[GtkTemplate (ui = "/os/nodalix/Store/bz-updates-card.ui")]
public class Bz.UpdatesCard : Adw.Bin {
    private Bz.StateInfo? _state;
    public Bz.StateInfo? state {
        get { return _state; }
        set {
            _state = value;
            if (_state != null)
                _state.notify["available-updates"].connect ((s, p) => repopulate ());

            repopulate ();
        }
    }

    [GtkChild]
    private unowned Adw.ExpanderRow expander_row;

    private GenericArray<Gtk.Widget> app_rows = new GenericArray<Gtk.Widget> ();
    private Gtk.Widget? runtimes_row = null;
    private GenericArray<Bz.Entry> runtime_entries = new GenericArray<Bz.Entry> ();
    private bool has_closed = false;

    public signal void update (ListModel entries);

    [GtkCallback]
    private string format_update_count (ListModel? updates) {
        if (updates == null)
            return "";

        uint n_updates = updates.get_n_items ();
        return ngettext ("%u Available Update", "%u Available Updates", n_updates).printf (n_updates);
    }

    [GtkCallback]
    private void update_all_cb (Gtk.Button button) {
        var updates = _state?.available_updates;
        if (updates == null || updates.get_n_items () == 0)
            return;

        var entries = new GenericArray<Bz.Entry> ();
        uint n_items = updates.get_n_items ();
        for (uint i = 0; i < n_items; i++) {
            var entry = (updates.get_item (i) as Bz.UpdatePermissionInfo)?.entry;
            if (entry != null)
                entries.add (entry);
        }
        emit_update (entries);
    }

    [GtkCallback]
    private void on_expander_expanded_cb (Object row, ParamSpec pspec) {
        if (!expander_row.expanded)
            has_closed = true;
    }

    private void emit_update (GenericArray<Bz.Entry> entries) {
        var store = new ListStore (typeof (Bz.Entry));
        foreach (var entry in entries)
            store.append (entry);
        update (store);
    }

    private void repopulate () {
        foreach (var row in app_rows)
            expander_row.remove (row);
        if (runtimes_row != null)
            expander_row.remove (runtimes_row);

        app_rows = new GenericArray<Gtk.Widget> ();
        runtime_entries = new GenericArray<Bz.Entry> ();
        runtimes_row = null;

        var updates = _state?.available_updates;
        if (updates == null)
            return;

        uint n_items = updates.get_n_items ();

        for (uint i = 0; i < n_items; i++) {
            var info = updates.get_item (i) as Bz.UpdatePermissionInfo;
            var entry = info?.entry;
            if (entry == null)
                continue;

            if (entry.is_of_kinds (Bz.EntryKind.APPLICATION)) {
                var row = build_app_row (info);
                expander_row.add_row (row);
                app_rows.add (row);
            } else if (entry.is_of_kinds (Bz.EntryKind.RUNTIME) ||
                       entry.is_of_kinds (Bz.EntryKind.ADDON)) {
                runtime_entries.add (entry);
            }
        }

        runtimes_row = build_runtimes_row ();
        expander_row.add_row (runtimes_row);

        if (!has_closed)
            expander_row.expanded = n_items <= 3;
    }

    private Gtk.Widget build_app_row (Bz.UpdatePermissionInfo info) {
        var entry = info.entry;
        var delta = info.additional_permissions;

        var row = new Adw.ActionRow () {
            title = entry.title,
        };

        var icon = new Gtk.Image () {
            pixel_size = 48,
            valign = Gtk.Align.CENTER,
            margin_top = 6,
            margin_bottom = 6,
        };
        if (entry.icon_paintable != null)
            icon.paintable = entry.icon_paintable;
        else
            icon.icon_name = "application-x-executable";
        icon.add_css_class ("icon-dropshadow");
        row.add_prefix (icon);

        ListModel? history = entry.version_history;
        string? installed_ver = entry.installed_version;
        bool has_history = history != null && history.get_n_items () > 0;

        if (has_history && installed_ver != null) {
            string? new_ver = ((Bz.Release) history.get_item (0)).version;

            if (new_ver != null)
                row.subtitle = installed_ver != new_ver
                    ? "%s → %s".printf (installed_ver, new_ver)
                    : new_ver;
        }

        if (delta != null && !delta.is_empty ()) {
            var permissions_icon = new Gtk.Image.from_icon_name ("permissions-warning-symbolic") {
                valign = Gtk.Align.CENTER,
                tooltip_text = _("This update requests new permissions"),
            };
            permissions_icon.add_css_class ("warning");
            row.add_suffix (permissions_icon);
        }

        var history_button = new Gtk.Button.from_icon_name ("view-list-bullet-symbolic") {
            valign = Gtk.Align.CENTER,
            tooltip_text = _("Version History"),
            visible = has_history,
        };
        history_button.add_css_class ("flat");
        history_button.clicked.connect (() => {
            var dialog = Bz.ReleasesList.dialog_new (entry.version_history, null);
            ((Adw.Dialog) dialog).present (this.get_root ());
        });

        row.add_suffix (history_button);

        var update_button = new Gtk.Button.with_label (_("Update")) {
            valign = Gtk.Align.CENTER,
        };
        update_button.clicked.connect (() => {
            var entries = new GenericArray<Bz.Entry> ();
            entries.add (info.entry);
            emit_update (entries);
        });
        row.add_suffix (update_button);

        return row;
    }

    private Gtk.Widget build_runtimes_row () {
        uint n_items = runtime_entries.length;

        var row = new Adw.ActionRow () {
            visible = n_items > 0,
        };

        if (n_items > 0)
            row.title = ngettext ("%u Runtime Update", "%u Runtime Updates", n_items).printf (n_items);

        var update_button = new Gtk.Button.with_label (_("Update")) {
            valign = Gtk.Align.CENTER,
        };
        update_button.clicked.connect (() => {
            if (runtime_entries.length == 0)
                return;
            emit_update (runtime_entries);
        });
        row.add_suffix (update_button);

        return row;
    }
}
