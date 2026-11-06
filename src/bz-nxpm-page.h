/* bz-nxpm-page.h
 *
 * Copyright 2026 Nodalix
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define BZ_TYPE_NXPM_PAGE (bz_nxpm_page_get_type ())
G_DECLARE_FINAL_TYPE (BzNxpmPage, bz_nxpm_page, BZ, NXPM_PAGE, AdwBin)

G_END_DECLS
