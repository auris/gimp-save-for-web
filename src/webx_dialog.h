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

#ifndef __WEBX_DIALOG_H__
#define __WEBX_DIALOG_H__

#include <libgimpwidgets/gimpwidgets.h>

G_BEGIN_DECLS

#define WEBX_TYPE_DIALOG             (webx_dialog_get_type ())
#define WEBX_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBX_TYPE_DIALOG, WebxDialog))
#define WEBX_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), WEBX_TYPE_DIALOG, WebxDialogClass))
#define WEBX_IS_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBX_TYPE_DIALOG))
#define WEBX_IS_DIALOG_CLASS (klass) (G_TYPE_CLASS_TYPE ((klass), WEBX_TYPE_DIALOG))
#define WEBX_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBX_TYPE_DIALOG, WebxDialogClass))

typedef struct _WebxDialog      WebxDialog;
typedef struct _WebxDialogClass WebxDialogClass;

struct _WebxDialog
{
  GimpDialog    parent_instance;

  GtkObject    *pipeline;

  /*
   * TOOLBOX */
  GtkWidget    *splitter;
  GtkWidget    *target;
  GSList       *target_list;
  GSList       *radio_list;
  GtkWidget    *crop;
  GtkWidget    *resize;

  /*
   * PREVIEW */
  GtkWidget    *preview;

  /*
   * STATUS BAR */
  gdouble       zoom;
  GtkWidget    *zoom_combo;

  gint          file_size;
  GtkWidget    *file_format_label;
  GtkWidget    *file_size_label;
  GtkWidget    *target_dimensions_label;


  /* Create this to keep away useless messages from user.
   * The widget is not shown */
  GtkWidget    *progress_bar;
};

struct _WebxDialogClass
{
  GimpDialogClass  parent_class;
};

GType      webx_dialog_get_type (void) G_GNUC_CONST;

GtkWidget* webx_dialog_new (gint  image_ID,
                            gint  drawable_ID);

void       webx_dialog_run (WebxDialog *dlg);

G_END_DECLS

#endif /* __WEBX_DIALOG_H__ */
