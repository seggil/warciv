/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifdef HAVE_CONFIG_H
#  include "../../config.hh"
#endif

#include <gtk/gtk.h>

#include "wc_intl.hh"
#include "support.hh"

#include "gui_main.hh"
#include "../chatline_common.hh"

#include "gui_stuff.hh"
#include "resources.hh"

#include "style.hh"

static GtkWidget *style_config_shell = NULL;

struct styleconf{
  const char *styletitle;
  const char *stylename;
  const char *stylewidget;
  GtkStyle *style;
};

#define style_iterate(sset)                     \
  {                                             \
    int _i=0;                                   \
    struct styleconf *sset;                     \
    while(TRUE)                                 \
      {                                         \
        sset=&styleconf[_i];                    \
        if(sset->stylewidget==NULL)             \
          break;

#define style_iterate_end                       \
  _i++;                                         \
  }                                             \
    }

static struct styleconf styleconf[] = {
  {
    N_("City"), "city_label", "Warciv*.city label",
    NULL
  },
  {
    N_("Notify"), "notify_label", "Warciv*.notify label",
    NULL
  },
  {
    N_("Spaceship"), "spaceship_label", "Warciv*.spaceship label",
    NULL
  },
  {
    N_("Help"), "help_label", "Warciv*.help label",
    NULL
  },
  {
    N_("Help Links"), "help_link", "Warciv*.help link",
    NULL
  },
  {
    N_("Help texts"), "help_text", "Warciv*.help text",
    NULL
  },
  {
    N_("Chatline"), "chatline", "Warciv*.chatline",
    NULL
  },
  {
    N_("Beta"), "beta_label", "Warciv*.beta label",
    NULL
  },
  {
    N_("Small Font"), "small_font", "Warciv*.small font",
    NULL
  },
  {
    N_("Comment"), "comment_label", "Warciv*.comment label",
    NULL
  },
  {
    N_("City Names Font"), "city_names_font", "Warciv*.city names",
    NULL
  },
  {
    N_("City Productions Font"), "city_productions_font", "Warciv*.city productions",
    NULL
  },
  {
    N_("Vote Label"), "vote label", "Warciv*.vote label",
    NULL
  },
  {
    N_("Vote Yes Button"), "vote yes button", "Warciv*.vote yes button",
    NULL
  },
  {
    N_("Vote No Button"), "vote no button", "Warciv*.vote no button",
    NULL
  },
  {
    N_("Vote Abstain Button"), "vote abstain button", "Warciv*.vote abstain button",
    NULL
  },
  {
    N_("Tooltips"), "tooltips", "gtk-tooltips*.*",
    NULL
  },
  {
    NULL, NULL, NULL,
    NULL
  }
};

/**************************************************************************
...
**************************************************************************/
static void change_style(GtkFontButton *widget, gpointer data)
{
  struct styleconf *tstyleconf= (struct styleconf*)data;

  tstyleconf->style->font_desc =
    pango_font_description_from_string(gtk_font_button_get_font_name(widget));
}

/**************************************************************************
...
**************************************************************************/
static void destroy_callback(GtkWidget *w,
                             gpointer user_data)
{
  style_config_shell = NULL;
}

/**************************************************************************
...
**************************************************************************/
static void read_style_from_rc(void)
{
  GtkSettings *gtksettings;
  GdkScreen *screen;

  screen = gdk_screen_get_default();
  gtksettings = gtk_settings_get_for_screen(screen);
  style_iterate(sset){
    sset->style =
      gtk_style_copy(gtk_rc_get_style_by_paths(gtksettings,
                                               sset->stylewidget,
                                               NULL, G_TYPE_NONE));
    if (!sset->style) {
      sset->style = gtk_style_new();
    }
  } style_iterate_end;

}

/**************************************************************************
  Unlike the real gdk_color_to_string (only in gdk >= 2.12), this returns
  a pointer to a static buffer. So do not call this twice in the same
  expression (e.g. as arguments to a function)!
**************************************************************************/
static gchar *colorstr(const GdkColor *color)
{
  static char buf[32];
  my_snprintf(buf, sizeof(buf), "#%04x%04x%04x",
              color->red, color->green, color->blue);
  return buf;
}

