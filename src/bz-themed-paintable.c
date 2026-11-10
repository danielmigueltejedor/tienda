/* bz-themed-paintable.c
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

#include "bz-themed-paintable.h"

struct _BzThemedPaintable
{
  GObject parent_instance;

  char         *icon_name;
  GdkPaintable *fallback;
  GdkPaintable *resolved;

  GtkSettings  *settings;
  gulong        settings_id;
  GtkIconTheme *watched_theme;
  gulong        theme_id;
  gboolean      watching;
};

static void paintable_iface_init (GdkPaintableInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (BzThemedPaintable, bz_themed_paintable, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE, paintable_iface_init))

static void bz_themed_paintable_invalidate (BzThemedPaintable *self);
static void ensure_watch (BzThemedPaintable *self);
static GdkPaintable *resolve_icon (BzThemedPaintable *self);

static void
fallback_invalidated (GdkPaintable     *paintable,
                      BzThemedPaintable *self)
{
  if (self->resolved == paintable)
    gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static void
bz_themed_paintable_dispose (GObject *object)
{
  BzThemedPaintable *self = BZ_THEMED_PAINTABLE (object);

  if (self->settings != NULL && self->settings_id != 0)
    g_signal_handler_disconnect (self->settings, self->settings_id);
  self->settings    = NULL;
  self->settings_id = 0;

  if (self->watched_theme != NULL && self->theme_id != 0)
    g_signal_handler_disconnect (self->watched_theme, self->theme_id);
  self->watched_theme = NULL;
  self->theme_id      = 0;

  g_clear_pointer (&self->icon_name, g_free);
  g_clear_object (&self->fallback);
  g_clear_object (&self->resolved);

  G_OBJECT_CLASS (bz_themed_paintable_parent_class)->dispose (object);
}

static void
bz_themed_paintable_class_init (BzThemedPaintableClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = bz_themed_paintable_dispose;
}

static void
bz_themed_paintable_init (BzThemedPaintable *self)
{
}

static void
bz_themed_paintable_invalidate (BzThemedPaintable *self)
{
  g_clear_object (&self->resolved);
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static gboolean
theme_name_is_nodalix (const char *name)
{
  return name != NULL && g_str_has_prefix (name, "Nodalix");
}

static void
add_unique (GPtrArray  *names,
            const char *name)
{
  guint i;

  if (name == NULL || *name == '\0')
    return;

  for (i = 0; i < names->len; i++)
    {
      if (g_strcmp0 (g_ptr_array_index (names, i), name) == 0)
        return;
    }

  g_ptr_array_add (names, g_strdup (name));
}

static void
scan_icon_dir (const char *directory,
               GPtrArray  *names)
{
  g_autoptr (GDir) dir = NULL;
  const char *entry;

  dir = g_dir_open (directory, 0, NULL);
  if (dir == NULL)
    return;

  while ((entry = g_dir_read_name (dir)) != NULL)
    {
      g_autofree char *index_path = NULL;

      if (!theme_name_is_nodalix (entry))
        continue;

      index_path = g_build_filename (directory, entry, "index.theme", NULL);
      if (g_file_test (index_path, G_FILE_TEST_IS_REGULAR))
        add_unique (names, entry);
    }
}

static GPtrArray *
scan_nodalix_theme_names (void)
{
  GPtrArray *names            = g_ptr_array_new_with_free_func (g_free);
  GtkSettings *settings       = gtk_settings_get_default ();
  g_autofree char *active     = NULL;
  g_autofree char *user_icons = NULL;
  g_autofree char *legacy     = NULL;
  const char *const *data_dirs;
  guint i;

  if (settings != NULL)
    g_object_get (settings, "gtk-icon-theme-name", &active, NULL);
  if (theme_name_is_nodalix (active))
    add_unique (names, active);
  add_unique (names, "Nodalix");

  user_icons = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  scan_icon_dir (user_icons, names);
  legacy = g_build_filename (g_get_home_dir (), ".icons", NULL);
  scan_icon_dir (legacy, names);

  data_dirs = g_get_system_data_dirs ();
  for (i = 0; data_dirs != NULL && data_dirs[i] != NULL; i++)
    {
      g_autofree char *icons = g_build_filename (data_dirs[i], "icons", NULL);
      scan_icon_dir (icons, names);
    }

  return names;
}

/* Resolving an icon used to scan the icon directories and build a new
 * GtkIconTheme for every tile of a catalog with hundreds of apps. The theme
 * list and one GtkIconTheme per theme are shared and refreshed after a short
 * while, which keeps scrolling smooth and uses far less memory and CPU. */
