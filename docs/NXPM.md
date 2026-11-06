# NXPM

NXPM ya no vive en este repositorio. Es el gestor de paquetes de Nodalix OS y se mantiene en
[nodalix-os](https://github.com/danielmigueltejedor/nodalix-os) (carpeta `nxpm/`, documentación en `docs/nxpm.md`),
empaquetado como `nodalix-nxpm`.

Nodalix Store es un cliente de su servicio D-Bus de sesión: `os.nodalix.Nxpm`, interfaz `os.nodalix.Nxpm1`,
ruta `/os/nodalix/Nxpm`. La lista de métodos, los identificadores de paquete y los códigos de error están en
la documentación de nodalix-os. La tienda solo necesita que el servicio esté instalado; Nodalix OS lo trae
como dependencia de `nodalix-store`.
