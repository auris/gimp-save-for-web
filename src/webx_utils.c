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

#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "webx_main.h"
#include "webx_utils.h"

#include "plugin-intl.h"

GdkPixbuf*
webx_drawable_to_pixbuf (gint layer)
{
  gint             width;
  gint             height;
  gint             bpp;
  guchar          *buf;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;
  GdkPixbuf       *pixbuf;

  width = gimp_drawable_width (layer);
  height = gimp_drawable_height (layer);
  bpp = gimp_drawable_bpp (layer);
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);
  buf = g_malloc (width * height * bpp);
  gimp_pixel_rgn_get_rect (&pixel_rgn, buf, 0, 0, width, height);
  gimp_drawable_detach (drawable);

  pixbuf = gdk_pixbuf_new_from_data (buf, GDK_COLORSPACE_RGB,
                                     gimp_drawable_has_alpha (layer), 8,
                                     width, height, width * bpp,
                                     (GdkPixbufDestroyNotify)g_free, NULL);
  return pixbuf;
}

GdkPixbuf*
webx_image_to_pixbuf (gint image)
{
  gint           layer;

  layer = gimp_image_merge_visible_layers (image, GIMP_CLIP_TO_IMAGE);
  if (! gimp_drawable_is_rgb (layer))
    gimp_image_convert_rgb (image);
  return webx_drawable_to_pixbuf (layer);
}

gint
webx_get_file_size (const gchar *file_name)
{
  struct stat buf;
  
  g_stat (file_name, &buf);
  return buf.st_size;
}
