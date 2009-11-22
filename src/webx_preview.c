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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "webx_main.h"
#include "webx_preview.h"
#include "cursors.h"

#include "plugin-intl.h"

#define WEBX_PREVIEW_PADDING    20
#define WEBX_NAV_PEN_WIDTH      3
#define WEBX_THUMBNAIL_SIZE     128

#define WEBX_DRAG_HAND          (1<<0)
#define WEBX_DRAG_LEFT          (1<<1)
#define WEBX_DRAG_RIGHT         (1<<2)
#define WEBX_DRAG_TOP           (1<<3)
#define WEBX_DRAG_BOTTOM        (1<<4)

#define WEBX_HANDLE_SIZE        4

#define WEBX_DEFAULT_ZOOMLEVEL  3

enum
{
  WEBX_ZOOM_MODE_NORMAL,
  WEBX_ZOOM_MODE_FIT_WIDTH,
  WEBX_ZOOM_MODE_FIT_VISIBLE
};

enum
{
  CROP_CHANGED,
  LAST_SIGNAL
};

static void     webx_preview_destroy            (GtkObject       *object);


static gboolean webx_preview_area_expose        (GtkWidget       *widget,
                                                 GdkEventExpose  *event,
                                                 WebxPreview     *preview);
static void     webx_preview_area_realize       (GtkWidget      *widget,
                                                 WebxPreview    *preview);
static void     webx_preview_area_unrealize     (GtkWidget      *widget,
                                                 WebxPreview    *preview);
static void     webx_preview_area_size_allocate (GtkWidget      *widget,
                                                 GtkAllocation  *allocation,
                                                 WebxPreview    *preview);

static void     webx_preview_hscroll_update     (WebxPreview    *preview);
static void     webx_preview_vscroll_update     (WebxPreview    *preview);

static void     webx_preview_hscroll            (GtkAdjustment  *adj,
                                                 WebxPreview    *preview);
static void     webx_preview_vscroll            (GtkAdjustment  *vadj,
                                                 WebxPreview    *preview);

static void      webx_preview_area_button_press   (GtkWidget       *widget,
                                                   GdkEventButton  *event,
                                                   WebxPreview     *preview);
static void      webx_preview_area_button_release (GtkWidget       *widget,
                                                   GdkEventButton  *event,
                                                   WebxPreview     *preview);
static void      webx_preview_area_motion_notify  (GtkWidget       *widget,
                                                   GdkEventMotion  *event,
                                                   WebxPreview     *preview);
static gboolean webx_preview_area_scroll          (GtkWidget      *widget,
                                                   GdkEventScroll *event,
                                                   WebxPreview    *preview);
static void      webx_preview_draw_outline      (WebxPreview    *preview,
                                                 GdkRectangle   *rect,
                                                 gboolean        filled,
                                                 gboolean        active);
static void      webx_preview_hand_start        (WebxPreview    *preview);
static void      webx_preview_hand_drag         (WebxPreview    *preview,
                                                 gdouble         delta_x,
                                                 gdouble         delta_y);
static void      webx_preview_crop_start        (WebxPreview    *preview);
static void      webx_preview_crop_drag         (WebxPreview    *preview,
                                                 gint            delta_x,
                                                 gint            delta_y);
static void      webx_preview_update_cursor     (WebxPreview    *preview,
                                                 gint            x,
                                                 gint            y);

static GdkPixbuf* webx_preview_create_background (GdkPixbuf *src);

static void     webx_preview_get_background_rect (WebxPreview  *preview,
                                                  GdkRectangle *rect);
static void     webx_preview_get_target_rect     (WebxPreview  *preview,
                                                  GdkRectangle *rect);
static gint     webx_preview_get_drag_type       (WebxPreview  *preview,
                                                  gint          x,
                                                  gint          y);

static gboolean webx_preview_nav_button_press   (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 WebxPreview    *preview);
static void     webx_preview_nav_popup_realize  (GtkWidget      *widget,
                                                 WebxPreview    *preview);
static void     webx_preview_nav_popup_unrealize (GtkWidget      *widget,
                                                  WebxPreview    *preview);
static gboolean webx_preview_nav_popup_event    (GtkWidget      *widget,
                                                 GdkEvent       *event,
                                                 WebxPreview    *preview);
static gboolean webx_preview_nav_popup_expose   (GtkWidget      *widget,
                                                 GdkEventExpose *event,
                                                 WebxPreview    *preview);

/* zoom functionality */
static void     webx_preview_zoom               (WebxPreview   *preview,
                                                 gdouble        level);
static void     webx_preview_set_zoom_mode      (WebxPreview   *preview,
                                                 gint           zoom_mode);
static void     webx_preview_zoom_combo_changed (GtkWidget     *combo,
                                                 WebxPreview   *preview);
static void     webx_preview_zoom_in            (WebxPreview   *preview);
static void     webx_preview_zoom_out           (WebxPreview   *preview);
static gint     webx_preview_closest_zoomlevel  (WebxPreview   *preview);

static void     webx_preview_show_toggled       (WebxPreview   *preview);

G_DEFINE_TYPE (WebxPreview, webx_preview, GTK_TYPE_VBOX)

#define parent_class webx_preview_parent_class

const static gdouble webx_zoomlevels[] = {
  0.125, 0.25, 0.5,
  1.0,
  1.5, 2.0, 4.0, 8.0, 16.0,
};

#define WEBX_MIN_ZOOMLEVEL      0.125
#define WEBX_MAX_ZOOMLEVEL      16.0

static guint webx_preview_signals[LAST_SIGNAL] = { 0 };

