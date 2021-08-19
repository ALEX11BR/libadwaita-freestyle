/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "adw-tab-button.h"

/* Copied from GtkInspector code */
#define XFT_DPI_MULTIPLIER (96.0 * PANGO_SCALE)

/**
 * AdwTabButton:
 *
 * A button that displays the number of [class@Adw.TabView] pages.
 *
 * The `AdwTabButton` widget is a button that displays the number of pages in a
 * given `AdwTabView`.
 *
 * It can be used to open a tab switcher view in a mobile UI.
 *
 * ## CSS nodes
 *
 * `AdwTabButton` has a main CSS node with name tabbutton.
 *
 * It contains the subnode `button`, which contains the subnode `overlay`, which
 * contains nodes `image` and `label`. The `label` subnode can have the `.small`
 * style class for 10 or more tabs.
 *
 * # Accessibility
 *
 * `GtkMenuButton` uses the %GTK_ACCESSIBLE_ROLE_BUTTON role.
 *
 * Since: 1.0
 */

struct _AdwTabButton
{
  GtkWidget parent_instance;

  GtkWidget *button;
  GtkLabel *label;
  GtkImage *icon;

  AdwTabView *view;
};

static void adw_tab_button_actionable_init (GtkActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (AdwTabButton, adw_tab_button, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ACTIONABLE, adw_tab_button_actionable_init))

enum {
  PROP_0,
  PROP_VIEW,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};

static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_CLICKED,
  SIGNAL_ACTIVATE,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
clicked_cb (AdwTabButton *self)
{
  g_signal_emit (self, signals[SIGNAL_CLICKED], 0);
}

static void
activate_cb (AdwTabButton *self)
{
  g_signal_emit_by_name (self->button, "activate");
}

/* FIXME: I hope there is a better way to prevent the label from changing scale */
static void
update_label_scale (AdwTabButton *self,
                    GtkSettings  *settings)
{
  gint xft_dpi;
  PangoAttrList *attrs;
  PangoAttribute *scale_attribute;

  g_object_get (settings, "gtk-xft-dpi", &xft_dpi, NULL);

  attrs = pango_attr_list_new ();

  scale_attribute = pango_attr_scale_new (XFT_DPI_MULTIPLIER / (gdouble) xft_dpi);

  pango_attr_list_change (attrs, scale_attribute);

  gtk_label_set_attributes (self->label, attrs);

  pango_attr_list_unref (attrs);
}

static void
xft_dpi_changed (AdwTabButton *self,
                 GParamSpec   *pspec,
                 GtkSettings  *settings)
{
  update_label_scale (self, settings);
}

static void
update_icon (AdwTabButton *self)
{
  gboolean display_label = FALSE;
  gboolean small_label = FALSE;
  const gchar *icon_name = "adw-tab-counter-symbolic";
  g_autofree gchar *label_text = NULL;
  GtkStyleContext *context;

  if (self->view) {
    guint n_pages = adw_tab_view_get_n_pages (self->view);

    small_label = n_pages >= 10;

    if (n_pages < 100) {
      display_label = TRUE;
      label_text = g_strdup_printf ("%u", n_pages);
    } else {
      icon_name = "adw-tab-overflow-symbolic";
    }
  }

  context = gtk_widget_get_style_context (GTK_WIDGET (self->label));

  if (small_label)
    gtk_style_context_add_class (context, "small");
  else
    gtk_style_context_remove_class (context, "small");

  gtk_widget_set_visible (GTK_WIDGET (self->label), display_label);
  gtk_label_set_text (self->label, label_text);
  gtk_image_set_from_icon_name (self->icon, icon_name);
}

static void
adw_tab_button_dispose (GObject *object)
{
  AdwTabButton *self = ADW_TAB_BUTTON (object);

  adw_tab_button_set_view (self, NULL);

  gtk_widget_unparent (GTK_WIDGET (self->button));

  G_OBJECT_CLASS (adw_tab_button_parent_class)->dispose (object);
}

