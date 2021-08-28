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

#include "adw-tab-item-private.h"

G_BEGIN_DECLS

#define ADW_TYPE_TAB_THUMBNAIL (adw_tab_thumbnail_get_type())

G_DECLARE_FINAL_TYPE (AdwTabThumbnail, adw_tab_thumbnail, ADW, TAB_THUMBNAIL, AdwTabItem)

GtkPicture *adw_tab_thumbnail_get_picture (AdwTabThumbnail *self);

G_END_DECLS
