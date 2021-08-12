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

#define PLUG_IN_PROC             "file-web-export"
#define PLUG_IN_BINARY           "webexport"

#define RESPONSE_RESET           1

#define WEBX_MAX_SIZE            (10000)

extern gint     global_image_ID;
extern gint     global_drawable_ID;