#define THEME_CACHE_SECONDS 30

static GPtrArray  *cached_names       = NULL;
static char       *cached_active      = NULL;
static gint64      cached_names_time  = 0;
static GHashTable *cached_themes      = NULL;
static gint64      cached_themes_time = 0;

static GPtrArray *
nodalix_theme_names (void)
{
  GtkSettings *settings   = gtk_settings_get_default ();
  g_autofree char *active = NULL;
  gint64 now              = g_get_monotonic_time ();

  if (settings != NULL)
    g_object_get (settings, "gtk-icon-theme-name", &active, NULL);

  if (cached_names == NULL ||
      g_strcmp0 (active, cached_active) != 0 ||
      now - cached_names_time > THEME_CACHE_SECONDS * G_USEC_PER_SEC)
    {
      g_clear_pointer (&cached_names, g_ptr_array_unref);
      g_free (cached_active);
      cached_names       = scan_nodalix_theme_names ();
      cached_active      = g_steal_pointer (&active);
      cached_names_time  = now;
    }

  return g_ptr_array_ref (cached_names);
}

static char *
desktop_icon_name (const char *app_id)
{
  const char *dirs[4];
  g_autofree char *user_apps    = NULL;
  g_autofree char *user_exports = NULL;
  guint i;

  if (app_id == NULL || *app_id == '\0')
    return NULL;

  user_exports = g_build_filename (g_get_home_dir (),
                                   ".local", "share", "flatpak", "exports",
                                   "share", "applications", NULL);
  user_apps    = g_build_filename (g_get_user_data_dir (), "applications", NULL);
  dirs[0]      = user_exports;
  dirs[1]      = "/var/lib/flatpak/exports/share/applications";
  dirs[2]      = user_apps;
  dirs[3]      = "/usr/share/applications";

  for (i = 0; i < G_N_ELEMENTS (dirs); i++)
    {
      g_autofree char *path = NULL;
      g_autoptr (GKeyFile) key = NULL;
      g_autofree char *icon    = NULL;

      path = g_strdup_printf ("%s/%s.desktop", dirs[i], app_id);

      key = g_key_file_new ();
      if (!g_key_file_load_from_file (key, path, G_KEY_FILE_NONE, NULL))
        continue;

      icon = g_key_file_get_string (key, "Desktop Entry", "Icon", NULL);
      if (icon == NULL || *icon == '\0' || strchr (icon, '/') != NULL)
        continue;
      if (g_str_has_suffix (icon, ".png") || g_str_has_suffix (icon, ".svg") || g_str_has_suffix (icon, ".xpm"))
        {
          char *dot = strrchr (icon, '.');
          if (dot != NULL)
            *dot = '\0';
        }
      return g_steal_pointer (&icon);
    }

  return NULL;
}

static gboolean
file_is_in_theme (GFile      *file,
                  const char *theme_name)
{
  g_autofree char *path   = NULL;
  g_autofree char *needle = NULL;
  gboolean found;

  if (file == NULL || theme_name == NULL)
    return FALSE;

  path = g_file_get_path (file);
  if (path == NULL)
    return FALSE;

  needle = g_strdup_printf ("/icons/%s/", theme_name);
  found  = strstr (path, needle) != NULL;
  if (!found)
    {
      g_free (needle);
      needle = g_strdup_printf ("/.icons/%s/", theme_name);
      found  = strstr (path, needle) != NULL;
    }
  return found;
}

static void
add_icon_search_paths (GtkIconTheme *theme)
{
  g_autofree char *user_icons = NULL;
  g_autofree char *legacy     = NULL;
  const char *const *data_dirs;
  guint i;

  user_icons = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  legacy     = g_build_filename (g_get_home_dir (), ".icons", NULL);
  gtk_icon_theme_add_search_path (theme, user_icons);
  gtk_icon_theme_add_search_path (theme, legacy);

  data_dirs = g_get_system_data_dirs ();
  for (i = 0; data_dirs != NULL && data_dirs[i] != NULL; i++)
    {
      g_autofree char *icons = g_build_filename (data_dirs[i], "icons", NULL);
      gtk_icon_theme_add_search_path (theme, icons);
    }
}

