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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gprintf.h>

#include "webx_main.h"
#include "webx_pipeline.h"
#include "webx_utils.h"
#include "webx_target.h"

#include "plugin-intl.h"

#define WEBX_PIPELINE_UPDATE_DELAY      150

static void     webx_pipeline_destroy      (GtkObject  *object);
static void     webx_pipeline_crop_clip    (WebxPipeline *pipeline);
static gboolean webx_pipeline_check_update (WebxPipeline *pipeline);
static gboolean webx_pipeline_timeout_update     (WebxPipeline     *pipeline);

static void     webx_pipeline_invalidate         (WebxPipeline     *pipeline,
                                                gboolean        update_all);

enum
{
  INVALIDATED,
  OUTPUT_CHANGED,
  LAST_SIGNAL
};


G_DEFINE_TYPE (WebxPipeline, webx_pipeline, GTK_TYPE_OBJECT);

#define parent_class webx_pipeline_parent_class

static guint webx_pipeline_signals[LAST_SIGNAL] = { 0 };

static void
webx_pipeline_class_init (WebxPipelineClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);
  object_class->destroy = webx_pipeline_destroy;

  klass->invalidated = NULL;
  klass->output_changed = NULL;

  webx_pipeline_signals[INVALIDATED] =
      g_signal_new ("invalidated",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (WebxPipelineClass, invalidated),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  webx_pipeline_signals[OUTPUT_CHANGED] =
    g_signal_new ("output-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (WebxPipelineClass, output_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);
}

static void
webx_pipeline_init (WebxPipeline *pipeline)
{
  pipeline->user_image = -1;
  pipeline->user_drawable = -1;
  pipeline->rgb_image = -1;
  pipeline->rgb_layer = -1;
  pipeline->indexed_image = -1;
  pipeline->indexed_layer = -1;

  pipeline->crop_scale_x = 1.0;
  pipeline->crop_scale_y = 1.0;

  pipeline->timeout_id = 0;
}

GtkObject*
webx_pipeline_new (gint image_ID,
                   gint drawable_ID)
{
  WebxPipeline *pipeline;

  pipeline = g_object_new (WEBX_TYPE_PIPELINE, NULL);
  pipeline->user_image = image_ID;
  pipeline->user_drawable = drawable_ID;
  pipeline->original_width = gimp_image_width (image_ID);
  pipeline->original_height = gimp_image_height (image_ID);
  pipeline->resize_width = pipeline->original_width;
  pipeline->resize_height = pipeline->original_height;
  pipeline->crop_width = pipeline->original_width;
  pipeline->crop_height = pipeline->original_height;
  pipeline->crop_offsx = 0;
  pipeline->crop_offsy = 0;

  return GTK_OBJECT (pipeline);
}


static void
webx_pipeline_destroy (GtkObject *object)
{
  WebxPipeline *pipeline;
  pipeline = WEBX_PIPELINE (object);

  if (pipeline->rgb_image != -1)
    {
      gimp_image_delete (pipeline->rgb_image);
      pipeline->rgb_image = -1;
    }
  if (pipeline->indexed_image != -1)
    {
      gimp_image_delete (pipeline->indexed_image);
      pipeline->indexed_image = -1;
    }
  if (pipeline->background)
    {
      g_object_unref (pipeline->background);
      pipeline->background = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (GTK_OBJECT (pipeline));
}

void
webx_pipeline_run (WebxPipeline    *pipeline)
{
  webx_pipeline_invalidate (pipeline, TRUE);
  pipeline->last_update = pipeline->update_count; /* don't wait too long */
}

void
webx_pipeline_get_target_rect (WebxPipeline   *pipeline,
                             GdkRectangle *rect)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));
  g_return_if_fail (rect != NULL);

  rect->x = pipeline->crop_offsx;
  rect->y = pipeline->crop_offsy;
  rect->width = pipeline->crop_width;
  rect->height = pipeline->crop_height;
}

void
webx_pipeline_get_background_dimensions (WebxPipeline *pipeline,
                                       gint       *width,
                                       gint       *height)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));

  if (width)
    *width = pipeline->resize_width;
  if (height)
    *height = pipeline->resize_height;
}

GdkPixbuf*
webx_pipeline_get_background_pixbuf (WebxPipeline *pipeline)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), NULL);

  return pipeline->background;
}


