# Estado

## Hecho y comprobado en código

- Importación trazable de Bazaar 0.9.11, commit `8a7556f`.
- Identidad `os.nodalix.Store` y nombre visible Nodalix Store, sin pisar el clon anterior de GNOME Software en `nodalix/main`.
- Navegación Discover, Search, Library y Updates, además de la vista curada que ya traía Bazaar.
- Resolución visual de iconos del tema Nodalix, con vuelta al icono original y a un icono genérico.
- NXPM 0.1 con motor único para la CLI y D-Bus.
- Backend Flatpak real para buscar, listar, instalar y retirar instalaciones de usuario, y para detectar actualizaciones.
- Backend Pacman real para buscar, listar, ver dependencias a través de los objetos de libalpm y detectar actualizaciones completas.
- La tienda muestra esas actualizaciones y puede lanzar una operación Flatpak de usuario o pedir una actualización oficial.
- Pruebas de unidad del modelo, la agrupación, las transacciones y los backends con dobles.
- Temporizador de comprobación en solo lectura. No instala nada.

## Presente pero no automático

- La actualización oficial pide `pkexec` y solo aplica `sysupgrade` completo como root.
- Los permisos Flatpak siguen en los diálogos que ya traía Bazaar. No hay perfiles de compatibilidad nuevos y no se concede acceso total al sistema, al directorio personal ni a dispositivos.
- La comprobación periódica solo queda instalada después de `nxpm install-session` y de activar el temporizador.

## No implementado

- AUR. La fuente está deshabilitada y el motor se niega a construirla.
- AppImage. La fuente está deshabilitada. No hay importación ni actualización.
- Instalación o actualización paquete a paquete de repositorios oficiales. Se rechaza para no crear una actualización parcial.
- Cancelación de una transacción Flatpak que ya está en curso.
- Instantáneas de sistema y recuperación automática. Solo se guarda un inventario de nombres y versiones antes de una actualización oficial.
- Instalación desatendida de cualquier origen. Aunque la configuración mencione instalación automática, esta versión no la ejecuta.
- Sustituir el navegador Flatpak de Bazaar por NXPM. Discover, Search y Library siguen usando el código Flatpak de Bazaar. Updates y su buscador usan NXPM.

## Permisos y actualizaciones

La política por defecto es conservadora. Flatpak de usuario puede cambiarse desde la sesión porque no pide root. Los paquetes oficiales y Flatpak de sistema necesitan autorización. AUR no se ejecuta. No hay una tarea que actualice Arch o AUR sin alguien delante.
