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
- `src/ndl/nodalix-overrides.css` es una hoja nuestra: apunta la tipografía de
  NDL a Adwaita Sans (Nodalix no empaqueta Inter ni Sora).
- `src/ndl/nodalix-store.css` es la otra: viste con tokens de NDL lo que es
  propio de la Tienda y no de libadwaita (búsqueda, teselas de categoría,
  píldoras, indicadores de color, secciones de la portada). Se carga por
  **encima** de `style.css` (`APPLICATION + 10`), porque esa hoja define los
  degradados y colores originales, y solo con NDL activo.
- Los botones de la ventana son los de NDL: `bz-ndl-theme.c` cambia los iconos
  `window-*-symbolic` por los `ndl-window-*` y centra los controles en la
  cabecera, que es más alta que ellos.
- `NODALIX_NDL=0` desactiva la capa y deja libadwaita tal cual, para depurar.

## Estado

Comprobado en GTK 4.22 y libadwaita 1.9 con un arnés propio: las hojas se
analizan sin avisos, y el cambio claro/oscuro recarga la hoja de color. Falta
decidir si las vistas principales pasan a islas (`.ndl-island`).