static void
webx_preview_class_init (WebxPreviewClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  object_class->destroy = webx_preview_destroy;

  widget_class = GTK_WIDGET_CLASS (klass);

  webx_preview_signals[CROP_CHANGED] =
    g_signal_new ("crop-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (WebxPreviewClass, crop_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_RECTANGLE);
}

static void
webx_preview_init (WebxPreview *preview)
{
  preview->area = NULL;
  preview->width = 0;
  preview->height = 0;
  preview->background = NULL;
  preview->target = NULL;
  preview->target_rect.x = 0;
  preview->target_rect.y = 0;
  preview->target_rect.width = 0;
  preview->target_rect.height = 0;
  preview->zoom = 1.0;
  preview->cursor_type = 0;
  preview->drag_mode = 0;
}

GtkWidget*
webx_preview_new (gint width,
                  gint height)
{
  WebxPreview  *preview;
  GtkWidget    *image;
  GtkWidget    *button_bar;
  GtkWidget    *button;
  GtkObject    *adj;
  gchar        *zoom_labels[G_N_ELEMENTS (webx_zoomlevels)+2];
  gint          i;

  preview = g_object_new (WEBX_TYPE_PREVIEW, NULL);
  preview->width = width;
  preview->height = height;
  preview->target_rect.x = 0;
  preview->target_rect.y = 0;
  preview->target_rect.width = width;
  preview->target_rect.height = height;

  preview->table = gtk_table_new (4, 4, FALSE);
  gtk_box_pack_start (GTK_BOX (preview), preview->table, TRUE, TRUE, 0);
  gtk_widget_show (preview->table);

  preview->area = gtk_drawing_area_new ();
  gtk_table_attach (GTK_TABLE (WEBX_PREVIEW (preview)->table),
                    preview->area, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_add_events (GTK_WIDGET (preview->area),
                         GDK_POINTER_MOTION_MASK
                         | GDK_POINTER_MOTION_HINT_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         | GDK_SCROLL_MASK);

  g_signal_connect (preview->area, "expose-event",
                    G_CALLBACK (webx_preview_area_expose),
                    preview);
  g_signal_connect (preview->area, "realize",
                    G_CALLBACK (webx_preview_area_realize),
                    preview);
  g_signal_connect (preview->area, "unrealize",
                    G_CALLBACK (webx_preview_area_unrealize),
                    preview);
  g_signal_connect (preview->area, "size-allocate",
                    G_CALLBACK (webx_preview_area_size_allocate),
                    preview);
  g_signal_connect (preview->area, "button-press-event",
                    G_CALLBACK (webx_preview_area_button_press),
                    preview);
  g_signal_connect (preview->area, "button-release-event",
                    G_CALLBACK (webx_preview_area_button_release),
                    preview);
  g_signal_connect (preview->area, "motion-notify-event",
                    G_CALLBACK (webx_preview_area_motion_notify),
                    preview);
  g_signal_connect (preview->area, "scroll-event",
                    G_CALLBACK (webx_preview_area_scroll),
                    preview);

  gtk_widget_show (preview->area);
  
  adj = gtk_adjustment_new (0, 0, width -1, 1.0,
                            width, width);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (webx_preview_hscroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (WEBX_PREVIEW (preview)->table),
                    preview->hscr, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (preview->hscr);

  adj = gtk_adjustment_new (0, 0, height - 1, 1.0,
                            height, height);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (webx_preview_vscroll),
                    preview);
 
  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (WEBX_PREVIEW (preview)->table),
                    preview->vscr, 1, 2, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (preview->vscr);

  preview->nav_icon = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (WEBX_PREVIEW (preview)->table),
                    preview->nav_icon, 1, 2, 1, 2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (preview->nav_icon);

  image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (preview->nav_icon), image);
  gtk_widget_show (image);

  g_signal_connect (preview->nav_icon, "button-press-event",
                    G_CALLBACK (webx_preview_nav_button_press),
                    preview);

  button_bar = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (preview), button_bar, FALSE, FALSE, 0);
  gtk_widget_show (button_bar);

  preview->show_preview = gtk_check_button_new_with_label (_("Show preview"));
  gtk_box_pack_start (GTK_BOX (button_bar), preview->show_preview,
                      FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->show_preview),
                                TRUE);
  g_signal_connect_swapped (preview->show_preview, "toggled",
                            G_CALLBACK (webx_preview_show_toggled),
                            preview);
  gtk_widget_show (preview->show_preview);

  preview->progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (button_bar), preview->progress_bar,
                      TRUE, TRUE, 4);
  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (preview->progress_bar),
                                  PANGO_ELLIPSIZE_END);
  gtk_widget_show (preview->progress_bar);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_box_pack_end (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_show (image);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (webx_preview_zoom_out),
                            preview);
  gtk_widget_show (button);
  preview->zoom_out = button;
  
  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
  gtk_box_pack_end (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (webx_preview_zoom_in),
                            preview);
  gtk_widget_show (button);
  preview->zoom_in = button;

  for (i = 0; i < G_N_ELEMENTS (webx_zoomlevels); i++)
    {
      zoom_labels[i] =
          g_strdup_printf ("%d%%",
                           (gint) (webx_zoomlevels[i] * 100.0 + 0.5));
    }
  zoom_labels[i++] = _("Fit Width");
  zoom_labels[i++] = _("Fit Visible");

  preview->zoom_combo =
      gimp_int_combo_box_new_array (i, (const char **) zoom_labels);
  gtk_box_pack_end (GTK_BOX (button_bar), preview->zoom_combo,
                    FALSE, FALSE, 0);
  gtk_widget_show (preview->zoom_combo);

  for (i = 0; i < G_N_ELEMENTS (webx_zoomlevels); i++)
    {
      g_free (zoom_labels[i]);
    }

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (preview->zoom_combo),
                              WEBX_DEFAULT_ZOOMLEVEL,
                              G_CALLBACK (webx_preview_zoom_combo_changed),
                              preview);

  return GTK_WIDGET (preview);
}

static void
webx_preview_destroy (GtkObject *object)
{
  WebxPreview *preview;

  preview = WEBX_PREVIEW (object);
  if (preview->target)
    {
      g_object_unref (preview->target);
      preview->target = NULL;
    }
  if (preview->background)
    {
      g_object_unref (preview->background);
      preview->background = NULL;
    }
  if (preview->original)
    {
      g_object_unref (preview->original);
      preview->original = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (GTK_OBJECT (object));
}

static void
webx_preview_update_file_size (WebxPreview     *preview,
                               gint             file_size)
{

  if (file_size)
    {
      gchar text[512];
      g_snprintf (text, sizeof (text),
                  _("File size: %02.01f kB"),
                  (gdouble) file_size / 1024.0);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (preview->progress_bar),
                                     0.0);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (preview->progress_bar),
                                 text);
    }
  else
    {
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (preview->progress_bar),
                                     1.0);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (preview->progress_bar),
                                 _("File size: unknown"));
    }
}

void
webx_preview_begin_update (WebxPreview         *preview)
{
  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  webx_preview_update_file_size (preview, 0);
}

void
webx_preview_update_target (WebxPreview        *preview,
                            GdkPixbuf          *target,
                            gint                file_size)
{
  GdkRectangle clipbox;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  if (target)
    g_object_ref_sink (G_OBJECT (target));
  if (preview->target)
    g_object_unref (G_OBJECT (preview->target));
  preview->target = target;

  webx_preview_get_target_rect (preview, &clipbox);
  clipbox.x -= WEBX_PREVIEW_PADDING;
  clipbox.y -= WEBX_PREVIEW_PADDING;
  clipbox.width += WEBX_PREVIEW_PADDING * 2;
  clipbox.height += WEBX_PREVIEW_PADDING * 2;

  if (GTK_WIDGET_REALIZED (preview->area))
    gdk_window_invalidate_rect (GDK_WINDOW (preview->area->window),
                                &clipbox, FALSE);

  webx_preview_update_file_size (preview, file_size);
}

