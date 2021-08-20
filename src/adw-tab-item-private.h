/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include <gtk/gtk.h>
#include "adw-tab-view.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_ITEM (adw_tab_item_get_type())

G_DECLARE_DERIVABLE_TYPE (AdwTabItem, adw_tab_item, ADW, TAB_ITEM, GtkWidget)

struct _AdwTabItemClass
{
  GtkWidgetClass parent_class;

  void (*connect_page)    (AdwTabItem *self);
  void (*disconnect_page) (AdwTabItem *self);
};

AdwTabView *adw_tab_item_get_view (AdwTabItem *self);

AdwTabPage *adw_tab_item_get_page (AdwTabItem *self);
void        adw_tab_item_set_page (AdwTabItem *self,
                                   AdwTabPage *page);

gboolean adw_tab_item_get_pinned (AdwTabItem *self);

int  adw_tab_item_get_display_width (AdwTabItem *self);
void adw_tab_item_set_display_width (AdwTabItem *self,
                                     int         width);

gboolean adw_tab_item_get_hovering (AdwTabItem *self);

gboolean adw_tab_item_get_dragging (AdwTabItem *self);
void     adw_tab_item_set_dragging (AdwTabItem *self,
                                    gboolean    dragging);

gboolean adw_tab_item_get_inverted (AdwTabItem *self);
void     adw_tab_item_set_inverted (AdwTabItem *self,
                                    gboolean    inverted);

gboolean adw_tab_item_get_fully_visible (AdwTabItem *self);
void     adw_tab_item_set_fully_visible (AdwTabItem *self,
                                         gboolean    fully_visible);

void adw_tab_item_setup_extra_drop_target (AdwTabItem    *self,
                                           GdkDragAction  actions,
                                           GType         *types,
                                           gsize          n_types);

void adw_tab_item_close (AdwTabItem *self);

void adw_tab_item_activate_indicator (AdwTabItem *self);

G_END_DECLS