gboolean
webx_pipeline_resize (WebxPipeline *pipeline,
                      gint        new_width,
                      gint        new_height)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), FALSE);
  
  new_width = CLAMP (new_width, 1, WEBX_MAX_SIZE);
  new_height = CLAMP (new_height, 1, WEBX_MAX_SIZE);
  
  if (pipeline->resize_width == new_width
      && pipeline->resize_height == new_height)
    return FALSE;

  /* scale crop */
  if (pipeline->resize_width == pipeline->crop_width
      && pipeline->resize_height == pipeline->crop_height
      && pipeline->crop_offsx == 0
      && pipeline->crop_offsy == 0)
    {
      /* avoid roundoff errors when user doesn't want to crop */
      pipeline->crop_width = new_width;
      pipeline->crop_height = new_height;
        
      pipeline->crop_scale_x = 1.0;
      pipeline->crop_scale_y = 1.0;
    }
  else
    {
      pipeline->crop_scale_x *= (gdouble)new_width/pipeline->resize_width;
      pipeline->crop_scale_y *= (gdouble)new_height/pipeline->resize_height;
    }

  pipeline->resize_width = new_width;
  pipeline->resize_height = new_height;
  webx_pipeline_invalidate (pipeline, TRUE);
  return TRUE;
}

static void
webx_pipeline_crop_clip (WebxPipeline *pipeline)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));

  /* clip to image bounds */
  pipeline->crop_offsx = CLAMP (pipeline->crop_offsx, 0, pipeline->resize_width - 1);
  pipeline->crop_offsy = CLAMP (pipeline->crop_offsy, 0, pipeline->resize_height - 1);
  pipeline->crop_width = CLAMP (pipeline->crop_width, 1, pipeline->resize_width);
  pipeline->crop_height = CLAMP (pipeline->crop_height, 1, pipeline->resize_height);
  if (pipeline->crop_offsx + pipeline->crop_width > pipeline->resize_width)
    pipeline->crop_width = pipeline->resize_width - pipeline->crop_offsx;
  if (pipeline->crop_offsy + pipeline->crop_height > pipeline->resize_height)
    pipeline->crop_height = pipeline->resize_height - pipeline->crop_offsy;
}

gboolean
webx_pipeline_crop (WebxPipeline *pipeline,
                  gint        width,
                  gint        height,
                  gint        offsx,
                  gint        offsy,
                  gboolean    clip_offsets)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), FALSE);

  /* clip to image bounds */
  if (offsx < 0)
    {
      width += offsx;
      offsx = 0;
    }
  else if (offsx >= pipeline->resize_width)
    {
      offsx = pipeline->resize_width - 1;
    }

  if (offsy < 0)
    {
      height += offsy;
      offsy = 0;
    }
  else if (offsy >= pipeline->resize_height)
    {
      offsy = pipeline->resize_height - 1;
    }

  width = CLAMP (width, 1, pipeline->resize_width);
  height = CLAMP (height, 1, pipeline->resize_height);

  /* ensure this is a valid offset/dimension combination */
  if (clip_offsets)
    {
      if (offsx + width > pipeline->resize_width)
        offsx = pipeline->resize_width - width;
      if (offsy + height > pipeline->resize_height)
        offsy = pipeline->resize_height - height;
    }
  else
    {
      if (offsx + width > pipeline->resize_width)
        width = pipeline->resize_width - offsx;
      if (offsy + height > pipeline->resize_height)
        height = pipeline->resize_height - offsy;
    }

  if (width == pipeline->crop_width
      && height == pipeline->crop_height
      && offsx == pipeline->crop_offsx
      && offsy == pipeline->crop_offsy)
    return FALSE; /* crop options not changed */

  pipeline->crop_width = width;
  pipeline->crop_height = height;
  pipeline->crop_offsx = offsx;
  pipeline->crop_offsy = offsy;
  pipeline->crop_scale_x = 1.0;
  pipeline->crop_scale_y = 1.0;
  webx_pipeline_invalidate (pipeline, TRUE);
  return TRUE;
}

void
webx_pipeline_set_target (WebxPipeline     *pipeline,
                        GtkObject      *target)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));
  g_return_if_fail (WEBX_IS_TARGET (target));

  pipeline->target = target;
  webx_pipeline_invalidate (pipeline, FALSE);
}

