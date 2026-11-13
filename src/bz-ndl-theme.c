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
install_resource_at (GdkDisplay *display,
                     const char *name,
                     int         priority)
{
  GtkCssProvider *provider = gtk_css_provider_new ();

  load_resource (provider, name);
  gtk_style_context_add_provider_for_display (
      display, GTK_STYLE_PROVIDER (provider), priority);
  g_object_unref (provider);
}

static void
install_resource (GdkDisplay *display,
                  const char *name)
{
  install_resource_at (display, name, NDL_PRIORITY);
}

static void
style_changed_cb (AdwStyleManager *manager,
                  GParamSpec      *pspec,
                  gpointer         user_data)
{
  if (mode_provider != NULL)
    load_resource (mode_provider, mode_sheet (manager));
}

/* GtkWindowControls ask the icon theme for window-minimize-symbolic and its
 * siblings by name, so the CSS `-gtk-icon-source` of the sheet never gets to
 * draw them and the system icon theme's glyphs show instead. Swap the names for
 * NDL's own glyphs (registered from the gresource), and keep doing it when GTK
 * changes the name itself, as it does between maximise and restore. */
static void
window_glyph_notify_cb (GtkImage *image)
{
  const char *name = gtk_image_get_icon_name (image);

  if (name == NULL)
    return;

  if (g_str_equal (name, "window-minimize-symbolic"))
    gtk_image_set_from_icon_name (image, "ndl-window-minimize-symbolic");
  else if (g_str_equal (name, "window-maximize-symbolic"))
    gtk_image_set_from_icon_name (image, "ndl-window-maximize-symbolic");
  else if (g_str_equal (name, "window-restore-symbolic"))
    gtk_image_set_from_icon_name (image, "ndl-window-restore-symbolic");
  else if (g_str_equal (name, "window-close-symbolic"))
    gtk_image_set_from_icon_name (image, "ndl-window-close-symbolic");
}

static void
use_ndl_glyph (GtkImage *image)
{
  if (g_object_get_data (G_OBJECT (image), "ndl-glyph") != NULL)
    return;

  g_object_set_data (G_OBJECT (image), "ndl-glyph", GINT_TO_POINTER (1));
  g_signal_connect (image, "notify::icon-name",
                    G_CALLBACK (window_glyph_notify_cb), NULL);
  window_glyph_notify_cb (image);
}

/* The header is as tall as its tallest child (the view switcher, a search
 * entry), and the buttons of the window controls are stretched to that height,
 * which turns the round NDL controls into ovals. Centre the controls and their
 * buttons so each stays the 28 px circle the sheet asks for. */
static void
style_window_controls (GtkWidget *controls)
{
  GtkWidget *button = NULL;

  gtk_widget_set_valign (controls, GTK_ALIGN_CENTER);
  for (button = gtk_widget_get_first_child (controls);
       button != NULL;
       button = gtk_widget_get_next_sibling (button))
    {
      GtkWidget *child = NULL;

      gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
      child = gtk_widget_get_first_child (button);
      if (GTK_IS_IMAGE (child))
        use_ndl_glyph (GTK_IMAGE (child));
    }
}

/* Runs for every widget that maps; only the window controls matter. */
static gboolean
window_controls_map_hook (GSignalInvocationHint *ihint,
                          guint                  n_param_values,
                          const GValue          *param_values,
                          gpointer               user_data)
{
  gpointer instance = g_value_get_object (&param_values[0]);

  if (GTK_IS_WINDOW_CONTROLS (instance))
    style_window_controls (GTK_WIDGET (instance));
  else if (GTK_IS_IMAGE (instance))
    {
      GtkWidget *button   = gtk_widget_get_parent (GTK_WIDGET (instance));
      GtkWidget *controls = button != NULL ? gtk_widget_get_parent (button) : NULL;

      if (GTK_IS_BUTTON (button) && GTK_IS_WINDOW_CONTROLS (controls))
        use_ndl_glyph (GTK_IMAGE (instance));
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
    g_signal_add_emission_hook (map_signal, 0, window_controls_map_hook, NULL, NULL);

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
  /* The Store's own widgets, above the Store's style.css (which is at the
   * application priority) so they take NDL's look. */
  install_resource_at (display, "nodalix-store.css",
                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 10);

  g_signal_connect (manager, "notify::dark", G_CALLBACK (style_changed_cb), NULL);
  g_signal_connect (manager, "notify::high-contrast", G_CALLBACK (style_changed_cb), NULL);
}
