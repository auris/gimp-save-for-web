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
#include "webx_jpeg_target.h"

#include "plugin-intl.h"

static gboolean webx_jpeg_target_save_image    (WebxTarget             *widget,
                                                WebxTargetInput        *input,
                                                const gchar            *file_name);
static gchar* webx_jpeg_target_get_unique_name (WebxTarget             *widget);
static gchar* webx_jpeg_target_get_extension   (WebxTarget             *widget);

G_DEFINE_TYPE (WebxJpegTarget, webx_jpeg_target, WEBX_TYPE_TARGET)

#define parent_class webx_jpeg_target_parent_class


static void
webx_jpeg_target_class_init (WebxJpegTargetClass *klass)
{
  GObjectClass        *object_class;
  WebxTargetClass *target_class;

  object_class = G_OBJECT_CLASS (klass);

  target_class = WEBX_TARGET_CLASS (klass);
  target_class->save_image      = webx_jpeg_target_save_image;
  target_class->get_unique_name = webx_jpeg_target_get_unique_name;
  target_class->get_extension   = webx_jpeg_target_get_extension;
}

static void
webx_jpeg_target_init (WebxJpegTarget *jpeg)
{
  jpeg->quality       = 0.85;
  jpeg->smoothing     = 0.0;
  jpeg->subsmp        = 0;
  jpeg->restart       = 0;
  jpeg->dct           = 0;
  jpeg->optimize      = TRUE;
  jpeg->progressive   = FALSE;
  jpeg->baseline      = TRUE;
  jpeg->strip_exif    = FALSE;

  jpeg->quality_adj     = NULL;
  jpeg->smoothing_adj   = NULL;
  jpeg->optimize_adj    = NULL;
  jpeg->progressive_adj = NULL;
  jpeg->baseline_adj    = NULL;
  jpeg->strip_exif_adj  = NULL;
}

GtkWidget*
webx_jpeg_target_new (void)
{
  WebxJpegTarget *jpeg;
  gint            row = 0;

  jpeg = g_object_new (WEBX_TYPE_JPEG_TARGET, NULL);

  jpeg->quality_adj = webx_percent_entry_new (WEBX_TARGET (jpeg),
                                              row++,
                                              _("_Quality"), 6,
                                              &jpeg->quality);
  jpeg->smoothing_adj = webx_percent_entry_new (WEBX_TARGET (jpeg),
                                                row++,
                                                _("_Smoothing"), 0,
                                                &jpeg->smoothing);
  jpeg->optimize_adj = webx_checkbox_new (WEBX_TARGET (jpeg),
                                          row++,
                                          _("_Optimize"),
                                          &jpeg->optimize);
  jpeg->progressive_adj = webx_checkbox_new (WEBX_TARGET (jpeg),
                                             row++,
                                             _("_Progressive"),
                                             &jpeg->progressive);
  jpeg->baseline_adj = webx_checkbox_new (WEBX_TARGET (jpeg),
                                          row++,
                                          _("_Baseline"),
                                          &jpeg->baseline);
  jpeg->strip_exif_adj = webx_checkbox_new (WEBX_TARGET (jpeg),
                                            row++,
                                            _("Strip _EXIF"),
                                            &jpeg->strip_exif);

  return GTK_WIDGET (jpeg);
}

static gboolean
webx_jpeg_target_save_image (WebxTarget        *widget,
                             WebxTargetInput   *input,
                             const gchar       *file_name)
{
  WebxJpegTarget *jpeg;
  GimpParam      *return_vals;
  gint            n_return_vals;
  gint32          image;
  gint32          layer;
  gint32          save_image;
  gint32          save_layer;
  gboolean        save_res;

  jpeg = WEBX_JPEG_TARGET (widget);

  image = input->rgb_image;
  layer = input->rgb_layer;
  if (gimp_drawable_has_alpha (layer) || jpeg->strip_exif)
    {
      /* jpeg doesn't support alpha */
      save_image = gimp_image_duplicate (image);
      gimp_image_undo_disable (image);
      save_layer = gimp_image_flatten (save_image);

      if (jpeg->strip_exif)
        {
          gimp_image_parasite_detach (save_image, "exif-data");
          gimp_image_parasite_detach (save_image, "gimp-metadata");
        }
    }
  else
    {
      save_image = image;
      save_layer = layer;
    }

  return_vals =
      gimp_run_procedure ("file-jpeg-save", &n_return_vals,
                          GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                          GIMP_PDB_IMAGE, save_image,
                          GIMP_PDB_DRAWABLE, save_layer,
                          GIMP_PDB_STRING, file_name,
                          GIMP_PDB_STRING, file_name,
                          GIMP_PDB_FLOAT, jpeg->quality,
                          GIMP_PDB_FLOAT, jpeg->smoothing,
                          GIMP_PDB_INT32, (gint)jpeg->optimize,
                          GIMP_PDB_INT32, (gint)jpeg->progressive,
                          GIMP_PDB_STRING, "",
                          GIMP_PDB_INT32, jpeg->subsmp,
                          GIMP_PDB_INT32, (gint)jpeg->baseline,
                          GIMP_PDB_INT32, jpeg->restart,
                          GIMP_PDB_INT32, jpeg->dct,
                          GIMP_PDB_END);
  if (return_vals[0].data.d_int32 == GIMP_PDB_SUCCESS)
    save_res = TRUE;
  else
    save_res = FALSE;
  gimp_destroy_params (return_vals, n_return_vals);

  if (gimp_drawable_has_alpha (layer))
    {
      gimp_image_delete (save_image);
    }
  return save_res;
}

static gchar*
webx_jpeg_target_get_unique_name (WebxTarget *widget)
{
  return "jpeg";
}

static gchar*
webx_jpeg_target_get_extension (WebxTarget *widget)
{
  return "jpg";
}
