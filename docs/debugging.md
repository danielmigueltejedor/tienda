# Debugging Nodalix Store

## Running Nodalix Store with Debug Messages Enabled

You do this the same as with any other GLib application:

```sh
pkill nodalix-store; env G_MESSAGES_DEBUG=all nodalix-store
```

This will produce a flood of output. If you know the specific log domain you
want to see, you can replace `all` with that domain name, such as `flatpak` or
`NODALIX-STORE::CORE`.

## Nodalix Store in GtkInspector

Nodalix Store has its own gtk inspector page, which you can spawn with the key combo
`control+shift+i`. This is meant to be useful for developers and distributors.

The global debug mode toggle in this page enables some extra widgets in the UI
that display useful debugging information and some shortcut buttons to, for
example, easily inspect the UI entry for the visible application or copy an
appid.

You can also disable blocklists from this debug page. This is a temporary
internal property, not a configuration option; It will not be saved for the next
Nodalix Store process.

The rest of the contents of this page should be self-explanatory.

## Debugging Crashes

### Flatpak

#### Installing the Debug Symbols

This is mostly based on [flatpak
docs](https://docs.flatpak.org/en/latest/debugging.html).

Installing debug extensions so the stacktrace is actually useful for developers:

```sh
flatpak install --include-debug --include-sdk os.nodalix.Store
```

You can remove all the gnome sdk and debug extensions again when you are
finished with debugging. This is quite a big download, please have patience.

#### Actually Start Debugging

Nodalix Store starts a background service once started, make sure it is not running first:

```sh
flatpak kill os.nodalix.Store
```

```sh
flatpak run --devel --command=bash os.nodalix.Store
```

This will get you a shell inside the flatpak sandbox: run this: `gdb
/app/bin/nodalix-store-daemon`

```sh
[📦 os.nodalix.Store ~]$ gdb /app/bin/nodalix-store-daemon
```

Actually run nodalix-store:
```sh
$ (gdb) run
```

(reproduce the bug here so it crashes)

after it crashed, retrieve the useful information:

```sh
$ (gdb) thread apply all backtrace
```
