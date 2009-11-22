/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "plugin-intl.h"

#include "webx_prefs.h"


static gint
parse_yes_no (void)
{
  char *token = strtok(NULL, " )");
  return (gint) strcmp(token, "no");
}

static gint
parse_int (void)
{
  char *token = strtok(NULL, " )");
  return (gint) atoi(token);
}

static void
parse_line (WebxPrefs *prefs, char *line)
{
  char *token;

  line++;			/* Skip '(' */
  token = strtok(line, " ");

  if (!strcmp(token, "dialog-layout"))
    {
      prefs->dlg_x = parse_int ();
      prefs->dlg_y = parse_int ();
      prefs->dlg_width = parse_int ();
      prefs->dlg_height = parse_int ();
      prefs->dlg_splitpos = parse_int ();
    }
  else
    {
      /* Unrecognized, just ignore rest of line */
    }
}

gboolean
webx_prefs_load (WebxPrefs *prefs)
{
  FILE        *in;
  char         buf[256];
  gchar       *filename;

  memset (prefs, 0, sizeof (WebxPrefs));

  filename = gimp_personal_rc_file ("webxrc");

  in = g_fopen(filename, "r");
  g_free(filename);
  if (in)
    {
      while (fgets(buf, sizeof(buf), in))
        {
          if (*buf != '\n' && *buf != '#')
            {
              parse_line (prefs, buf);
            }
        }
      fclose(in);
      return TRUE;
    }
  return FALSE;
}

void
webx_prefs_save (WebxPrefs *prefs)
{
  FILE        *out;
  gchar       *filename;

  filename = gimp_personal_rc_file ("webxrc");

  out = g_fopen(filename, "w");
  if (out)
    {
      fprintf(out, "# Save-for-web plug-in resource file\n\n");

      fprintf(out, "(dialog-layout %d %d %d %d %d)\n",
              prefs->dlg_x, prefs->dlg_y,
              prefs->dlg_width, prefs->dlg_height,
              prefs->dlg_splitpos);

      fclose(out);
    }
  else
    {
      g_message (_("Couldn't save resource file: %s"), filename);
    }

  g_free(filename);
}


