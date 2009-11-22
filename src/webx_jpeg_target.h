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

#ifndef __WEBX_JPEG_TARGET_H__
#define __WEBX_JPEG_TARGET_H__

#include "webx_target.h"

G_BEGIN_DECLS

#define WEBX_TYPE_JPEG_TARGET            (webx_jpeg_target_get_type ())
#define WEBX_JPEG_TARGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_JPEG_TARGET, WebxJpegTarget))
#define WEBX_JPEG_TARGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_JPEG_TARGET, WebxJpegTargetClass))
#define WEBX_IS_JPEG_TARGET(obj)         (G_TYPE_CKECK_INSTANCE_TYPE ((obj), WEBX_TYPE_JPEG_TARGET))
#define WEBX_IS_JPEG_TARGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_JPEG_TARGET))
#define WEBX_JPEG_TARGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_JPEG_TARGET, WebxJpegTargetClass))

typedef struct _WebxJpegTargetClass WebxJpegTargetClass;
typedef struct _WebxJpegTarget      WebxJpegTarget;

struct _WebxJpegTarget
{
  WebxTarget  parent_instance;

  gdouble     quality;
  gdouble     smoothing;
  gint        subsmp;
  gint        restart;
  gint        dct;
  gboolean    optimize;
  gboolean    progressive;
  gboolean    baseline;
  gboolean    strip_exif;

  GtkObject  *quality_adj;
  GtkObject  *smoothing_adj;
  GtkObject  *optimize_adj;
  GtkObject  *progressive_adj;
  GtkObject  *baseline_adj;
  GtkObject  *strip_exif_adj;
};

struct _WebxJpegTargetClass
{
  WebxTargetClass parent_class;
};

GType           webx_jpeg_target_get_type (void) G_GNUC_CONST;

GtkWidget*      webx_jpeg_target_new  (void);

gboolean        webx_jpeg_target_save (WebxTarget      *widget,
                                       WebxTargetInput *input,
                                       gchar           *file_name);

G_END_DECLS

#endif /* __WEBX_JPEG_TARGET_H__ */
