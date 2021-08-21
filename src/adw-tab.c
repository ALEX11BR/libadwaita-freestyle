/*
 * Copyright (C) 2020-2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include "adw-tab-private.h"

#include "adw-animation-util-private.h"
#include "adw-animation-private.h"
#include "adw-bidi-private.h"
#include "adw-fading-label-private.h"

#define FADE_WIDTH 18
#define CLOSE_BTN_ANIMATION_DURATION 150

#define BASE_WIDTH 118
#define BASE_WIDTH_PINNED 28

struct _AdwTab
{
  AdwTabItem parent_instance;

  GtkWidget *title;
  GtkWidget *icon_stack;
  GtkImage *icon;
  GtkSpinner *spinner;
  GtkImage *indicator_icon;
  GtkWidget *indicator_btn;
  GtkWidget *close_btn;

  gboolean selected;
  gboolean title_inverted;
  gboolean close_overlap;
  gboolean show_close;

  AdwAnimation *close_btn_animation;

  GskGLShader *shader;
  gboolean shader_compiled;
};

G_DEFINE_TYPE (AdwTab, adw_tab, ADW_TYPE_TAB_ITEM)

static inline void
set_style_class (GtkWidget  *widget,
                 const char *style_class,
                 gboolean    enabled)
{
  if (enabled)
    gtk_widget_add_css_class (widget, style_class);
  else
    gtk_widget_remove_css_class (widget, style_class);
}

static void
close_btn_animation_value_cb (double  value,
                              AdwTab *self)
{
  gtk_widget_set_opacity (self->close_btn, value);
  gtk_widget_set_can_target (self->close_btn, value > 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
close_btn_animation_done_cb (AdwTab *self)
{
  gtk_widget_set_opacity (self->close_btn, self->show_close ? 1 : 0);
  gtk_widget_set_can_target (self->close_btn, self->show_close);
  g_clear_object (&self->close_btn_animation);
}

static void
update_state (AdwTab *self)
{
  GtkStateFlags new_state;
  gboolean show_close;
  gboolean dragging = adw_tab_item_get_dragging (ADW_TAB_ITEM (self));
  gboolean hovering = adw_tab_item_get_hovering (ADW_TAB_ITEM (self));
  gboolean fully_visible = adw_tab_item_get_fully_visible (ADW_TAB_ITEM (self));

  new_state = gtk_widget_get_state_flags (GTK_WIDGET (self)) &
    ~(GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_CHECKED);

  if (dragging)
    new_state |= GTK_STATE_FLAG_PRELIGHT;

  if (self->selected || dragging)
    new_state |= GTK_STATE_FLAG_CHECKED;

  gtk_widget_set_state_flags (GTK_WIDGET (self), new_state, TRUE);

  show_close = (hovering && fully_visible) || self->selected || dragging;

  if (self->show_close != show_close) {
    double opacity = gtk_widget_get_opacity (self->close_btn);

    if (self->close_btn_animation)
      adw_animation_stop (self->close_btn_animation);

    self->show_close = show_close;

    self->close_btn_animation =
      adw_animation_new (GTK_WIDGET (self),
                         opacity,
                         self->show_close ? 1 : 0,
                         CLOSE_BTN_ANIMATION_DURATION,
                         (AdwAnimationTargetFunc) close_btn_animation_value_cb,
                         self);

    adw_animation_set_interpolator (self->close_btn_animation,
                                    ADW_ANIMATION_INTERPOLATOR_EASE_IN_OUT);

    g_signal_connect_swapped (self->close_btn_animation, "done", G_CALLBACK (close_btn_animation_done_cb), self);

    adw_animation_start (self->close_btn_animation);
  }
}

static void
update_tooltip (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));
  const char *tooltip = adw_tab_page_get_tooltip (page);

  if (tooltip && g_strcmp0 (tooltip, "") != 0)
    gtk_widget_set_tooltip_markup (GTK_WIDGET (self), tooltip);
  else
    gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                                 adw_tab_page_get_title (page));
}

static void
update_title (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));
  const char *title = adw_tab_page_get_title (page);
  PangoDirection title_direction = PANGO_DIRECTION_NEUTRAL;
  GtkTextDirection direction = gtk_widget_get_direction (GTK_WIDGET (self));
  gboolean title_inverted;

  if (title)
    title_direction = adw_find_base_dir (title, -1);

  title_inverted =
    (title_direction == PANGO_DIRECTION_LTR && direction == GTK_TEXT_DIR_RTL) ||
    (title_direction == PANGO_DIRECTION_RTL && direction == GTK_TEXT_DIR_LTR);

  if (self->title_inverted != title_inverted) {
    self->title_inverted = title_inverted;
    gtk_widget_queue_allocate (GTK_WIDGET (self));
  }

  update_tooltip (self);
}

static void
update_spinner (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));
  gboolean loading = page && adw_tab_page_get_loading (page);
  gboolean mapped = gtk_widget_get_mapped (GTK_WIDGET (self));

  /* Don't use CPU when not needed */
  gtk_spinner_set_spinning (self->spinner, loading && mapped);
}

