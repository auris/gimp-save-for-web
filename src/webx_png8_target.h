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
 * Free Software Foundation, Inc., 51 Franklin St. 5th Floor Boston,
 * MA 02110-1301, USA.
 */

#ifndef __WEBX_PNG8_TARGET_H__
#define __WEBX_PNG8_TARGET_H__

#include "webx_indexed_target.h"

G_BEGIN_DECLS

#define WEBX_TYPE_PNG8_TARGET            (webx_png8_target_get_type ())
#define WEBX_PNG8_TARGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_PNG8_TARGET, WebxPng8Target))
#define WEBX_PNG8_TARGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_PNG8_TARGET, WebxPng8TargetClass))
#define WEBX_IS_PNG8_TARGET(obj)         (G_TYPE_CKECK_INSTANCE_TYPE ((obj), WEBX_TYPE_PNG8_TARGET))
#define WEBX_IS_PNG8_TARGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_PNG8_TARGET))
#define WEBX_PNG8_TARGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_PNG8_TARGET, WebxPng8TargetClass))

typedef struct _WebxPng8TargetClass WebxPng8TargetClass;
typedef struct _WebxPng8Target      WebxPng8Target;

struct _WebxPng8Target
{
  WebxIndexedTarget     parent_instance;

  gint                  interlace;
  gint                  compression;
  gint                  bkgd;
  gint                  gama;
  gint                  offs;
  gint                  phys;
  gint                  time;
  gint                  comment;
  gint                  svtrans;

  GtkObject            *interlace_o;
  GtkObject            *compression_o;
};

struct _WebxPng8TargetClass
{
  WebxTargetClass parent_class;
};

GType           webx_png8_target_get_type (void) G_GNUC_CONST;

GtkWidget*      webx_png8_target_new  (void);

gboolean        webx_png8_target_save (WebxTarget      *widget,
                                       WebxTargetInput *input,
                                       gchar           *file_name);

G_END_DECLS

#endif /* __WEBX_PNG8_TARGET_H__ */
