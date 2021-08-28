/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"

#include "adw-tab-grid-private.h"

#include "adw-tab-thumbnail-private.h"

struct _AdwTabGrid
{
  AdwTabListBase parent_instance;

  AdwTabOverview *tab_overview;
};

G_DEFINE_TYPE (AdwTabGrid, adw_tab_grid, ADW_TYPE_TAB_LIST_BASE)

static gboolean
adw_tab_grid_tabs_have_visible_focus (AdwTabListBase *base)
{
  return FALSE;
}

static void
adw_tab_grid_activate_item (AdwTabListBase *base,
                            AdwTabItem     *item)
{
  AdwTabGrid *self = ADW_TAB_GRID (base);

  adw_tab_overview_close (self->tab_overview);
}

static void
adw_tab_grid_class_init (AdwTabGridClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  AdwTabListBaseClass *base_class = ADW_TAB_LIST_BASE_CLASS (klass);

  base_class->tabs_have_visible_focus = adw_tab_grid_tabs_have_visible_focus;
  base_class->activate_item = adw_tab_grid_activate_item;

  base_class->item_type = ADW_TYPE_TAB_THUMBNAIL;

  gtk_widget_class_set_css_name (widget_class, "tabgrid");
}

static void
adw_tab_grid_init (AdwTabGrid *self)
{
  adw_tab_list_base_set_expand_tabs (ADW_TAB_LIST_BASE (self), FALSE);
}

void
adw_tab_grid_set_tab_overview (AdwTabGrid     *self,
                               AdwTabOverview *tab_overview)
{
  self->tab_overview = tab_overview;
}

GtkPicture *
adw_tab_grid_get_transition_picture (AdwTabGrid *self)
{
  GtkWidget *item = adw_tab_list_base_get_selected_item (ADW_TAB_LIST_BASE (self));
  AdwTabThumbnail *thumbnail = ADW_TAB_THUMBNAIL (item);

  return adw_tab_thumbnail_get_picture (thumbnail);
}
