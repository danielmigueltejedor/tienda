# Política de permisos y actualizaciones

Flatpak sigue siendo la fuente preferida cuando hay una aplicación gráfica equivalente. Los paquetes oficiales se prefieren para componentes que no tienen una aplicación Flatpak. El usuario puede pedir otra fuente porque el identificador de NXPM incluye la fuente y la instalación rechaza una mezcla.

AUR y AppImage no se ofrecen como origen instalable. Activarlos en `~/.config/nxpm/config.json` no tiene efecto: el cargador solo acepta `enabled` en fuentes que el código realmente implementa.

Las operaciones de usuario y las privilegiadas están separadas. El servicio de sesión puede leer Pacman y modificar Flatpak del usuario. Escribir en la base de Pacman exige el proceso `nxpm privileged-upgrade`, que termina de inmediato si no es root. La tienda lo arranca con `pkexec`. La política de Polkit pide `auth_admin` y no permite sesiones inactivas ni clientes anónimos.

Los registros de NXPM guardan identificadores, versiones y mensajes de error. No hay contraseñas ni tokens en ese estado.

No se ejecutan shell ni PKGBUILD. Los identificadores pasan por expresiones fijas antes de llegar a libflatpak o libalpm.
