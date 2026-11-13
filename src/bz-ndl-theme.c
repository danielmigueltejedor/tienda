/* bz-ndl-theme.c
 *
 * Applies the Nodalia Design Language (NDL) on top of libadwaita.
 *
 * The sheets live in src/ndl/ (see PROVENANCE.md). They are loaded below the
 * application priority so the store's own style.css still wins, and above the
 * libadwaita theme so the palette, shapes and typography are NDL's. The colour
 * sheet is swapped when the system style or contrast changes.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "NODALIX-STORE::NDL"

#include "config.h"

#include <adwaita.h>

#include "bz-ndl-theme.h"

#define NDL_PREFIX "/os/nodalix/Store/ndl/"
/* Between GTK_STYLE_PROVIDER_PRIORITY_SETTINGS and _APPLICATION. */
#define NDL_PRIORITY (GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 50)

static GtkCssProvider *mode_provider = NULL;
static gboolean        installed     = FALSE;

static const char *
mode_sheet (AdwStyleManager *manager)
{
  gboolean dark = adw_style_manager_get_dark (manager);
  gboolean hc   = adw_style_manager_get_high_contrast (manager);

  if (hc)
    return dark ? "nodalia-dark-hc.css" : "nodalia-light-hc.css";
  return dark ? "nodalia-dark.css" : "nodalia-light.css";
}

static void
load_resource (GtkCssProvider *provider,
               const char     *name)
{
  g_autofree char *path = g_strconcat (NDL_PREFIX, name, NULL);

  gtk_css_provider_load_from_resource (provider, path);
}

static void
install_resource (GdkDisplay *display,
                  const char *name)
{
  GtkCssProvider *provider = gtk_css_provider_new ();

  load_resource (provider, name);
  gtk_style_context_add_provider_for_display (
      display, GTK_STYLE_PROVIDER (provider), NDL_PRIORITY);
  g_object_unref (provider);
}

static void
style_changed_cb (AdwStyleManager *manager,
                  GParamSpec      *pspec,
                  gpointer         user_data)
{
  if (mode_provider != NULL)
    load_resource (mode_provider, mode_sheet (manager));
}

/* GtkWindowControls sit in a header that is as tall as its tallest child (the
 * view switcher, a search entry), and the buttons are stretched to that height,
 * which turns the round NDL controls into ovals. Centre the controls and their
 * buttons so each stays the 28 px circle the sheet asks for. */
static gboolean
center_window_controls_hook (GSignalInvocationHint *ihint,
                             guint                  n_param_values,
                             const GValue          *param_values,
                             gpointer               user_data)
{
  gpointer instance = g_value_get_object (&param_values[0]);

  if (GTK_IS_WINDOW_CONTROLS (instance))
    {
      GtkWidget *controls = GTK_WIDGET (instance);
      GtkWidget *child    = NULL;

      gtk_widget_set_valign (controls, GTK_ALIGN_CENTER);
      for (child = gtk_widget_get_first_child (controls);
           child != NULL;
           child = gtk_widget_get_next_sibling (child))
        gtk_widget_set_valign (child, GTK_ALIGN_CENTER);
    }

  return TRUE;
}

void
bz_ndl_theme_init (void)
{
  GdkDisplay      *display = NULL;
  AdwStyleManager *manager = NULL;
  const char      *opt_out = NULL;
  guint            map_signal = 0;

  if (installed)
    return;

  opt_out = g_getenv ("NODALIX_NDL");
  if (opt_out != NULL && g_strcmp0 (opt_out, "0") == 0)
    return;

  display = gdk_display_get_default ();
  if (display == NULL)
    return;

  installed = TRUE;
  manager   = adw_style_manager_get_for_display (display);

  /* NDL windows have the three round controls. The desktop's own button
   * layout often leaves only "close", so ask for all three here. */
  g_object_set (gtk_settings_get_for_display (display),
                "gtk-decoration-layout", ":minimize,maximize,close",
                NULL);

  /* The class must exist before its signal can be looked up. */
  g_type_class_ref (GTK_TYPE_WIDGET);
  map_signal = g_signal_lookup ("map", GTK_TYPE_WIDGET);
  if (map_signal != 0)
    g_signal_add_emission_hook (map_signal, 0, center_window_controls_hook, NULL, NULL);

  /* Order matters among providers of one priority: colours first, then the
   * metrics and the widget sheet that read them, then the Nodalix overrides. */
  mode_provider = gtk_css_provider_new ();
  load_resource (mode_provider, mode_sheet (manager));
  gtk_style_context_add_provider_for_display (
      display, GTK_STYLE_PROVIDER (mode_provider), NDL_PRIORITY);

  /* The widget sheet draws check marks and window glyphs with these icons. */
  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_for_display (display),
                                    "/os/nodalix/Store/icons");

  install_resource (display, "nodalia-metrics-regular.css");
  install_resource (display, "nodalia-gtk.css");
  install_resource (display, "nodalix-overrides.css");

  g_signal_connect (manager, "notify::dark", G_CALLBACK (style_changed_cb), NULL);
  g_signal_connect (manager, "notify::high-contrast", G_CALLBACK (style_changed_cb), NULL);
}
