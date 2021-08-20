/*
 * Copyright (C) 2020-2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"

#include "adw-tab-box-private.h"
#include "adw-tab-private.h"

struct _AdwTabBox
{
  AdwTabListBase parent_instance;

  AdwTabBar *tab_bar;
};

G_DEFINE_TYPE (AdwTabBox, adw_tab_box, ADW_TYPE_TAB_LIST_BASE)

static gboolean
adw_tab_box_tabs_have_visible_focus (AdwTabListBase *base)
{
  AdwTabBox *self = ADW_TAB_BOX (base);

  return adw_tab_bar_tabs_have_visible_focus (self->tab_bar);
}

static void
adw_tab_box_class_init (AdwTabBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  AdwTabListBaseClass *base_class = ADW_TAB_LIST_BASE_CLASS (klass);

  base_class->tabs_have_visible_focus = adw_tab_box_tabs_have_visible_focus;

  base_class->item_type = ADW_TYPE_TAB;

  gtk_widget_class_set_css_name (widget_class, "tabbox");
}

static void
adw_tab_box_init (AdwTabBox *self)
{
}

void
adw_tab_box_set_tab_bar (AdwTabBox *self,
                         AdwTabBar *tab_bar)
{
  self->tab_bar = tab_bar;
}