void
webx_preview_update (WebxPreview       *preview,
                     GdkPixbuf         *original,
                     GdkPixbuf         *target,
                     GdkRectangle      *target_rect,
                     gint               file_size)
{
  g_return_if_fail (WEBX_IS_PREVIEW (preview));
  g_return_if_fail (target_rect != NULL);
  
  if (target)
    g_object_ref_sink (G_OBJECT (target));
  if (preview->target)
    g_object_unref (G_OBJECT (preview->target));
  preview->target = target;

  if (original)
    g_object_ref_sink (G_OBJECT (original));
  if (preview->original)
    g_object_unref (G_OBJECT (preview->original));
  preview->original = original;
  preview->width = gdk_pixbuf_get_width (original);
  preview->height = gdk_pixbuf_get_height (original);

  if (preview->background)
    g_object_unref (preview->background);
  if (preview->original)
    preview->background = webx_preview_create_background (preview->original);
  else
    preview->background = NULL;

  preview->target_rect = *target_rect;

  gtk_widget_queue_draw (preview->area);

  webx_preview_update_file_size (preview, file_size);
}

void
webx_preview_resize (WebxPreview  *preview,
                     gint          width,
                     gint          height)
{
  GdkRectangle old_clipbox, clipbox;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  if (preview->target)
    {
      g_object_unref (preview->target);
      preview->target = NULL;
    }
  if (preview->background)
    {
      g_object_unref (preview->background);
      preview->background = NULL;
    }

  webx_preview_get_background_rect (preview, &old_clipbox);
  old_clipbox.x -= WEBX_PREVIEW_PADDING;
  old_clipbox.y -= WEBX_PREVIEW_PADDING;
  old_clipbox.width += WEBX_PREVIEW_PADDING * 2;
  old_clipbox.height += WEBX_PREVIEW_PADDING * 2;

  preview->width = width;
  preview->height = height;

  preview->target_rect.x = 0;
  preview->target_rect.y = 0;
  preview->target_rect.width = width;
  preview->target_rect.height = height;

  webx_preview_get_background_rect (preview, &clipbox);
  clipbox.x -= WEBX_PREVIEW_PADDING;
  clipbox.y -= WEBX_PREVIEW_PADDING;
  clipbox.width += WEBX_PREVIEW_PADDING * 2;
  clipbox.height += WEBX_PREVIEW_PADDING * 2;

  gtk_widget_queue_resize_no_redraw (preview->area);
  gdk_rectangle_union (&old_clipbox, &clipbox, &clipbox);
  if (GTK_WIDGET_REALIZED (preview->area))
      gdk_window_invalidate_rect (preview->area->window, &clipbox, FALSE);
}

void
webx_preview_crop (WebxPreview  *preview,
                   GdkRectangle *crop)
{
  GdkRectangle clipbox, old_clipbox;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  if (preview->target)
    {
      g_object_unref (preview->target);
      preview->target = NULL;
    }

  webx_preview_get_target_rect (preview, &old_clipbox);
  old_clipbox.x -= WEBX_PREVIEW_PADDING;
  old_clipbox.y -= WEBX_PREVIEW_PADDING;
  old_clipbox.width += WEBX_PREVIEW_PADDING * 2;
  old_clipbox.height += WEBX_PREVIEW_PADDING * 2;

  preview->target_rect = *crop;

  webx_preview_get_target_rect (preview, &clipbox);
  clipbox.x -= WEBX_PREVIEW_PADDING;
  clipbox.y -= WEBX_PREVIEW_PADDING;
  clipbox.width += WEBX_PREVIEW_PADDING * 2;
  clipbox.height += WEBX_PREVIEW_PADDING * 2;

  gdk_rectangle_union (&old_clipbox, &clipbox, &clipbox);
  if (GTK_WIDGET_REALIZED (preview->area))
    gdk_window_invalidate_rect (preview->area->window, &clipbox, FALSE);
}

static GdkPixbuf*
webx_preview_create_background (GdkPixbuf *src)
{
  GdkPixbuf    *dst;
  guchar       *pixels;
  guchar       *row;
  gint          rowstride;
  gint          width;
  gint          height;
  gint          i;
  gint          j;
  gboolean      has_alpha;

  dst = gdk_pixbuf_copy (src);

  pixels    = gdk_pixbuf_get_pixels (dst);
  width     = gdk_pixbuf_get_width (dst);
  height    = gdk_pixbuf_get_height (dst);
  rowstride = gdk_pixbuf_get_rowstride (dst);
  has_alpha = gdk_pixbuf_get_has_alpha (dst);
  
  for (i = 0; i < height; i++)
    {
      row = pixels;
      for (j = 0; j < width; j++)
        {
          *row = *row / 4 + 127;
          row++;
          *row = *row / 4 + 127;
          row++;
          *row = *row / 4 + 127;
          row++;
          if (has_alpha)
            row++;
        }
      pixels += rowstride;
    }

  return dst;
}

/* Centers image coordinates in view  */
static void
webx_preview_get_background_rect (WebxPreview  *preview,
                                  GdkRectangle *rect)
{
  gint        width;
  gint        height;
  gint        view_width;
  gint        view_height;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));
  g_return_if_fail (rect != NULL);

  width = preview->width * preview->zoom;
  height = preview->height * preview->zoom;
    
  view_width = preview->area->allocation.width;
  view_height = preview->area->allocation.height;

  if (width + WEBX_PREVIEW_PADDING * 2 < view_width)
    rect->x = (view_width - width) / 2;
  else
    rect->x = WEBX_PREVIEW_PADDING;
  if (height + WEBX_PREVIEW_PADDING*2 < view_height)
    rect->y = (view_height - height) / 2;
  else
    rect->y = WEBX_PREVIEW_PADDING;
  rect->width = width;
  rect->height = height;

  rect->x -= preview->xoffs;
  rect->y -= preview->yoffs;
}

/* Centers image coordinates in view
 * but takes cropping into account */
static void
webx_preview_get_target_rect (WebxPreview  *preview,
                              GdkRectangle *rect)
{
  GdkRectangle    bg_rect;
  GdkRectangle    target_rect;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));
  g_return_if_fail (rect != NULL);

  webx_preview_get_background_rect (preview, &bg_rect);
  target_rect = preview->target_rect;
  target_rect.x *= preview->zoom;
  target_rect.y *= preview->zoom;
  target_rect.width *= preview->zoom;
  target_rect.height *= preview->zoom;
  target_rect.x += bg_rect.x;
  target_rect.y += bg_rect.y;
  *rect = target_rect;
}

