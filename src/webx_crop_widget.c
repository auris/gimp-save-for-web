/* Save for Web plug-in for The GIMP
 *
 * Copyright (C) 2006-2007, Aurimas Juška
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "webx_main.h"
#include "webx_crop_widget.h"

#include "plugin-intl.h"

enum
{
  CROP_CHANGED,
  LAST_SIGNAL
};

static void     webx_crop_widget_reset          (GtkButton       *switcher,
                                                 WebxCropWidget  *crop);
static void     webx_crop_widget_value_changed  (GtkSpinButton   *spinbutton,
                                                 WebxCropWidget  *crop);

G_DEFINE_TYPE (WebxCropWidget, webx_crop_widget, GTK_TYPE_VBOX)

#define parent_class webx_crop_widget_parent_class

static guint webx_crop_widget_signals[LAST_SIGNAL] = { 0 };


static void
webx_crop_widget_class_init (WebxCropWidgetClass        *klass)
{
  webx_crop_widget_signals[CROP_CHANGED] =
      g_signal_new ("crop-changed",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (WebxCropWidgetClass, crop_changed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__BOXED,
                    G_TYPE_NONE, 1,
                    GDK_TYPE_RECTANGLE);
}

static void
webx_crop_widget_init (WebxCropWidget   *crop)
{
}

GtkWidget*
webx_crop_widget_new (gint      width,
                      gint      height)
{
  WebxCropWidget        *crop;
  GtkWidget             *label;
  GtkWidget             *spinbtn;
  GtkWidget            *frame;
  GtkWidget            *button;

  crop = g_object_new (WEBX_TYPE_CROP_WIDGET, NULL);
  crop->rect.x = 0;
  crop->rect.y = 0;
  crop->rect.width = width;
  crop->rect.height = height;
  crop->clip_width = width;
  crop->clip_height = height;

  frame = gtk_frame_new (_("Crop"));
  gtk_box_pack_start (GTK_BOX (crop), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  crop->table = gtk_table_new (2, 5, FALSE);
  gtk_container_add (GTK_CONTAINER (frame), crop->table);
  gtk_container_set_border_width (GTK_CONTAINER (crop->table), 4);
 
  label = gtk_label_new (_("X Offset:"));
  gtk_table_attach (GTK_TABLE (crop->table), label,
                    0, 1, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (0, WEBX_MAX_SIZE-1, 1);
  crop->xoffs = spinbtn;
  gtk_table_attach (GTK_TABLE (crop->table), spinbtn,
                    1, 2, 0, 1,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             0.0);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_crop_widget_value_changed),
                    crop);

  label = gtk_label_new (_("Y Offset:"));
  gtk_table_attach (GTK_TABLE (crop->table), label,
                    0, 1, 1, 2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (0, WEBX_MAX_SIZE-1, 1);
  crop->yoffs = spinbtn;
  gtk_table_attach (GTK_TABLE (crop->table), spinbtn,
                    1, 2, 1, 2,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             0.0);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_crop_widget_value_changed),
                    crop);

  label = gtk_label_new (_("Width:"));
  gtk_table_attach (GTK_TABLE (crop->table), label,
                    0, 1, 2, 3,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (1, WEBX_MAX_SIZE, 1);
  crop->width = spinbtn;
  gtk_table_attach (GTK_TABLE (crop->table), spinbtn,
                    1, 2, 2, 3,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             width);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_crop_widget_value_changed),
                    crop);

  label = gtk_label_new (_("Height:"));
  gtk_table_attach (GTK_TABLE (crop->table), label,
                    0, 1, 3, 4,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (1, WEBX_MAX_SIZE, 1);
  crop->height = spinbtn;
  gtk_table_attach (GTK_TABLE (crop->table), spinbtn,
                    1, 2, 3, 4,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             height);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_crop_widget_value_changed),
                    crop);

  button = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  gtk_table_attach (GTK_TABLE (crop->table), button,
                    0, 2, 4, 5,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (webx_crop_widget_reset),
                    crop);

  gtk_widget_show_all (crop->table);
 
  return GTK_WIDGET (crop);
}

void
webx_crop_widget_update_target (WebxCropWidget *crop,
                                GdkRectangle   *rect)
{
  g_return_if_fail (WEBX_IS_CROP_WIDGET (crop));
  g_return_if_fail (rect != NULL);

  if (crop->stop_recursion > 0)
    return;
  crop->stop_recursion++;
    
  crop->rect = *rect;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (crop->xoffs), rect->x);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (crop->yoffs), rect->y);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (crop->width), rect->width);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (crop->height), rect->height);

  g_signal_emit (crop, webx_crop_widget_signals[CROP_CHANGED], 0,
                 &crop->rect);

  crop->stop_recursion--;
}

void
webx_crop_widget_update (WebxCropWidget *crop,
                         GdkRectangle   *rect,
                         gint            clip_width,
                         gint            clip_height)
{
  g_return_if_fail (WEBX_IS_CROP_WIDGET (crop));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (clip_width > 0);
  g_return_if_fail (clip_height > 0);

  crop->clip_width = clip_width;
  crop->clip_height = clip_height;

  if (crop->stop_recursion > 0)
    return;
  crop->stop_recursion++;

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (crop->xoffs),
                             0, clip_width);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (crop->yoffs),
                             0, clip_height);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (crop->width),
                             0, clip_width);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (crop->height),
                             0, clip_height);

  crop->stop_recursion--;

  webx_crop_widget_update_target (crop, rect);
}

static void
webx_crop_widget_reset (GtkButton      *button,
                        WebxCropWidget *crop)
{
  GdkRectangle  rect;

  g_return_if_fail (WEBX_IS_CROP_WIDGET (crop));

  rect.x = 0;
  rect.y = 0;
  rect.width = crop->clip_width;
  rect.height = crop->clip_height;
  webx_crop_widget_update_target (crop, &rect);
}

static void
webx_crop_widget_value_changed (GtkSpinButton  *spinbutton,
                                WebxCropWidget *crop)
{
  gint            xoffs;
  gint            yoffs;
  gint            width;
  gint            height;
  GdkRectangle    rect;

  g_return_if_fail (WEBX_IS_CROP_WIDGET (crop));

  xoffs = gtk_spin_button_get_value (GTK_SPIN_BUTTON (crop->xoffs));
  yoffs = gtk_spin_button_get_value (GTK_SPIN_BUTTON (crop->yoffs));
  width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (crop->width));
  height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (crop->height));
    
  /* clip to image bounds */
  if (xoffs < 0)
    {
      width += xoffs;
      xoffs = 0;
    }
  else if (xoffs >= crop->clip_width)
    {
      xoffs = crop->clip_width - 1;
    }

  if (yoffs < 0)
    {
      height += yoffs;
      yoffs = 0;
    }
  else if (yoffs >= crop->clip_height)
    {
      yoffs = crop->clip_height - 1;
    }

  width = CLAMP (width, 1, crop->clip_width);
  height = CLAMP (height, 1, crop->clip_height);

  /* ensure this is a valid offset/dimension combination */
  if (spinbutton == GTK_SPIN_BUTTON (crop->width)
      || spinbutton == GTK_SPIN_BUTTON (crop->height))
    {
      if (xoffs + width > crop->clip_width)
        xoffs = crop->clip_width - width;
      if (yoffs + height > crop->clip_height)
        yoffs = crop->clip_height - height;
    }
  else
    {
      if (xoffs + width > crop->clip_width)
        width = crop->clip_width - xoffs;
      if (yoffs + height > crop->clip_height)
        height = crop->clip_height - yoffs;
    }

  rect.x = xoffs;
  rect.y = yoffs;
  rect.width = width;
  rect.height = height;

  webx_crop_widget_update_target (crop, &rect);
}
