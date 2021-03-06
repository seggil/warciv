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
#  include "../config.h"
#endif

#include <gtk/gtk.h>

#include "city.h"
#include "wc_intl.h"
#include "game.h"
#include "government.h"
#include "mem.h"
#include "support.h"

#include "../text.h"
#include "../tilespec.h"

#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "mapview.h"

/* semi-arbitrary number that controls the width of the happiness widget */
#define HAPPINESS_PIX_WIDTH 23

#define PIXCOMM_WIDTH   (HAPPINESS_PIX_WIDTH * SMALL_TILE_WIDTH)
#define PIXCOMM_HEIGHT  (SMALL_TILE_HEIGHT)

#define NUM_HAPPINESS_MODIFIERS 5

enum { CITIES, LUXURIES, BUILDINGS, UNITS, WONDERS };

struct happiness_dialog {
  city_t *pcity;
  GtkWidget *shell;
  GtkWidget *cityname_label;
  GtkWidget *hpixmaps[NUM_HAPPINESS_MODIFIERS];
  GtkWidget *hlabels[NUM_HAPPINESS_MODIFIERS];
  GtkWidget *close;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct happiness_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct happiness_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list = NULL;

static struct happiness_dialog *get_happiness_dialog(city_t *pcity);
static struct happiness_dialog *create_happiness_dialog(city_t
                                                        *pcity);
static void happiness_dialog_update_cities(struct happiness_dialog
                                           *pdialog);
static void happiness_dialog_update_luxury(struct happiness_dialog
                                           *pdialog);
static void happiness_dialog_update_buildings(struct happiness_dialog
                                              *pdialog);
static void happiness_dialog_update_units(struct happiness_dialog
                                          *pdialog);
static void happiness_dialog_update_wonders(struct happiness_dialog
                                            *pdialog);

/****************************************************************
...
*****************************************************************/
static struct happiness_dialog *get_happiness_dialog(city_t *pcity)
{
  if (!dialog_list) {
    dialog_list = dialog_list_new();
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}

/**************************************************************************
...
**************************************************************************/
static struct happiness_dialog *create_happiness_dialog(city_t *pcity)
{
  int i;
  struct happiness_dialog *pdialog;
  GtkWidget *vbox;

  pdialog = wc_malloc(sizeof(struct happiness_dialog));
  pdialog->pcity = pcity;

  pdialog->shell = gtk_vbox_new(FALSE, 0);

  pdialog->cityname_label = gtk_frame_new(_("Happiness"));
  gtk_box_pack_start(GTK_BOX(pdialog->shell),
                     pdialog->cityname_label, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE, 18);
  gtk_container_add(GTK_CONTAINER(pdialog->cityname_label), vbox);

  for (i = 0; i < NUM_HAPPINESS_MODIFIERS; i++) {
    GtkWidget *box;

    box = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

    pdialog->hpixmaps[i] = gtk_pixcomm_new(PIXCOMM_WIDTH, PIXCOMM_HEIGHT);
    gtk_box_pack_start(GTK_BOX(box), pdialog->hpixmaps[i], FALSE, FALSE, 0);

    pdialog->hlabels[i] = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), pdialog->hlabels[i], TRUE, FALSE, 0);

    gtk_misc_set_alignment(GTK_MISC(pdialog->hpixmaps[i]), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(pdialog->hlabels[i]), 0, 0);
    gtk_label_set_justify(GTK_LABEL(pdialog->hlabels[i]), GTK_JUSTIFY_LEFT);

    /* gtk_label_set_line_wrap(GTK_LABEL(pdialog->hlabels[i]), TRUE); */
  }

  gtk_widget_show_all(pdialog->shell);

  if (!dialog_list) {
    dialog_list = dialog_list_new();
  }

  dialog_list_prepend(dialog_list, pdialog);

  refresh_happiness_dialog(pcity);

  return pdialog;
}

/**************************************************************************
...
**************************************************************************/
static void refresh_pixcomm(GtkPixcomm *dst, city_t *pcity, int index)
{
  int i;
  struct citizen_type citizens[MAX_CITY_SIZE];
  int num_citizens = pcity->common.pop_size;
  int offset = MIN(SMALL_TILE_WIDTH, PIXCOMM_WIDTH / num_citizens);

  get_city_citizen_types(pcity, index, citizens);

  gtk_pixcomm_freeze(dst);
  gtk_pixcomm_clear(dst);

  for (i = 0; i < num_citizens; i++) {
    gtk_pixcomm_copyto(dst, get_citizen_sprite(citizens[i], i, pcity),
                       i * offset, 0);
  }

  gtk_pixcomm_thaw(dst);
}