/* Determine what do we have under mouse pointer. */
static gint
webx_preview_get_drag_type (WebxPreview *preview,
                            gint         x,
                            gint         y)
{
  GdkRectangle    target_rect;
  gint            drag_type = 0;

  g_return_val_if_fail (WEBX_IS_PREVIEW (preview), 0);

  webx_preview_get_target_rect (preview, &target_rect);
    
  if (x < target_rect.x - WEBX_PREVIEW_PADDING / 2
      || y < target_rect.y - WEBX_PREVIEW_PADDING / 2
      || x > target_rect.x + target_rect.width + WEBX_PREVIEW_PADDING / 2 
      || y > target_rect.y + target_rect.height + WEBX_PREVIEW_PADDING / 2)
    return WEBX_DRAG_HAND;
        
  if (x < target_rect.x)
    drag_type |= WEBX_DRAG_LEFT;
  else if (x > target_rect.x + target_rect.width)
    drag_type |= WEBX_DRAG_RIGHT;
  if (y < target_rect.y)
    drag_type |= WEBX_DRAG_TOP;
  else if (y > target_rect.y + target_rect.height)
    drag_type |= WEBX_DRAG_BOTTOM;
        
  if (! drag_type)
    return WEBX_DRAG_HAND;

  return drag_type;
}

static gboolean
webx_preview_area_expose (GtkWidget       *widget,
                          GdkEventExpose  *event,
                          WebxPreview     *preview)
{
  GdkPixbuf      *pixbuf;
  GdkRectangle    bg_rect;
  GdkRectangle    target_rect;
  GdkRectangle    clipbox;
  GdkGC          *gc;
  gboolean        show_preview;

  g_return_val_if_fail (WEBX_IS_PREVIEW (preview), TRUE);

  gc = preview->area_gc;

  webx_preview_get_background_rect (preview, &bg_rect);
  webx_preview_get_target_rect (preview, &target_rect);

  /* If we cannot draw anything else, then draw original image */
  if (!preview->background
      && !preview->target
      && preview->original
      && gdk_rectangle_intersect (&event->area, &bg_rect, &clipbox))
    {
      gdouble zoom_x = preview->zoom * (gdouble) preview->width /
          (gdouble) gdk_pixbuf_get_width (preview->original);
      gdouble zoom_y = preview->zoom * (gdouble) preview->height /
          (gdouble) gdk_pixbuf_get_height (preview->original);

      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               clipbox.width, clipbox.height);
      gdk_pixbuf_composite_color (preview->original, pixbuf,
                                  0, 0, clipbox.width, clipbox.height,
                                  bg_rect.x - clipbox.x, bg_rect.y - clipbox.y,
                                  zoom_x, zoom_y,
                                  GDK_INTERP_TILES, 255,
                                  bg_rect.x, bg_rect.y,
                                  16, 0xaaaaaa, 0x555555);

      gdk_draw_pixbuf (widget->window, gc,
                       pixbuf,
                       0, 0, clipbox.x, clipbox.y,
                       clipbox.width, clipbox.height,
                       GDK_RGB_DITHER_NORMAL,
                       clipbox.x, clipbox.y);

      g_object_unref (pixbuf);
    }

  /* Draw background if some cropping has been done or 
   * target is recreated and cannot be drawn. */
  if ((bg_rect.width != target_rect.width
       || bg_rect.height != target_rect.height
       || !preview->target)
      && preview->background
      && gdk_rectangle_intersect (&event->area, &bg_rect, &clipbox) )
    {
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               clipbox.width, clipbox.height);
      gdk_pixbuf_composite_color (preview->background, pixbuf,
                                  0, 0, clipbox.width, clipbox.height,
                                  bg_rect.x - clipbox.x, bg_rect.y - clipbox.y,
                                  preview->zoom,
                                  preview->zoom,
                                  GDK_INTERP_TILES, 255,
                                  clipbox.x, clipbox.y,
                                  16, 0xaaaaaa, 0x555555);

      gdk_draw_pixbuf (widget->window, gc,
                       pixbuf,
                       0, 0, clipbox.x, clipbox.y,
                       clipbox.width, clipbox.height,
                       GDK_RGB_DITHER_NORMAL,
                       clipbox.x, clipbox.y);

      g_object_unref (pixbuf);
    }

  show_preview = 
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (preview->show_preview)); 

  /* Draw target pixbuf */
  if (preview->target
      && show_preview
      && gdk_rectangle_intersect (&event->area, &target_rect, &clipbox))
    {
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               clipbox.width, clipbox.height);
      gdk_pixbuf_composite_color (preview->target, pixbuf,
                                  0, 0, clipbox.width, clipbox.height,
                                  target_rect.x - clipbox.x,
                                  target_rect.y - clipbox.y,
                                  preview->zoom, preview->zoom,
                                  GDK_INTERP_TILES, 255,
                                  clipbox.x - target_rect.x,
                                  clipbox.y - target_rect.y,
                                  16, 0xaaaaaa, 0x555555);

      gdk_draw_pixbuf (widget->window, gc,
                       pixbuf,
                       0, 0, clipbox.x, clipbox.y,
                       clipbox.width, clipbox.height,
                       GDK_RGB_DITHER_NORMAL,
                       clipbox.x, clipbox.y);

      g_object_unref (pixbuf);

    }
  else if (preview->original
           && preview->background
           && gdk_rectangle_intersect (&event->area, &target_rect, &clipbox))
    {
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                               clipbox.width, clipbox.height);
      gdk_pixbuf_composite_color (preview->original, pixbuf,
                                  0, 0, clipbox.width, clipbox.height,
                                  bg_rect.x - clipbox.x,
                                  bg_rect.y - clipbox.y,
                                  preview->zoom, preview->zoom,
                                  GDK_INTERP_TILES, 255,
                                  clipbox.x, clipbox.y,
                                  16, 0xaaaaaa, 0x555555);

      gdk_draw_pixbuf (widget->window, gc,
                       pixbuf,
                       0, 0, clipbox.x, clipbox.y,
                       clipbox.width, clipbox.height,
                       GDK_RGB_DITHER_NORMAL,
                       clipbox.x, clipbox.y);

      g_object_unref (pixbuf);

    }

  if (preview->target
      && show_preview)
    {
      webx_preview_draw_outline (preview, &target_rect, FALSE, FALSE);
    }
  else if (preview->original
      && preview->background)
    {
      webx_preview_draw_outline (preview, &target_rect, FALSE, TRUE);
    }

  return TRUE;
}

/* Draw an outline outside target image (which can be cropped)
 * and crop handles. */
