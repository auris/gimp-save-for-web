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

#ifndef __WEBX_PREVIEW_H__
#define __WEBX_PREVIEW_H__

G_BEGIN_DECLS

#define WEBX_TYPE_PREVIEW            (webx_preview_get_type ())
#define WEBX_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_PREVIEW, WebxPreview))
#define WEBX_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_PREVIEW, WebxPreviewClass))
#define WEBX_IS_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_PREVIEW))
#define WEBX_IS_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_PREVIEW))
#define WEBX_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((klass), WEBX_TYPE_PREVIEW, WebxPreviewClass))


typedef struct _WebxPreview      WebxPreview;
typedef struct _WebxPreviewClass WebxPreviewClass;


struct _WebxPreview
{
  GtkVBox               parent_instance;

  GtkWidget            *table;
  GtkWidget            *area;
  GtkWidget            *hscr;
  GtkWidget            *vscr;
  GtkWidget            *zoom_combo;
  GtkWidget            *zoom_in;
  GtkWidget            *zoom_out;
  GtkWidget            *show_preview;
  GtkWidget            *progress_bar;
  GdkCursor            *cursor_move;
  GdkPixbuf            *target;
  GdkPixbuf            *background;
  GdkPixbuf            *original;
  GdkGC                *area_gc;
  
  GtkWidget            *nav_icon;
  GtkWidget            *nav_popup;
  GdkGC                *nav_gc;
  GdkPixbuf            *nav_thumbnail;

  gint                  width;
  gint                  height;
  GdkRectangle          target_rect;
  gdouble               zoom;
  gint                  zoom_mode;
  gint                  cursor_type;
  gint                  drag_mode;
  gint                  xoffs;
  gint                  yoffs;
  /* drag start point  */
  gint                  drag_x;
  gint                  drag_y;
  /* visual crop rect adjustment */
  GdkRectangle          drag_crop;
  /* hand scrolling */
  gint                  scroll_offs_x;
  gint                  scroll_offs_y;
  gint                  scroll_max_x;
  gint                  scroll_max_y;

  gint                  stop_recursion;
};

struct _WebxPreviewClass
{
  GtkVBoxClass          parent_class;

  void (*crop_changed) (GtkWidget    *widget,
                        GdkRectangle *new_crop,
                        gpointer      user_data);
};

GType      webx_preview_get_type      (void) G_GNUC_CONST;

GtkWidget* webx_preview_new           (gint width,
                                       gint height);

void       webx_preview_begin_update  (WebxPreview  *preview);

void       webx_preview_update_target (WebxPreview  *preview,
                                       GdkPixbuf    *target,
                                       gint          file_size);
void       webx_preview_update        (WebxPreview  *preview,
                                       GdkPixbuf    *background,
                                       GdkPixbuf    *target,
                                       GdkRectangle *target_rect,
                                       gint          file_size);

void       webx_preview_resize        (WebxPreview  *preview,
                                       gint          width,
                                       gint          height);
void       webx_preview_crop          (WebxPreview  *preview,
                                       GdkRectangle *crop);

G_END_DECLS

#endif /* __WEBX_PREVIEW_H__ */