static void
update_icons (AdwTab *self)
{
  AdwTabView *view = adw_tab_item_get_view (ADW_TAB_ITEM (self));
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));
  gboolean pinned = adw_tab_item_get_pinned (ADW_TAB_ITEM (self));
  GIcon *gicon = adw_tab_page_get_icon (page);
  gboolean loading = adw_tab_page_get_loading (page);
  GIcon *indicator = adw_tab_page_get_indicator_icon (page);
  const char *name = loading ? "spinner" : "icon";

  if (pinned && !gicon)
    gicon = adw_tab_view_get_default_icon (view);

  gtk_image_set_from_gicon (self->icon, gicon);
  gtk_widget_set_visible (self->icon_stack,
                          (gicon != NULL || loading) &&
                          (!pinned || indicator == NULL));
  gtk_stack_set_visible_child_name (GTK_STACK (self->icon_stack), name);

  gtk_widget_set_visible (self->indicator_btn, indicator != NULL);
}

static void
update_indicator (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));
  gboolean pinned = adw_tab_item_get_pinned (ADW_TAB_ITEM (self));
  gboolean fully_visible = adw_tab_item_get_fully_visible (ADW_TAB_ITEM (self));
  gboolean activatable = page && adw_tab_page_get_indicator_activatable (page);
  gboolean clickable = activatable && (self->selected || (!pinned && fully_visible));

  gtk_widget_set_can_target (self->indicator_btn, clickable);
}

static void
update_needs_attention (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));

  set_style_class (GTK_WIDGET (self), "needs-attention",
                   adw_tab_page_get_needs_attention (page));
}

static void
update_loading (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));

  update_icons (self);
  update_spinner (self);
  set_style_class (GTK_WIDGET (self), "loading",
                   adw_tab_page_get_loading (page));
}

static void
update_selected (AdwTab *self)
{
  AdwTabPage *page = adw_tab_item_get_page (ADW_TAB_ITEM (self));

  self->selected = adw_tab_item_get_dragging (ADW_TAB_ITEM (self));

  if (page)
    self->selected |= adw_tab_page_get_selected (page);

  update_state (self);
  update_indicator (self);
}

static void
notify_dragging_cb (AdwTab *self)
{
  update_state (self);
  update_selected (self);
}

static void
notify_fully_visible_cb (AdwTab *self)
{
  update_state (self);
  update_indicator (self);
}

static void
close_clicked_cb (AdwTab *self)
{
  adw_tab_item_close (ADW_TAB_ITEM (self));
}

static void
indicator_clicked_cb (AdwTab *self)
{
  adw_tab_item_activate_indicator (ADW_TAB_ITEM (self));
}