static GtkIconTheme *
shared_theme (const char *theme_name)
{
  GtkIconTheme *theme = NULL;
  gint64 now          = g_get_monotonic_time ();

  if (cached_themes == NULL ||
      now - cached_themes_time > THEME_CACHE_SECONDS * G_USEC_PER_SEC)
    {
      g_clear_pointer (&cached_themes, g_hash_table_unref);
      cached_themes      = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
      cached_themes_time = now;
    }

  theme = g_hash_table_lookup (cached_themes, theme_name);
  if (theme == NULL)
    {
      theme = gtk_icon_theme_new ();
      add_icon_search_paths (theme);
      gtk_icon_theme_set_theme_name (theme, theme_name);
      g_hash_table_insert (cached_themes, g_strdup (theme_name), theme);
    }

  return theme;
}

static GtkIconPaintable *
lookup_in_theme (const char *theme_name,
                 const char *icon_name)
{
  GtkIconTheme *theme                    = shared_theme (theme_name);
  g_autoptr (GtkIconPaintable) paintable = NULL;
  GFile *file                            = NULL;

  if (!gtk_icon_theme_has_icon (theme, icon_name))
    return NULL;

  paintable = gtk_icon_theme_lookup_icon (theme,
                                          icon_name,
                                          NULL,
                                          128,
                                          1,
                                          GTK_TEXT_DIR_NONE,
                                          0);
  if (paintable == NULL)
    return NULL;

  file = gtk_icon_paintable_get_file (paintable);
  if (!file_is_in_theme (file, theme_name))
    return NULL;

  return g_steal_pointer (&paintable);
}

static GdkPaintable *
generic_icon (void)
{
  GdkDisplay *display = gdk_display_get_default ();
  GtkIconTheme *theme;
  GtkIconPaintable *paintable;

  if (display == NULL)
    return NULL;

  theme = gtk_icon_theme_get_for_display (display);
  paintable = gtk_icon_theme_lookup_icon (theme,
                                          "application-x-executable",
                                          NULL,
                                          128,
                                          1,
                                          GTK_TEXT_DIR_NONE,
                                          0);
  return GDK_PAINTABLE (paintable);
}

/* Catalog entries that are not installed have no .desktop file to name their
 * icon, and reverse-DNS application ids (org.mozilla.Thunderbird) are not always
 * what a theme calls its artwork (thunderbird). Offer the conventional names
 * after the exact id, never a generic word that could match an unrelated icon. */
static gboolean
is_generic_word (const char *word)
{
  static const char *const generic[] = {
    "client", "desktop", "editor", "player", "manager", "browser", "viewer", "reader",
    "studio", "launcher", "settings", "terminal", "console", "mobile", "server",
    "service", "gtk", "qt", "app", "application", "tool", "tools", "main", "stable",
    "beta", "nightly", "devel", "development", "portable", "flatpak", NULL
  };
  guint i;

  for (i = 0; generic[i] != NULL; i++)
    {
      if (g_strcmp0 (word, generic[i]) == 0)
        return TRUE;
    }
  return FALSE;
}

static void
add_derived_names (GPtrArray  *icons,
                   const char *app_id)
{
  g_autofree char *trimmed = NULL;
  g_autofree char *lower   = NULL;
  const char *last         = NULL;
  const char *dot          = NULL;

  if (app_id == NULL || strchr (app_id, '.') == NULL)
    return;

  trimmed = g_strdup (app_id);
  if (g_str_has_suffix (trimmed, ".desktop"))
    trimmed[strlen (trimmed) - strlen (".desktop")] = '\0';
  add_unique (icons, trimmed);

  dot  = strrchr (trimmed, '.');
  last = dot != NULL ? dot + 1 : trimmed;
  if (strlen (last) < 5)
    return;

  lower = g_ascii_strdown (last, -1);
  if (!is_generic_word (lower))
    add_unique (icons, lower);
}

