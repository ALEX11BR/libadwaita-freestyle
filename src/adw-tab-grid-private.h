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

#include "adw-tab-list-base-private.h"

#include "adw-tab-overview.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_GRID (adw_tab_grid_get_type())

G_DECLARE_FINAL_TYPE (AdwTabGrid, adw_tab_grid, ADW, TAB_GRID, AdwTabListBase)

void adw_tab_grid_set_tab_overview (AdwTabGrid     *self,
                                    AdwTabOverview *tab_overview);

GtkPicture *adw_tab_grid_get_transition_picture (AdwTabGrid *self);

G_END_DECLS
