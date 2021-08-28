/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "adw-tab-overview.h"

#include "adw-animation-private.h"
#include "adw-tab-grid-private.h"
#include "adw-widget-utils-private.h"

#define TRANSITION_DURATION 400
#define OFFSET_FACTOR 0.1f
#define THUMBNAIL_BORDER_RADIUS 8

/**
 * AdwTabOverview:
 *
 * Since: 1.0
 */

struct _AdwTabOverview
{
  GtkWidget parent_instance;

  GtkWidget *child;
  GtkWidget *overview;
  AdwTabView *view;

  AdwTabListBase *grid;

  gboolean is_open;
  AdwAnimation *open_animation;
  gdouble progress;

  GtkWidget *hidden_thumbnail;
  GdkPaintable *transition_paintable;

  GskGLShader *shader;
  gboolean shader_compiled;
};

static void adw_tab_overview_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AdwTabOverview, adw_tab_overview, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_tab_overview_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_VIEW,
  PROP_CHILD,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_EXTRA_DRAG_DROP,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static gboolean
extra_drag_drop_cb (AdwTabOverview *self,
                    AdwTabPage     *page,
                    GValue         *value)
{
  gboolean ret = GDK_EVENT_PROPAGATE;

  g_signal_emit (self, signals[SIGNAL_EXTRA_DRAG_DROP], 0, page, value, &ret);

  return ret;
}

static void
view_destroy_cb (AdwTabOverview *self)
{
  adw_tab_overview_set_view (self, NULL);
}

static void
notify_selected_page_cb (AdwTabOverview *self)
{
  AdwTabPage *page = adw_tab_view_get_selected_page (self->view);

  if (!page)
    return;

  adw_tab_list_base_select_page (self->grid, page);
}

static void
notify_pinned_cb (AdwTabPage     *page,
                  GParamSpec     *pspec,
                  AdwTabOverview *self)
{
}

static void
page_attached_cb (AdwTabOverview *self,
                  AdwTabPage     *page,
                  int             position)
{
  g_signal_connect_object (page, "notify::pinned",
                           G_CALLBACK (notify_pinned_cb), self,
                           0);
}

static void
page_detached_cb (AdwTabOverview *self,
                  AdwTabPage     *page,
                  int             position)
{
  g_signal_handlers_disconnect_by_func (page, notify_pinned_cb, self);
}

