/* bz-ndl-theme.h
 *
 * Applies the Nodalia Design Language (NDL) on top of libadwaita.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* Installs the NDL style sheets for the default display and keeps the colour
 * mode (light, dark, high contrast) in sync with AdwStyleManager. Safe to call
 * more than once. Setting NODALIX_NDL=0 in the environment skips it, which
 * leaves the stock libadwaita look for debugging. */
void bz_ndl_theme_init (void);

G_END_DECLS
