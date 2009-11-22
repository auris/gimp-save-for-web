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

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "webx_main.h"
#include "webx_dialog.h"
#include "webx_pipeline.h"
#include "webx_preview.h"
#include "webx_utils.h"
#include "webx_prefs.h"

#include "webx_crop_widget.h"
#include "webx_resize_widget.h"

#include "webx_jpeg_target.h"
#include "webx_png8_target.h"
#include "webx_png24_target.h"
#include "webx_gif_target.h"

#include "plugin-intl.h"




static void     webx_dialog_destroy             (GtkObject *object);

static gboolean webx_dialog_save_dialog         (WebxDialog    *dlg);

static void     webx_dialog_reset               (WebxDialog    *dlg);
static void     webx_dialog_update              (WebxDialog    *dlg,
                                                 WebxPipelineOutput    *output);
static void     webx_dialog_target_changed      (WebxTarget    *target,
                                                 WebxDialog    *dialog);
static void     webx_dialog_crop_changed        (GtkWidget     *widget,
                                                 GdkRectangle  *crop,
                                                 WebxDialog    *dlg);
static void     webx_dialog_target_resized      (GtkWidget     *widget,
                                                 WebxDialog    *dlg);

static void     webx_dialog_format_set          (WebxDialog    *dlg,
                                                 WebxTarget    *format);
static void     webx_dialog_format_changed      (GtkToggleButton *togglebutton,
                                                 WebxDialog      *dlg);

G_DEFINE_TYPE (WebxDialog, webx_dialog, GIMP_TYPE_DIALOG)

#define parent_class webx_dialog_parent_class


static WebxPrefs webx_prefs;


static void
webx_dialog_class_init (WebxDialogClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);
  object_class->destroy = webx_dialog_destroy;
}

static void
webx_dialog_init (WebxDialog *dlg)
{
  dlg->target = NULL;
  dlg->target_list = NULL;
}

static gboolean
webx_dialog_close (WebxDialog  *dlg)
{
  static gboolean is_closed = FALSE;

  if (is_closed)
    return FALSE;

#if 0
  if (webx_pipeline_is_busy (WEBX_PIPELINE (dlg->pipeline)))
    {
      /* let pipeline finish it's processing,
         otherwise it may break some pipes. */
      g_idle_add ((GSourceFunc) (webx_dialog_close), dlg);
      printf ("waiting for pipeline to finish... \n");
      return FALSE;
    }

  printf ("cleaning up\n");
#endif

  if (GTK_WIDGET_REALIZED (dlg))
    {
      gdk_window_get_geometry (GDK_WINDOW (GTK_WIDGET (dlg)->window),
                               &webx_prefs.dlg_x,
                               &webx_prefs.dlg_y,
                               &webx_prefs.dlg_width,
                               &webx_prefs.dlg_height,
                               NULL);
    }

  webx_prefs.dlg_splitpos = gtk_paned_get_position (GTK_PANED (dlg->splitter));
  webx_prefs_save (&webx_prefs);

  gtk_widget_destroy (GTK_WIDGET (dlg));

  is_closed = TRUE;
  return FALSE;
}

static void
webx_dialog_destroy (GtkObject *object)
{
  WebxDialog    *dlg = WEBX_DIALOG (object);

  if (dlg->pipeline)
    {
      g_object_unref (dlg->pipeline);
      dlg->pipeline = NULL;
    }

  if (dlg->target_list)
    {
      g_slist_free (dlg->target_list);
      dlg->target_list = NULL;
    }

  if (dlg->radio_list)
    {
      g_slist_free (dlg->radio_list);
      dlg->radio_list = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (GTK_OBJECT (dlg));
}


static void
webx_dialog_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      if (webx_dialog_save_dialog (WEBX_DIALOG (widget)))
        {
          webx_dialog_close (WEBX_DIALOG (widget));
        }
      break;

    default:
      webx_dialog_close (WEBX_DIALOG (widget));
      break;
    }
}

