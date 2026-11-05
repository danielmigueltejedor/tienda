# Política de permisos y actualizaciones

Flatpak sigue siendo la fuente preferida cuando hay una aplicación gráfica equivalente. Los paquetes oficiales se prefieren para componentes del sistema. Un programa que solo llega como AppImage o como paquete Debian entra en `~/Applications` y no en la base de pacman. El usuario puede retirar esa copia desde la tienda o con `nxpm remove`.

AUR no se ofrece como origen instalable. Activarlo en `~/.config/nxpm/config.json` no tiene efecto. AppImage sí está implementado, y se puede desactivar con `"enabled": false` porque el código existe.

NXPM no convierte el sistema operativo en AppImages. El kernel, los controladores y las bibliotecas de arranque siguen gestionados por pacman. Convertirlos rompería el arranque. Las aplicaciones Flatpak tampoco se convierten: pasarlas a AppImage les quitaría el aislamiento, que es lo contrario de darles solo los permisos que necesitan.

Los permisos efectivos de una aplicación Flatpak son los que declara su propio manifiesto. Un override local puede ampliarlos; `permissions-reset` lo elimina para la instalación de usuario. NXPM no escribe permisos `filesystem=host`, `filesystem=home` ni `devices=all`. Si el manifiesto ya pide uno de esos, se muestra con riesgo alto y no se concede ninguno adicional.

Las operaciones de usuario y las privilegiadas están separadas. El servicio de sesión puede leer Pacman, modificar Flatpak del usuario y escribir en `~/Applications`. Escribir en la base de Pacman exige el proceso `nxpm privileged-upgrade`, que termina de inmediato si no es root. La tienda lo arranca con `pkexec`. La política de Polkit pide `auth_admin` y no permite sesiones inactivas ni clientes anónimos.

Los registros de NXPM guardan identificadores, versiones y mensajes de error. No hay contraseñas ni tokens en ese estado.

No se ejecutan shell de mantenedor, PKGBUILD ni archivos de control de un paquete Debian. Los identificadores pasan por expresiones fijas antes de llegar a libflatpak, libalpm o al directorio Applications. Una actualización de AppImage solo acepta el canal escrito dentro del propio archivo y solo descarga por HTTPS desde GitHub.
