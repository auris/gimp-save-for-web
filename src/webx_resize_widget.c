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
#include "webx_resize_widget.h"

#include "plugin-intl.h"


enum
{
  RESIZED,
  LAST_SIGNAL
};

static void     webx_resize_widget_reset           (GtkButton       *button,
                                                    WebxResizeWidget *resize);
static void     webx_resize_widget_width_changed   (GtkSpinButton    *spinbtn,
                                                    WebxResizeWidget *resize);
static void     webx_resize_widget_height_changed  (GtkSpinButton    *spinbtn,
                                                    WebxResizeWidget *resize);

G_DEFINE_TYPE (WebxResizeWidget, webx_resize_widget, GTK_TYPE_VBOX)

#define parent_class webx_resize_widget_parent_class

static guint webx_resize_widget_signals[LAST_SIGNAL] = { 0 };


static void
webx_resize_widget_class_init (WebxResizeWidgetClass        *klass)
{
  webx_resize_widget_signals[RESIZED] =
      g_signal_new ("resized",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (WebxResizeWidgetClass, resized),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
}

static void
webx_resize_widget_init (WebxResizeWidget   *resize)
{
  resize->aspect_ratio = 1.0;
}

GtkWidget*
webx_resize_widget_new (gint    default_width,
                        gint    default_height)
{
  WebxResizeWidget     *resize;
  GtkWidget            *label;
  GtkWidget            *spinbtn;
  GtkWidget            *frame;
  GtkWidget            *button;

  resize = g_object_new (WEBX_TYPE_RESIZE_WIDGET, NULL);
  resize->default_width = default_width;
  resize->default_height = default_height;
  resize->aspect_ratio = (gdouble) default_width / default_height;

  frame = gtk_frame_new (_("Resize"));
  gtk_box_pack_start (GTK_BOX (resize), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  resize->table = gtk_table_new (2, 5, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (resize->table), 4);
  gtk_container_add (GTK_CONTAINER (frame), resize->table);

  label = gtk_label_new (_("Width:"));
  gtk_table_attach (GTK_TABLE (resize->table), label,
                    0, 1, 0, 1,
                    0, 0, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (1, WEBX_MAX_SIZE, 1);
  resize->width = spinbtn;
  gtk_table_attach (GTK_TABLE (resize->table), spinbtn,
                    1, 2, 0, 1,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             default_width);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_resize_widget_width_changed), resize);

  label = gtk_label_new (_("Height:"));
  gtk_table_attach (GTK_TABLE (resize->table), label,
                    0, 1, 1, 2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  spinbtn = gtk_spin_button_new_with_range (1, WEBX_MAX_SIZE, 1);
  resize->height = spinbtn;
  gtk_table_attach (GTK_TABLE (resize->table), spinbtn,
                    1, 2, 1, 2,
                    0, 0, 0, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbtn),
                             default_height);
  g_signal_connect (spinbtn, "value-changed",
                    G_CALLBACK (webx_resize_widget_height_changed), resize);
  resize->chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach (GTK_TABLE (resize->table), resize->chain,
                    2, 3, 0, 2,
                    0, 0, 0, 0);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (resize->chain), TRUE);

  button = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  gtk_table_attach (GTK_TABLE (resize->table), button,
                    0, 2, 2, 3,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (webx_resize_widget_reset),
                    resize);

  gtk_widget_show_all (resize->table);
 
  return GTK_WIDGET (resize);
}

void
webx_resize_widget_update (WebxResizeWidget     *resize,
                           gint                  width,
                           gint                  height)
{
  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));

  if (resize->stop_recursion > 0)
    return;
  resize->stop_recursion++;

  width = CLAMP (width, 1, WEBX_MAX_SIZE);
  height = CLAMP (height, 1, WEBX_MAX_SIZE);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (resize->width), width);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (resize->height), height);

  g_signal_emit (resize, webx_resize_widget_signals[RESIZED], 0);

  resize->stop_recursion--;
}

void
webx_resize_widget_set_default_size (WebxResizeWidget *resize,
                                     gint              width,
                                     gint              height)
{
  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  resize->default_width = width;
  resize->default_height = height;
  resize->aspect_ratio = (gdouble) width / height;
}

void
webx_resize_widget_get_size (WebxResizeWidget   *resize,
                             gint               *width,
                             gint               *height)
{
  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));

  if (width)
    *width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->width));
  if (height)
    *height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->height));
}

static void
webx_resize_widget_reset (GtkButton            *button,
                          WebxResizeWidget     *resize)
{
  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));

  webx_resize_widget_update (resize,
                             resize->default_width, resize->default_height);
}

static void
webx_resize_widget_width_changed (GtkSpinButton        *spinbutton,
                                  WebxResizeWidget     *resize)
{
  gint            width;
  gint            height;
  gboolean        chain;

  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));
    
  width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->width));
  height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->height));
  chain = gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (resize->chain));
  if ((gdouble) width / resize->aspect_ratio > WEBX_MAX_SIZE)
    width = (gdouble) height * resize->aspect_ratio;
  else if (chain)
    height = (gdouble)width / resize->aspect_ratio;

  webx_resize_widget_update (resize, width, height);
}

static void
webx_resize_widget_height_changed (GtkSpinButton        *spinbutton,
                                   WebxResizeWidget     *resize)
{
  gint            width;
  gint            height;
  gboolean        chain;

  g_return_if_fail (WEBX_IS_RESIZE_WIDGET (resize));
    
  width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->width));
  height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (resize->height));
  chain = gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (resize->chain));
  if ((gdouble)height * resize->aspect_ratio > WEBX_MAX_SIZE)
    height = (gdouble) width / resize->aspect_ratio;
  else if (chain)
    width = (gdouble)height * resize->aspect_ratio;

  webx_resize_widget_update (resize, width, height);
}