static void
ensure_shader (AdwTab *self)
{
  GtkNative *native;
  GskRenderer *renderer;
  g_autoptr (GError) error = NULL;

  if (self->shader)
    return;

  self->shader = gsk_gl_shader_new_from_resource ("/org/gnome/Adwaita/glsl/fade.glsl");

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
adw_tab_connect_page (AdwTabItem *item)
{
  AdwTab *self = ADW_TAB (item);
  AdwTabPage *page = adw_tab_item_get_page (item);

  update_selected (self);
  update_state (self);
  update_title (self);
  update_tooltip (self);
  update_spinner (self);
  update_icons (self);
  update_indicator (self);
  update_needs_attention (self);
  update_loading (self);

  g_signal_connect_object (page, "notify::selected",
                           G_CALLBACK (update_selected), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::title",
                           G_CALLBACK (update_title), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::tooltip",
                           G_CALLBACK (update_tooltip), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::icon",
                           G_CALLBACK (update_icons), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::indicator-icon",
                           G_CALLBACK (update_icons), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::indicator-activatable",
                           G_CALLBACK (update_indicator), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::needs-attention",
                           G_CALLBACK (update_needs_attention), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::loading",
                           G_CALLBACK (update_loading), self,
                           G_CONNECT_SWAPPED);
}

static void
adw_tab_disconnect_page (AdwTabItem *item)
{
  AdwTab *self = ADW_TAB (item);
  AdwTabPage *page = adw_tab_item_get_page (item);

  g_signal_handlers_disconnect_by_func (page, update_selected, self);
  g_signal_handlers_disconnect_by_func (page, update_title, self);
  g_signal_handlers_disconnect_by_func (page, update_tooltip, self);
  g_signal_handlers_disconnect_by_func (page, update_icons, self);
  g_signal_handlers_disconnect_by_func (page, update_indicator, self);
  g_signal_handlers_disconnect_by_func (page, update_needs_attention, self);
  g_signal_handlers_disconnect_by_func (page, update_loading, self);
}

static void
adw_tab_measure (GtkWidget      *widget,
                 GtkOrientation  orientation,
                 int             for_size,
                 int            *minimum,
                 int            *natural,
                 int            *minimum_baseline,
                 int            *natural_baseline)
{
  AdwTab *self = ADW_TAB (widget);
  gboolean pinned = adw_tab_item_get_pinned (ADW_TAB_ITEM (self));
  int min = 0, nat = 0;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    nat = pinned ? BASE_WIDTH_PINNED : BASE_WIDTH;
  } else {
    int child_min, child_nat;

    gtk_widget_measure (self->icon_stack, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);

    gtk_widget_measure (self->title, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);

    gtk_widget_measure (self->close_btn, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);

    gtk_widget_measure (self->indicator_btn, orientation, for_size,
                        &child_min, &child_nat, NULL, NULL);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);
  }

  if (minimum)
    *minimum = min;
  if (natural)
    *natural = nat;
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static inline void
measure_child (GtkWidget *child,
               int        height,
               int       *width)
{
  if (gtk_widget_get_visible (child))
    gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, height, NULL, width, NULL, NULL);
  else
    *width = 0;
}

static inline void
allocate_child (GtkWidget     *child,
                GtkAllocation *alloc,
                int            x,
                int            width,
                int            baseline)
{
  GtkAllocation child_alloc = *alloc;

  if (gtk_widget_get_direction (child) == GTK_TEXT_DIR_RTL)
    child_alloc.x += alloc->width - width - x;
  else
    child_alloc.x += x;

  child_alloc.width = width;

  gtk_widget_size_allocate (child, &child_alloc, baseline);
}

