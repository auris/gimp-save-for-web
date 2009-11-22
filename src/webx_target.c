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
#include <glib/gstdio.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "webx_target.h"
#include "webx_utils.h"

enum
{
  SETTINGS_CHANGED,
  LAST_SIGNAL
};


static GdkPixbuf* webx_target_real_render_preview (WebxTarget          *widget,
                                                   WebxTargetInput     *input,
                                                   gint                *file_size);


static void   webx_percent_entry_update (GtkObject *object,
                                         gdouble   *value);
static void   webx_checkbox_update      (GtkObject *object,
                                         gboolean  *value);
static void   webx_range_entry_update   (GtkObject *object,
                                         gint      *value);

G_DEFINE_TYPE (WebxTarget, webx_target, GTK_TYPE_TABLE);

#define parent_class webx_target_parent_class

static guint webx_target_signals[LAST_SIGNAL] = { 0, };

static void
webx_target_class_init (WebxTargetClass *klass)
{
  GtkObjectClass  *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  klass->save_image     = NULL;
  klass->render_preview = webx_target_real_render_preview;
  klass->get_unique_name = NULL;
  klass->get_extension   = NULL;

  klass->target_changed  = NULL;

  webx_target_signals[SETTINGS_CHANGED] =
    g_signal_new ("target-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (WebxTargetClass, target_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
webx_target_init (WebxTarget *widget)
{
}

void
webx_target_changed (WebxTarget *widget)
{
  g_return_if_fail (WEBX_IS_TARGET (widget));

  g_signal_emit (widget, webx_target_signals[SETTINGS_CHANGED], 0);
}

static GdkPixbuf*
webx_target_real_render_preview (WebxTarget            *widget,
                                 WebxTargetInput       *input,
                                 gint                  *file_size)
{
  gchar       *file_name;
  gchar       *extension;
  GdkPixbuf   *pixbuf = NULL;
  gint32       image;

  g_return_val_if_fail (WEBX_IS_TARGET (widget), NULL);

  extension = webx_target_get_extension (WEBX_TARGET (widget));
  file_name = gimp_temp_name (extension);
  if (webx_target_save_image (WEBX_TARGET (widget), input, file_name))
    {
      image = gimp_file_load (GIMP_RUN_NONINTERACTIVE,
                              file_name, file_name);
      if (image != -1)
        {
          pixbuf = webx_image_to_pixbuf (image);
          gimp_image_delete (image);
        }
      if (file_size)
        *file_size = webx_get_file_size (file_name);
    }
  g_unlink (file_name);
  g_free (file_name);

  return pixbuf;
}

gboolean
webx_target_save_image (WebxTarget             *widget,
                        WebxTargetInput        *input,
                        const gchar            *file_name)
{
  g_return_val_if_fail (WEBX_IS_TARGET (widget), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  g_assert (WEBX_TARGET_GET_CLASS (widget)->save_image != NULL);

  return WEBX_TARGET_GET_CLASS (widget)->save_image (widget,
                                                     input,
                                                     file_name);
}

GdkPixbuf*
webx_target_render_preview (WebxTarget         *widget,
                            WebxTargetInput    *input,
                            gint               *file_size)
{
  g_return_val_if_fail (WEBX_IS_TARGET (widget), NULL);

  return WEBX_TARGET_GET_CLASS (widget)->render_preview (widget,
                                                         input,
                                                         file_size);
}

gchar*
webx_target_get_unique_name (WebxTarget  *widget)
{
  g_return_val_if_fail (WEBX_IS_TARGET (widget), NULL);

  return WEBX_TARGET_GET_CLASS (widget)->get_unique_name (widget);
}

gchar*
webx_target_get_extension (WebxTarget  *widget)
{
  g_return_val_if_fail (WEBX_IS_TARGET (widget), NULL);

  return WEBX_TARGET_GET_CLASS (widget)->get_extension (widget);
}

GtkObject*
webx_percent_entry_new (WebxTarget *target,
                        gint        row,
                        gchar      *label,
                        gint        min,
                        gdouble    *value)
{
  GtkObject *adj;

  adj = gimp_scale_entry_new (GTK_TABLE (target), 0, row,
                              label, 90, 6,
                              *value * 100.0, (gdouble)min, 100.0, 1.0, 1.0, 0,
                              TRUE, 0, 0, NULL, NULL);
  g_object_set_data (G_OBJECT (adj), "target", target);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (webx_percent_entry_update), value);
  return adj;
}

void
webx_percent_entry_set (GtkObject *entry,
                        gdouble    value)
{
  gtk_range_set_value (GTK_RANGE (GIMP_SCALE_ENTRY_SCALE (entry)),
                       value * 100.0);
}

static void
webx_percent_entry_update (GtkObject *object,
                           gdouble   *value)
{
  WebxTarget *target;
  GtkRange   *range;
  gdouble     newval;

  target = g_object_get_data (G_OBJECT (object), "target");

  range = GIMP_SCALE_ENTRY_SCALE (object);
  newval = gtk_range_get_value (GTK_RANGE (range)) / 100.0;
  if (*value != newval)
    {
      *value = newval;
      webx_target_changed (WEBX_TARGET (target));
    }
}

GtkObject*
webx_checkbox_new (WebxTarget *target,
                   gint        row,
                   gchar      *label,
                   gboolean   *value)
{
  GtkWidget *checkbox;

  checkbox = gtk_check_button_new_with_mnemonic (label);
  gtk_table_attach (GTK_TABLE (target), checkbox,
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), *value);
  g_object_set_data (G_OBJECT (checkbox), "target", target);
  g_signal_connect (checkbox, "toggled",
                    G_CALLBACK (webx_checkbox_update),
                    value);
  gtk_widget_show (checkbox);

  return GTK_OBJECT (checkbox);
}

void
webx_checkbox_set (GtkObject *object,
                   gboolean   value)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (object),
                                value);
}

static void
webx_checkbox_update (GtkObject *object,
                      gboolean  *value)
{
  WebxTarget *target;
  gboolean    newval;

  target = g_object_get_data (G_OBJECT (object), "target");

  newval = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (object));
  if (*value != newval)
    {
      *value = newval;
      webx_target_changed (WEBX_TARGET (target));
    }
}

GtkObject*
webx_range_entry_new (WebxTarget *target,
                      gint        row,
                      gchar      *label,
                      gint        min,
                      gint        max,
                      gint       *value)
{
  GtkObject *adj;

  adj = gimp_scale_entry_new (GTK_TABLE (target), 0, row,
                              label, 90, 6,
                              *value, (gdouble)min, (gdouble)max, 1.0, 1.0, 0,
                              TRUE, 0, 0, NULL, NULL);
  g_object_set_data (G_OBJECT (adj), "target", target);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (webx_range_entry_update), value);

  return adj;
}

void
webx_range_entry_set (GtkObject *entry,
                      gint       value)
{
  gtk_range_set_value (GTK_RANGE (GIMP_SCALE_ENTRY_SCALE (entry)),
                       value * 100.0);
}

static void
webx_range_entry_update (GtkObject *object,
                         gint      *value)
{
  WebxTarget *target;
  GtkRange   *range;
  gdouble     newval;

  target = g_object_get_data (G_OBJECT (object), "target");

  range = GIMP_SCALE_ENTRY_SCALE (object);
  newval = (gint) gtk_range_get_value (range);
  if (*value != newval)
    {
      *value = newval;
      webx_target_changed (WEBX_TARGET (target));
    }
}