GtkWidget*
webx_dialog_new (gint image_ID,
                 gint drawable_ID)
{
  WebxDialog   *dlg;
  GtkWidget    *splitter;
  GtkWidget    *tbscroll;
  GtkWidget    *toolbox;
  GtkWidget    *format_table;
  GtkWidget    *left_box;
  GtkWidget    *preview_box;
  GtkWidget    *box;
  GtkWidget    *vbox;
  GtkWidget    *frame;
  GtkWidget    *separator;
  GtkWidget    *target;
  GtkWidget    *radio;
  GtkWidget    *notebook;
  GtkWidget    *label;
  GSList       *group;
  GSList       *target_list;
  GSList       *radio_list;
  gint          bg_width;
  gint          bg_height;
  gint          align;
  gint          row;
  
  
  webx_prefs_load (&webx_prefs);

  dlg = g_object_new (WEBX_TYPE_DIALOG,
                      "title",     (gchar*)_("Save for Web"),
                      "role",      (gchar*)PLUG_IN_BINARY,
                      "modal",     (GtkDialogFlags)GTK_DIALOG_MODAL,
                      "help-func", (GimpHelpFunc)gimp_standard_help_func,
                      "help-id",   (gchar*)PLUG_IN_PROC,
                      NULL);
  gimp_dialog_add_buttons (GIMP_DIALOG (dlg),

                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                           NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gimp_window_set_transient (GTK_WINDOW (dlg));

  if (webx_prefs.dlg_width && webx_prefs.dlg_height)
    {
      gtk_window_set_default_size (GTK_WINDOW (dlg),
                                   webx_prefs.dlg_width, webx_prefs.dlg_height);
    }
  else
    {
      gtk_window_set_default_size (GTK_WINDOW (dlg), 728, 480);
    }

  if (webx_prefs.dlg_x && webx_prefs.dlg_y)
    {
      gtk_window_move (GTK_WINDOW (dlg),
                       webx_prefs.dlg_x,
                       webx_prefs.dlg_y);
    }

  g_signal_connect (dlg, "response",
                    G_CALLBACK (webx_dialog_response), NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);


  dlg->pipeline = webx_pipeline_new (image_ID,
                                 drawable_ID);
  g_signal_connect_swapped (dlg->pipeline, "invalidated",
                            G_CALLBACK (webx_dialog_reset),
                            dlg);
  g_signal_connect_swapped (dlg->pipeline, "output-changed",
                            G_CALLBACK (webx_dialog_update),
                            dlg);
  g_object_ref_sink (dlg->pipeline);
  
  bg_width = gimp_image_width (image_ID);
  bg_height = gimp_image_height (image_ID);

  splitter = gtk_hpaned_new ();
  dlg->splitter = splitter;
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), GTK_WIDGET (splitter),
                      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (splitter));

  left_box = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (splitter), GTK_WIDGET (left_box),
                   FALSE, FALSE);
  gtk_widget_show (left_box);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (left_box), frame,
                      FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (left_box), notebook,
                      TRUE, TRUE, 0);
  gtk_widget_show (notebook);
  
  tbscroll = gtk_scrolled_window_new (NULL, NULL);
  label = gtk_image_new_from_stock (GIMP_STOCK_IMAGE,
                                    GTK_ICON_SIZE_MENU);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            tbscroll,
                            label);

  gtk_container_set_border_width (GTK_CONTAINER (tbscroll), 4);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tbscroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (GTK_WIDGET (tbscroll));

  toolbox = gtk_vbox_new (FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (tbscroll),
                                         GTK_WIDGET (toolbox));
  gtk_container_set_border_width (GTK_CONTAINER (toolbox), 4);
  gtk_widget_show (GTK_WIDGET (toolbox));

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_widget_show (vbox);

  format_table = gtk_table_new (3, 2, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (format_table),
                      FALSE, FALSE, 0);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), separator,
                      FALSE, FALSE, 8);

  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (box),
                      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (box));
  dlg->file_size_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (dlg->file_size_label),
                      FALSE, FALSE, 0);

  target = webx_jpeg_target_new ();
  dlg->target_list = g_slist_append (dlg->target_list, target);
  radio = gtk_radio_button_new_with_label (NULL, _("JPEG"));
  dlg->radio_list = g_slist_append (dlg->radio_list, radio);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  target = NULL; 
  dlg->target_list = g_slist_append (dlg->target_list, target);
  radio = gtk_radio_button_new_with_label (group, _("JPEG 2000"));
  dlg->radio_list = g_slist_append (dlg->radio_list, radio);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  target = webx_png8_target_new ();
  dlg->target_list = g_slist_append (dlg->target_list, target);
  radio = gtk_radio_button_new_with_label (group, _("PNG-8"));
  dlg->radio_list = g_slist_append (dlg->radio_list, radio);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  target = webx_png24_target_new ();
  dlg->target_list = g_slist_append (dlg->target_list, target);
  radio = gtk_radio_button_new_with_label (group, _("PNG-24"));
  dlg->radio_list = g_slist_append (dlg->radio_list, radio);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  target = webx_gif_target_new ();
  dlg->target_list = g_slist_append (dlg->target_list, target);
  radio = gtk_radio_button_new_with_label (group, _("GIF"));
  dlg->radio_list = g_slist_append (dlg->radio_list, radio);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));

  align = 0;
  row = 0;
  for (target_list = dlg->target_list, radio_list = dlg->radio_list;
       target_list && radio_list;
       target_list = target_list->next, radio_list = radio_list->next)
    {
      gtk_table_attach (GTK_TABLE (format_table), GTK_WIDGET (radio_list->data),
                        align, align+1, row, row+1,
                        GTK_FILL, GTK_FILL, 0, 0);
      g_signal_connect (radio_list->data, "toggled",
                        G_CALLBACK (webx_dialog_format_changed), dlg);
      gtk_widget_show (GTK_WIDGET (radio_list->data));

      if (target_list->data)
        {
          gtk_box_pack_start (GTK_BOX (toolbox), GTK_WIDGET (target_list->data),
                              FALSE, FALSE, 0);
          g_signal_connect (target_list->data, "target-changed",
                            G_CALLBACK (webx_dialog_target_changed), dlg);
        }
      else
        {
          gtk_widget_set_sensitive (GTK_WIDGET (radio_list->data), FALSE);
        }

      align ^= 1;
      if (! align)
        row++;
    }

  gtk_widget_show_all (GTK_WIDGET (format_table));
  dlg->target = NULL;

  tbscroll = gtk_scrolled_window_new (NULL, NULL);
  label = gtk_image_new_from_stock (GIMP_STOCK_RESIZE,
                                    GTK_ICON_SIZE_MENU);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            tbscroll,
                            label);

  gtk_container_set_border_width (GTK_CONTAINER (tbscroll), 4);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tbscroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tbscroll),
                                       GTK_SHADOW_NONE);
  gtk_widget_show (GTK_WIDGET (tbscroll));

  toolbox = gtk_vbox_new (FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (tbscroll),
                                         GTK_WIDGET (toolbox));
  gtk_container_set_border_width (GTK_CONTAINER (toolbox), 4);
  gtk_widget_show (GTK_WIDGET (toolbox));

  dlg->resize = webx_resize_widget_new (bg_width, bg_height);
  gtk_box_pack_start (GTK_BOX (toolbox), dlg->resize,
                      FALSE, FALSE, 0);
  g_signal_connect (WEBX_RESIZE_WIDGET (dlg->resize), "resized",
                    G_CALLBACK (webx_dialog_target_resized), dlg);
  webx_resize_widget_set_default_size (WEBX_RESIZE_WIDGET (dlg->resize),
                                       bg_width, bg_height);
  gtk_widget_show (dlg->resize);

  dlg->crop = webx_crop_widget_new (bg_width, bg_height);
  gtk_box_pack_start (GTK_BOX (toolbox), dlg->crop,
                      FALSE, FALSE, 0);
  g_signal_connect (WEBX_CROP_WIDGET (dlg->crop), "crop-changed",
                    G_CALLBACK (webx_dialog_crop_changed), dlg);
  gtk_widget_show (dlg->crop);

  preview_box = gtk_vbox_new (TRUE, 0);
  gtk_paned_pack2 (GTK_PANED (splitter), GTK_WIDGET (preview_box),
                   TRUE, FALSE);
  gtk_widget_show (GTK_WIDGET (preview_box));

  dlg->preview = webx_preview_new (bg_width, bg_height);
  gtk_box_pack_start (GTK_BOX (preview_box), dlg->preview,
                      TRUE, TRUE, 0);
  g_signal_connect (WEBX_PREVIEW (dlg->preview), "crop-changed",
                    G_CALLBACK (webx_dialog_crop_changed), dlg);
  gtk_widget_show (GTK_WIDGET (dlg->preview));

  dlg->progress_bar = gimp_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), dlg->progress_bar,
                      FALSE, FALSE, 0);

  if (webx_prefs.dlg_splitpos)
    {
      gtk_paned_set_position (GTK_PANED (splitter),
                              webx_prefs.dlg_splitpos);
    }

  return GTK_WIDGET (dlg);
}

