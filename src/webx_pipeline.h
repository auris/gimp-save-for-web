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

/*
   pipeline performs following stages of processing:
   1) resize
   2) crop
   3) compress
*/

#ifndef __WEBX_PIPELINE_H__
#define __WEBX_PIPELINE_H__

G_BEGIN_DECLS

#define WEBX_TYPE_PIPELINE             (webx_pipeline_get_type ())
#define WEBX_PIPELINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_PIPELINE, WebxPipeline))
#define WEBX_PIPELINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_PIPELINE, WebxPipelineClass))
#define WEBX_IS_PIPELINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_PIPELINE))
#define WEBX_IS_PIPELINE_CLASS (klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_PIPELINE))
#define WEBX_PIPELINE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_PIPELINE, WebxPipeline))

typedef struct _WebxPipelineOutput      WebxPipelineOutput;

typedef struct _WebxPipeline            WebxPipeline;
typedef struct _WebxPipelineClass       WebxPipelineClass;

struct _WebxPipelineOutput
{
  GdkPixbuf    *target;
  GdkPixbuf    *background;

  GdkRectangle  target_rect;
  gint          bg_width;
  gint          bg_height;

  gint          file_size;
};

struct _WebxPipeline
{
  GtkObject     parent_instance;

  /* image dimensions, as they come from original image */
  gint          original_width;
  gint          original_height;
  /* after resizing stage */
  gint          resize_width;
  gint          resize_height;
  gdouble       crop_scale_x;
  gdouble       crop_scale_y;
  /* after crop stage */
  gint          crop_width;
  gint          crop_height;
  gint          crop_offsx;
  gint          crop_offsy;

  /* image from user (never touched) */
  gint          user_image;
  gint          user_drawable;
  /* rgb image after resize & crop transformations */
  gint          rgb_image;
  gint          rgb_layer;
  /* paletted image after resize & crop transformations */
  gint          indexed_image;
  gint          indexed_layer;

  GdkPixbuf    *background;

  GtkObject    *target;

  /* we use this for timeout function to make sure
   * user has stopped editing. If we find the same
   * update count 2 cycles, we can check for changes. */
  guint         timeout_id;
  gint          update_count;
  gint          last_update;
  gboolean      update_all;
  gboolean      updating;
};

struct _WebxPipelineClass
{
  GtkObjectClass   parent_class;

  void  (* invalidated)        (WebxPipeline             *src,
                                gpointer                user_data);
  void  (* output_changed)     (WebxPipeline             *src,
                                WebxPipelineOutput     *output,
                                gpointer                user_data);
};

GType       webx_pipeline_get_type (void) G_GNUC_CONST;

GtkObject*  webx_pipeline_new                   (gint image_ID,
                                               gint drawable_ID);

void            webx_pipeline_run        (WebxPipeline     *pipeline);

/* target */
void        webx_pipeline_get_target_rect       (WebxPipeline    *pipeline,
                                               GdkRectangle  *rect);
gint        webx_pipeline_get_rgb_target        (WebxPipeline *pipeline,
                                               gint       *layer);
gint        webx_pipeline_get_indexed_target    (WebxPipeline *pipeline,
                                               gint       *layer);
/* background */
void        webx_pipeline_get_background_dimensions (WebxPipeline  *pipeline,
                                                   gint        *width,
                                                   gint        *height);
GdkPixbuf*  webx_pipeline_get_background_pixbuf     (WebxPipeline *pipeline);

/* operations */
gboolean     webx_pipeline_resize (WebxPipeline *pipeline,
                                 gint        newheight,
                                 gint        newwidth);
gboolean     webx_pipeline_crop   (WebxPipeline *pipeline,
                                 gint        width,
                                 gint        height,
                                 gint        offsx,
                                 gint        offsy,
                                 gboolean    clip_offsets);

void            webx_pipeline_set_target (WebxPipeline *pipeline,
                                          GtkObject    *target);

gboolean        webx_pipeline_save_image (WebxPipeline *pipeline,
                                          gchar        *filename);

gboolean        webx_pipeline_is_busy    (WebxPipeline *pipeline);

G_END_DECLS

#endif /* __WEBX_PIPELINE_H__ */