/**************************************************************************
...
**************************************************************************/
static void create_style_str(char *buf, int buflen)
{
  char *p = buf, *tmp;

#define SSCAT(fmt, arg) do {                                            \
    p += my_snprintf(p, buflen - (unsigned) (p - buf), (fmt), (arg));   \
  } while(0)

  style_iterate(sset){
    SSCAT("style \"%s\"\n{\n", sset->stylename);

    tmp = pango_font_description_to_string(sset->style->font_desc);
    SSCAT("font_name = \"%s\"\n", tmp);
    g_free(tmp);

    SSCAT("base[NORMAL] = \"%s\"\n", colorstr(&sset->style->base[0]));
    SSCAT("base[ACTIVE] = \"%s\"\n", colorstr(&sset->style->base[1]));
    SSCAT("base[PRELIGHT] = \"%s\"\n", colorstr(&sset->style->base[2]));
    SSCAT("base[SELECTED] = \"%s\"\n", colorstr(&sset->style->base[3]));
    SSCAT("base[INSENSITIVE] = \"%s\"\n", colorstr(&sset->style->base[4]));
    SSCAT("text[NORMAL] = \"%s\"\n", colorstr(&sset->style->text[0]));
    SSCAT("text[ACTIVE] = \"%s\"\n", colorstr(&sset->style->text[1]));
    SSCAT("text[PRELIGHT] = \"%s\"\n", colorstr(&sset->style->text[2]));
    SSCAT("text[SELECTED] = \"%s\"\n", colorstr(&sset->style->text[3]));
    SSCAT("text[INSENSITIVE] = \"%s\"\n", colorstr(&sset->style->text[4]));
    SSCAT("bg[NORMAL] = \"%s\"\n", colorstr(&sset->style->bg[0]));
    SSCAT("bg[ACTIVE] = \"%s\"\n", colorstr(&sset->style->bg[1]));
    SSCAT("bg[PRELIGHT] = \"%s\"\n", colorstr(&sset->style->bg[2]));
    SSCAT("bg[SELECTED] = \"%s\"\n", colorstr(&sset->style->bg[3]));
    SSCAT("bg[INSENSITIVE] = \"%s\"\n", colorstr(&sset->style->bg[4]));

    p += my_snprintf(p, buflen - (unsigned) (p - buf), "}\n\n");
  } style_iterate_end;

  style_iterate(sset){
    SSCAT("widget \"%s\" ", sset->stylewidget);
    SSCAT("style \"%s\"\n", sset->stylename);
  } style_iterate_end;

#undef SSCAT
}

/**************************************************************************
...
**************************************************************************/
static void cancel_callback(GtkWidget *w,
                            gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;
  gtk_widget_destroy(dialog);
}

