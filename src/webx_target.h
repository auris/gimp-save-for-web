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

#ifndef __WEBX_TARGET_H__
#define __WEBX_TARGET_H__

G_BEGIN_DECLS

#define WEBX_TYPE_TARGET            (webx_target_get_type ())
#define WEBX_TARGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_TARGET, WebxTarget))
#define WEBX_TARGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_TARGET, WebxTargetClass))
#define WEBX_IS_TARGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_TARGET))
#define WEBX_IS_TARGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBX_TYPE_TARGET))
#define WEBX_TARGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_TARGET, WebxTargetClass))

typedef struct _WebxTargetInput WebxTargetInput;

typedef struct _WebxTargetClass WebxTargetClass;
typedef struct _WebxTarget      WebxTarget;

struct _WebxTargetInput
{
  gint  rgb_image;
  gint  rgb_layer;

  gint  indexed_image;
  gint  indexed_layer;

  gint  width;
  gint  height;
};

struct _WebxTarget
{
  GtkTable    parent_instance;
};

struct _WebxTargetClass
{
  GtkTableClass parent_class;

  gboolean   (* save_image)       (WebxTarget          *widget,
                                   WebxTargetInput     *input,
                                   const gchar         *file_name);
  GdkPixbuf* (* render_preview)   (WebxTarget          *widget,
                                   WebxTargetInput     *input,
                                   gint                *file_size);
  gchar*     (* get_unique_name)  (WebxTarget  *widget);
  gchar*     (* get_extension)    (WebxTarget  *widget);

  void       (* target_changed) (WebxTarget  *widget);
};

GType      webx_target_get_type        (void) G_GNUC_CONST;

void       webx_target_changed         (WebxTarget *widget);

gboolean   webx_target_save_image      (WebxTarget             *widget,
                                        WebxTargetInput        *input,
                                        const gchar            *file_name);
GdkPixbuf* webx_target_render_preview  (WebxTarget             *widget,
                                        WebxTargetInput        *input,
                                        gint                   *file_size);
gchar*     webx_target_get_unique_name (WebxTarget  *widget);
gchar*     webx_target_get_extension   (WebxTarget  *widget);


/* convenience routines */

GtkObject*        webx_percent_entry_new (WebxTarget *target,
                                          gint        row,
                                          gchar      *label,
                                          gint        min,
                                          gdouble    *value);
void              webx_percent_entry_set (GtkObject  *entry,
                                          gdouble     value);

GtkObject*        webx_checkbox_new      (WebxTarget *target,
                                          gint        row,
                                          gchar      *label,
                                          gboolean   *value);
void              webx_checkbox_set      (GtkObject  *checkbox,
                                          gboolean    value);

GtkObject*        webx_range_entry_new   (WebxTarget *target,
                                          gint        row,
                                          gchar      *label,
                                          gint        min,
                                          gint        max,
                                          gint       *value);
void              webx_range_entry_set   (GtkObject  *object,
                                          gint        value);

G_END_DECLS

#endif /* __WEBX_TARGET_H__ */