static void
allocate_contents (AdwTab        *self,
                   GtkAllocation *alloc,
                   int            baseline)
{
  int indicator_width, close_width, icon_width, title_width;
  int center_x, center_width = 0;
  int start_width = 0, end_width = 0;
  gboolean inverted = adw_tab_item_get_inverted (ADW_TAB_ITEM (self));

  measure_child (self->icon_stack, alloc->height, &icon_width);
  measure_child (self->title, alloc->height, &title_width);
  measure_child (self->indicator_btn, alloc->height, &indicator_width);
  measure_child (self->close_btn, alloc->height, &close_width);

  if (gtk_widget_get_visible (self->indicator_btn)) {
    if (adw_tab_item_get_pinned (ADW_TAB_ITEM (self))) {
      /* Center it in a pinned tab */
      allocate_child (self->indicator_btn, alloc,
                      (alloc->width - indicator_width) / 2, indicator_width,
                      baseline);
    } else if (inverted) {
      allocate_child (self->indicator_btn, alloc,
                      alloc->width - indicator_width, indicator_width,
                      baseline);

      end_width = indicator_width;
    } else {
      allocate_child (self->indicator_btn, alloc, 0, indicator_width, baseline);

      start_width = indicator_width;
    }
  }

  if (gtk_widget_get_visible (self->close_btn)) {
    if (inverted) {
      allocate_child (self->close_btn, alloc, 0, close_width, baseline);

      start_width = close_width;
    } else {
      allocate_child (self->close_btn, alloc,
                      alloc->width - close_width, close_width, baseline);

      if (self->title_inverted)
        end_width = close_width;
    }
  }

  center_width = MIN (alloc->width - start_width - end_width,
                      icon_width + title_width);
  center_x = CLAMP ((alloc->width - center_width) / 2,
                    start_width,
                    alloc->width - center_width - end_width);

  self->close_overlap = !inverted &&
                        !self->title_inverted &&
                        gtk_widget_get_visible (self->title) &&
                        gtk_widget_get_visible (self->close_btn) &&
                        center_x + center_width > alloc->width - close_width;

  if (gtk_widget_get_visible (self->icon_stack)) {
    allocate_child (self->icon_stack, alloc, center_x, icon_width, baseline);

    center_x += icon_width;
    center_width -= icon_width;
  }

  if (gtk_widget_get_visible (self->title))
    allocate_child (self->title, alloc, center_x, center_width, baseline);
}

static void
adw_tab_size_allocate (GtkWidget *widget,
                       int        width,
                       int        height,
                       int        baseline)
{
  AdwTab *self = ADW_TAB (widget);
  int display_width = adw_tab_item_get_display_width (ADW_TAB_ITEM (self));
  GtkAllocation child_alloc;
  int allocated_width, width_diff;

  if (!self->icon_stack ||
      !self->indicator_btn ||
      !self->title ||
      !self->close_btn)
    return;

  allocated_width = gtk_widget_get_allocated_width (widget);
  width_diff = MAX (0, display_width - allocated_width);

  child_alloc.x = -width_diff / 2;
  child_alloc.y = 0;
  child_alloc.height = height;
  child_alloc.width = width + width_diff;

  allocate_contents (self, &child_alloc, baseline);
}

static void
adw_tab_map (GtkWidget *widget)
{
  AdwTab *self = ADW_TAB (widget);

  GTK_WIDGET_CLASS (adw_tab_parent_class)->map (widget);

  update_spinner (self);
}

static void
adw_tab_unmap (GtkWidget *widget)
{
  AdwTab *self = ADW_TAB (widget);

  GTK_WIDGET_CLASS (adw_tab_parent_class)->unmap (widget);

  update_spinner (self);
}

static void
adw_tab_snapshot (GtkWidget   *widget,
                  GtkSnapshot *snapshot)
{
  AdwTab *self = ADW_TAB (widget);
  float opacity = gtk_widget_get_opacity (self->close_btn);
  gboolean draw_fade = self->close_overlap && opacity > 0;

  gtk_widget_snapshot_child (widget, self->indicator_btn, snapshot);
  gtk_widget_snapshot_child (widget, self->icon_stack, snapshot);

  if (draw_fade) {
    gboolean is_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
    int width = gtk_widget_get_width (widget);
    int height = gtk_widget_get_height (widget);
    float offset =
      gtk_widget_get_allocated_width (self->close_btn) +
      gtk_widget_get_margin_end (self->title);
    graphene_rect_t bounds;

    ensure_shader (self);

    graphene_rect_init (&bounds, 0, 0, width, height);

    if (self->shader_compiled) {
      gtk_snapshot_push_gl_shader (snapshot, self->shader, &bounds,
                                   gsk_gl_shader_format_args (self->shader,
                                                              "offsetLeft", is_rtl ? offset : 0.0f,
                                                              "offsetRight", is_rtl ? 0.0f : offset,
                                                              "strengthLeft", is_rtl ? opacity : 0.0f,
                                                              "strengthRight", is_rtl ? 0.0f : opacity,
                                                              NULL));
    } else {
      bounds.size.width -= offset;

      if (is_rtl)
        bounds.origin.x += offset;

      gtk_snapshot_push_clip (snapshot, &bounds);
    }
  }

  gtk_widget_snapshot_child (widget, self->title, snapshot);

  if (draw_fade) {
    if (self->shader_compiled)
      gtk_snapshot_gl_shader_pop_texture (snapshot);

    gtk_snapshot_pop (snapshot);
  }

  gtk_widget_snapshot_child (widget, self->close_btn, snapshot);
}

