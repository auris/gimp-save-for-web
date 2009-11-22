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

#ifndef __WEBX_INDEXED_TARGET_H__
#define __WEBX_INDEXED_TARGET_H__

#include "webx_target.h"

G_BEGIN_DECLS

#define WEBX_TYPE_INDEXED_TARGET            (webx_indexed_target_get_type ())
#define WEBX_INDEXED_TARGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_INDEXED_TARGET, WebxIndexedTarget))
#define WEBX_INDEXED_TARGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_INDEXED_TARGET, WebxIndexedTargetClass))
#define WEBX_IS_INDEXED_TARGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_INDEXED_TARGET))
#define WEBX_IS_INDEXED_TARGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_INDEXED_TARGET))
#define WEBX_INDEXED_TARGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_INDEXED_TARGET, WebxIndexedTargetClass))

typedef struct _WebxIndexedTargetClass WebxIndexedTargetClass;
typedef struct _WebxIndexedTarget      WebxIndexedTarget;

struct _WebxIndexedTarget
{
  WebxTarget  parent_instance;

  gint        image;
  gint        layer;
    
  gint        dither_type;
  gint        palette_type;
  gint        num_colors;
    
  gchar      *custom_palette;

  GtkWidget  *reuse_pal_w;
  GtkWidget  *make_pal_w;
  GtkWidget  *web_pal_w;
  GtkWidget  *bw_pal_w;
  GtkWidget  *dither_type_w;
  GtkWidget  *num_colors_w;
  GtkWidget  *alpha_dither_w;
  GtkWidget  *remove_unused_w;
 
  gboolean    alpha_dither;
  gboolean    remove_unused;

  gint        last_row;
};

struct _WebxIndexedTargetClass
{
  WebxTargetClass parent_class;
};

GType           webx_indexed_target_get_type    (void) G_GNUC_CONST;

gint            webx_indexed_target_get_image   (WebxIndexedTarget     *indexed,
                                                 WebxTargetInput       *input,
                                                 gint                  *layer);
void            webx_indexed_target_free_image  (WebxIndexedTarget     *indexed,
                                                 WebxTargetInput       *input,
                                                 gint                   image);


G_END_DECLS

#endif /* __WEBX_INDEXED_TARGET_H__ */
