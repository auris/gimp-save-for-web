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

#ifndef __WEBX_CROP_WIDGET_H__
#define __WEBX_CROP_WIDGET_H__

G_BEGIN_DECLS

#define WEBX_TYPE_CROP_WIDGET                   (webx_crop_widget_get_type ())
#define WEBX_CROP_WIDGET(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_CROP_WIDGET, WebxCropWidget))
#define WEBX_CROP_WIDGET_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_CROP_WIDGET, WebxCropWidgetClass))
#define WEBX_IS_CROP_WIDGET(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_CROP_WIDGET))
#define WEBX_IS_CROP_WIDGET_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_CROP_WIDGET))
#define WEBX_CROP_WIDGET_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((klass), WEBX_TYPE_CROP_WIDGET))

typedef struct _WebxCropWidget          WebxCropWidget;
typedef struct _WebxCropWidgetClass     WebxCropWidgetClass;

struct _WebxCropWidget
{
  GtkVBox        parent_instance;

  GtkWidget     *xoffs;
  GtkWidget     *yoffs;
  GtkWidget     *width;
  GtkWidget     *height;
  GtkWidget     *table;

  GdkRectangle   rect;
  gint           clip_width;
  gint           clip_height;

  gint           stop_recursion;
};

struct _WebxCropWidgetClass
{
  GtkVBoxClass   parent_class;

  void (* crop_changed) (GtkWidget      *widget,
                         GdkRectangle   *new_crop,
                         gpointer        user_data);
};

GType           webx_crop_widget_get_type       (void) G_GNUC_CONST;

GtkWidget*      webx_crop_widget_new            (gint           width,
                                                 gint           height);

void            webx_crop_widget_update_target  (WebxCropWidget *crop,
                                                 GdkRectangle   *rect);

void            webx_crop_widget_update         (WebxCropWidget *crop,
                                                 GdkRectangle   *rect,
                                                 gint            clip_width,
                                                 gint            clip_height);

G_END_DECLS

#endif /* __WEBX_CROP_WIDGET_H__ */