static void
adw_tab_button_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  AdwTabButton *self = ADW_TAB_BUTTON (object);

  switch (prop_id) {
  case PROP_VIEW:
    g_value_set_object (value, adw_tab_button_get_view (self));
    break;
  case PROP_ACTION_NAME:
    g_value_set_string (value, gtk_actionable_get_action_name (GTK_ACTIONABLE (self)));
    break;
  case PROP_ACTION_TARGET:
    g_value_set_variant (value, gtk_actionable_get_action_target_value (GTK_ACTIONABLE (self)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_tab_button_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  AdwTabButton *self = ADW_TAB_BUTTON (object);

  switch (prop_id) {
  case PROP_VIEW:
    adw_tab_button_set_view (self, g_value_get_object (value));
    break;
  case PROP_ACTION_NAME:
    gtk_actionable_set_action_name (GTK_ACTIONABLE (self), g_value_get_string (value));
    break;
  case PROP_ACTION_TARGET:
    gtk_actionable_set_action_target_value (GTK_ACTIONABLE (self), g_value_get_variant (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_tab_button_class_init (AdwTabButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_tab_button_dispose;
  object_class->get_property = adw_tab_button_get_property;
  object_class->set_property = adw_tab_button_set_property;

  /**
   * AdwTabButton:view: (attributes org.gtk.Property.get=adw_tab_button_get_view org.gtk.Property.set=adw_tab_button_set_view)
   *
   * The view the tab button displays.
   *
   * Since: 1.0
   */
  props[PROP_VIEW] =
    g_param_spec_object ("view",
                         _("View"),
                         _("The view the tab button displays"),
                         ADW_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");

  /**
   * AdwTabButton::clicked:
   * @self: the object that received the signal
   *
   * Emitted when the button has been activated (pressed and released).
   */
  signals[SIGNAL_CLICKED] =
    g_signal_new ("clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * AdwTabButton::activate:
   * @self: the object which received the signal.
   *
   * Emitted to animate press then release.
   *
   * This is an action signal. Applications should never connect
   * to this signal, but use the [signal@Adw.TabButton::clicked] signal.
   */
  signals[SIGNAL_ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  gtk_widget_class_set_activate_signal (widget_class, signals[SIGNAL_ACTIVATE]);

  g_signal_override_class_handler ("activate",
                                   G_TYPE_FROM_CLASS (klass),
                                   G_CALLBACK (activate_cb));

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Adwaita/ui/adw-tab-button.ui");

  gtk_widget_class_bind_template_child (widget_class, AdwTabButton, button);
  gtk_widget_class_bind_template_child (widget_class, AdwTabButton, label);
  gtk_widget_class_bind_template_child (widget_class, AdwTabButton, icon);
  gtk_widget_class_bind_template_callback (widget_class, clicked_cb);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "tabbutton");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_BUTTON);
}

static void
adw_tab_button_init (AdwTabButton *self)
{
  GtkSettings *settings;

  gtk_widget_init_template (GTK_WIDGET (self));

  update_icon (self);

  settings = gtk_widget_get_settings (GTK_WIDGET (self));

  update_label_scale (self, settings);
  g_signal_connect_object (settings, "notify::gtk-xft-dpi",
                           G_CALLBACK (xft_dpi_changed), self,
                           G_CONNECT_SWAPPED);
}

static const char *
adw_tab_button_get_action_name (GtkActionable *actionable)
{
  AdwTabButton *self = ADW_TAB_BUTTON (actionable);

  return gtk_actionable_get_action_name (GTK_ACTIONABLE (self->button));
}

static void
adw_tab_button_set_action_name (GtkActionable *actionable,
                                const char    *action_name)
{
  AdwTabButton *self = ADW_TAB_BUTTON (actionable);

  return gtk_actionable_set_action_name (GTK_ACTIONABLE (self->button),
                                         action_name);
}

static GVariant *
adw_tab_button_get_action_target_value (GtkActionable *actionable)
{
  AdwTabButton *self = ADW_TAB_BUTTON (actionable);

  return gtk_actionable_get_action_target_value (GTK_ACTIONABLE (self->button));
}

static void
adw_tab_button_set_action_target_value (GtkActionable *actionable,
                                        GVariant      *action_target)
{
  AdwTabButton *self = ADW_TAB_BUTTON (actionable);

  return gtk_actionable_set_action_target_value (GTK_ACTIONABLE (self->button),
                                                 action_target);
}

static void
adw_tab_button_actionable_init (GtkActionableInterface *iface)
{
  iface->get_action_name = adw_tab_button_get_action_name;
  iface->set_action_name = adw_tab_button_set_action_name;
  iface->get_action_target_value = adw_tab_button_get_action_target_value;
  iface->set_action_target_value = adw_tab_button_set_action_target_value;
}

/**
 * adw_tab_button_new:
 *
 * Creates a new `AdwTabButton`.
 *
 * Returns: the newly created `AdwTabButton`
 *
 * Since: 1.0
 */
GtkWidget *
adw_tab_button_new (void)
{
  return g_object_new (ADW_TYPE_TAB_BUTTON, NULL);
}

/**
 * adw_tab_button_get_view: (attributes org.gtk.Method.get_property=view)
 * @self: a `AdwTabButton`
 *
 * Gets the tab view @self displays.
 *
 * Returns: (transfer none) (nullable): the tab view
 *
 * Since: 1.0
 */
AdwTabView *
adw_tab_button_get_view (AdwTabButton *self)
{
  g_return_val_if_fail (ADW_IS_TAB_BUTTON (self), NULL);

  return self->view;
}

/**
 * adw_tab_button_set_view: (attributes org.gtk.Method.set_property=view)
 * @self: a `AdwTabButton`
 * @view: (nullable): a tab view
 *
 * Sets the tab view to display.
 *
 * Since: 1.0
 */
void
adw_tab_button_set_view (AdwTabButton *self,
                         AdwTabView   *view)
{
  g_return_if_fail (ADW_IS_TAB_BUTTON (self));
  g_return_if_fail (view == NULL || ADW_IS_TAB_VIEW (view));

  if (self->view == view)
    return;

  if (self->view)
    g_signal_handlers_disconnect_by_func (self->view,
                                          update_icon,
                                          self);

  g_set_object (&self->view, view);

  if (self->view)
    g_signal_connect_object (self->view, "notify::n-pages",
                             G_CALLBACK (update_icon), self,
                             G_CONNECT_SWAPPED);

  update_icon (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VIEW]);
}