void
webx_dialog_run (WebxDialog *dlg)
{
  g_return_if_fail (WEBX_IS_DIALOG (dlg));
 
  if (! dlg->target)
    webx_dialog_format_set (WEBX_DIALOG (dlg),
                            WEBX_TARGET (dlg->target_list->data));

  webx_dialog_reset (dlg);
  webx_pipeline_run (WEBX_PIPELINE (dlg->pipeline));
  gtk_window_present (GTK_WINDOW (dlg));
  gtk_main();
}

static gboolean
webx_dialog_save_dialog (WebxDialog       *dlg)
{
  GtkWidget  *save_dlg;
  gchar       default_name[1024];
  gint        source_image;
  gboolean    saved = FALSE;

  save_dlg = gtk_file_chooser_dialog_new (_("Save Image"),
                                          GTK_WINDOW (dlg),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,

                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,

                                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (save_dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (save_dlg),
                                                  TRUE);

  source_image = webx_pipeline_get_rgb_target (WEBX_PIPELINE (dlg->pipeline), NULL);
  g_snprintf (default_name, sizeof (default_name),
              "%s.%s",
              gimp_image_get_name (source_image),
              webx_target_get_extension (WEBX_TARGET (dlg->target)));
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_dlg), default_name);

  if (gtk_dialog_run (GTK_DIALOG (save_dlg)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (save_dlg));
      saved = webx_pipeline_save_image (WEBX_PIPELINE (dlg->pipeline), filename);
      if (! saved)
        g_message (_("Failed to save the file!")); 
      g_free (filename);
    }

  gtk_widget_destroy (save_dlg);
  return saved;
}