static void
webx_preview_draw_outline (WebxPreview   *preview,
                           GdkRectangle  *rect,
                           gboolean       filled,
                           gboolean       active)
{
  GtkWidget    *widget;
  GdkGC        *gc;

  widget = preview->area;
  gc = preview->area_gc;

  /*if (active)
    {
      GdkColor color;

      gdk_gc_set_function (gc, GDK_COPY);
      gdk_gc_set_line_attributes (gc, 1, GDK_LINE_DOUBLE_DASH,
                                  GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

      color.red = 65535;
      color.green = 0;
      color.blue = 0;
      gdk_gc_set_rgb_fg_color (gc, &color);

      color.red = 65535;
      color.green = 65535;
      color.blue = 0;
      gdk_gc_set_rgb_bg_color (gc, &color);
    }*/

  if (active)
    {
      gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID,
                                  GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    }
  else
    {
      gdk_gc_set_line_attributes (gc, 1, GDK_LINE_ON_OFF_DASH,
                                  GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    }

  gdk_gc_set_function (gc, GDK_INVERT);

  gdk_draw_line (widget->window, gc,
                 rect->x-1, rect->y - WEBX_PREVIEW_PADDING,
                 rect->x-1, rect->y + rect->height + WEBX_PREVIEW_PADDING);
  gdk_draw_line (widget->window, gc,
                 rect->x+rect->width, rect->y - WEBX_PREVIEW_PADDING,
                 rect->x+rect->width, rect->y + rect->height + WEBX_PREVIEW_PADDING);

  gdk_draw_line (widget->window, gc,
                 rect->x - WEBX_PREVIEW_PADDING, rect->y - 1,
                 rect->x + rect->width + WEBX_PREVIEW_PADDING, rect->y - 1);
  gdk_draw_line (widget->window, gc,
                 rect->x - WEBX_PREVIEW_PADDING, rect->y + rect->height,
                 rect->x + rect->width + WEBX_PREVIEW_PADDING, rect->y + rect->height);


  /*
  gdk_draw_rectangle (widget->window, gc,
                      filled,
                      rect->x-1, rect->y-1,
                      rect->width+1, rect->height+1);

  gdk_gc_set_line_attributes (gc, 1,
                              GDK_LINE_SOLID, GDK_CAP_NOT_LAST,
                              GDK_JOIN_MITER);
  gdk_draw_rectangle (widget->window,
                      gc,
                      FALSE,
                      rect->x - WEBX_HANDLE_SIZE - 2,
                      rect->y - WEBX_HANDLE_SIZE - 2,
                      WEBX_HANDLE_SIZE, WEBX_HANDLE_SIZE);
  gdk_draw_rectangle (widget->window,
                      gc,
                      FALSE,
                      rect->x+rect->width + 1,
                      rect->y - WEBX_HANDLE_SIZE - 2,
                      WEBX_HANDLE_SIZE, WEBX_HANDLE_SIZE);
  gdk_draw_rectangle (widget->window,
                      gc,
                      FALSE,
                      rect->x + rect->width + 1,
                      rect->y + rect->height + 1,
                      WEBX_HANDLE_SIZE, WEBX_HANDLE_SIZE);
  gdk_draw_rectangle (widget->window,
                      gc,
                      FALSE,
                      rect->x - WEBX_HANDLE_SIZE - 2,
                      rect->y + rect->height + 1,
                      WEBX_HANDLE_SIZE, WEBX_HANDLE_SIZE);
                      */

  gdk_gc_set_function (gc, GDK_COPY);
}

static void
webx_preview_vscroll_update (WebxPreview *preview)
{
  GtkAdjustment *adj;
  gint           height;

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

  height = preview->height * preview->zoom + WEBX_PREVIEW_PADDING * 2;

  adj->lower            = 0;
  adj->upper            = height;
  adj->page_size        = MIN (preview->area->allocation.height, height);
  adj->step_increment   = MAX (adj->page_size / 10.0, 1.0);
  adj->page_increment   = MAX (adj->page_size / 2.0, adj->step_increment);
  adj->value            = CLAMP (adj->value,
                                 adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);
}

static void
webx_preview_hscroll_update (WebxPreview *preview)
{
  GtkAdjustment *adj;
  gint           width;

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

  width = preview->width * preview->zoom + WEBX_PREVIEW_PADDING * 2;

  adj->lower            = 0;
  adj->upper            = width;
  adj->page_size        = MIN (preview->area->allocation.width, width);
  adj->step_increment   = MAX (adj->page_size / 10.0, 1.0);
  adj->page_increment   = MAX (adj->page_size / 2.0, adj->step_increment);
  adj->value            = CLAMP (adj->value,
                                 adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);
}

static void
webx_preview_area_realize (GtkWidget       *widget,
                           WebxPreview     *preview)
{
  if (! preview->area_gc)
    {
      preview->area_gc = gdk_gc_new (widget->window);
    }
}

static void
webx_preview_area_unrealize (GtkWidget     *widget,
                             WebxPreview   *preview)
{
  if (preview->area_gc)
    {
      g_object_unref (preview->area_gc);
      preview->area_gc = NULL;
    }
}



static void
webx_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation,
                                 WebxPreview   *preview)
{
  GtkAdjustment        *adj;
  gdouble               old_hmid, old_vmid;

  if (preview->zoom_mode != WEBX_ZOOM_MODE_NORMAL)
    {
      gdouble   zoom_x, zoom_y;

      zoom_x = (gdouble) (preview->area->allocation.width
                          - 2 * WEBX_PREVIEW_PADDING) / preview->width;
      zoom_y = (gdouble) (preview->area->allocation.height
                          - 2 * WEBX_PREVIEW_PADDING) / preview->height;

      switch (preview->zoom_mode)
        {
          case WEBX_ZOOM_MODE_FIT_WIDTH:
              preview->zoom = zoom_x;
              break;
          case WEBX_ZOOM_MODE_FIT_VISIBLE:
              preview->zoom = MIN (zoom_x, zoom_y);
              break;
        }
      
      preview->zoom = CLAMP (preview->zoom,
                             WEBX_MIN_ZOOMLEVEL,
                             WEBX_MAX_ZOOMLEVEL);
    }



  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  old_hmid = (adj->value + adj->page_size/2) / adj->upper;
  g_signal_handlers_block_by_func (adj, webx_preview_hscroll, preview);
  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  old_vmid = (adj->value + adj->page_size/2) / adj->upper;
  g_signal_handlers_block_by_func (adj, webx_preview_vscroll, preview);

  webx_preview_hscroll_update (preview);
  webx_preview_vscroll_update (preview);
      
  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  adj->value = CLAMP (old_hmid * adj->upper - adj->page_size / 2,
                      0, adj->upper - adj->page_size);
  preview->xoffs = adj->value;
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (adj));
  g_signal_handlers_unblock_by_func (adj, webx_preview_hscroll, preview);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  adj->value = CLAMP (old_vmid * adj->upper - adj->page_size / 2,
                      0, adj->upper - adj->page_size);
  preview->yoffs = adj->value;
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (adj));
  g_signal_handlers_unblock_by_func (adj, webx_preview_vscroll, preview);
}

static void
webx_preview_hscroll (GtkAdjustment *hadj,
                      WebxPreview   *preview)
{
  gint  dx;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  dx = preview->xoffs - (gint)hadj->value;
  preview->xoffs = hadj->value;
  if (GTK_WIDGET_REALIZED (preview->area))
    {
      gdk_window_scroll (preview->area->window, dx, 0);
      gdk_window_process_updates (preview->area->window, FALSE);
    }
}

static void
webx_preview_vscroll (GtkAdjustment *vadj,
                      WebxPreview   *preview)
{
  gint  dy;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  dy = preview->yoffs - (gint)vadj->value;
  preview->yoffs = vadj->value;
  if (GTK_WIDGET_REALIZED (preview->area))
    {
      gdk_window_scroll (preview->area->window, 0, dy);
      gdk_window_process_updates (preview->area->window, FALSE);
    }
}