static void
open_animation_value_cb (double          value,
                         AdwTabOverview *self)
{
  self->progress = value;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
open_animation_done_cb (AdwTabOverview *self)
{
  g_clear_object (&self->open_animation);

  gtk_widget_set_opacity (self->hidden_thumbnail, 1);
  g_clear_object (&self->hidden_thumbnail);
  g_clear_object (&self->transition_paintable);

  if (!self->is_open) {
//    grid.did_close ();
    gtk_widget_hide (self->overview);
    gtk_widget_set_can_target (self->overview, FALSE);
  }
}

static void
set_open (AdwTabOverview *self,
          gboolean        is_open)
{
  AdwTabPage *selected_page;
  AdwTabGrid *grid;
  GtkPicture *picture;

  self->is_open = is_open;

  if (is_open || !self->open_animation) {
      if (self->open_animation)
        adw_animation_stop (self->open_animation);

      gtk_widget_show (self->overview);
      gtk_widget_set_can_target (self->overview, TRUE);
  }

//  if (is_open)
//    grid.will_open ();

  selected_page = adw_tab_view_get_selected_page (self->view);

  picture = adw_tab_grid_get_transition_picture (ADW_TAB_GRID (self->grid));

  self->hidden_thumbnail = g_object_ref (GTK_WIDGET (picture));
  gtk_widget_set_opacity (self->hidden_thumbnail, 0);

  self->transition_paintable = g_object_ref (gtk_picture_get_paintable (picture));

  self->open_animation =
    adw_animation_new (GTK_WIDGET (self),
                       self->progress,
                       is_open ? 1 : 0,
                       TRANSITION_DURATION,
                       (AdwAnimationTargetFunc) open_animation_value_cb,
                       self);

  g_signal_connect_swapped (self->open_animation, "done", G_CALLBACK (open_animation_done_cb), self);

  adw_animation_start (self->open_animation);
}

static void
ensure_shader (AdwTabOverview *self)
{
  GtkNative *native;
  GskRenderer *renderer;
  g_autoptr (GError) error = NULL;

  if (self->shader)
    return;

  self->shader = gsk_gl_shader_new_from_resource ("/org/gnome/Adwaita/glsl/tab-overview.glsl");

  native = gtk_widget_get_native (GTK_WIDGET (self));
  renderer = gtk_native_get_renderer (native);

  self->shader_compiled = gsk_gl_shader_compile (self->shader, renderer, &error);

  if (error) {
    /* If shaders aren't supported, the error doesn't matter and we just
     * silently fall back */
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
      g_critical ("Couldn't compile shader: %s\n", error->message);
  }
}

static void
adw_tab_overview_snapshot (GtkWidget   *widget,
                           GtkSnapshot *snapshot)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (widget);

  if (self->open_animation) {
    graphene_rect_t view_bounds, thumbnail_bounds, transition_bounds;
    graphene_point_t view_center;
    int width, height;
    float scale;
    GskRoundedRect transition_rect;
    GdkRGBA borders_color;
    GtkSnapshot *child_snapshot;
    g_autoptr (GskRenderNode) child_node = NULL;

    ensure_shader (self);

    width = gtk_widget_get_width (widget);
    height = gtk_widget_get_height (widget);
    scale = 1 - OFFSET_FACTOR * (float) (1 - CLAMP (self->progress, 0, 1));

    if (!gtk_widget_compute_bounds (GTK_WIDGET (self->view), widget, &view_bounds))
      g_critical ("View must be inside the overview"); // TODO

    if (!gtk_widget_compute_bounds (self->hidden_thumbnail, widget, &thumbnail_bounds))
      graphene_rect_init (&thumbnail_bounds, 0, 0, 0, 0);

    graphene_rect_get_center (&view_bounds, &view_center);

    graphene_rect_interpolate (&view_bounds, &thumbnail_bounds,
                               self->progress, &transition_bounds);

    gsk_rounded_rect_init_from_rect (&transition_rect,
                                     &GRAPHENE_RECT_INIT (0, 0,
                                                          transition_bounds.size.width,
                                                          transition_bounds.size.height),
                                     (float) (THUMBNAIL_BORDER_RADIUS * self->progress));

    if (!gtk_style_context_lookup_color (gtk_widget_get_style_context (widget),
                                         "borders", &borders_color))
      borders_color.alpha = 0;

    /* Draw overview */
    gtk_snapshot_save (snapshot);
    gtk_snapshot_push_opacity (snapshot, self->progress);
    gtk_snapshot_translate (snapshot, &view_center);
    gtk_snapshot_scale (snapshot, scale, scale);
    gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-view_center.x, -view_center.y));
    gtk_widget_snapshot_child (widget, self->overview, snapshot);
    gtk_snapshot_pop (snapshot);
    gtk_snapshot_restore (snapshot);

    /* Draw the transition thumbnail. Unfortunately, since GTK widgets have
     * integer sizes, we can't use a real widget for this and have to custom
     * draw it instead. We also want to interpolate border-radius. */
    if (self->transition_paintable) {
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &transition_bounds.origin);
      gtk_snapshot_append_outset_shadow (snapshot, &transition_rect,
                                         &borders_color, 0, 0, 1, 0);
      gtk_snapshot_push_rounded_clip (snapshot, &transition_rect);
      gdk_paintable_snapshot (self->transition_paintable,
                              GDK_SNAPSHOT (snapshot),
                              transition_rect.bounds.size.width,
                              transition_rect.bounds.size.height);
      gtk_snapshot_pop (snapshot);
      gtk_snapshot_restore (snapshot);
    }

    /* Draw the child */
    scale += OFFSET_FACTOR;
    gtk_snapshot_save (snapshot);
    gtk_snapshot_translate (snapshot, &view_center);
    gtk_snapshot_scale (snapshot, scale, scale);
    gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-view_center.x, -view_center.y));

    child_snapshot = gtk_snapshot_new ();

    if (self->shader_compiled) {
      graphene_vec2_t origin, size;
      double opacity;

      opacity = 1 - CLAMP (self->progress, 0, 1);
      graphene_point_to_vec2 (&view_bounds.origin, &origin);
      graphene_vec2_init (&size, view_bounds.size.width, view_bounds.size.height);

      gtk_snapshot_push_gl_shader (child_snapshot,
                                   self->shader,
                                   &GRAPHENE_RECT_INIT (0, 0, width, height),
                                   gsk_gl_shader_format_args (self->shader,
                                                              "opacity", opacity,
                                                              "origin", &origin,
                                                              "size", &size,
                                                              NULL));
    } else {
      gtk_snapshot_push_opacity (child_snapshot, 1 - self->progress);
    }

    gtk_widget_snapshot_child (widget, self->child, child_snapshot);

    if (self->shader_compiled)
      gtk_snapshot_gl_shader_pop_texture (child_snapshot);

    gtk_snapshot_pop (child_snapshot);

    child_node = gtk_snapshot_free_to_node (child_snapshot);
    gtk_snapshot_append_node (snapshot, child_node);

    gtk_snapshot_restore (snapshot);

    return;
  }

  if (self->progress > 0.5)
    gtk_widget_snapshot_child (widget, self->overview, snapshot);

  if (!self->child)
    return;

  /* We don't want to actually draw the child, but we do need it
   * to redraw so that it can be displayed by the paintables */
  if (self->progress > 0.5) {
    g_autoptr (GtkSnapshot) child_snapshot = gtk_snapshot_new ();

    gtk_widget_snapshot_child (widget, self->child, child_snapshot);
  } else {
    gtk_widget_snapshot_child (widget, self->child, snapshot);
  }
}

