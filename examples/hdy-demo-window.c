#include "hdy-demo-window.h"

#include <glib/gi18n.h>
#include "hdy-flap-demo-window.h"
#include "hdy-view-switcher-demo-window.h"

struct _HdyDemoWindow
{
  HdyApplicationWindow parent_instance;

  HdyLeaflet *content_box;
  GtkBox *right_box;
  GtkStack *header_stack;
  GtkImage *theme_variant;
  GtkStackSidebar *sidebar;
  GtkStack *stack;
  HdyComboRow *leaflet_transition_row;
  HdyLeaflet *content_leaflet;
  GtkListBox *lists_listbox;
  HdyCarousel *carousel;
  GtkBox *carousel_box;
  GtkListBox *carousel_listbox;
  GtkStack *carousel_indicators_stack;
  HdyAvatar *avatar;
  GtkEntry *avatar_text;
  GtkLabel *avatar_file_chooser_label;
  GtkButton *avatar_remove_button;
  GtkFileChooserNative *avatar_file_chooser;
  GtkListBox *avatar_contacts;
};

G_DEFINE_TYPE (HdyDemoWindow, hdy_demo_window, HDY_TYPE_APPLICATION_WINDOW)

static void
theme_variant_button_clicked_cb (HdyDemoWindow *self)
{
  GtkSettings *settings = gtk_settings_get_default ();
  gboolean prefer_dark_theme;

  g_object_get (settings, "gtk-application-prefer-dark-theme", &prefer_dark_theme, NULL);
  g_object_set (settings, "gtk-application-prefer-dark-theme", !prefer_dark_theme, NULL);
}

static gboolean
prefer_dark_theme_to_icon_name_cb (GBinding     *binding,
                                   const GValue *from_value,
                                   GValue       *to_value,
                                   gpointer      user_data)
{
  g_value_set_string (to_value,
                      g_value_get_boolean (from_value) ? "light-mode-symbolic" :
                                                         "dark-mode-symbolic");

  return TRUE;
}

static void
update (HdyDemoWindow *self)
{
  const gchar *header_bar_name = "default";

  if (g_strcmp0 (gtk_stack_get_visible_child_name (self->stack), "leaflet") == 0)
    header_bar_name = "leaflet";

  gtk_stack_set_visible_child_name (self->header_stack, header_bar_name);
}

static void
notify_leaflet_visible_child_cb (HdyDemoWindow *self)
{
  update (self);
}

static void
notify_visible_child_cb (GObject       *sender,
                         GParamSpec    *pspec,
                         HdyDemoWindow *self)
{
  update (self);

  hdy_leaflet_navigate (self->content_box, HDY_NAVIGATION_DIRECTION_FORWARD);
}

static void
back_clicked_cb (GtkWidget     *sender,
                 HdyDemoWindow *self)
{
  hdy_leaflet_navigate (self->content_box, HDY_NAVIGATION_DIRECTION_BACK);
}

static void
leaflet_back_clicked_cb (GtkWidget     *sender,
                         HdyDemoWindow *self)
{
  hdy_leaflet_navigate (self->content_leaflet, HDY_NAVIGATION_DIRECTION_BACK);
}

static gchar *
leaflet_transition_name (HdyEnumValueObject *value,
                         gpointer            user_data)
{
  g_return_val_if_fail (HDY_IS_ENUM_VALUE_OBJECT (value), NULL);

  switch (hdy_enum_value_object_get_value (value)) {
  case HDY_LEAFLET_TRANSITION_TYPE_OVER:
    return g_strdup (_("Over"));
  case HDY_LEAFLET_TRANSITION_TYPE_UNDER:
    return g_strdup (_("Under"));
  case HDY_LEAFLET_TRANSITION_TYPE_SLIDE:
    return g_strdup (_("Slide"));
  default:
    return NULL;
  }
}