static void
webx_preview_hand_start (WebxPreview *preview)
{
  GtkAdjustment   *hadj;
  GtkAdjustment   *vadj;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  preview->scroll_offs_x = hadj->value;
  preview->scroll_offs_y = vadj->value;
  preview->scroll_max_x = hadj->upper - hadj->page_size;
  preview->scroll_max_y = vadj->upper - vadj->page_size;
}

/* Scroll view. */
static void
webx_preview_hand_drag (WebxPreview  *preview,
                        gdouble       delta_x,
                        gdouble       delta_y)
{
  GtkAdjustment   *hadj;
  GtkAdjustment   *vadj;
  gdouble          new_x, new_y;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  new_x = CLAMP (preview->scroll_offs_x - delta_x, 0, preview->scroll_max_x);
  new_y = CLAMP (preview->scroll_offs_y - delta_y, 0, preview->scroll_max_y);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (hadj), new_x);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (vadj), new_y);
}

static void
webx_preview_crop_start (WebxPreview *preview)
{
  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  webx_preview_get_target_rect (preview, &preview->drag_crop);
}

/* Change crop settings by dragging crop handles. */
static void
webx_preview_crop_drag (WebxPreview *preview,
                        gint         delta_x,
                        gint         delta_y)
{
  GdkRectangle  crop, back_rect;
  gint          x1, y1, x2, y2;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  x1 = preview->drag_crop.x;
  y1 = preview->drag_crop.y;
  x2 = x1 + preview->drag_crop.width;
  y2 = y1 + preview->drag_crop.height;

  webx_preview_get_background_rect (preview, &back_rect);
   
  if (preview->drag_mode & WEBX_DRAG_LEFT)
    {
      x1 += delta_x;
      x1 = CLAMP (x1, back_rect.x, back_rect.x + back_rect.width);
    }
  else if (preview->drag_mode & WEBX_DRAG_RIGHT)
    {
      x2 += delta_x;
      x2 = CLAMP (x2, back_rect.x, back_rect.x + back_rect.width);
    }
  if (preview->drag_mode & WEBX_DRAG_TOP)
    {
      y1 += delta_y;
      y1 = CLAMP (y1, back_rect.y, back_rect.y + back_rect.height);
    }
  else if (preview->drag_mode & WEBX_DRAG_BOTTOM)
    {
      y2 += delta_y;
      y2 = CLAMP (y2, back_rect.y, back_rect.y + back_rect.height);
    }

  if (preview->drag_mode & WEBX_DRAG_LEFT)
    {
      if (x1 >= x2)
        x1 = x2 - 1;
    }
  else if (preview->drag_mode & WEBX_DRAG_RIGHT)
    {
      if (x2 <= x1)
        x2 = x1 + 1;
    }
  if (preview->drag_mode & WEBX_DRAG_TOP)
    {
      if (y1 >= y2)
        y1 = y2 - 1;
    }
  else if (preview->drag_mode & WEBX_DRAG_BOTTOM)
    {
      if (y2 <= y1)
        y2 = y1 + 1;
    }

  crop.x = x1;
  crop.y = y1;
  crop.width = x2 - x1;
  crop.height = y2 - y1;

  crop.x -= back_rect.x;
  crop.y -= back_rect.y;

  crop.x = (gdouble) (crop.x + 0.5) / preview->zoom;
  crop.y = (gdouble) (crop.y + 0.5) / preview->zoom;
  crop.width = ceil ((gdouble) (crop.width) / preview->zoom);
  crop.height = ceil ((gdouble) (crop.height) / preview->zoom);

  g_signal_emit (preview, webx_preview_signals[CROP_CHANGED], 0,
                 &crop);
}

static void
webx_preview_update_cursor (WebxPreview  *preview,
                            gint          x,
                            gint          y)
{
  gint            drag_type;
  GdkCursor      *cursor;
  GdkCursorType   cursor_type;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  drag_type = webx_preview_get_drag_type (preview, x, y);
  switch (drag_type)
    {
    case WEBX_DRAG_LEFT:
      cursor_type = GDK_LEFT_SIDE;
      break;
    case WEBX_DRAG_RIGHT:
      cursor_type = GDK_RIGHT_SIDE;
      break;
    case WEBX_DRAG_TOP:
      cursor_type = GDK_TOP_SIDE;
      break;
    case WEBX_DRAG_BOTTOM:
      cursor_type = GDK_BOTTOM_SIDE;
      break;
    case (WEBX_DRAG_LEFT|WEBX_DRAG_TOP):
      cursor_type = GDK_TOP_LEFT_CORNER;
      break;
    case (WEBX_DRAG_RIGHT|WEBX_DRAG_TOP):
      cursor_type = GDK_TOP_RIGHT_CORNER;
      break;
    case (WEBX_DRAG_LEFT|WEBX_DRAG_BOTTOM):
      cursor_type = GDK_BOTTOM_LEFT_CORNER;
      break;
    case (WEBX_DRAG_RIGHT|WEBX_DRAG_BOTTOM):
      cursor_type = GDK_BOTTOM_RIGHT_CORNER;
      break;
    default:
      cursor_type = GDK_FLEUR;
      break;
    }

  if (preview->cursor_type != cursor_type)
    {
      GdkDisplay *display;
      preview->cursor_type = cursor_type;
      if (cursor_type == GDK_FLEUR)
        {
          cursor = cursor_get (preview->area, CURSOR_HAND_OPEN);
        }
      else
        {
          display = gtk_widget_get_display (preview->area);
          cursor = gdk_cursor_new_for_display (display, cursor_type);
        }
      gdk_window_set_cursor (preview->area->window, cursor);
      gdk_cursor_unref (cursor);
    }
}

static void
webx_preview_area_button_press (GtkWidget       *widget,
                                GdkEventButton  *event,
                                WebxPreview     *preview)
{
  GdkCursor    *cursor = NULL;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  preview->drag_mode = webx_preview_get_drag_type (preview,
                                                   event->x, event->y);
  preview->drag_x = event->x_root;
  preview->drag_y = event->y_root;

  if (preview->drag_mode & WEBX_DRAG_HAND)
    {
      webx_preview_hand_start (preview);
      cursor = cursor_get (preview->area, CURSOR_HAND_CLOSED);
    }
  else
    {
      webx_preview_crop_start (preview);
    }

  gtk_grab_add (preview->area);

  gdk_pointer_grab (preview->area->window, TRUE,
                    GDK_BUTTON_RELEASE_MASK
                    | GDK_BUTTON_MOTION_MASK
                    | GDK_POINTER_MOTION_HINT_MASK,
                    NULL, cursor,
                    event->time);

  if (cursor)
    gdk_cursor_unref (cursor);
}