static void
adw_tab_overview_unrealize (GtkWidget *widget)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (widget);

  GTK_WIDGET_CLASS (adw_tab_overview_parent_class)->unrealize (widget);

  g_clear_object (&self->shader);
}

static void
adw_tab_overview_dispose (GObject *object)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (object);

  adw_tab_overview_set_view (self, NULL);
  adw_tab_overview_set_child (self, NULL);

  gtk_widget_unparent (GTK_WIDGET (self->overview));

  G_OBJECT_CLASS (adw_tab_overview_parent_class)->dispose (object);
}

static void
adw_tab_overview_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (object);

  switch (prop_id) {
  case PROP_VIEW:
    g_value_set_object (value, adw_tab_overview_get_view (self));
    break;
  case PROP_CHILD:
    g_value_set_object (value, adw_tab_overview_get_child (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_tab_overview_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (object);

  switch (prop_id) {
  case PROP_VIEW:
    adw_tab_overview_set_view (self, g_value_get_object (value));
    break;
  case PROP_CHILD:
    adw_tab_overview_set_child (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
overview_open_cb (GtkWidget  *widget,
                  const char *action_name,
                  GVariant   *param)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (widget);

  adw_tab_overview_open (self);
}

static void
adw_tab_overview_class_init (AdwTabOverviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_tab_overview_dispose;
  object_class->get_property = adw_tab_overview_get_property;
  object_class->set_property = adw_tab_overview_set_property;

  widget_class->snapshot = adw_tab_overview_snapshot;
  widget_class->unrealize = adw_tab_overview_unrealize;
  widget_class->compute_expand = adw_widget_compute_expand;

  /**
   * AdwTabOverview:view: (attributes org.gtk.Property.get=adw_tab_overview_get_view org.gtk.Property.set=adw_tab_overview_set_view)
   *
   * The tab view the overview controls.
   *
   * Since: 1.0
   */
  props[PROP_VIEW] =
    g_param_spec_object ("view",
                         _("View"),
                         _("The tab view the overview controls"),
                         ADW_TYPE_TAB_VIEW,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * AdwTabOverview:child: (attributes org.gtk.Property.get=adw_tab_overview_get_child org.gtk.Property.set=adw_tab_overview_set_child)
   *
   * The child widget.
   *
   * Since: 1.0
   */
  props[PROP_CHILD] =
      g_param_spec_object ("child",
                           "Child",
                           "The child widget",
                           GTK_TYPE_WIDGET,
                           G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * AdwTabOverview::extra-drag-drop:
   * @self: a `AdwTabOverview`
   * @page: the page matching the tab the content was dropped onto
   * @value: the `GValue` being dropped
   *
   * This signal is emitted when content is dropped onto a tab.
   *
   * The content must be of one of the types set up via
   * [method@Adw.TabOverview.setup_extra_drop_target].
   *
   * See [signal@Gtk.DropTarget::drop].
   *
   * Returns: whether the drop was accepted for @page
   *
   * Since: 1.0
   */
  signals[SIGNAL_EXTRA_DRAG_DROP] =
    g_signal_new ("extra-drag-drop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  g_signal_accumulator_first_wins, NULL, NULL,
                  G_TYPE_BOOLEAN,
                  2,
                  ADW_TYPE_TAB_PAGE,
                  G_TYPE_VALUE);

  gtk_widget_class_install_action (widget_class, "overview.open", NULL, overview_open_cb);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Adwaita/ui/adw-tab-overview.ui");

  gtk_widget_class_bind_template_child (widget_class, AdwTabOverview, overview);
  gtk_widget_class_bind_template_child (widget_class, AdwTabOverview, grid);
  gtk_widget_class_bind_template_callback (widget_class, extra_drag_drop_cb);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "taboverview");
}

static void
adw_tab_overview_init (AdwTabOverview *self)
{
  g_type_ensure (ADW_TYPE_TAB_GRID);

  gtk_widget_init_template (GTK_WIDGET (self));

  adw_tab_grid_set_tab_overview (ADW_TAB_GRID (self->grid), self);
}

static void
adw_tab_overview_buildable_add_child (GtkBuildable *buildable,
                                      GtkBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  AdwTabOverview *self = ADW_TAB_OVERVIEW (buildable);

  if (!self->overview)
    parent_buildable_iface->add_child (buildable, builder, child, type);
  else if (GTK_IS_WIDGET (child))
    adw_tab_overview_set_child (self, GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
adw_tab_overview_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = adw_tab_overview_buildable_add_child;
}

/**
 * adw_tab_overview_new:
 *
 * Creates a new `AdwTabOverview`.
 *
 * Returns: the newly created `AdwTabOverview`
 *
 * Since: 1.0
 */
GtkWidget *
adw_tab_overview_new (void)
{
  return g_object_new (ADW_TYPE_TAB_OVERVIEW, NULL);
}

/**
 * adw_tab_overview_get_view: (attributes org.gtk.Method.get_property=view)
 * @self: a `AdwTabOverview`
 *
 * Gets the tab view @self controls.
 *
 * Returns: (transfer none) (nullable): the tab view
 *
 * Since: 1.0
 */
AdwTabView *
adw_tab_overview_get_view (AdwTabOverview *self)
{
  g_return_val_if_fail (ADW_IS_TAB_OVERVIEW (self), NULL);

  return self->view;
}

/**
 * adw_tab_overview_set_view: (attributes org.gtk.Method.set_property=view)
 * @self: a `AdwTabOverview`
 * @view: (nullable): a tab view
 *
 * Sets the tab view to control.
 *
 * Since: 1.0
 */
void
adw_tab_overview_set_view (AdwTabOverview *self,
                           AdwTabView     *view)
{
  g_return_if_fail (ADW_IS_TAB_OVERVIEW (self));
  g_return_if_fail (view == NULL || ADW_IS_TAB_VIEW (view));

  if (self->view == view)
    return;

  if (self->view) {
    int i, n;

    g_signal_handlers_disconnect_by_func (self->view, notify_selected_page_cb, self);
    g_signal_handlers_disconnect_by_func (self->view, page_attached_cb, self);
    g_signal_handlers_disconnect_by_func (self->view, page_detached_cb, self);
    g_signal_handlers_disconnect_by_func (self->view, view_destroy_cb, self);

    n = adw_tab_view_get_n_pages (self->view);

    for (i = 0; i < n; i++)
      page_detached_cb (self, adw_tab_view_get_nth_page (self->view, i), i);

    adw_tab_list_base_set_view (self->grid, NULL);
  }

  g_set_object (&self->view, view);

  if (self->view) {
    int i, n;

    adw_tab_list_base_set_view (self->grid, view);

    g_signal_connect_object (self->view, "notify::selected-page",
                             G_CALLBACK (notify_selected_page_cb), self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (self->view, "page-attached",
                             G_CALLBACK (page_attached_cb), self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (self->view, "page-detached",
                             G_CALLBACK (page_detached_cb), self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (self->view, "destroy",
                             G_CALLBACK (view_destroy_cb), self,
                             G_CONNECT_SWAPPED);

    n = adw_tab_view_get_n_pages (self->view);

    for (i = 0; i < n; i++)
      page_attached_cb (self, adw_tab_view_get_nth_page (self->view, i), i);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VIEW]);
}

/**
 * adw_tab_overview_get_child: (attributes org.gtk.Method.get_property=child)
 * @self: a `AdwTabOveview`
 *
 * Gets the child widget of @self.
 *
 * Returns: (nullable) (transfer none): the child widget of @self
 *
 * Since: 1.0
 */
GtkWidget *
adw_tab_overview_get_child (AdwTabOverview *self)
{
  g_return_val_if_fail (ADW_IS_TAB_OVERVIEW (self), NULL);

  return self->child;
}

/**
 * adw_tab_overview_set_child: (attributes org.gtk.Method.set_property=child)
 * @self: a `AdwTabOverview`
 * @child: (nullable): the child widget
 *
 * Sets the child widget of @self.
 *
 * Since: 1.0
 */
void
adw_tab_overview_set_child (AdwTabOverview *self,
                            GtkWidget      *child)
{
  g_return_if_fail (ADW_IS_TAB_OVERVIEW (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  if (self->child == child)
    return;

  if (self->child)
    gtk_widget_unparent (self->child);

  self->child = child;

  if (self->child)
    gtk_widget_insert_after (self->child, GTK_WIDGET (self), NULL);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);
}

/**
 * adw_tab_overview_open:
 * @self: a `AdwTabOverview`
 *
 * Opens the overview.
 *
 * Since: 1.0
 */
void
adw_tab_overview_open (AdwTabOverview *self)
{
  g_return_if_fail (ADW_IS_TAB_OVERVIEW (self));

  if (self->is_open)
    return;

  set_open (self, TRUE);
}

// TODO
void
adw_tab_overview_close (AdwTabOverview *self)
{
  g_return_if_fail (ADW_IS_TAB_OVERVIEW (self));

  if (!self->is_open)
    return;

  set_open (self, FALSE);
}

/**
 * adw_tab_overview_setup_extra_drop_target:
 * @self: a `AdwTabOverview`
 * @actions: the supported actions
 * @types: (nullable) (transfer none) (array length=n_types):
 *   all supported `GType`s that can be dropped
 * @n_types: number of @types
 *
 * Sets the supported types for this drop target.
 *
 * Sets up an extra drop target on tabs.
 *
 * This allows to drag arbitrary content onto tabs, for example URLs in a web
 * browser.
 *
 * If a tab is hovered for a certain period of time while dragging the content,
 * it will be automatically selected.
 *
 * The [signal@Adw.TabOverview::extra-drag-drop] signal can be used to handle the
 * drop.
 *
 * Since: 1.0
 */
void
adw_tab_overview_setup_extra_drop_target (AdwTabOverview *self,
                                          GdkDragAction   actions,
                                          GType          *types,
                                          gsize           n_types)
{
  g_return_if_fail (ADW_IS_TAB_OVERVIEW (self));
  g_return_if_fail (n_types == 0 || types != NULL);

  adw_tab_list_base_setup_extra_drop_target (self->grid, actions, types, n_types);
}
