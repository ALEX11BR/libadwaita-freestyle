/*
 * Copyright (C) 2020-2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include "adw-tab-item-private.h"

typedef struct {
  GtkDropTarget *drop_target;

  AdwTabView *view;
  AdwTabPage *page;
  gboolean pinned;
  gboolean hovering;
  gboolean dragging;
  gboolean fully_visible;
  int display_width;
  gboolean inverted;
} AdwTabItemPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (AdwTabItem, adw_tab_item, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_VIEW,
  PROP_PAGE,
  PROP_PINNED,
  PROP_HOVERING,
  PROP_DRAGGING,
  PROP_FULLY_VISIBLE,
  PROP_DISPLAY_WIDTH,
  PROP_INVERTED,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_EXTRA_DRAG_DROP,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static gboolean
close_idle_cb (AdwTabItem *self)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);

  adw_tab_view_close_page (priv->view, priv->page);

  return G_SOURCE_REMOVE;
}

static void
motion_cb (AdwTabItem         *self,
           double              x,
           double              y,
           GtkEventController *controller)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);
  GdkDevice *device = gtk_event_controller_get_current_event_device (controller);
  GdkInputSource input_source = gdk_device_get_source (device);

  if (input_source == GDK_SOURCE_TOUCHSCREEN)
    return;

  if (priv->hovering)
    return;

  priv->hovering = TRUE;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOVERING]);
}

static void
leave_cb (AdwTabItem         *self,
          GtkEventController *controller)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);

  if (priv->hovering)
    return;

  priv->hovering = FALSE;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOVERING]);
}

static gboolean
drop_cb (AdwTabItem *self,
         GValue     *value)
{
  gboolean ret = GDK_EVENT_PROPAGATE;

  g_signal_emit (self, signals[SIGNAL_EXTRA_DRAG_DROP], 0, value, &ret);

  return ret;
}

static gboolean
activate_cb (AdwTabItem *self,
             GVariant   *args)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);
  GtkWidget *child;

  if (!priv->page || !priv->view)
    return GDK_EVENT_PROPAGATE;

  child = adw_tab_page_get_child (priv->page);

  gtk_widget_grab_focus (child);

  return GDK_EVENT_STOP;
}

static void
adw_tab_item_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  AdwTabItem *self = ADW_TAB_ITEM (object);

  switch (prop_id) {
  case PROP_VIEW:
    g_value_set_object (value, adw_tab_item_get_view (self));
    break;

  case PROP_PAGE:
    g_value_set_object (value, adw_tab_item_get_page (self));
    break;

  case PROP_PINNED:
    g_value_set_boolean (value, adw_tab_item_get_pinned (self));
    break;

  case PROP_HOVERING:
    g_value_set_boolean (value, adw_tab_item_get_hovering (self));
    break;

  case PROP_DRAGGING:
    g_value_set_boolean (value, adw_tab_item_get_dragging (self));
    break;

  case PROP_FULLY_VISIBLE:
    g_value_set_boolean (value, adw_tab_item_get_fully_visible (self));
    break;

  case PROP_DISPLAY_WIDTH:
    g_value_set_int (value, adw_tab_item_get_display_width (self));
    break;

  case PROP_INVERTED:
    g_value_set_boolean (value, adw_tab_item_get_inverted (self));
    break;

    default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_tab_item_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  AdwTabItem *self = ADW_TAB_ITEM (object);
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);

  switch (prop_id) {
  case PROP_VIEW:
    priv->view = g_value_get_object (value);
    break;

  case PROP_PAGE:
    adw_tab_item_set_page (self, g_value_get_object (value));
    break;

  case PROP_PINNED:
    priv->pinned = g_value_get_boolean (value);
    break;

  case PROP_DRAGGING:
    adw_tab_item_set_dragging (self, g_value_get_boolean (value));
    break;

  case PROP_FULLY_VISIBLE:
    adw_tab_item_set_fully_visible (self, g_value_get_boolean (value));
    break;

  case PROP_DISPLAY_WIDTH:
    adw_tab_item_set_display_width (self, g_value_get_int (value));
    break;

  case PROP_INVERTED:
    adw_tab_item_set_inverted (self, g_value_get_boolean (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_tab_item_dispose (GObject *object)
{
  AdwTabItem *self = ADW_TAB_ITEM (object);

  adw_tab_item_set_page (self, NULL);

  G_OBJECT_CLASS (adw_tab_item_parent_class)->dispose (object);
}

static void
adw_tab_item_class_init (AdwTabItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_tab_item_dispose;
  object_class->get_property = adw_tab_item_get_property;
  object_class->set_property = adw_tab_item_set_property;

  props[PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "View",
                         ADW_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_PAGE] =
    g_param_spec_object ("page",
                         "Page",
                         "Page",
                         ADW_TYPE_TAB_PAGE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_PINNED] =
    g_param_spec_boolean ("pinned",
                          "Pinned",
                          "Pinned",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_HOVERING] =
    g_param_spec_boolean ("hovering",
                          "Hovering",
                          "Hovering",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_DRAGGING] =
    g_param_spec_boolean ("dragging",
                          "Dragging",
                          "Dragging",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_FULLY_VISIBLE] =
    g_param_spec_boolean ("fully-visible",
                          "Fully visible",
                          "Fully visible",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_DISPLAY_WIDTH] =
    g_param_spec_int ("display-width",
                      "Display Width",
                      "Display Width",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_INVERTED] =
    g_param_spec_boolean ("inverted",
                          "Inverted",
                          "Inverted",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  signals[SIGNAL_EXTRA_DRAG_DROP] =
    g_signal_new ("extra-drag-drop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  g_signal_accumulator_first_wins, NULL, NULL,
                  G_TYPE_BOOLEAN,
                  1,
                  G_TYPE_VALUE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_add_binding (widget_class, GDK_KEY_space,     0, (GtkShortcutFunc) activate_cb, NULL);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_KP_Space,  0, (GtkShortcutFunc) activate_cb, NULL);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_Return,    0, (GtkShortcutFunc) activate_cb, NULL);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_ISO_Enter, 0, (GtkShortcutFunc) activate_cb, NULL);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_KP_Enter,  0, (GtkShortcutFunc) activate_cb, NULL);
}

static void
adw_tab_item_init (AdwTabItem *self)
{
  GtkEventController *controller = gtk_event_controller_motion_new ();
  g_signal_connect_swapped (controller, "motion", G_CALLBACK (motion_cb), self);
  g_signal_connect_swapped (controller, "leave", G_CALLBACK (leave_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), controller);
}

AdwTabView *
adw_tab_item_get_view (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), NULL);

  priv = adw_tab_item_get_instance_private (self);

  return priv->view;
}

AdwTabPage *
adw_tab_item_get_page (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), NULL);

  priv = adw_tab_item_get_instance_private (self);

  return priv->page;
}

void
adw_tab_item_set_page (AdwTabItem *self,
                       AdwTabPage *page)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));
  g_return_if_fail (page == NULL || ADW_IS_TAB_PAGE (page));

  priv = adw_tab_item_get_instance_private (self);

  if (priv->page == page)
    return;

  if (priv->page)
    ADW_TAB_ITEM_GET_CLASS (self)->disconnect_page (self);

  g_set_object (&priv->page, page);

  if (priv->page)
    ADW_TAB_ITEM_GET_CLASS (self)->connect_page (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PAGE]);
}

gboolean
adw_tab_item_get_pinned (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), FALSE);

  priv = adw_tab_item_get_instance_private (self);

  return priv->pinned;
}

int
adw_tab_item_get_display_width (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), 0);

  priv = adw_tab_item_get_instance_private (self);

  return priv->display_width;
}

void
adw_tab_item_set_display_width (AdwTabItem *self,
                                int         width)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));
  g_return_if_fail (width >= 0);

  priv = adw_tab_item_get_instance_private (self);

  if (priv->display_width == width)
    return;

  priv->display_width = width;

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DISPLAY_WIDTH]);
}

gboolean
adw_tab_item_get_hovering (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), FALSE);

  priv = adw_tab_item_get_instance_private (self);

  return priv->hovering;
}

gboolean
adw_tab_item_get_dragging (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), FALSE);

  priv = adw_tab_item_get_instance_private (self);

  return priv->dragging;
}

void
adw_tab_item_set_dragging (AdwTabItem *self,
                           gboolean    dragging)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));

  priv = adw_tab_item_get_instance_private (self);

  dragging = !!dragging;

  if (priv->dragging == dragging)
    return;

  priv->dragging = dragging;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DRAGGING]);
}

gboolean
adw_tab_item_get_inverted (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), FALSE);

  priv = adw_tab_item_get_instance_private (self);

  return priv->inverted;
}

void
adw_tab_item_set_inverted (AdwTabItem *self,
                           gboolean    inverted)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));

  priv = adw_tab_item_get_instance_private (self);

  inverted = !!inverted;

  if (priv->inverted == inverted)
    return;

  priv->inverted = inverted;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INVERTED]);
}

gboolean
adw_tab_item_get_fully_visible (AdwTabItem *self)
{
  AdwTabItemPrivate *priv;

  g_return_val_if_fail (ADW_IS_TAB_ITEM (self), FALSE);

  priv = adw_tab_item_get_instance_private (self);

  return priv->fully_visible;
}

void
adw_tab_item_set_fully_visible (AdwTabItem *self,
                                gboolean    fully_visible)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));

  priv = adw_tab_item_get_instance_private (self);

  fully_visible = !!fully_visible;

  if (priv->fully_visible == fully_visible)
    return;

  priv->fully_visible = fully_visible;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FULLY_VISIBLE]);
}

void
adw_tab_item_setup_extra_drop_target (AdwTabItem    *self,
                                      GdkDragAction  actions,
                                      GType         *types,
                                      gsize          n_types)
{
  AdwTabItemPrivate *priv;

  g_return_if_fail (ADW_IS_TAB_ITEM (self));
  g_return_if_fail (n_types == 0 || types != NULL);

  priv = adw_tab_item_get_instance_private (self);

  if (!priv->drop_target) {
    priv->drop_target = gtk_drop_target_new (G_TYPE_INVALID, actions);
    g_signal_connect_swapped (priv->drop_target, "drop", G_CALLBACK (drop_cb), self);
    gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (priv->drop_target));
  } else {
    gtk_drop_target_set_actions (priv->drop_target, actions);
  }

  gtk_drop_target_set_gtypes (priv->drop_target, types, n_types);
}

void
adw_tab_item_close (AdwTabItem *self)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);

  if (!priv->page)
    return;

  /* When animations are disabled, we don't want to immediately remove the
   * whole tab mid-click. Instead, defer it until the click has happened.
   */
  g_idle_add ((GSourceFunc) close_idle_cb, self);
}

void
adw_tab_item_activate_indicator (AdwTabItem *self)
{
  AdwTabItemPrivate *priv = adw_tab_item_get_instance_private (self);

  if (!priv->page)
    return;

  g_signal_emit_by_name (priv->view, "indicator-activated", priv->page);
}