static void
webx_preview_area_button_release (GtkWidget       *widget,
                                  GdkEventButton  *event,
                                  WebxPreview     *preview)
{
  gtk_grab_remove (widget);
  gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                              event->time);

  preview->drag_mode = 0;
}

static void
webx_preview_area_motion_notify (GtkWidget       *widget,
                                 GdkEventMotion  *event,
                                 WebxPreview     *preview)
{
  gint     x, y;
  gint     x_root, y_root;
  gint     delta_x, delta_y;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));

  if (event->is_hint)
    {
      gdk_window_get_pointer (event->window, &x, &y, NULL);
      gdk_window_get_origin (event->window, &x_root, &y_root);
      x_root += x;
      y_root += y;
    }
  else
    {
      x = event->x;
      y = event->y;
      x_root = event->x_root;
      y_root = event->y_root;
    }

  delta_x = x_root - preview->drag_x;
  delta_y = y_root - preview->drag_y;

  if (! preview->drag_mode)
    {
      webx_preview_update_cursor (preview, x, y);
    }
  else if (preview->drag_mode & WEBX_DRAG_HAND)
    {
      webx_preview_hand_drag (preview, delta_x, delta_y);
    }
  else
    {
      webx_preview_crop_drag (preview, delta_x, delta_y);
    }
}

static gboolean 
webx_preview_area_scroll (GtkWidget            *widget,
                          GdkEventScroll       *event,
                          WebxPreview          *preview)
{
  if (event->state & GDK_CONTROL_MASK)
    {   /* zoom in/out */
      switch (event->direction)
        {
          case GDK_SCROLL_UP:
          case GDK_SCROLL_LEFT:
              webx_preview_zoom_in (preview);
              return TRUE;
          case GDK_SCROLL_DOWN:
          case GDK_SCROLL_RIGHT:
              webx_preview_zoom_out (preview);
              return TRUE;
         }
    }
  else
    { /* scroll */
      GtkAdjustment     *adj;

      switch (event->direction)
        {
          case GDK_SCROLL_UP:
              adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
              adj->value -= adj->step_increment;
              adj->value = CLAMP (adj->value, 0, adj->upper- adj->page_size);
              gtk_adjustment_value_changed (adj);
              return TRUE;
          case GDK_SCROLL_LEFT:
              adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
              adj->value -= adj->step_increment;
              adj->value = CLAMP (adj->value, 0, adj->upper- adj->page_size);
              gtk_adjustment_value_changed (adj);
              return TRUE;
          case GDK_SCROLL_DOWN:
              adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
              adj->value += adj->step_increment;
              adj->value = CLAMP (adj->value, 0, adj->upper- adj->page_size);
              gtk_adjustment_value_changed (adj);
              return TRUE;
          case GDK_SCROLL_RIGHT:
              adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
              adj->value += adj->step_increment;
              adj->value = CLAMP (adj->value, 0, adj->upper- adj->page_size);
              gtk_adjustment_value_changed (adj);
              return TRUE;
        }
    }
  return FALSE;
}

static gboolean 
webx_preview_nav_button_press (GtkWidget        *widget,
                               GdkEventButton   *event,
                               WebxPreview      *preview)
{
  GtkAdjustment *adj;
  GtkWidget     *outer;
  GtkWidget     *inner;
  GtkWidget     *area;
  GdkCursor     *cursor;
  gint           x, y;
  gdouble        h, v;
  gint           width, height;

  if (preview->nav_popup)
    return TRUE;

  if (event->type != GDK_BUTTON_PRESS
      || event->button != 1)
    return TRUE;

  preview->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_set_screen (GTK_WINDOW (preview->nav_popup),
                         gtk_widget_get_screen (widget));

  outer = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (outer), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (preview->nav_popup), outer);
  gtk_widget_show (outer);

  inner = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (inner), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (outer), inner);
  gtk_widget_show (inner);

  if (preview->width > preview->height)
    {
      width = WEBX_THUMBNAIL_SIZE;
      height = WEBX_THUMBNAIL_SIZE * (gdouble)preview->height / preview->width;
    }
  else
    {
      width = WEBX_THUMBNAIL_SIZE * (gdouble)preview->width / preview->height;
      height = WEBX_THUMBNAIL_SIZE;
    }

  preview->nav_thumbnail = gdk_pixbuf_scale_simple (preview->original,
                                                    width, height,
                                                    GDK_INTERP_BILINEAR);
  area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (inner), area);
  gtk_widget_set_size_request (area, width, height);

  g_signal_connect (area, "realize",
                    G_CALLBACK (webx_preview_nav_popup_realize),
                    preview);
  g_signal_connect (area, "unrealize",
                    G_CALLBACK (webx_preview_nav_popup_unrealize),
                    preview);
  g_signal_connect (area, "event",
                    G_CALLBACK (webx_preview_nav_popup_event),
                    preview);
  g_signal_connect_after (area, "expose-event",
                          G_CALLBACK (webx_preview_nav_popup_expose),
                          preview);

  gtk_widget_realize (area);
  gtk_widget_show (area);

  gdk_window_get_origin (widget->window, &x, &y);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
  h = ((gdouble) preview->xoffs / preview->zoom) / (gdouble) preview->width
      + ((gdouble) adj->page_size / (gdouble) adj->upper) / 2;
  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));
  v = ((gdouble) preview->yoffs / preview->zoom) / (gdouble) preview->height
      + ((gdouble) adj->page_size / (gdouble) adj->upper) / 2;

  x += event->x - h * (gdouble) width;
  y += event->y - v * (gdouble) height;

  gtk_window_move (GTK_WINDOW (preview->nav_popup),
                   x - 2 * widget->style->xthickness,
                   y - 2 * widget->style->ythickness);

  gtk_widget_show (preview->nav_popup);

  gtk_grab_add (area);

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                       GDK_FLEUR);
  
  gdk_pointer_grab (area->window, TRUE,
                    GDK_BUTTON_RELEASE_MASK
                    | GDK_BUTTON_MOTION_MASK
                    | GDK_POINTER_MOTION_HINT_MASK,
                    area->window, cursor,
                    event->time);

  gdk_cursor_unref (cursor);

  return TRUE;
}

