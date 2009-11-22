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
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "webx_main.h"
#include "webx_indexed_target.h"
#include "plugin-intl.h"

static GObject* webx_indexed_target_constructor (GType                  type,
                                                 guint                  n_params,
                                                 GObjectConstructParam *params);

static void     webx_indexed_target_changed     (WebxIndexedTarget     *indexed);

G_DEFINE_TYPE (WebxIndexedTarget, webx_indexed_target, WEBX_TYPE_TARGET)

#define parent_class webx_indexed_target_parent_class


static void
webx_indexed_target_class_init (WebxIndexedTargetClass *klass)
{
  GObjectClass         *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->constructor = webx_indexed_target_constructor;
}

static void
webx_indexed_target_init (WebxIndexedTarget *indexed)
{
}

static GObject*
webx_indexed_target_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject              *object;
  WebxIndexedTarget    *indexed;
  GtkWidget            *radio;
  GSList               *radio_group;
  GtkWidget            *combo;
  GtkWidget   *label;
  GtkWidget            *separator;
  gint         row = 0;

  object = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_params,
                                                       params);
  indexed = WEBX_INDEXED_TARGET (object);

  /*
   * reuse existing
   */

  radio = gtk_radio_button_new_with_label (NULL, _("Reuse existing palette"));
  gtk_table_attach (GTK_TABLE (indexed), radio, 
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  g_signal_connect_swapped (GTK_WIDGET (radio), "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  indexed->reuse_pal_w = radio;
  gtk_widget_show (radio);

  /*
   * generate optimum
   */

  row++;
  radio = gtk_radio_button_new_with_label (radio_group,
                                           _("Generate optimum palette"));
  gtk_table_attach (GTK_TABLE (indexed), radio, 
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  g_signal_connect_swapped (GTK_WIDGET (radio), "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  indexed->make_pal_w = radio;
  gtk_widget_show (radio);

  row++;
  label = gtk_label_new (_("Number of colors:"));
  gtk_table_attach (GTK_TABLE (indexed), label,
                    0, 2, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  indexed->num_colors_w = gtk_spin_button_new_with_range (2, 256, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (indexed->num_colors_w), 256);
  g_signal_connect_swapped (indexed->num_colors_w, "value-changed",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  gtk_table_attach (GTK_TABLE (indexed), indexed->num_colors_w,
                    2, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (indexed->num_colors_w);

  /*
   * web-optimized
   */

  row++;
  radio = gtk_radio_button_new_with_label (radio_group,
                                           _("Use web-optimized palette"));
  gtk_table_attach (GTK_TABLE (indexed), radio, 
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  g_signal_connect_swapped (GTK_WIDGET (radio), "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  indexed->web_pal_w = radio;
  gtk_widget_show (radio);

  /*
   * b&w
   */

  row++;
  radio = gtk_radio_button_new_with_label (radio_group,
                                           _("Use black and white palette"));
  gtk_table_attach (GTK_TABLE (indexed), radio, 
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
  g_signal_connect_swapped (GTK_WIDGET (radio), "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  indexed->bw_pal_w = radio;
  gtk_widget_show (radio);

  row++;
  indexed->remove_unused_w = gtk_check_button_new_with_label (_("Remove unused colors"));
  gtk_table_attach (GTK_TABLE (indexed), indexed->remove_unused_w,
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect_swapped (indexed->remove_unused_w, "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  gtk_widget_show (indexed->remove_unused_w);

  row++;
  separator = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (indexed), separator,
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 8);
    
  row++;
  label = gtk_label_new (_("Dither:"));
  gtk_table_attach (GTK_TABLE (indexed), label,
                    0, 1, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  combo = gimp_int_combo_box_new (_("None"),
                                  GIMP_NO_DITHER,
                                  _("Floyd-Steinberg"),
                                  GIMP_FS_DITHER,
                                  _("Floyd-Steinberg 2"),
                                  GIMP_FSLOWBLEED_DITHER,
                                  _("Positioned"),
                                  GIMP_FIXED_DITHER,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), GIMP_NO_DITHER);
  gtk_table_attach (GTK_TABLE (indexed), combo,
                    1, 3, row, row+1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  indexed->dither_type_w = combo;
  gtk_widget_show (combo);

  row++;
  indexed->alpha_dither_w = gtk_check_button_new_with_label (_("Dithering of transparency"));
  gtk_table_attach (GTK_TABLE (indexed), indexed->alpha_dither_w,
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect_swapped (indexed->alpha_dither_w, "toggled",
                            G_CALLBACK (webx_indexed_target_changed),
                            indexed);
  gtk_widget_show (indexed->alpha_dither_w);

  row++;
  separator = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (indexed), separator,
                    0, 3, row, row+1,
                    GTK_FILL, GTK_FILL, 0, 8);
  gtk_widget_show (separator);

  indexed->last_row = ++row;

  webx_indexed_target_changed (indexed);

  return object;
}

gint
webx_indexed_target_get_image (WebxIndexedTarget       *indexed,
                               WebxTargetInput         *input,
                               gint                    *layer)
{
  gint          tmp_image;
  gint          tmp_layer;
  gint       *layers;
  gint        num_layers;
  gchar      *custom_palette;
  gint        num_colors;
  gboolean    converted;

  g_return_val_if_fail (WEBX_IS_INDEXED_TARGET (indexed), -1);

  if (indexed->palette_type == GIMP_REUSE_PALETTE)
    {
      *layer = input->indexed_layer;
      return input->indexed_image;
    }

  custom_palette = indexed->custom_palette;
  if (! custom_palette)
    custom_palette = "";

  tmp_image = input->rgb_image;
  tmp_layer = input->rgb_layer;
  num_colors = indexed->num_colors;
  if (num_colors == 256 && gimp_drawable_has_alpha (tmp_layer))
    num_colors = 255;
  tmp_image = gimp_image_duplicate (tmp_image);
  converted = gimp_image_convert_indexed (tmp_image, indexed->dither_type,
                                          indexed->palette_type,
                                          num_colors,
                                          indexed->alpha_dither,
                                          indexed->remove_unused,
                                          custom_palette);
  if (! converted)
    {
      gimp_image_delete (tmp_image);
      *layer = -1;
      return -1;
    }
  layers = gimp_image_get_layers (tmp_image, &num_layers);
  g_assert (num_layers == 1);
  tmp_layer = layers[0];
  indexed->image = tmp_image;
  indexed->layer = tmp_layer;

  *layer= tmp_layer;
  return tmp_image;
}

void
webx_indexed_target_free_image (WebxIndexedTarget      *indexed,
                                WebxTargetInput        *input,
                                gint                    image)
{
  if (indexed->image == image)
    {
      gimp_image_delete (image);
      indexed->image = -1;
    }
}

static void
webx_indexed_target_changed (WebxIndexedTarget *indexed)
{
  gtk_widget_set_sensitive (GTK_WIDGET (indexed->remove_unused_w), TRUE);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->reuse_pal_w)))
    {
      if (!gimp_drawable_is_indexed (global_drawable_ID))
        { /* no user palette */
          gtk_widget_set_sensitive (indexed->reuse_pal_w, FALSE);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (indexed->make_pal_w),
                                        TRUE);
        }
      else
        {
          gtk_widget_set_sensitive (indexed->reuse_pal_w, TRUE);
          gtk_widget_set_sensitive (indexed->remove_unused_w, FALSE);
          indexed->palette_type = GIMP_REUSE_PALETTE;
        }
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->make_pal_w)))
    {
      indexed->palette_type = GIMP_MAKE_PALETTE;
      gtk_widget_set_sensitive (indexed->num_colors_w, TRUE);
      gtk_widget_set_sensitive (indexed->remove_unused_w, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (indexed->num_colors_w, FALSE);
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->web_pal_w)))
    {
      indexed->palette_type = GIMP_WEB_PALETTE;
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->bw_pal_w)))
    {
      indexed->palette_type = GIMP_MONO_PALETTE;
    }

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (indexed->dither_type_w),
                                 &indexed->dither_type);
  indexed->num_colors = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (indexed->num_colors_w));

  indexed->remove_unused = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->remove_unused_w));
  indexed->alpha_dither = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (indexed->alpha_dither_w));

  webx_target_changed (WEBX_TARGET (indexed));
}
