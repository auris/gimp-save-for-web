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

#include "webx_main.h"
#include "webx_png8_target.h"

#include "plugin-intl.h"

static gboolean webx_png8_target_save_image    (WebxTarget             *widget,
                                                WebxTargetInput        *input,
                                                const gchar            *file_name);
static gchar* webx_png8_target_get_unique_name (WebxTarget     *widget);
static gchar* webx_png8_target_get_extension   (WebxTarget     *widget);

G_DEFINE_TYPE (WebxPng8Target, webx_png8_target, WEBX_TYPE_INDEXED_TARGET)

#define parent_class webx_png8_target_parent_class


static void
webx_png8_target_class_init (WebxPng8TargetClass *klass)
{
  GObjectClass         *object_class;
  WebxTargetClass      *target_class;

  object_class = G_OBJECT_CLASS (klass);

  target_class = WEBX_TARGET_CLASS (klass);
  target_class->save_image      = webx_png8_target_save_image;
  target_class->get_unique_name = webx_png8_target_get_unique_name;
  target_class->get_extension   = webx_png8_target_get_extension;
}

static void
webx_png8_target_init (WebxPng8Target *png8)
{
  GimpParam  *return_vals;
  gint        n_return_vals;

  return_vals = gimp_run_procedure ("file-png-get-defaults", &n_return_vals,
                                    GIMP_PDB_END);
  png8->interlace    = return_vals[1].data.d_int32;
  png8->compression  = return_vals[2].data.d_int32;
  png8->bkgd         = return_vals[3].data.d_int32;
  png8->gama         = return_vals[4].data.d_int32;
  png8->offs         = return_vals[5].data.d_int32;
  png8->phys         = return_vals[6].data.d_int32;
  png8->time         = return_vals[7].data.d_int32;
  png8->comment      = return_vals[8].data.d_int32;
  png8->svtrans      = return_vals[9].data.d_int32;
  gimp_destroy_params (return_vals, n_return_vals);
}

GtkWidget*
webx_png8_target_new (void)
{
  WebxPng8Target *png8;
  gint            row = 0;

  png8 = g_object_new (WEBX_TYPE_PNG8_TARGET, NULL);

  row = WEBX_INDEXED_TARGET (png8)->last_row;
  png8->interlace_o = webx_checkbox_new (WEBX_TARGET (png8),
                                         row++,
                                         _("_Interlace"),
                                         &png8->interlace);
  png8->compression_o = webx_range_entry_new (WEBX_TARGET (png8),
                                              row++,
                                              _("_Compression"), 0, 9,
                                              &png8->compression);

  return GTK_WIDGET (png8);
}

static gboolean
webx_png8_target_save_image (WebxTarget        *widget,
                             WebxTargetInput   *input,
                             const gchar       *file_name)
{
  WebxPng8Target       *png8;
  GimpParam            *return_vals;
  gint                  n_return_vals;
  gint                  image;
  gint                  layer;
  gboolean              save_res;

  png8 = WEBX_PNG8_TARGET (widget);
  image = webx_indexed_target_get_image (WEBX_INDEXED_TARGET (widget),
                                         input,
                                         &layer);

  return_vals = gimp_run_procedure ("file-png-save", &n_return_vals,
                                    GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                    GIMP_PDB_IMAGE, image,
                                    GIMP_PDB_DRAWABLE, layer,
                                    GIMP_PDB_STRING, file_name,
                                    GIMP_PDB_STRING, file_name,
                                    GIMP_PDB_INT32, png8->interlace,
                                    GIMP_PDB_INT32, png8->compression,
                                    GIMP_PDB_INT32, png8->bkgd,
                                    GIMP_PDB_INT32, png8->gama,
                                    GIMP_PDB_INT32, png8->offs,
                                    GIMP_PDB_INT32, png8->phys,
                                    GIMP_PDB_INT32, png8->time,
                                    GIMP_PDB_END);
  if (return_vals[0].data.d_int32 == GIMP_PDB_SUCCESS)
    save_res = TRUE;
  else
    save_res = FALSE;
  gimp_destroy_params (return_vals, n_return_vals);
  webx_indexed_target_free_image (WEBX_INDEXED_TARGET (widget), input, image);

  return save_res;
}

static gchar*
webx_png8_target_get_unique_name (WebxTarget *widget)
{
  return "png8";
}

static gchar*
webx_png8_target_get_extension (WebxTarget *widget)
{
  return "png";
}
