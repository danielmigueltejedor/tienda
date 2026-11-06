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

NXPM se desarrolla en el repositorio de Nodalix OS (`nxpm/`). Para probar la tienda en un árbol de desarrollo hace falta el servicio instalado (`nodalix-nxpm`) o arrancado desde ese repositorio con `PYTHONPATH=nxpm/src python -m nxpm daemon`.