gboolean
webx_pipeline_save_image (WebxPipeline *pipeline,
                          gchar        *filename)
{
  WebxPipelineOutput    output;
  WebxTargetInput       target_input;
  gboolean              result;

  memset (&output, 0, sizeof (output));

  if (pipeline->update_all)
    {
      webx_pipeline_check_update (pipeline);

      output.background = pipeline->background;
      output.bg_width = pipeline->resize_width;
      output.bg_height = pipeline->resize_height;
      output.target_rect.x = pipeline->crop_offsx;
      output.target_rect.y = pipeline->crop_offsy;
      output.target_rect.width = pipeline->crop_width;
      output.target_rect.height = pipeline->crop_height;
    }

  target_input.rgb_image = pipeline->rgb_image;
  target_input.rgb_layer = pipeline->rgb_layer;
  target_input.indexed_image = pipeline->indexed_image;
  target_input.indexed_layer = pipeline->indexed_layer;
  target_input.width = pipeline->crop_width;
  target_input.height = pipeline->crop_height;
  result = webx_target_save_image (WEBX_TARGET (pipeline->target),
                                   &target_input,
                                   filename);

  pipeline->update_count = 0;
  pipeline->last_update = 0;
  pipeline->update_all = FALSE;

  if (output.target)
    g_object_ref_sink (output.target);
  g_signal_emit (pipeline, webx_pipeline_signals[OUTPUT_CHANGED], 0,
                 &output);
  if (output.target)
    g_object_unref (output.target);

  return result;
}



gint
webx_pipeline_get_rgb_target (WebxPipeline *pipeline,
                              gint       *layer)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), -1);

  if (layer)
    *layer = pipeline->rgb_layer;
  return pipeline->rgb_image;
}

gint
webx_pipeline_get_indexed_target (WebxPipeline *pipeline,
                                gint       *layer)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), -1);

  if (layer)
    *layer = pipeline->indexed_layer;
  return pipeline->indexed_image;
}

static void
webx_pipeline_invalidate (WebxPipeline *pipeline,
                          gboolean      update_all)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));

  if (update_all)
    {
      pipeline->update_all = TRUE;
    }
  pipeline->update_count++;

  if (pipeline->timeout_id == 0)
    {
      g_signal_emit (pipeline, webx_pipeline_signals[INVALIDATED], 0);
      pipeline->timeout_id = g_timeout_add (WEBX_PIPELINE_UPDATE_DELAY,
                                           (GSourceFunc)webx_pipeline_timeout_update,
                                           pipeline);
    }
}

static void
webx_pipeline_create_background (WebxPipeline *pipeline)
{
  g_return_if_fail (WEBX_IS_PIPELINE (pipeline));

  if (gimp_drawable_is_rgb (pipeline->rgb_layer))
    {
      pipeline->background = webx_drawable_to_pixbuf (pipeline->rgb_layer);
    }
  else
    {
      /* pipeline->rgb_image is still original image, which can be
         non rgb (as we create background early in pipeline). */
      gint duplicate = gimp_image_duplicate (pipeline->rgb_image);
      gimp_image_undo_disable (duplicate);
      pipeline->background = webx_image_to_pixbuf (duplicate);
      gimp_image_delete (duplicate);
    }
}

