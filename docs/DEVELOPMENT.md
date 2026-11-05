# Desarrollo

## Tienda

Hace falta el entorno de compilación de Bazaar: GTK 4.22 o posterior, Libadwaita 1.8 o posterior, libflatpak, AppStream, libdex, libsoup, glycin, WebKitGTK 6, libsecret, libproxy, malcontent, gtksourceview 5 y `blueprint-compiler` 0.20 o posterior.

En este equipo `blueprint-compiler` 0.22 se instaló en el prefijo del usuario porque no estaba en el sistema.

```bash
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

La prueba de AppStream del metainfo puede exigir red o datos que no están en un árbol de desarrollo. El objetivo de esta entrega es que los binarios `nodalix-store`, `nodalix-store-daemon` y `nodalix-store-dl-worker` se enlacen.

Para ejecutar la tienda sin instalarla:

```bash
GSETTINGS_SCHEMA_DIR=build/data ./build/src/nodalix-store
```

El servicio de usuario que Bazaar espera sigue siendo el demonio de la tienda. NXPM es otro servicio.

## NXPM

```bash
cd nxpm
python -m pytest
PYTHONPATH=src python -m nxpm sources
```

Las pruebas no instalan ni borran paquetes. El backend de Pacman se prueba con un handle falso. Flatpak se prueba con una referencia falsa. Las consultas reales de solo lectura pueden ejecutarse con la CLI.

PyGObject, Flatpak, pyalpm y pycman son dependencias del sistema, no de pip.

## Sesión

```bash
PYTHONPATH=nxpm/src python -m nxpm install-session
systemctl --user enable --now nxpm-check.timer
PYTHONPATH=nxpm/src python -m nxpm daemon
```

La política de Polkit está en `nxpm/share/os.nodalix.nxpm.policy`. Instalarla en `/usr/share/polkit-1/actions` requiere administración y no se hace desde el árbol de desarrollo.