static void
notify_leaflet_transition_cb (GObject       *sender,
                              GParamSpec    *pspec,
                              HdyDemoWindow *self)
{
  HdyComboRow *row = HDY_COMBO_ROW (sender);

  g_assert (HDY_IS_COMBO_ROW (row));
  g_assert (HDY_IS_DEMO_WINDOW (self));

  hdy_leaflet_set_transition_type (HDY_LEAFLET (self->content_box), hdy_combo_row_get_selected (row));
}

static void
leaflet_go_next_row_activated_cb (HdyDemoWindow *self)
{
  g_assert (HDY_IS_DEMO_WINDOW (self));

  hdy_leaflet_navigate (self->content_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
}

static void
view_switcher_demo_clicked_cb (GtkButton     *btn,
                               HdyDemoWindow *self)
{
  HdyViewSwitcherDemoWindow *window = hdy_view_switcher_demo_window_new ();

  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (self));
  gtk_window_present (GTK_WINDOW (window));
}

static gchar *
carousel_orientation_name (HdyEnumValueObject *value,
                           gpointer            user_data)
{
  g_return_val_if_fail (HDY_IS_ENUM_VALUE_OBJECT (value), NULL);

  switch (hdy_enum_value_object_get_value (value)) {
  case GTK_ORIENTATION_HORIZONTAL:
    return g_strdup (_("Horizontal"));
  case GTK_ORIENTATION_VERTICAL:
    return g_strdup (_("Vertical"));
  default:
    return NULL;
  }
}

static void
notify_carousel_orientation_cb (GObject       *sender,
                                GParamSpec    *pspec,
                                HdyDemoWindow *self)
{
  HdyComboRow *row = HDY_COMBO_ROW (sender);

  g_assert (HDY_IS_COMBO_ROW (row));
  g_assert (HDY_IS_DEMO_WINDOW (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->carousel_box),
                                  1 - hdy_combo_row_get_selected (row));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->carousel),
                                  hdy_combo_row_get_selected (row));
}

static gchar *
carousel_indicators_name (GtkStringObject *value)
{
  const gchar *style;

  g_assert (GTK_IS_STRING_OBJECT (value));

  style = gtk_string_object_get_string (value);

  if (!g_strcmp0 (style, "dots"))
    return g_strdup (_("Dots"));

  if (!g_strcmp0 (style, "lines"))
    return g_strdup (_("Lines"));

  return NULL;
}

static void
notify_carousel_indicators_cb (GObject       *sender,
                               GParamSpec    *pspec,
                               HdyDemoWindow *self)
{
  HdyComboRow *row = HDY_COMBO_ROW (sender);
  GtkStringObject *obj;

  g_assert (HDY_IS_COMBO_ROW (row));
  g_assert (HDY_IS_DEMO_WINDOW (self));

  obj = hdy_combo_row_get_selected_item (row);

  gtk_stack_set_visible_child_name (self->carousel_indicators_stack,
                                    gtk_string_object_get_string (obj));
}

static void
carousel_return_clicked_cb (GtkButton     *btn,
                            HdyDemoWindow *self)
{
  hdy_carousel_scroll_to (self->carousel,
                          hdy_carousel_get_nth_page (self->carousel, 0));
}

HdyDemoWindow *
hdy_demo_window_new (GtkApplication *application)
{
  return g_object_new (HDY_TYPE_DEMO_WINDOW, "application", application, NULL);
}

static void
avatar_file_remove_cb (HdyDemoWindow *self)
{
  g_assert (HDY_IS_DEMO_WINDOW (self));

  g_signal_handlers_disconnect_by_data (self->avatar, self);

  gtk_label_set_label (self->avatar_file_chooser_label, _("(None)"));
  gtk_widget_set_sensitive (GTK_WIDGET (self->avatar_remove_button), FALSE);
  hdy_avatar_set_image_load_func (self->avatar, NULL, NULL, NULL);
}

