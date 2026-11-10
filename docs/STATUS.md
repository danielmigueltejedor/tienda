# Estado

## Hecho y comprobado en código

- Importación trazable de Bazaar 0.9.11, commit `8a7556f`.
- Identidad `os.nodalix.Store`; el nombre visible es «Store» y se traduce como hacen las apps de Apple (español: «Tienda»), sin pisar el clon anterior de GNOME Software en `nodalix/main`. El icono es el de `nodalix-icon-theme` (`os.nodalix.Store`), no el de Bazaar.
- El catálogo muestra los iconos del tema Nodalix-Icons también para apps aún no instaladas y para entradas leídas de la caché (antes solo las recién analizadas pasaban por el tema).
- Navegación Discover, Search, Library y Updates, además de la vista curada que ya traía Bazaar.
- Resolución visual de iconos del tema Nodalix, con vuelta al icono original y a un icono genérico.
- NXPM 0.1 con motor único para la CLI y D-Bus.
- Backend Flatpak real para buscar, listar, instalar y retirar instalaciones de usuario, para detectar actualizaciones y para leer los permisos que la aplicación declara.
- Backend Pacman real para buscar, listar, ver dependencias a través de los objetos de libalpm y detectar actualizaciones completas.
- Carpeta `~/Applications`: importar un AppImage o un paquete Debian. El Debian se desempaqueta sin ejecutar scripts de mantenedor y sin tocar la base de pacman. Si `appimagetool` está instalado, el resultado se empaqueta como AppImage; si no, queda como AppDir ejecutable.
- Actualización de un AppImage cuando trae un canal `gh-releases-zsync`: se descarga el archivo publicado por HTTPS y solo sustituye al anterior si el resultado es un ELF. Un canal `zsync` se reconoce y no se aplica.
- La tienda muestra esas actualizaciones y puede lanzar una operación Flatpak de usuario, una actualización de AppImage importado o pedir una actualización oficial.
- Pruebas de unidad del modelo, la agrupación, las transacciones y los backends con dobles.
- Temporizador de comprobación en solo lectura. No instala nada.

## Presente pero no automático

- La actualización oficial pide `pkexec` y solo aplica `sysupgrade` completo como root.
- Los permisos Flatpak se consultan desde los metadatos de la aplicación. `nxpm permissions-reset` borra solo el override local de esa aplicación de usuario y la devuelve a lo que ella declara. No se añade `host`, `home` ni acceso a todos los dispositivos.
- La comprobación periódica solo queda instalada después de `nxpm install-session` y de activar el temporizador. Esa comprobación no descarga ni instala AppImages.

## No implementado

- AUR. La fuente está deshabilitada y el motor se niega a construirla.
- Instalar o actualizar un paquete oficial suelto. Se rechaza para no crear una actualización parcial.
- Convertir el sistema instalado por pacman, o las aplicaciones Flatpak, en AppImages. El sistema base sigue siendo nativo y Flatpak conserva su aislamiento.
- Empaquetar un Debian como AppImage cuando `appimagetool` no está instalado. En ese caso el programa queda como AppDir dentro de `~/Applications`.
- Actualizar automáticamente un AppImage que solo publica zsync, o hacerlo desde el temporizador. La descarga de un canal de GitHub ocurre cuando el usuario pulsa Actualizar.
- Cancelación de una transacción Flatpak que ya está en curso.
- Instantáneas de sistema y recuperación automática. Solo se guarda un inventario de nombres y versiones antes de una actualización oficial.
- Instalación desatendida de cualquier origen. Aunque la configuración mencione instalación automática, esta versión no la ejecuta.
- Sustituir el navegador Flatpak de Bazaar por NXPM. Discover, Search y Library siguen usando el código Flatpak de Bazaar. Updates y su buscador usan NXPM.

## Permisos y actualizaciones

La política por defecto es conservadora. Flatpak de usuario puede cambiarse desde la sesión porque no pide root. Los paquetes oficiales y Flatpak de sistema necesitan autorización. AUR no se ejecuta. No hay una tarea que actualice Arch o AUR sin alguien delante.