static void
webx_dialog_format_set (WebxDialog     *dlg,
                        WebxTarget     *format)
{
  GSList               *item;
  gint                  position;

  g_return_if_fail (WEBX_IS_DIALOG (dlg));
  g_return_if_fail (WEBX_IS_TARGET (format));

  webx_dialog_reset (dlg);

  if (format == WEBX_TARGET (dlg->target)
      || format == NULL)
    return;

  for (item = dlg->target_list; item; item = g_slist_next (item))
    {
      if (! item->data)
        continue;

      if (format == item->data)
        gtk_widget_show (GTK_WIDGET (item->data));
      else
        gtk_widget_hide (GTK_WIDGET (item->data));
    }

  position = g_slist_index (dlg->target_list, format);
  item = g_slist_nth (dlg->radio_list, position);
  g_assert (item->data);
  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (item->data)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item->data), TRUE);

  dlg->target = GTK_WIDGET (format);

  webx_pipeline_set_target (WEBX_PIPELINE (dlg->pipeline),
                          GTK_OBJECT (dlg->target));
}

static void
webx_dialog_format_changed (GtkToggleButton *togglebutton,
                            WebxDialog      *dlg)
{
  g_return_if_fail (WEBX_IS_DIALOG (dlg));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    {
      GtkWidget    *widget;
      gint          position;

      position = g_slist_index (dlg->radio_list, togglebutton);
      widget = GTK_WIDGET (g_slist_nth (dlg->target_list, position)->data);
      webx_dialog_format_set (WEBX_DIALOG (dlg), WEBX_TARGET (widget));
    }
}

static void
webx_dialog_reset (WebxDialog *dlg)
{
  webx_preview_begin_update (WEBX_PREVIEW (dlg->preview));

  gtk_label_set_text (GTK_LABEL (dlg->file_size_label),
                      _("File size: unknown"));
}

static void
webx_dialog_update (WebxDialog         *dlg,
                    WebxPipelineOutput *output)
{
  gchar         text[256];

  g_return_if_fail (WEBX_DIALOG (dlg));

  if (output->background)
    {
      webx_preview_update (WEBX_PREVIEW (dlg->preview),
                           output->background, output->target,
                           &output->target_rect,
                           output->file_size);
      webx_crop_widget_update (WEBX_CROP_WIDGET (dlg->crop),
                               &output->target_rect,
                               output->bg_width, output->bg_height);
      webx_resize_widget_update (WEBX_RESIZE_WIDGET (dlg->resize),
                                 output->bg_width, output->bg_height);
    }
  else
    {
      webx_preview_update_target (WEBX_PREVIEW (dlg->preview),
                                  output->target,
                                  output->file_size);
    }

  g_snprintf (text, sizeof (text),
              _("File size: %02.01f kB"),
              (gdouble) output->file_size / 1024.0);
  gtk_label_set_text (GTK_LABEL (dlg->file_size_label), text);
}


static void
webx_dialog_target_changed (WebxTarget *target,
                            WebxDialog *dlg)
{
  g_return_if_fail (WEBX_DIALOG (dlg));

  webx_pipeline_set_target (WEBX_PIPELINE (dlg->pipeline),
                          GTK_OBJECT (target));
}

static void
webx_dialog_crop_changed (GtkWidget    *widget,
                          GdkRectangle *crop,
                          WebxDialog   *dlg)
{
  g_return_if_fail (WEBX_DIALOG (dlg));

  if (! webx_pipeline_crop (WEBX_PIPELINE (dlg->pipeline),
                          crop->width, crop->height,
                          crop->x, crop->y,
                          FALSE))
    return;

  webx_crop_widget_update_target (WEBX_CROP_WIDGET (dlg->crop),
                                  crop);
  webx_preview_crop (WEBX_PREVIEW (dlg->preview),
                     crop);
}

static void
webx_dialog_target_resized (GtkWidget   *widget,
                            WebxDialog  *dlg)
{
  gint  width;
  gint  height;


  webx_resize_widget_get_size (WEBX_RESIZE_WIDGET (widget),
                               &width, &height);
  if (! webx_pipeline_resize (WEBX_PIPELINE (dlg->pipeline),
                            width, height))
    return;

  webx_preview_resize (WEBX_PREVIEW (dlg->preview),
                       width, height);
}