static GdkPixbuf *
avatar_load_file (gint size, HdyDemoWindow *self)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GFile) file = NULL;
  g_autofree gchar *filename = NULL;
  gint width, height;

  g_assert (HDY_IS_DEMO_WINDOW (self));

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (self->avatar_file_chooser));
  filename = g_file_get_path (file);

  gdk_pixbuf_get_file_info (filename, &width, &height);

  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename,
                                              (width <= height) ? size : -1,
                                              (width >= height) ? size : -1,
                                              TRUE,
                                              &error);
  if (error != NULL) {
    g_critical ("Failed to create pixbuf from file: %s", error->message);

    return NULL;
  }

  return g_steal_pointer (&pixbuf);
}

static void
avatar_file_chooser_response_cb (HdyDemoWindow *self,
                                 gint           response)
{
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFileInfo) info = NULL;

  g_assert (HDY_IS_DEMO_WINDOW (self));

  if (response != GTK_RESPONSE_ACCEPT)
    return;

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (self->avatar_file_chooser));
  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                            G_FILE_QUERY_INFO_NONE,
                            NULL,
                            NULL);

  if (info)
    gtk_label_set_label (self->avatar_file_chooser_label,
                         g_file_info_get_display_name (info));

  gtk_widget_set_sensitive (GTK_WIDGET (self->avatar_remove_button), TRUE);
  hdy_avatar_set_image_load_func (self->avatar, (HdyAvatarImageLoadFunc) avatar_load_file, self, NULL);
}

static void
avatar_file_chooser_clicked_cb (HdyDemoWindow *self)
{
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (self->avatar_file_chooser));
}

static void
file_chooser_response_cb (HdyDemoWindow  *self,
                          gint            response_id,
                          GtkFileChooser *chooser)
{
  if (response_id == GTK_RESPONSE_ACCEPT) {
    g_autoptr (GFile) file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
    g_autoptr (GdkPixbuf) pixbuf =
      hdy_avatar_draw_to_pixbuf (self->avatar,
                                 hdy_avatar_get_size (self->avatar),
                                 gtk_widget_get_scale_factor (GTK_WIDGET (self)));

    if (pixbuf != NULL)
      gdk_pixbuf_save (pixbuf, g_file_get_path (file), "png", NULL, NULL);
  }

  g_object_unref (chooser);
}

static void
avatar_save_to_file_cb (HdyDemoWindow *self)
{
  GtkFileChooserNative *chooser = NULL;

  g_assert (HDY_IS_DEMO_WINDOW (self));

  chooser = gtk_file_chooser_native_new (_("Save Avatar"),
                                         GTK_WINDOW (self),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         _("_Save"),
                                         _("_Cancel"));

  g_signal_connect_swapped (chooser, "response", G_CALLBACK (file_chooser_response_cb), self);

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (chooser));
}

static gchar *
avatar_new_random_name (void)
{
  static const char *first_names[] = {
    "Adam",
    "Adrian",
    "Anna",
    "Charlotte",
    "Frédérique",
    "Ilaria",
    "Jakub",
    "Jennyfer",
    "Julia",
    "Justin",
    "Mario",
    "Miriam",
    "Mohamed",
    "Nourimane",
    "Owen",
    "Peter",
    "Petra",
    "Rachid",
    "Rebecca",
    "Sarah",
    "Thibault",
    "Wolfgang",
  };
  static const char *last_names[] = {
    "Bailey",
    "Berat",
    "Chen",
    "Farquharson",
    "Ferber",
    "Franco",
    "Galinier",
    "Han",
    "Lawrence",
    "Lepied",
    "Lopez",
    "Mariotti",
    "Rossi",
    "Urasawa",
    "Zwickelman",
  };

  return g_strdup_printf ("%s %s",
                          first_names[g_random_int_range (0, G_N_ELEMENTS (first_names))],
                          last_names[g_random_int_range (0, G_N_ELEMENTS (last_names))]);
}

