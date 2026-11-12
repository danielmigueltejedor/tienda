# Diseño NDL (Nodalia Design Language)

La Tienda de Nodalix usa el lenguaje de diseño de las apps de Nodalia
([nodalia-design](https://github.com/danielmigueltejedor/nodalia-design))
**sobre libadwaita**, sin reescribir la interfaz: es la vía de adopción gradual
que NDL documenta para las apps GTK existentes.

## Cómo funciona

- `src/ndl/` contiene las hojas generadas por `@nodalia/gtk`, fijadas a un
  commit (ver `src/ndl/PROVENANCE.md`). No se editan a mano; `SHA256SUMS` lo
  comprueba en CI.
- `src/bz-ndl-theme.c` las instala al arrancar con prioridad
  `APPLICATION - 50`: por encima del tema de libadwaita (paleta, formas,
  píldoras, fila activa invertida) y por debajo de `style.css`, de modo que lo
  propio de la Tienda sigue mandando donde lo define.
- La hoja de color cambia sola entre claro, oscuro y alto contraste según
  `AdwStyleManager`. La densidad es la regular.
- `src/ndl/nodalix-overrides.css` es la única hoja nuestra: apunta la
  tipografía de NDL a Adwaita Sans (Nodalix no empaqueta Inter ni Sora).
- `NODALIX_NDL=0` desactiva la capa y deja libadwaita tal cual, para depurar.

## Estado

Comprobado en GTK 4.22 y libadwaita 1.9 con un arnés propio: las hojas se
analizan sin avisos, y el cambio claro/oscuro recarga la hoja de color. **No
se ha revisado la Tienda real a mano**: queda por ajustar `style.css` donde
choque (degradados propios, banners) y decidir si las vistas principales pasan
a islas (`.ndl-island`).
