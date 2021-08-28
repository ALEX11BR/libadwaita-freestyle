/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include "adw-tab-thumbnail-private.h"

#define MIN_ASPECT_RATIO 0.8
#define MAX_ASPECT_RATIO 1.6
#define XALIGN 0.5
#define YALIGN 0.5

#define ADW_TYPE_TAB_PAINTABLE (adw_tab_paintable_get_type ())

G_DECLARE_FINAL_TYPE (AdwTabPaintable, adw_tab_paintable, ADW, TAB_PAINTABLE, GObject)

struct _AdwTabPaintable
{
  GObject parent_instance;

  GtkWidget *view;
  GtkWidget *child;

  GdkPaintable *paintable;
  GdkPaintable *view_paintable;
  GdkPaintable *cached_paintable;

  GdkRGBA cached_bg;
  int cached_width;
  int cached_height;
  bool schedule_clear_cache;
};

static double
adw_tab_paintable_get_intrinsic_aspect_ratio (GdkPaintable *paintable)
{
  AdwTabPaintable *self = ADW_TAB_PAINTABLE (paintable);
  int width = gtk_widget_get_width (self->view);
  int height = gtk_widget_get_height (self->view);
  double ratio = (double) width / height;

  return CLAMP (ratio, MIN_ASPECT_RATIO, MAX_ASPECT_RATIO);
}

static GdkPaintable *
adw_tab_paintable_get_current_image (GdkPaintable *paintable)
{
  AdwTabPaintable *self = ADW_TAB_PAINTABLE (paintable);

  if (self->cached_paintable)
    return g_object_ref (self->cached_paintable);

  return gdk_paintable_get_current_image (self->paintable);
}

static void
snapshot_paintable (GdkSnapshot  *snapshot,
                    double        width,
                    double        height,
                    GdkPaintable *paintable)
{
  double snapshot_ratio = width / height;
  double paintable_ratio = gdk_paintable_get_intrinsic_aspect_ratio (paintable);

  if (paintable_ratio > snapshot_ratio) {
    double new_width = width * paintable_ratio / snapshot_ratio;

    gtk_snapshot_translate (GTK_SNAPSHOT (snapshot),
                            &GRAPHENE_POINT_INIT ((float) (width - new_width) * XALIGN, 0));

    width = new_width;
  } if (paintable_ratio < snapshot_ratio) {
    double new_height = height * snapshot_ratio / paintable_ratio;

    gtk_snapshot_translate (GTK_SNAPSHOT (snapshot),
                            &GRAPHENE_POINT_INIT (0, (float) (height - new_height) * YALIGN));

    height = new_height;
  }

  gdk_paintable_snapshot (paintable, snapshot, width, height);
}

static void
get_background_color (AdwTabPaintable *self,
                      GdkRGBA         *rgba)
{
  GtkStyleContext *context = gtk_widget_get_style_context (self->view);

  if (gtk_style_context_lookup_color (context, "theme_bg_color", rgba))
    return;

  rgba->red = 1;
  rgba->green = 1;
  rgba->blue = 1;
  rgba->alpha = 1;
}

static void
adw_tab_paintable_snapshot (GdkPaintable *paintable,
                            GdkSnapshot  *snapshot,
                            double        width,
                            double        height)
{
  AdwTabPaintable *self = ADW_TAB_PAINTABLE (paintable);
  GdkRGBA bg;

  if (self->cached_paintable) {
    gtk_snapshot_append_color (GTK_SNAPSHOT (snapshot), &self->cached_bg,
                               &GRAPHENE_RECT_INIT (0, 0, width, height));

    snapshot_paintable (snapshot, width, height, self->cached_paintable);

    return;
  }

  if (!gtk_widget_get_mapped (self->child))
    return;

  get_background_color (self, &bg);
  gtk_snapshot_append_color (GTK_SNAPSHOT (snapshot), &bg,
                             &GRAPHENE_RECT_INIT (0, 0, width, height));

  snapshot_paintable (snapshot, width, height, self->paintable);
}

static void
adw_tab_paintable_iface_init (GdkPaintableInterface *iface)
{
  iface->get_intrinsic_aspect_ratio = adw_tab_paintable_get_intrinsic_aspect_ratio;
  iface->get_current_image = adw_tab_paintable_get_current_image;
  iface->snapshot = adw_tab_paintable_snapshot;
}

G_DEFINE_TYPE_WITH_CODE (AdwTabPaintable, adw_tab_paintable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE, adw_tab_paintable_iface_init))

static void
adw_tab_paintable_dispose (GObject *object)
{
  AdwTabPaintable *self = ADW_TAB_PAINTABLE (object);

  g_clear_object (&self->paintable);
  g_clear_object (&self->view_paintable);
  g_clear_object (&self->cached_paintable);

  G_OBJECT_CLASS (adw_tab_paintable_parent_class)->dispose (object);
}

static void
adw_tab_paintable_class_init (AdwTabPaintableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = adw_tab_paintable_dispose;
}

static void
adw_tab_paintable_init (AdwTabPaintable *self)
{
}

static void
invalidate_contents_cb (AdwTabPaintable *self)
{
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));

  if (self->schedule_clear_cache) {
    g_clear_object (&self->cached_paintable);
    self->schedule_clear_cache = FALSE;
  }
}

static void
child_map_cb (AdwTabPaintable *self)
{
  if (self->cached_paintable)
    self->schedule_clear_cache = TRUE;
}

static void
child_unmap_cb (AdwTabPaintable *self)
{
  self->cached_paintable = gdk_paintable_get_current_image (self->paintable);
  self->cached_width = gtk_widget_get_width (self->view);
  self->cached_height = gtk_widget_get_height (self->view);
  get_background_color (self, &self->cached_bg);
}