static gboolean
webx_pipeline_check_update (WebxPipeline *pipeline)
{
  gint *layers;
  gint  num_layers;
  gint  i;

  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), FALSE);

  if (pipeline->rgb_image != -1)
    {
      gimp_image_delete (pipeline->rgb_image);
      pipeline->rgb_image = -1;
    }
  if (pipeline->indexed_image != -1)
    {
      gimp_image_delete (pipeline->indexed_image);
      pipeline->indexed_image = -1;
    }
  if (pipeline->background)
    {
      g_object_unref (pipeline->background);
      pipeline->background = NULL;
    }

  pipeline->rgb_image = gimp_image_duplicate (pipeline->user_image);
  gimp_image_undo_disable (pipeline->rgb_image);
  pipeline->rgb_layer =
    gimp_image_merge_visible_layers (pipeline->rgb_image,
                                     GIMP_CLIP_TO_IMAGE);

  /* make sure there is only one layer, where all visible layers were merged */
  layers = gimp_image_get_layers (pipeline->rgb_image, &num_layers);
  for (i = 0; i < num_layers; i++)
    {
      if (layers[i] != pipeline->rgb_layer)
        gimp_image_remove_layer (pipeline->rgb_image, layers[i]);
    }
  g_free (layers);

  /* we don't want layer to be smaller than image */
  gimp_layer_resize_to_image_size (pipeline->rgb_layer);
  gimp_image_scale (pipeline->rgb_image,
                    pipeline->resize_width, pipeline->resize_height);
  webx_pipeline_create_background (pipeline);

  pipeline->crop_offsx *= pipeline->crop_scale_x;
  pipeline->crop_offsy *= pipeline->crop_scale_y;
  pipeline->crop_width *= pipeline->crop_scale_x;
  pipeline->crop_height *= pipeline->crop_scale_y;
  pipeline->crop_scale_x = 1.0;
  pipeline->crop_scale_y = 1.0;
  webx_pipeline_crop_clip (pipeline);

  if (pipeline->crop_width != pipeline->resize_width
      || pipeline->crop_height != pipeline->resize_height )
    {
      gimp_image_crop (pipeline->rgb_image,
                       pipeline->crop_width, pipeline->crop_height,
                       pipeline->crop_offsx, pipeline->crop_offsy);
    }
    
  if (gimp_drawable_is_indexed (pipeline->rgb_layer))
    {
      pipeline->indexed_image = gimp_image_duplicate (pipeline->rgb_image);
      gimp_image_undo_disable (pipeline->indexed_image);
      pipeline->indexed_layer =
        gimp_image_merge_visible_layers (pipeline->indexed_image,
                                         GIMP_CLIP_TO_IMAGE);
    }
  else
    {
      pipeline->indexed_image = -1;
      pipeline->indexed_layer = -1;
    }

  if ( ! gimp_drawable_is_rgb (pipeline->rgb_layer))
    gimp_image_convert_rgb (pipeline->rgb_image);

  return TRUE;
}

static void
webx_pipeline_update (WebxPipeline *pipeline)
{
  WebxPipelineOutput    output;
  WebxTargetInput       target_input;

  memset (&output, 0, sizeof (output));

  if (pipeline->update_all)
    {
      webx_pipeline_check_update (pipeline);

      output.background = pipeline->background;
      output.bg_width = pipeline->resize_width;
      output.bg_height = pipeline->resize_height;
      output.target_rect.x = pipeline->crop_offsx;
      output.target_rect.y = pipeline->crop_offsy;
      output.target_rect.width = pipeline->crop_width;
      output.target_rect.height = pipeline->crop_height;

      pipeline->update_all = FALSE;
    }

  target_input.rgb_image = pipeline->rgb_image;
  target_input.rgb_layer = pipeline->rgb_layer;
  target_input.indexed_image = pipeline->indexed_image;
  target_input.indexed_layer = pipeline->indexed_layer;
  target_input.width = pipeline->crop_width;
  target_input.height = pipeline->crop_height;
  output.target = webx_target_render_preview (WEBX_TARGET (pipeline->target),
                                              &target_input,
                                              &output.file_size);

  pipeline->update_count = 0;
  pipeline->last_update = 0;

  g_signal_emit (pipeline, webx_pipeline_signals[OUTPUT_CHANGED], 0,
                 &output);
  if (output.target)
    g_object_unref (output.target);
}

static gboolean
webx_pipeline_timeout_update (WebxPipeline  *pipeline)
{
  g_return_val_if_fail (WEBX_IS_PIPELINE (pipeline), TRUE);

  if (pipeline->update_count)
    {
      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
          pipeline->update_count++;
        }

      if (pipeline->update_count != pipeline->last_update
          || gdk_pointer_is_grabbed ())
        {
          /* user has changed something; wait */
          pipeline->last_update = pipeline->update_count;
          return TRUE;
        }
      else
        {
          pipeline->updating = TRUE;
          webx_pipeline_update (pipeline);
          pipeline->updating = FALSE;
        }
    }

  pipeline->timeout_id = 0;
  return FALSE;
}

gboolean
webx_pipeline_is_busy (WebxPipeline    *pipeline)
{
  return pipeline->updating;
}