static void
adw_tab_direction_changed (GtkWidget        *widget,
                           GtkTextDirection  previous_direction)
{
  AdwTab *self = ADW_TAB (widget);

  update_title (self);

  GTK_WIDGET_CLASS (adw_tab_parent_class)->direction_changed (widget,
                                                              previous_direction);
}

static void
adw_tab_unrealize (GtkWidget *widget)
{
  AdwTab *self = ADW_TAB (widget);

  GTK_WIDGET_CLASS (adw_tab_parent_class)->unrealize (widget);

  g_clear_object (&self->shader);
}

static void
adw_tab_constructed (GObject *object)
{
  AdwTab *self = ADW_TAB (object);
  AdwTabView *view = adw_tab_item_get_view (ADW_TAB_ITEM (self));

  G_OBJECT_CLASS (adw_tab_parent_class)->constructed (object);

  if (adw_tab_item_get_pinned (ADW_TAB_ITEM (self))) {
    gtk_widget_add_css_class (GTK_WIDGET (self), "pinned");
    gtk_widget_hide (self->title);
    gtk_widget_hide (self->close_btn);
    gtk_widget_set_margin_start (self->icon_stack, 0);
    gtk_widget_set_margin_end (self->icon_stack, 0);
  }

  g_signal_connect_object (view, "notify::default-icon",
                           G_CALLBACK (update_icons), self,
                           G_CONNECT_SWAPPED);
}

static void
adw_tab_dispose (GObject *object)
{
  AdwTab *self = ADW_TAB (object);

  g_clear_object (&self->shader);
  gtk_widget_unparent (self->indicator_btn);
  gtk_widget_unparent (self->icon_stack);
  gtk_widget_unparent (self->title);
  gtk_widget_unparent (self->close_btn);

  G_OBJECT_CLASS (adw_tab_parent_class)->dispose (object);
}

static void
adw_tab_class_init (AdwTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  AdwTabItemClass *item_class = ADW_TAB_ITEM_CLASS (klass);

  object_class->dispose = adw_tab_dispose;
  object_class->constructed = adw_tab_constructed;

  widget_class->measure = adw_tab_measure;
  widget_class->size_allocate = adw_tab_size_allocate;
  widget_class->map = adw_tab_map;
  widget_class->unmap = adw_tab_unmap;
  widget_class->snapshot = adw_tab_snapshot;
  widget_class->direction_changed = adw_tab_direction_changed;
  widget_class->unrealize = adw_tab_unrealize;

  item_class->connect_page = adw_tab_connect_page;
  item_class->disconnect_page = adw_tab_disconnect_page;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Adwaita/ui/adw-tab.ui");
  gtk_widget_class_bind_template_child (widget_class, AdwTab, title);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, icon_stack);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, icon);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, spinner);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, indicator_icon);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, indicator_btn);
  gtk_widget_class_bind_template_child (widget_class, AdwTab, close_btn);
  gtk_widget_class_bind_template_callback (widget_class, update_state);
  gtk_widget_class_bind_template_callback (widget_class, notify_dragging_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_fully_visible_cb);
  gtk_widget_class_bind_template_callback (widget_class, close_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, indicator_clicked_cb);

  gtk_widget_class_set_css_name (widget_class, "tab");
}

static void
adw_tab_init (AdwTab *self)
{
  g_type_ensure (ADW_TYPE_FADING_LABEL);

  gtk_widget_init_template (GTK_WIDGET (self));
}
