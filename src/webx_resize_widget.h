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

#ifndef __WEBX_RESIZE_WIDGET_H__
#define __WEBX_RESIZE_WIDGET_H__

G_BEGIN_DECLS

#define WEBX_TYPE_RESIZE_WIDGET                   (webx_resize_widget_get_type ())
#define WEBX_RESIZE_WIDGET(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_RESIZE_WIDGET, WebxResizeWidget))
#define WEBX_RESIZE_WIDGET_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_RESIZE_WIDGET, WebxResizeWidgetClass))
#define WEBX_IS_RESIZE_WIDGET(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_RESIZE_WIDGET))
#define WEBX_IS_RESIZE_WIDGET_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_RESIZE_WIDGET))
#define WEBX_RESIZE_WIDGET_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((klass), WEBX_TYPE_RESIZE_WIDGET))

typedef struct _WebxResizeWidget          WebxResizeWidget;
typedef struct _WebxResizeWidgetClass     WebxResizeWidgetClass;

struct _WebxResizeWidget
{
  GtkVBox        parent_instance;

  GtkWidget     *width;
  GtkWidget     *height;
  GtkWidget     *chain;
  GtkWidget     *table;

  gdouble        aspect_ratio;

  /* dimensions of original image */
  gint           default_width;
  gint           default_height;

  gint           stop_recursion;
};

struct _WebxResizeWidgetClass
{
  GtkVBoxClass   parent_class;

  void (* resized) (GtkWidget      *widget,
                    gpointer        user_data);
};

GType           webx_resize_widget_get_type       (void) G_GNUC_CONST;

GtkWidget*      webx_resize_widget_new            (gint         default_width,
                                                   gint         default_height);

void            webx_resize_widget_update         (WebxResizeWidget *resize,
                                                   gint              width,
                                                   gint              height);

void            webx_resize_widget_set_default_size (WebxResizeWidget *resize,
                                                     gint              width,
                                                     gint              height);

void            webx_resize_widget_get_size       (WebxResizeWidget  *resize,
                                                   gint              *width,
                                                   gint              *height);

G_END_DECLS

#endif /* __WEBX_RESIZE_WIDGET_H__ */