/**************************************************************************
  ...
**************************************************************************/
static void undo_callback(GtkWidget *w,
                          gpointer user_data)
{
  char *tmp;
  GtkWidget *sel, *hbox;
  GtkWidget *dialog = (GtkWidget *) user_data;

  read_style_from_rc();

  style_iterate(sset)
  {
    hbox = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), sset->stylename);
    sel = (GtkWidget*)g_object_get_data(G_OBJECT(hbox), "fontname");
    tmp = pango_font_description_to_string(sset->style->font_desc);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(sel), tmp);
    g_free(tmp);
  } style_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void reset_style(const char * stylebuf)
{
  GtkSettings *gtksettings;
  GdkScreen *screen;
  GtkStyle *style;

  gtk_rc_parse_string(stylebuf);
  screen = gdk_screen_get_default();
  gtksettings = gtk_settings_get_for_screen(screen);
  gtk_rc_reset_styles(gtksettings);

  /* font names shouldn't be in spec files! */
  style = gtk_rc_get_style_by_paths(gtksettings,
                                    "Warciv*.city names",
                                    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  main_font = style->font_desc;

  style = gtk_rc_get_style_by_paths(gtksettings,
                                    "Warciv*.city productions",
                                    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  city_productions_font = style->font_desc;

  update_map_canvas_visible(MAP_UPDATE_NORMAL);
}

/**************************************************************************
  ...
**************************************************************************/
static void save_callback(GtkWidget *w,
                          gpointer user_data)
{
  const gchar *home;
  FILE *fp;
  char buf[255], stylebuf[16384];

  create_style_str(stylebuf, sizeof(stylebuf));

  home = g_get_home_dir();
  if (home) {
    gchar *str;

    str = g_build_filename(home, ".warciv.rc-2.0", NULL);
    fp = fopen(str, "wr");
    fputs(stylebuf, fp);
    fclose(fp);
    my_snprintf(buf, sizeof(buf), _("Styles are saved in %s"), str);
    append_output_window(buf);
    gtk_rc_parse(str);
    g_free(str);
  } else {
    append_output_window(_("Save failed, cannot find home dir."));
  }
  reset_style(stylebuf);
}

/**************************************************************************
  ...
**************************************************************************/
static void apply_callback(GtkWidget *w,
                           gpointer user_data)
{
  char stylebuf[16384];

  create_style_str(stylebuf, sizeof(stylebuf));
  reset_style(stylebuf);
}

/**************************************************************************
  ...
**************************************************************************/
static void reset_callback(GtkWidget *w,
                           gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;

  reset_style(fallback_resources);
  undo_callback(NULL, dialog);
}

/**************************************************************************
...
**************************************************************************/
static void ok_callback(GtkWidget *w,
                        gpointer user_data)
{
  GtkWidget *dialog = (GtkWidget *) user_data;

  apply_callback(NULL, dialog);
  gtk_widget_destroy (dialog);
}

/**************************************************************************
  ...
**************************************************************************/
static GtkWidget *create_style_config_shell(void)
{
  GtkWidget *dialog, *hbox, *button, *vbox, *sep;
  GtkWidget *vbox2, *label, *sel;
  char *tmp;

  /* create the window */
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog),
                       _("Font Configuration"));
  //  gtk_window_set_default_size (GTK_WINDOW (dialog), 800, 600);
  gtk_window_set_position(GTK_WINDOW (dialog),
                          GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_resizable(GTK_WINDOW (dialog), TRUE);
  gtk_window_set_decorated(GTK_WINDOW (dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER(dialog), 5);

  gtk_widget_set_name(dialog, "Warciv");
  setup_dialog(dialog, toplevel);

  g_signal_connect(dialog, "destroy",
                   G_CALLBACK(destroy_callback), dialog);

  /* layout boxes */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add (GTK_CONTAINER(dialog), vbox);

  vbox2 = gtk_vbox_new(TRUE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(TRUE, 10);
  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", NULL,
                       "label", _("Label") ,
                       "xalign", 0.5, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(hbox), label , TRUE, TRUE, 0);
  label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                       "use-underline", TRUE,
                       "mnemonic-widget", NULL,
                       "label", _("Font") ,
                       "xalign", 0.5, "yalign", 0.5, NULL);
  gtk_box_pack_start(GTK_BOX(hbox), label , TRUE, TRUE, 0);

  /*   block list of differents style used  */
  gtk_box_pack_start(GTK_BOX(vbox2), hbox , TRUE, TRUE, 0);

  read_style_from_rc();

  style_iterate(sset){
    hbox = gtk_hbox_new(TRUE, 5);

    g_object_set_data(G_OBJECT (dialog), sset->stylename, hbox);

    label = (GtkWidget*)g_object_new(GTK_TYPE_LABEL,
                         "use-underline", TRUE,
                         "mnemonic-widget", NULL,
                         "label", _(sset->styletitle) ,
                         "xalign", 0.0, "yalign", 0.5, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), label , TRUE, TRUE, 0);

    tmp = pango_font_description_to_string(sset->style->font_desc);
    sel = gtk_font_button_new_with_font(tmp);
    g_free(tmp);
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(sel), TRUE);
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(sel), TRUE);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(sel), TRUE);
    gtk_font_button_set_title(GTK_FONT_BUTTON(sel), sset->styletitle);
    g_signal_connect(sel, "font-set" , G_CALLBACK(change_style), sset);
    g_object_set_data (G_OBJECT(hbox), "fontname", sel);

    gtk_box_pack_start(GTK_BOX(hbox), sel , TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox2), hbox , TRUE, TRUE, 0);
  } style_iterate_end;

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX (vbox), sep, FALSE, FALSE, 0);

  /* Cancel, Valid, Apply buttons */
  hbox = gtk_hbox_new(TRUE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_CANCEL, _("_Cancel"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(cancel_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_OK, _("_Ok"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(ok_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_APPLY, _("_Apply"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(apply_callback), dialog);
  gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0);


  /* save/undo/reset buttons */
  hbox = gtk_hbox_new(TRUE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  button = gtk_stockbutton_new(GTK_STOCK_SAVE, _("_Save"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect (button, "clicked",
                    G_CALLBACK(save_callback), dialog);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_UNDO, _("_Undo"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(undo_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  button = gtk_stockbutton_new(GTK_STOCK_CLEAR, _("_Reset"));
  gtk_widget_set_size_request(button, 120, 30);
  g_signal_connect(button, "clicked",
                   G_CALLBACK(reset_callback), dialog);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  /*   button = gtk_stockbutton_new(GTK_STOCK_HELP, _("_Help")); */
  /*   gtk_widget_set_size_request(button, 120, 30); */
  /*   g_signal_connect(button, "clicked", */
  /*                G_CALLBACK(help_callback), dialog); */
  /*   gtk_box_pack_start(GTK_BOX (hbox), button, FALSE, FALSE, 0); */

  gtk_widget_show_all (dialog);

  return dialog;
}

/**************************************************************************
  ...
**************************************************************************/
void popup_style_config_dialog(void)
{
  if (!style_config_shell)  {
    style_config_shell = create_style_config_shell();
  }

  gtk_window_present(GTK_WINDOW(style_config_shell));
}
