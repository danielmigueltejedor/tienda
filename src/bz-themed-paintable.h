/* bz-themed-paintable.h
 *
 * Copyright 2026 Nodalix
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BZ_TYPE_THEMED_PAINTABLE (bz_themed_paintable_get_type ())
G_DECLARE_FINAL_TYPE (BzThemedPaintable, bz_themed_paintable, BZ, THEMED_PAINTABLE, GObject)

BzThemedPaintable *
bz_themed_paintable_new (const char   *icon_name,
                         GdkPaintable *fallback);

void
bz_themed_paintable_set_icon_name (BzThemedPaintable *self,
                                   const char       *icon_name);

GdkPaintable *
bz_themed_paintable_get_fallback (BzThemedPaintable *self);

G_END_DECLS
