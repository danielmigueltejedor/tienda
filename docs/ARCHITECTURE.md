# Arquitectura

## Nodalix Store

La tienda es una adaptación de Bazaar. Conserva GTK 4, Libadwaita y el navegador de Flatpak: curación, exploración de Flathub, biblioteca, búsqueda, instalación, permisos y transacciones que ya implementaba Bazaar.

Identidad:

| Elemento | Valor |
| --- | --- |
| Nombre visible | Nodalix Store |
| Identificador | `os.nodalix.Store` |
| Ejecutables | `nodalix-store`, `nodalix-store-daemon`, `nodalix-store-dl-worker` |
| Bus de búsqueda | `os.nodalix.Store.SearchProvider` |
| Ajustes | esquema `os.nodalix.Store` |

El dominio de traducciones sigue siendo `bazaar` para no invalidar los catálogos existentes. Las cadenas nuevas aparecen en inglés hasta que se traduzcan.

La navegación principal es Curated, Discover, Library, Search y Updates. Discover es la exploración de Flathub que Bazaar llamaba Explore. Updates habla con NXPM.

## Iconos

`BzThemedPaintable` envuelve el icono original de la ficha. Al dibujarlo resuelve en este orden:

1. Un icono cuyo archivo está dentro de un tema cuyo nombre empieza por `Nodalix`. Si el tema activo es uno de esos, se consulta primero.
2. El icono que ya traía la aplicación o sus metadatos.
3. `application-x-executable`.

La coincidencia usa el identificador de la aplicación y, si existe, el campo `Icon=` de su archivo desktop. No se acepta un icono heredado de `hicolor` u otro tema: el archivo tiene que vivir en el directorio del tema Nodalix. Cambiar el tema de iconos invalida la imagen. No se modifican los archivos de las aplicaciones instaladas.

## NXPM

NXPM vive en el repositorio de Nodalix OS (`nxpm/`) y se instala como `nodalix-nxpm`. La tienda lo usa por D-Bus y no importa su código.

```text
Nodalix Store  --D-Bus-->  os.nodalix.Nxpm1  (servicio de sesión de nodalix-nxpm)
```

La interfaz de sesión es `os.nodalix.Nxpm1`, en el nombre `os.nodalix.Nxpm` y la ruta `/os/nodalix/Nxpm`. La versión de API expuesta es 1.

AUR existe en la configuración como fuente deshabilitada e imposible de activar. No hay un backend que simule instalaciones de AUR. AppImage importa archivos reales; no hay un catálogo remoto inventado.