/**************************************************************************
...
**************************************************************************/
void refresh_happiness_dialog(city_t *pcity)
{
  int i;

  struct happiness_dialog *pdialog = get_happiness_dialog(pcity);

  for (i = 0; i < 5; i++) {
    refresh_pixcomm(GTK_PIXCOMM(pdialog->hpixmaps[i]), pdialog->pcity, i);
  }

  happiness_dialog_update_cities(pdialog);
  happiness_dialog_update_luxury(pdialog);
  happiness_dialog_update_buildings(pdialog);
  happiness_dialog_update_units(pdialog);
  happiness_dialog_update_wonders(pdialog);
}

/**************************************************************************
...
**************************************************************************/
void close_happiness_dialog(city_t *pcity)
{
  struct happiness_dialog *pdialog = get_happiness_dialog(pcity);

  gtk_widget_hide(pdialog->shell);
  dialog_list_unlink(dialog_list, pdialog);

  gtk_widget_destroy(pdialog->shell);
  free(pdialog);
}

/**************************************************************************
...
**************************************************************************/
static void happiness_dialog_update_cities(struct happiness_dialog
                                           *pdialog)
{
  char buf[512], *bptr = buf;
  int nleft = sizeof(buf);

  city_t *pcity = pdialog->pcity;
  player_t *pplayer = &game.players[pcity->common.owner];
  struct government *g = get_gov_pcity(pcity);
  int cities = city_list_size(pplayer->cities);
  int content = game.info.unhappysize;
  int basis = game.info.cityfactor + g->empire_size_mod;
  int step = g->empire_size_inc;
  int excess = cities - basis;
  int penalty = 0;

  if (excess > 0) {
    if (step > 0)
      penalty = 1 + (excess - 1) / step;
    else
      penalty = 1;
  } else {
    excess = 0;
    penalty = 0;
  }

  my_snprintf(bptr, nleft,
              _("Cities: %d total, %d over threshold of %d cities.\n"),
              cities, excess, basis);
  bptr = end_of_strn(bptr, &nleft);

  my_snprintf(bptr, nleft, _("%d content before penalty with "), content);
  bptr = end_of_strn(bptr, &nleft);
  my_snprintf(bptr, nleft, _("%d additional unhappy citizens."), penalty);
  bptr = end_of_strn(bptr, &nleft);

  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[CITIES]), buf);
}

/**************************************************************************
...
**************************************************************************/
static void happiness_dialog_update_luxury(struct happiness_dialog
                                           *pdialog)
{
  char buf[512], *bptr = buf;
  int nleft = sizeof(buf);
  city_t *pcity = pdialog->pcity;

  my_snprintf(bptr, nleft, _("Luxury: %d total."),
              pcity->common.luxury_total);

  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[LUXURIES]), buf);
}

/**************************************************************************
...
**************************************************************************/
static void happiness_dialog_update_buildings(struct happiness_dialog
                                              *pdialog)
{
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[BUILDINGS]),
                     get_happiness_buildings(pdialog->pcity));
}

/**************************************************************************
...
**************************************************************************/
static void happiness_dialog_update_units(struct happiness_dialog *pdialog)
{
  char buf[512], *bptr = buf;
  int nleft = sizeof(buf);
  city_t *pcity = pdialog->pcity;
  struct government *g = get_gov_pcity(pcity);
  int mlmax = g->martial_law_max;
  int uhcfac = g->unit_happy_cost_factor;

  my_snprintf(bptr, nleft, _("Units: "));
  bptr = end_of_strn(bptr, &nleft);

  if (mlmax > 0) {
    my_snprintf(bptr, nleft, _("Martial law in effect ("));
    bptr = end_of_strn(bptr, &nleft);

    if (mlmax == 100)
      my_snprintf(bptr, nleft, _("no maximum, "));
    else
      my_snprintf(bptr, nleft, PL_("%d unit maximum, ",
                                   "%d units maximum, ", mlmax), mlmax);
    bptr = end_of_strn(bptr, &nleft);

    my_snprintf(bptr, nleft, _("%d per unit). "), g->martial_law_per);
  }
  else if (uhcfac > 0) {
    my_snprintf(bptr, nleft,
                _("Military units in the field may cause unhappiness. "));
  }
  else {
    my_snprintf(bptr, nleft,
                _("Military units have no happiness effect. "));

  }

  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[UNITS]), buf);
}

/**************************************************************************
...
**************************************************************************/
static void happiness_dialog_update_wonders(struct happiness_dialog
                                            *pdialog)
{
  gtk_label_set_text(GTK_LABEL(pdialog->hlabels[WONDERS]),
                     get_happiness_wonders(pdialog->pcity));
}

/**************************************************************************
...
**************************************************************************/
GtkWidget *get_top_happiness_display(city_t *pcity)
{
  return create_happiness_dialog(pcity)->shell;
}