static void
avatar_update_contacts (HdyDemoWindow *self)
{
  GtkWidget *row;

  while ((row = gtk_widget_get_first_child (GTK_WIDGET (self->avatar_contacts))))
    gtk_list_box_remove (self->avatar_contacts, row);

  for (int i = 0; i < 30; i++) {
    g_autofree gchar *name = avatar_new_random_name ();
    GtkWidget *contact = hdy_action_row_new ();
    GtkWidget *avatar = hdy_avatar_new (40, name, TRUE);

    gtk_widget_set_margin_top (avatar, 12);
    gtk_widget_set_margin_bottom (avatar, 12);

    hdy_preferences_row_set_title (HDY_PREFERENCES_ROW (contact), name);
    hdy_action_row_add_prefix (HDY_ACTION_ROW (contact), avatar);
    gtk_list_box_append (self->avatar_contacts, contact);
  }
}

static void
flap_demo_clicked_cb (GtkButton     *btn,
                      HdyDemoWindow *self)
{
  HdyFlapDemoWindow *window = hdy_flap_demo_window_new ();

  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (self));
  gtk_window_present (GTK_WINDOW (window));
}

static void
hdy_demo_window_class_init (HdyDemoWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_q, GDK_CONTROL_MASK, "window.close", NULL);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/Handy/Demo/ui/hdy-demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, content_box);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, right_box);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, header_stack);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, theme_variant);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, sidebar);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, leaflet_transition_row);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, content_leaflet);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, lists_listbox);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, carousel);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, carousel_box);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, carousel_listbox);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, carousel_indicators_stack);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, avatar);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, avatar_text);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, avatar_file_chooser_label);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, avatar_remove_button);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoWindow, avatar_contacts);
  gtk_widget_class_bind_template_callback (widget_class, notify_visible_child_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_leaflet_visible_child_cb);
  gtk_widget_class_bind_template_callback (widget_class, back_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, leaflet_back_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, leaflet_transition_name);
  gtk_widget_class_bind_template_callback (widget_class, notify_leaflet_transition_cb);
  gtk_widget_class_bind_template_callback (widget_class, leaflet_go_next_row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, theme_variant_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, view_switcher_demo_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_carousel_orientation_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_carousel_indicators_cb);
  gtk_widget_class_bind_template_callback (widget_class, carousel_indicators_name);
  gtk_widget_class_bind_template_callback (widget_class, carousel_orientation_name);
  gtk_widget_class_bind_template_callback (widget_class, carousel_return_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, avatar_file_remove_cb);
  gtk_widget_class_bind_template_callback (widget_class, avatar_file_chooser_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, avatar_save_to_file_cb);
  gtk_widget_class_bind_template_callback (widget_class, flap_demo_clicked_cb);
}

static void
avatar_page_init (HdyDemoWindow *self)
{
  g_autofree gchar *name = avatar_new_random_name ();

  gtk_editable_set_text (GTK_EDITABLE (self->avatar_text), name);

  avatar_update_contacts (self);

  self->avatar_file_chooser =
    gtk_file_chooser_native_new (_("Select an Avatar"),
                                 GTK_WINDOW (self),
                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                 _("_Select"),
                                 _("_Cancel"));

  gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (self->avatar_file_chooser), TRUE);
  g_signal_connect_object (self->avatar_file_chooser, "response",
                           G_CALLBACK (avatar_file_chooser_response_cb), self,
                           G_CONNECT_SWAPPED);

  avatar_file_remove_cb (self);
}

static void
hdy_demo_window_init (HdyDemoWindow *self)
{
  GtkSettings *settings = gtk_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property_full (settings, "gtk-application-prefer-dark-theme",
                               self->theme_variant, "icon-name",
                               G_BINDING_SYNC_CREATE,
                               prefer_dark_theme_to_icon_name_cb,
                               NULL,
                               NULL,
                               NULL);

  avatar_page_init (self);
  update (self);

  hdy_leaflet_set_visible_child (self->content_box, GTK_WIDGET (self->right_box));
}
