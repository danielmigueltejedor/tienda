# Procedencia de Bazaar

Nodalix Store parte de Bazaar 0.9.11.

El archivo `~/Descargas/bazaar-main.zip` no contiene historial Git. Su comentario de archivo identifica el commit `8a7556f520e0b48db95f13c7badc17b18c30e4f4`. Ese commit es `HEAD` de `main` en el repositorio actual del proyecto:

https://gitlab.gnome.org/World/bazaar

La fecha del commit es el 3 de noviembre de 2026. `version.sh` declara la versión 0.9.11. La licencia del código importado sigue siendo GPL-3.0-or-later y se conserva en `COPYING`.

El remoto `bazaar` apunta a ese repositorio de GNOME GitLab para poder sincronizar en el futuro. La rama `nodalix/store` nace en ese commit.

https://github.com/bazaar-org/bazaar queda configurado como `bazaar-github`. Su `main` está en `3d347d5d0fb828a0ff6ff0c1dcd356b26ddfdd33` (18 de agosto de 2026, versión 0.9.5) y no contiene el commit del ZIP. No debe usarse como origen de una sincronización si se quiere conservar 0.9.11.

## Sincronización futura

```bash
git fetch bazaar
git rebase bazaar/main
```

Hay que repetir la compilación y las pruebas después de cada rebase. Los cambios propios están en la identidad de la aplicación, la resolución de iconos de Nodalix y la página de NXPM. El identificador de aplicación ya no es `io.github.kolunmi.Bazaar`, así que una sincronización producirá conflictos en esos archivos y hay que resolverlos a mano.

La rama local `nodalix/main` sigue conteniendo el clon previo de GNOME Software, sin cambios de Nodalix. No forma parte de esta tienda.

El icono instalado sigue siendo el dibujo original de Bazaar, renombrado al identificador nuevo. No es el logotipo definitivo de Nodalix.