static GdkPaintable *
adw_tab_paintable_new (AdwTabView *view,
                       AdwTabPage *page)
{
  AdwTabPaintable *self = g_object_new (ADW_TYPE_TAB_PAINTABLE, NULL);

  self->view = GTK_WIDGET (view);
  self->child = adw_tab_page_get_child (page);

  self->paintable = gtk_widget_paintable_new (self->child);
  self->view_paintable = gtk_widget_paintable_new (self->view);

  g_signal_connect_swapped (self->paintable, "invalidate-contents", G_CALLBACK (invalidate_contents_cb), self);
  g_signal_connect_swapped (self->view_paintable, "invalidate-size", G_CALLBACK (gdk_paintable_invalidate_size), self);

  g_signal_connect_swapped (self->child, "map", G_CALLBACK (child_map_cb), self);
  g_signal_connect_swapped (self->child, "unmap", G_CALLBACK (child_unmap_cb), self);

  return GDK_PAINTABLE (self);
}











struct _AdwTabThumbnail
{
  AdwTabItem parent_instance;

  GtkPicture *picture;
  GtkWidget *contents;
};

G_DEFINE_TYPE (AdwTabThumbnail, adw_tab_thumbnail, ADW_TYPE_TAB_ITEM)

static void
update_tooltip (AdwTabThumbnail *self)
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
adw_tab_thumbnail_connect_page (AdwTabItem *item)
{
  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (item);
  AdwTabView *view = adw_tab_item_get_view (item);
  AdwTabPage *page = adw_tab_item_get_page (item);

  gtk_picture_set_paintable (GTK_PICTURE (self->picture),
                             adw_tab_paintable_new (view, page));

  update_tooltip (self);

  g_signal_connect_object (page, "notify::title",
                           G_CALLBACK (update_tooltip), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (page, "notify::tooltip",
                           G_CALLBACK (update_tooltip), self,
                           G_CONNECT_SWAPPED);

/*
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
*/
}

static void
adw_tab_thumbnail_disconnect_page (AdwTabItem *item)
{
  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (item);
  AdwTabPage *page = adw_tab_item_get_page (item);

  gtk_picture_set_paintable (GTK_PICTURE (self->picture), NULL);

  g_signal_handlers_disconnect_by_func (page, update_tooltip, self);
/*

  g_signal_handlers_disconnect_by_func (page, update_selected, self);
  g_signal_handlers_disconnect_by_func (page, update_title, self);
  g_signal_handlers_disconnect_by_func (page, update_tooltip, self);
  g_signal_handlers_disconnect_by_func (page, update_icons, self);
  g_signal_handlers_disconnect_by_func (page, update_indicator, self);
  g_signal_handlers_disconnect_by_func (page, update_needs_attention, self);
  g_signal_handlers_disconnect_by_func (page, update_loading, self);
*/
}

static int
adw_tab_thumbnail_measure_contents (AdwTabItem     *item,
                                    GtkOrientation  orientation,
                                    int             for_size)
{
  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (item);
  int nat;

  gtk_widget_measure (self->contents, orientation, for_size,
                      NULL, &nat, NULL, NULL);

  return nat;
}

static void
adw_tab_thumbnail_allocate_contents (AdwTabItem    *item,
                                     GtkAllocation *alloc,
                                     int            baseline)
{
  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (item);
  gtk_widget_size_allocate (self->contents, alloc, baseline);
}

static GtkSizeRequestMode
adw_tab_thumbnail_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
adw_tab_thumbnail_constructed (GObject *object)
{
//  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (object);

  G_OBJECT_CLASS (adw_tab_thumbnail_parent_class)->constructed (object);

  /*
  g_signal_connect_object (view, "notify::default-icon",
                           G_CALLBACK (update_icons), object,
                           G_CONNECT_SWAPPED);
*/
}

static void
adw_tab_thumbnail_dispose (GObject *object)
{
  AdwTabThumbnail *self = ADW_TAB_THUMBNAIL (object);

  if (self->contents)
    gtk_widget_unparent (self->contents);

  G_OBJECT_CLASS (adw_tab_thumbnail_parent_class)->dispose (object);
}

static void
adw_tab_thumbnail_class_init (AdwTabThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  AdwTabItemClass *item_class = ADW_TAB_ITEM_CLASS (klass);

  object_class->dispose = adw_tab_thumbnail_dispose;
  object_class->constructed = adw_tab_thumbnail_constructed;

  widget_class->get_request_mode = adw_tab_thumbnail_get_request_mode;

  item_class->connect_page = adw_tab_thumbnail_connect_page;
  item_class->disconnect_page = adw_tab_thumbnail_disconnect_page;
  item_class->measure_contents = adw_tab_thumbnail_measure_contents;
  item_class->allocate_contents = adw_tab_thumbnail_allocate_contents;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Adwaita/ui/adw-tab-thumbnail.ui");
  gtk_widget_class_bind_template_child (widget_class, AdwTabThumbnail, picture);
  gtk_widget_class_bind_template_child (widget_class, AdwTabThumbnail, contents);

  gtk_widget_class_set_css_name (widget_class, "tabthumbnail");
}

static void
adw_tab_thumbnail_init (AdwTabThumbnail *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

GtkPicture *
adw_tab_thumbnail_get_picture (AdwTabThumbnail *self)
{
  g_return_val_if_fail (ADW_IS_TAB_THUMBNAIL (self), NULL);

  return GTK_PICTURE (self->picture);
}
