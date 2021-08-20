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
#include "adw-tab-list-base-private.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_BOX (adw_tab_box_get_type())

G_DECLARE_FINAL_TYPE (AdwTabBox, adw_tab_box, ADW, TAB_BOX, AdwTabListBase)

G_END_DECLS
