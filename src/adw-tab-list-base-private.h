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

#define ADW_TYPE_TAB_LIST_BASE (adw_tab_list_base_get_type())

G_DECLARE_DERIVABLE_TYPE (AdwTabListBase, adw_tab_list_base, ADW, TAB_LIST_BASE, GtkWidget)

struct _AdwTabListBaseClass
{
  GtkWidgetClass parent_class;

  gboolean (*tabs_have_visible_focus) (AdwTabListBase *self);
};

void adw_tab_list_base_set_view (AdwTabListBase *self,
                                 AdwTabView     *view);

void adw_tab_list_base_set_adjustment (AdwTabListBase *self,
                                       GtkAdjustment  *adjustment);

void adw_tab_list_base_attach_page (AdwTabListBase *self,
                                    AdwTabPage     *page,
                                    int             position);
void adw_tab_list_base_detach_page (AdwTabListBase *self,
                                    AdwTabPage     *page);
void adw_tab_list_base_select_page (AdwTabListBase *self,
                                    AdwTabPage     *page);

void adw_tab_list_base_try_focus_selected_tab (AdwTabListBase  *self);
gboolean adw_tab_list_base_is_page_focused    (AdwTabListBase *self,
                                               AdwTabPage     *page);

void adw_tab_list_base_setup_extra_drop_target (AdwTabListBase *self,
                                                GdkDragAction   actions,
                                                GType          *types,
                                                gsize           n_types);

gboolean adw_tab_list_base_get_expand_tabs (AdwTabListBase *self);
void     adw_tab_list_base_set_expand_tabs (AdwTabListBase *self,
                                            gboolean        expand_tabs);

gboolean adw_tab_list_base_get_inverted (AdwTabListBase *self);
void     adw_tab_list_base_set_inverted (AdwTabListBase *self,
                                         gboolean        inverted);

G_END_DECLS
