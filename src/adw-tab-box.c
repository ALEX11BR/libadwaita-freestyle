/*
 * Copyright (C) 2020-2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"

#include "adw-tab-box-private.h"
struct _AdwTabBox
{
  AdwTabListBase parent_instance;
};

G_DEFINE_TYPE (AdwTabBox, adw_tab_box, ADW_TYPE_TAB_LIST_BASE)

static void
adw_tab_box_class_init (AdwTabBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_css_name (widget_class, "tabbox");
}

static void
adw_tab_box_init (AdwTabBox *self)
{
}