static void
webx_preview_nav_popup_realize (GtkWidget       *widget,
                                WebxPreview     *preview)
{
  if (! preview->nav_gc)
    {
      preview->nav_gc = gdk_gc_new (widget->window);

      gdk_gc_set_function (preview->nav_gc, GDK_INVERT);
      gdk_gc_set_line_attributes (preview->nav_gc,
                                  WEBX_NAV_PEN_WIDTH,
                                  GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }
}

static void
webx_preview_nav_popup_unrealize (GtkWidget     *widget,
                                  WebxPreview   *preview)
{
  if (preview->nav_gc)
    {
      g_object_unref (preview->nav_gc);
      preview->nav_gc = NULL;
    }
}

static gboolean
webx_preview_nav_popup_event (GtkWidget         *widget,
                              GdkEvent          *event,
                              WebxPreview       *preview)
{
  switch (event->type)
    {
      case GDK_BUTTON_RELEASE:
        {
          GdkEventButton *button_event = (GdkEventButton *) event;

          if (button_event->button == 1)
            {
              gtk_grab_remove (widget);
              gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                          button_event->time);

              gtk_widget_destroy (preview->nav_popup);
              preview->nav_popup = NULL;

              g_object_unref (preview->nav_thumbnail);
              preview->nav_thumbnail = NULL;
            }
        }
      break;

      case GDK_MOTION_NOTIFY:
        {
          GtkAdjustment *hadj;
          GtkAdjustment *vadj;
          gint           cx, cy;
          gdouble        x, y;

          hadj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));
          vadj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

          gtk_widget_get_pointer (widget, &cx, &cy);

          x = cx * (hadj->upper - hadj->lower) / widget->allocation.width;
          y = cy * (vadj->upper - vadj->lower) / widget->allocation.height;

          x += hadj->lower - hadj->page_size / 2;
          y += vadj->lower - vadj->page_size / 2;

          x = CLAMP (x, hadj->lower, hadj->upper - hadj->page_size);
          gtk_adjustment_set_value (hadj, x);
          y = CLAMP (y, vadj->lower, vadj->upper - vadj->page_size);
          gtk_adjustment_set_value (vadj, y);

          gtk_widget_queue_draw (widget);
          gdk_window_process_updates (widget->window, FALSE);
        }
      break;

      default:
        break;
    }

  return FALSE;
}

static gboolean
webx_preview_nav_popup_expose (GtkWidget        *widget,
                               GdkEventExpose   *event,
                               WebxPreview      *preview)
{
  GtkAdjustment *adj;
  gdouble        x, y;
  gdouble        w, h;
  GdkGC         *gc;

  gc = preview->nav_gc;

  gdk_draw_pixbuf (widget->window, gc, preview->nav_thumbnail,
                   0, 0, 0, 0,
                   widget->allocation.width, widget->allocation.height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->hscr));

  x = adj->value / (adj->upper - adj->lower);
  w = adj->page_size / (adj->upper - adj->lower);

  adj = gtk_range_get_adjustment (GTK_RANGE (preview->vscr));

  y = adj->value / (adj->upper - adj->lower);
  h = adj->page_size / (adj->upper - adj->lower);

  if (w >= 1.0 && h >= 1.0)
    return FALSE;

  gdk_gc_set_clip_rectangle (preview->nav_gc, &event->area);

  x = x * (gdouble) widget->allocation.width + WEBX_NAV_PEN_WIDTH / 2;
  y = y * (gdouble) widget->allocation.height + WEBX_NAV_PEN_WIDTH / 2;
  w = MAX (1.0, ceil (w * widget->allocation.width) - WEBX_NAV_PEN_WIDTH);
  h = MAX (1.0, ceil (h * widget->allocation.height) - WEBX_NAV_PEN_WIDTH);

  gdk_draw_rectangle (widget->window, preview->nav_gc,
                      FALSE,
                      x, y, w, h);

  return FALSE;
}

static void
webx_preview_zoom (WebxPreview *preview,
                   gdouble      level)
{
  gint                  i;
  GimpIntComboBox      *intcombo;

  intcombo = GIMP_INT_COMBO_BOX (preview->zoom_combo);

  if (preview->stop_recursion > 0)
    preview->stop_recursion++;

  level = CLAMP (level,
                 WEBX_MIN_ZOOMLEVEL,
                 WEBX_MAX_ZOOMLEVEL);

  preview->zoom = level;
  preview->zoom_mode = WEBX_ZOOM_MODE_NORMAL;
  gtk_widget_queue_resize (preview->area);

  for (i = 0; i < G_N_ELEMENTS (webx_zoomlevels); i++)
    {
      if (ABS (webx_zoomlevels[i] - level) < 0.01)
        {
          gimp_int_combo_box_set_active (intcombo, i);

          if (i == 0)
            gtk_widget_set_sensitive (preview->zoom_out, FALSE);
          else
            gtk_widget_set_sensitive (preview->zoom_out, TRUE);

          if (i == G_N_ELEMENTS (webx_zoomlevels) - 1)
            gtk_widget_set_sensitive (preview->zoom_in, FALSE);
          else
            gtk_widget_set_sensitive (preview->zoom_in, TRUE);

          break;
        }
    }

  preview->stop_recursion--;
}

static void
webx_preview_set_zoom_mode (WebxPreview        *preview,
                            gint                zoom_mode)
{
  if (preview->zoom_mode == zoom_mode)
    return;

  preview->zoom_mode = zoom_mode;
  gtk_widget_queue_resize (preview->area);
}

static void
webx_preview_zoom_combo_changed (GtkWidget     *combo,
                                 WebxPreview   *preview)
{
  gint          index;

  g_return_if_fail (WEBX_IS_PREVIEW (preview));
    
  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &index))
    return;

  if (index >= 0 && index < G_N_ELEMENTS (webx_zoomlevels))
    {
      webx_preview_zoom (preview, webx_zoomlevels[index]);
    }
  else
    {
      if (index == G_N_ELEMENTS (webx_zoomlevels))
        {
          webx_preview_set_zoom_mode (preview,
                                      WEBX_ZOOM_MODE_FIT_WIDTH);
        }
      else
        {
          webx_preview_set_zoom_mode (preview,
                                      WEBX_ZOOM_MODE_FIT_VISIBLE);
        }
    }
}

static void
webx_preview_zoom_in (WebxPreview *preview)
{
  gint          zoomlevel;
  gdouble       value;

  zoomlevel = webx_preview_closest_zoomlevel (preview);
  zoomlevel++;
  value = webx_zoomlevels[MIN (zoomlevel, G_N_ELEMENTS (webx_zoomlevels) - 1)];
  webx_preview_zoom (preview, value);
}

static void
webx_preview_zoom_out (WebxPreview *preview)
{
  gint          zoomlevel;
  gdouble       value;

  zoomlevel = webx_preview_closest_zoomlevel (preview);
  zoomlevel--;
  value = webx_zoomlevels[MAX (zoomlevel, 0)];
  webx_preview_zoom (preview, value);
}

static gint
webx_preview_closest_zoomlevel (WebxPreview    *preview)
{
  gint          i, min_i;
  gdouble       dx, min_dx;

  min_dx = ABS (webx_zoomlevels[0] - preview->zoom); 
  min_i = 0;
  for (i = 1; i < G_N_ELEMENTS (webx_zoomlevels); i++)
    {
      dx = ABS (webx_zoomlevels[i] - preview->zoom);
      if (dx < min_dx)
        {
          min_dx = dx;
          min_i = i;
        }
    }

  return min_i;
}

static void
webx_preview_show_toggled (WebxPreview *preview)
{
  gtk_widget_queue_draw (preview->area);
}
