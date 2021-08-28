/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-version.h"

#include <gtk/gtk.h>
#include "adw-tab-view.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_OVERVIEW (adw_tab_overview_get_type())

ADW_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (AdwTabOverview, adw_tab_overview, ADW, TAB_OVERVIEW, GtkWidget)

ADW_AVAILABLE_IN_ALL
GtkWidget *adw_tab_overview_new (void);

ADW_AVAILABLE_IN_ALL
AdwTabView *adw_tab_overview_get_view (AdwTabOverview *self);
ADW_AVAILABLE_IN_ALL
void        adw_tab_overview_set_view (AdwTabOverview *self,
                                       AdwTabView     *view);

ADW_AVAILABLE_IN_ALL
GtkWidget *adw_tab_overview_get_child (AdwTabOverview *self);
ADW_AVAILABLE_IN_ALL
void       adw_tab_overview_set_child (AdwTabOverview *self,
                                       GtkWidget      *child);

ADW_AVAILABLE_IN_ALL
void adw_tab_overview_open  (AdwTabOverview *self);
ADW_AVAILABLE_IN_ALL
void adw_tab_overview_close (AdwTabOverview *self);

ADW_AVAILABLE_IN_ALL
void adw_tab_overview_setup_extra_drop_target (AdwTabOverview *self,
                                               GdkDragAction   actions,
                                               GType          *types,
                                               gsize           n_types);

G_END_DECLS
