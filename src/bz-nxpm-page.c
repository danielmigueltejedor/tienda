/* bz-nxpm-page.c
 *
 * Copyright 2026 Nodalix
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bz-nxpm-page.h"

#include <glib/gi18n.h>

#define NXPM_NAME "os.nodalix.Nxpm"
#define NXPM_PATH "/os/nodalix/Nxpm"
#define NXPM_IFACE "os.nodalix.Nxpm1"

struct _BzNxpmPage
{
  AdwBin parent_instance;

  GtkSearchEntry *search_entry;
  GtkWidget      *banner_box;
  GtkStack       *stack;
  GtkListBox     *results_list;
  AdwStatusPage  *error_page;
  GtkRevealer    *progress_revealer;
  GtkLabel       *progress_label;
  GtkProgressBar *progress_bar;

  GDBusConnection *connection;
  guint            signal_id;
  char            *transaction_id;
};

G_DEFINE_FINAL_TYPE (BzNxpmPage, bz_nxpm_page, ADW_TYPE_BIN)

static void refresh_updates (BzNxpmPage *self);
static void show_error (BzNxpmPage *self, const char *message);
static void set_busy (BzNxpmPage *self, gboolean busy, const char *message);

static const char *
variant_string (GVariant   *dict,
                const char *key)
{
  const char *value = NULL;

  if (dict != NULL)
    g_variant_lookup (dict, key, "&s", &value);
  return value != NULL ? value : "";
}

static gboolean
variant_bool (GVariant   *dict,
              const char *key)
{
  gboolean value = FALSE;

  if (dict != NULL)
    g_variant_lookup (dict, key, "b", &value);
  return value;
}

static void
clear_results (BzNxpmPage *self)
{
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->results_list))) != NULL)
    gtk_list_box_remove (self->results_list, child);
}

static void
on_action_finished (GObject      *source,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  BzNxpmPage *self           = user_data;
  g_autoptr (GError) error   = NULL;
  g_autoptr (GVariant) reply = NULL;
  const char *transaction_id = NULL;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
  if (reply == NULL)
    {
      set_busy (self, FALSE, NULL);
      show_error (self, error->message);
      return;
    }

  g_variant_get (reply, "(&s)", &transaction_id);
  g_free (self->transaction_id);
  self->transaction_id = g_strdup (transaction_id);
  gtk_revealer_set_reveal_child (self->progress_revealer, TRUE);
  gtk_label_set_text (self->progress_label, _ ("Waiting for NXPM…"));
}

static void
call_method (BzNxpmPage *self,
             const char *method,
             GVariant   *parameters)
{
  if (self->connection == NULL)
    {
      show_error (self, _ ("NXPM is not connected."));
      return;
    }

  gtk_revealer_set_reveal_child (self->progress_revealer, TRUE);
  gtk_label_set_text (self->progress_label, _ ("Requesting the operation…"));
  g_dbus_connection_call (self->connection,
                          NXPM_NAME,
                          NXPM_PATH,
                          NXPM_IFACE,
                          method,
                          parameters,
                          G_VARIANT_TYPE ("(s)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          on_action_finished,
                          self);
}

static void
on_row_clicked (GtkButton *button,
                gpointer   user_data)
{
  BzNxpmPage *self      = user_data;
  const char *method    = g_object_get_data (G_OBJECT (button), "nxpm-method");
  const char *package   = g_object_get_data (G_OBJECT (button), "nxpm-id");
  const char *source    = g_object_get_data (G_OBJECT (button), "nxpm-source");

  if (g_strcmp0 (method, "Update") == 0)
    call_method (self, "Update", g_variant_new ("(s)", package));
  else
    call_method (self, method, g_variant_new ("(ss)", package, source));
}

static GtkWidget *
make_row (BzNxpmPage *self,
          GVariant   *dict)
{
  const char *name       = variant_string (dict, "name");
  const char *summary    = variant_string (dict, "summary");
  const char *source     = variant_string (dict, "source");
  const char *version    = variant_string (dict, "version");
  const char *available  = variant_string (dict, "available_version");
  const char *package_id = variant_string (dict, "id");
  gboolean installed     = variant_bool (dict, "installed");
  gboolean update        = variant_bool (dict, "update");
  g_autofree char *subtitle = NULL;
  GtkWidget *row;
  GtkWidget *button = NULL;

  if (update && available[0] != '\0')
    subtitle = g_strdup_printf ("%s · %s → %s", source, version, available);
  else if (update)
    subtitle = g_strdup_printf ("%s · %s · %s", source, version, _ ("update available"));
  else if (version[0] != '\0')
    subtitle = g_strdup_printf ("%s · %s", source, version);
  else
    subtitle = g_strdup (source);

  if (summary[0] != '\0')
    {
      g_autofree char *with_summary = g_strdup_printf ("%s\n%s", subtitle, summary);
      g_free (subtitle);
      subtitle = g_steal_pointer (&with_summary);
    }

  row = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), name[0] != '\0' ? name : package_id);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (row), subtitle);

  if (g_strcmp0 (source, "flatpak") == 0 || g_strcmp0 (source, "appimage") == 0)
    {
      const char *label;
      const char *method;

      if (update)
        {
          label = _ ("Update");
          method = "Update";
        }
      else if (installed)
        {
          label = _ ("Remove");
          method = "Remove";
        }
      else if (g_strcmp0 (source, "flatpak") == 0)
        {
          label = _ ("Install");
          method = "Install";
        }
      else
        {
          label = NULL;
          method = NULL;
        }

      if (method != NULL)
        {
          button = gtk_button_new_with_label (label);
          g_object_set_data_full (G_OBJECT (button), "nxpm-method", g_strdup (method), g_free);
          g_object_set_data_full (G_OBJECT (button), "nxpm-id", g_strdup (package_id), g_free);
          g_object_set_data_full (G_OBJECT (button), "nxpm-source", g_strdup (source), g_free);
          g_signal_connect (button, "clicked", G_CALLBACK (on_row_clicked), self);
          gtk_widget_add_css_class (button, "pill");
          adw_action_row_add_suffix (ADW_ACTION_ROW (row), button);
        }
    }

  return row;
}

static void
present_packages (BzNxpmPage *self,
                  GVariant   *packages,
                  gboolean    updates)
{
  GVariantIter iter;
  GVariant *dict = NULL;
  guint count    = 0;
  gboolean pacman_updates = FALSE;

  clear_results (self);
  g_variant_iter_init (&iter, packages);
  while (g_variant_iter_next (&iter, "@a{sv}", &dict))
    {
      const char *source = variant_string (dict, "source");

      if (updates && g_strcmp0 (source, "pacman") == 0)
        pacman_updates = TRUE;
      gtk_list_box_append (self->results_list, make_row (self, dict));
      g_variant_unref (dict);
      count++;
    }

  gtk_widget_set_visible (self->banner_box, pacman_updates);
  if (count == 0)
    gtk_stack_set_visible_child_name (self->stack, "empty");
  else
    gtk_stack_set_visible_child_name (self->stack, "results");
}

static void
on_list_finished (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  BzNxpmPage *self           = user_data;
  gboolean updates           = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (self), "nxpm-updates"));
  g_autoptr (GError) error   = NULL;
  g_autoptr (GVariant) reply = NULL;
  GVariant *packages         = NULL;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
  if (reply == NULL)
    {
      show_error (self, error->message);
      return;
    }

  packages = g_variant_get_child_value (reply, 0);
  present_packages (self, packages, updates);
  g_variant_unref (packages);
}

static void
request_list (BzNxpmPage *self,
              const char *method,
              GVariant   *parameters,
              gboolean    updates)
{
  g_object_set_data (G_OBJECT (self), "nxpm-updates", GINT_TO_POINTER (updates));
  gtk_stack_set_visible_child_name (self->stack, "busy");
  gtk_widget_set_visible (self->banner_box, FALSE);
  g_dbus_connection_call (self->connection,
                          NXPM_NAME,
                          NXPM_PATH,
                          NXPM_IFACE,
                          method,
                          parameters,
                          G_VARIANT_TYPE ("(aa{sv})"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          on_list_finished,
                          self);
}

static void
refresh_updates (BzNxpmPage *self)
{
  if (self->connection == NULL)
    return;
  request_list (self, "ListUpdates", g_variant_new ("()"), TRUE);
}

static void
show_error (BzNxpmPage *self,
            const char *message)
{
  adw_status_page_set_description (self->error_page, message);
  gtk_stack_set_visible_child_name (self->stack, "error");
}

static void
set_busy (BzNxpmPage *self,
          gboolean    busy,
          const char *message)
{
  if (message != NULL)
    gtk_label_set_text (self->progress_label, message);
  gtk_revealer_set_reveal_child (self->progress_revealer, busy);
}

static void
on_transaction (GDBusConnection *connection,
                const char      *sender,
                const char      *path,
                const char      *interface,
                const char      *signal,
                GVariant        *parameters,
                gpointer         user_data)
{
  BzNxpmPage *self = user_data;
  GVariant *dict   = NULL;
  const char *state;
  const char *message;
  const char *tx_id;
  double progress = 0;

  g_variant_get (parameters, "(@a{sv})", &dict);
  tx_id   = variant_string (dict, "id");
  state   = variant_string (dict, "state");
  message = variant_string (dict, "message");
  g_variant_lookup (dict, "progress", "d", &progress);

  if (self->transaction_id != NULL && g_strcmp0 (tx_id, self->transaction_id) != 0)
    {
      g_variant_unref (dict);
      return;
    }

  gtk_revealer_set_reveal_child (self->progress_revealer, TRUE);
  gtk_label_set_text (self->progress_label, message[0] != '\0' ? message : state);
  if (progress >= 0)
    gtk_progress_bar_set_fraction (self->progress_bar, CLAMP (progress, 0, 1));
  else
    gtk_progress_bar_pulse (self->progress_bar);

  if (g_strcmp0 (state, "succeeded") == 0 ||
      g_strcmp0 (state, "failed") == 0 ||
      g_strcmp0 (state, "cancelled") == 0)
    {
      gtk_progress_bar_set_fraction (self->progress_bar, g_strcmp0 (state, "succeeded") == 0 ? 1 : 0);
      refresh_updates (self);
    }

  g_variant_unref (dict);
}

static void
on_bus (GObject      *source,
        GAsyncResult *result,
        gpointer      user_data)
{
  BzNxpmPage *self         = user_data;
  g_autoptr (GError) error = NULL;

  self->connection = g_bus_get_finish (result, &error);
  if (self->connection == NULL)
    {
      show_error (self, error->message);
      return;
    }

  self->signal_id = g_dbus_connection_signal_subscribe (self->connection,
                                                        NXPM_NAME,
                                                        NXPM_IFACE,
                                                        "TransactionChanged",
                                                        NXPM_PATH,
                                                        NULL,
                                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                                        on_transaction,
                                                        self,
                                                        NULL);
  refresh_updates (self);
}

static void
nxpm_search_activate (GtkSearchEntry *entry,
                      BzNxpmPage     *self)
{
  const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));

  if (self->connection == NULL)
    return;
  if (text == NULL || *text == '\0')
    {
      refresh_updates (self);
      return;
    }

  request_list (self, "Search", g_variant_new ("(s)", text), FALSE);
}

static void
nxpm_refresh_clicked (GtkButton  *button,
                      BzNxpmPage *self)
{
  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
  refresh_updates (self);
}

static void
nxpm_system_update_clicked (GtkButton  *button,
                            BzNxpmPage *self)
{
  call_method (self, "UpgradeSystem", g_variant_new ("()"));
}

static void
on_cancel_finished (GObject      *source,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  BzNxpmPage *self         = user_data;
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) reply = NULL;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
  if (reply == NULL)
    gtk_label_set_text (self->progress_label, error->message);
}

static void
nxpm_cancel_clicked (GtkButton  *button,
                     BzNxpmPage *self)
{
  if (self->connection == NULL || self->transaction_id == NULL)
    return;

  g_dbus_connection_call (self->connection,
                          NXPM_NAME,
                          NXPM_PATH,
                          NXPM_IFACE,
                          "Cancel",
                          g_variant_new ("(s)", self->transaction_id),
                          G_VARIANT_TYPE ("(b)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          on_cancel_finished,
                          self);
}

static void
bz_nxpm_page_dispose (GObject *object)
{
  BzNxpmPage *self = BZ_NXPM_PAGE (object);

  if (self->connection != NULL && self->signal_id != 0)
    g_dbus_connection_signal_unsubscribe (self->connection, self->signal_id);
  self->signal_id = 0;
  g_clear_object (&self->connection);
  g_clear_pointer (&self->transaction_id, g_free);

  G_OBJECT_CLASS (bz_nxpm_page_parent_class)->dispose (object);
}

static void
bz_nxpm_page_class_init (BzNxpmPageClass *klass)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = bz_nxpm_page_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/os/nodalix/Store/bz-nxpm-page.ui");
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, search_entry);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, banner_box);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, stack);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, results_list);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, error_page);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, progress_revealer);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, progress_label);
  gtk_widget_class_bind_template_child (widget_class, BzNxpmPage, progress_bar);
  gtk_widget_class_bind_template_callback (widget_class, nxpm_search_activate);
  gtk_widget_class_bind_template_callback (widget_class, nxpm_refresh_clicked);
  gtk_widget_class_bind_template_callback (widget_class, nxpm_system_update_clicked);
  gtk_widget_class_bind_template_callback (widget_class, nxpm_cancel_clicked);
}

static void
bz_nxpm_page_init (BzNxpmPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_stack_set_visible_child_name (self->stack, "busy");
  g_bus_get (G_BUS_TYPE_SESSION, NULL, on_bus, self);
}
