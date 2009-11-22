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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "webx_main.h"
#include "webx_dialog.h"

#include "plugin-intl.h"

gint    global_image_ID;
gint    global_drawable_ID;

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals);
static void     webx_run (gint32 image_ID,
                          gint32 drawable_ID);

const GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
  };


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
    {
      { GIMP_PDB_INT32,      "run-mode",    "Interactive" },
      { GIMP_PDB_IMAGE,      "image",       "Input image" },
      { GIMP_PDB_DRAWABLE,   "drawable",    "Input drawable" }
    };

  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Optimize & save image for web"),
                          "Optimize image for web.",
                          "Aurimas Juška",
                          "Aurimas Juška",
                          "0.25",
                          N_("Save for Web..."),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Save");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             image_ID;
  gint32             drawable_ID;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  *nreturn_vals = 1;
  *return_vals = values;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      webx_run (image_ID, drawable_ID);
    }

  values[0].data.d_status = status;
}

static void
webx_run (gint32 image_ID, gint32 drawable_ID)
{
  GtkWidget *dlg;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  global_image_ID = image_ID;
  global_drawable_ID = drawable_ID;
 
  if (gimp_image_width (image_ID) > WEBX_MAX_SIZE
      || gimp_image_height (image_ID) > WEBX_MAX_SIZE)
    {
      gimp_message (_("The image is too large for Save for Web!"));
      return;
    }

  dlg = webx_dialog_new (image_ID, drawable_ID);
  webx_dialog_run (WEBX_DIALOG (dlg));
}
