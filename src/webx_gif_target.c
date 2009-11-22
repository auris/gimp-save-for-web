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
#include "webx_gif_target.h"

#include "plugin-intl.h"

static gboolean webx_gif_target_save_image    (WebxTarget      *widget,
                                               WebxTargetInput *input,
                                               const gchar     *file_name);
static gchar* webx_gif_target_get_unique_name (WebxTarget     *widget);
static gchar* webx_gif_target_get_extension   (WebxTarget     *widget);

G_DEFINE_TYPE (WebxGifTarget, webx_gif_target, WEBX_TYPE_INDEXED_TARGET)

#define parent_class webx_gif_target_parent_class


static void
webx_gif_target_class_init (WebxGifTargetClass *klass)
{
  GObjectClass         *object_class;
  WebxTargetClass      *target_class;

  object_class = G_OBJECT_CLASS (klass);

  target_class = WEBX_TARGET_CLASS (klass);
  target_class->save_image      = webx_gif_target_save_image;
  target_class->get_unique_name = webx_gif_target_get_unique_name;
  target_class->get_extension   = webx_gif_target_get_extension;
}

static void
webx_gif_target_init (WebxGifTarget *gif)
{
}

GtkWidget*
webx_gif_target_new (void)
{
  WebxGifTarget *gif;
  gint           row = 0;

  gif = g_object_new (WEBX_TYPE_GIF_TARGET, NULL);

  row = WEBX_INDEXED_TARGET (gif)->last_row;
  gif->interlace_o = webx_checkbox_new (WEBX_TARGET (gif),
                                        row++,
                                        _("_Interlace"),
                                        &gif->interlace);

  return GTK_WIDGET (gif);
}

static gboolean
webx_gif_target_save_image (WebxTarget  *widget,
                            WebxTargetInput    *input,
                             const gchar *file_name)
{
  WebxGifTarget        *gif;
  GimpParam            *return_vals;
  gint                  n_return_vals;
  gint                  image;
  gint                  layer;
  gboolean              save_res;

  gif = WEBX_GIF_TARGET (widget);
  image = webx_indexed_target_get_image (WEBX_INDEXED_TARGET (widget),
                                         input,
                                         &layer);

  return_vals = gimp_run_procedure ("file-gif-save", &n_return_vals,
                                    GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                    GIMP_PDB_IMAGE, image,
                                    GIMP_PDB_DRAWABLE, layer,
                                    GIMP_PDB_STRING, file_name,
                                    GIMP_PDB_STRING, file_name,
                                    GIMP_PDB_INT32, gif->interlace,
                                    GIMP_PDB_INT32, 0,
                                    GIMP_PDB_INT32, 0,
                                    GIMP_PDB_INT32, 0,
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
webx_gif_target_get_unique_name (WebxTarget *widget)
{
  return "gif";
}

static gchar*
webx_gif_target_get_extension (WebxTarget *widget)
{
  return "gif";
}
