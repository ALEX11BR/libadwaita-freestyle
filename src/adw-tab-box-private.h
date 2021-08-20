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

#include "adw-tab-bar-private.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_BOX (adw_tab_box_get_type())

G_DECLARE_FINAL_TYPE (AdwTabBox, adw_tab_box, ADW, TAB_BOX, AdwTabListBase)

void adw_tab_box_set_tab_bar (AdwTabBox *self,
                              AdwTabBar *tab_bar);

G_END_DECLS