static GdkPaintable *
resolve_icon (BzThemedPaintable *self)
{
  g_autoptr (GPtrArray) themes = NULL;
  g_autoptr (GPtrArray) icons  = NULL;
  g_autofree char *desktop     = NULL;
  guint theme_index;
  guint icon_index;

  icons = g_ptr_array_new_with_free_func (g_free);
  add_unique (icons, self->icon_name);
  desktop = desktop_icon_name (self->icon_name);
  add_unique (icons, desktop);
  add_derived_names (icons, self->icon_name);

  themes = nodalix_theme_names ();
  for (theme_index = 0; theme_index < themes->len; theme_index++)
    {
      const char *theme_name = g_ptr_array_index (themes, theme_index);

      for (icon_index = 0; icon_index < icons->len; icon_index++)
        {
          GtkIconPaintable *match = NULL;

          match = lookup_in_theme (theme_name, g_ptr_array_index (icons, icon_index));
          if (match != NULL)
            return GDK_PAINTABLE (match);
        }
    }

  if (self->fallback != NULL)
    return g_object_ref (self->fallback);

  return generic_icon ();
}

static void
ensure_watch (BzThemedPaintable *self)
{
  GdkDisplay *display;
  GtkIconTheme *theme;

  if (!self->watching)
    {
      self->settings = gtk_settings_get_default ();
      if (self->settings != NULL)
        {
          self->settings_id = g_signal_connect_swapped (self->settings,
                                                        "notify::gtk-icon-theme-name",
                                                        G_CALLBACK (bz_themed_paintable_invalidate),
                                                        self);
          self->watching = TRUE;
        }
    }

  display = gdk_display_get_default ();
  if (display == NULL)
    return;

  theme = gtk_icon_theme_get_for_display (display);
  if (theme == self->watched_theme)
    return;

  if (self->watched_theme != NULL && self->theme_id != 0)
    g_signal_handler_disconnect (self->watched_theme, self->theme_id);

  self->watched_theme = theme;
  self->theme_id      = g_signal_connect_swapped (theme,
                                                  "changed",
                                                  G_CALLBACK (bz_themed_paintable_invalidate),
                                                  self);
}

static void
paintable_snapshot (GdkPaintable *paintable,
                    GdkSnapshot  *snapshot,
                    double        width,
                    double        height)
{
  BzThemedPaintable *self = BZ_THEMED_PAINTABLE (paintable);

  ensure_watch (self);
  if (self->resolved == NULL)
    self->resolved = resolve_icon (self);
  if (self->resolved != NULL)
    gdk_paintable_snapshot (self->resolved, snapshot, width, height);
}

static GdkPaintableFlags
paintable_get_flags (GdkPaintable *paintable)
{
  return 0;
}

static int
paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  return 128;
}

static int
paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  return 128;
}

static void
paintable_iface_init (GdkPaintableInterface *iface)
{
  iface->snapshot             = paintable_snapshot;
  iface->get_flags            = paintable_get_flags;
  iface->get_intrinsic_width  = paintable_get_intrinsic_width;
  iface->get_intrinsic_height = paintable_get_intrinsic_height;
}

BzThemedPaintable *
bz_themed_paintable_new (const char   *icon_name,
                         GdkPaintable *fallback)
{
  BzThemedPaintable *self = g_object_new (BZ_TYPE_THEMED_PAINTABLE, NULL);

  self->icon_name = g_strdup (icon_name);
  if (fallback != NULL)
    {
      self->fallback = g_object_ref (fallback);
      g_signal_connect (fallback, "invalidate-contents", G_CALLBACK (fallback_invalidated), self);
    }

  return self;
}

void
bz_themed_paintable_set_icon_name (BzThemedPaintable *self,
                                   const char       *icon_name)
{
  g_return_if_fail (BZ_IS_THEMED_PAINTABLE (self));

  if (g_strcmp0 (self->icon_name, icon_name) == 0)
    return;

  g_free (self->icon_name);
  self->icon_name = g_strdup (icon_name);
  bz_themed_paintable_invalidate (self);
}

GdkPaintable *
bz_themed_paintable_get_fallback (BzThemedPaintable *self)
{
  g_return_val_if_fail (BZ_IS_THEMED_PAINTABLE (self), NULL);
  return self->fallback;
}
