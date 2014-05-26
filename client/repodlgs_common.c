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

#include <assert.h>

#include "wc_intl.h"
#include "mem.h"                /* free */
#include "support.h"            /* my_snprintf */

#include "game.h"
#include "government.h"
#include "map.h"

#include "include/repodlgs_g.h"

#include "civclient.h"          /* can_client_issue_orders */
#include "clinet.h"
#include "control.h"
#include "repodlgs_common.h"
#include "packhand_gen.h"

struct options_settable *settable_options;
int num_settable_options;

char **options_categories;
int num_options_categories;

/****************************************************************
  Fills out the array of struct improvement_entry given by
  entries. The array must be able to hold at least B_LAST entries.
*****************************************************************/
void get_economy_report_data(struct improvement_entry *entries,
                             int *num_entries_used, int *total_cost,
                             int *total_income)
{
  *num_entries_used = 0;
  *total_cost = 0;

  impr_type_iterate(impr_id) {
    if (!is_wonder(impr_id)) {
      int count = 0, cost = 0;
      city_list_iterate(get_player_ptr()->cities, pcity) {
        if (city_got_building(pcity, impr_id)) {
          count++;
          cost += improvement_upkeep(pcity, impr_id);
        }
      }
      city_list_iterate_end;

      if (count == 0) {
        continue;
      }

      entries[*num_entries_used].type = impr_id;
      entries[*num_entries_used].count = count;
      entries[*num_entries_used].total_cost = cost;
      entries[*num_entries_used].cost = cost / count;
      (*num_entries_used)++;

      /* Currently there is no building expense under anarchy.  It's
       * not a good idea to hard-code this in the client, but what
       * else can we do? */
      if (get_player_ptr()->government != game.ruleset_control.government_when_anarchy) {
        *total_cost += cost;
      }
    }
  } impr_type_iterate_end;

  *total_income = 0;

  city_list_iterate(get_player_ptr()->cities, pcity) {
    *total_income += pcity->common.tax_total;
    if (get_current_construction_bonus(pcity, EFFECT_TYPE_PROD_TO_GOLD) > 0) {
      *total_income += MAX(0, pcity->common.shield_surplus);
    }
  } city_list_iterate_end;
}

/******************************************************************
  Returns an array of units with gold_upkeep. Number of units in
  the array is added to num_entries_used.
******************************************************************/
void get_economy_report_units_data(struct unit_entry *entries,
                                   int *num_entries_used, int *total_cost)
{
  int count, cost, partial_cost;
  struct unit_type *unittype;

  unit_type_iterate(utype) {
    unittype = get_unit_type(utype);
    cost = utype_gold_cost(unittype, get_gov_pplayer(get_player_ptr()));

    if (cost == 0) {
      continue;
    }

    count = 0;
    partial_cost = 0;

    city_list_iterate(get_player_ptr()->cities, pcity) {
      unit_list_iterate(pcity->common.units_supported, punit) {

        if (punit->type == utype) {
          count++;
          partial_cost += punit->upkeep_gold;
        }

      } unit_list_iterate_end;
    } city_list_iterate_end;

    if (count == 0) {
      continue;
    }

    (*total_cost) += partial_cost;

    entries[*num_entries_used].type = utype;
    entries[*num_entries_used].count = count;
    entries[*num_entries_used].cost = cost;
    entries[*num_entries_used].total_cost = partial_cost;
    (*num_entries_used)++;

  } unit_type_iterate_end;
}

static int frozen_level = 0;

/******************************************************************
 Turn off updating of reports
*******************************************************************/
void report_dialogs_freeze(void)
{
  frozen_level++;
}

/******************************************************************
 Turn on updating of reports
*******************************************************************/
void report_dialogs_thaw(void)
{
  frozen_level--;
  if (frozen_level < 0) {
    frozen_level = 0;
  }
  if (frozen_level == 0) {
    update_report_dialogs();
  }
}

/******************************************************************
 Turn on updating of reports
*******************************************************************/
void report_dialogs_force_thaw(void)
{
  frozen_level = 1;
  report_dialogs_thaw();
}

/******************************************************************
 ...
*******************************************************************/
bool is_report_dialogs_frozen(void)
{
  return frozen_level > 0;
}

/******************************************************************
 initialize settable_options[] and options_categories[]
*******************************************************************/
void settable_options_init(void)
{
  settable_options = NULL;
  num_settable_options = 0;

  options_categories = NULL;
  num_options_categories = 0;
}

/******************************************************************
 free settable_options[] and options_categories[]
*******************************************************************/
void settable_options_free(void)
{
  int i;

  for (i = 0; i < num_settable_options; i++) {
    if (settable_options[i].name) {
      free(settable_options[i].name);
    }
    if (settable_options[i].short_help) {
      free(settable_options[i].short_help);
    }
    if (settable_options[i].extra_help) {
      free(settable_options[i].extra_help);
    }
    if (settable_options[i].strval) {
      free(settable_options[i].strval);
    }
    if (settable_options[i].default_strval) {
      free(settable_options[i].default_strval);
    }
  }
  free(settable_options);

  for (i = 0; i < num_options_categories; i++) {
    free(options_categories[i]);
  }
  free(options_categories);

  settable_options_init();
}

/******************************************************************
 reinitialize the options_settable struct: allocate enough
 space for all the options that the server is going to send us.
*******************************************************************/
void handle_options_settable_control( /* 112 sc */
                               struct packet_options_settable_control *packet)
{
  int i;

# if REPLAY
  printf("OPTIONS_SETTABLE_CONTROL\n"); /* done */
  printf("nids=%d ", packet->nids);
  printf("ncategories=%d ", packet->ncategories);
  printf("category_names[256]={");
  for (i = 0; i < 256; i++) {
    printf("\"%s\" ", packet->category_names[i]);
  }
  printf("}\n");
# endif
  settable_options_free();

  options_categories = wc_malloc(packet->ncategories * sizeof(char *));
  num_options_categories = packet->ncategories;

  for (i = 0; i < num_options_categories; i++) {
    options_categories[i] = mystrdup(packet->category_names[i]);
  }

  /* avoid a malloc of size 0 warning */
  if (packet->nids == 0) {
    return;
  }

  settable_options = wc_malloc(packet->nids * sizeof(struct options_settable));
  num_settable_options = packet->nids;

  for (i = 0; i < num_settable_options; i++) {
    settable_options[i].name = NULL;
    settable_options[i].short_help = NULL;
    settable_options[i].extra_help = NULL;
    settable_options[i].strval = NULL;
    settable_options[i].default_strval = NULL;
  }
}

/******************************************************************
 Fill the settable_options array with an option.
 If we've filled the last option, popup the dialog.
*******************************************************************/
void handle_options_settable(struct packet_options_settable *packet) /* 113 sc */
{
  int i = packet->id;

# if REPLAY
  printf("OPTIONS_SETTABLE\n"); /* done */
  printf("id=%d ", packet->id);
  printf("name=%s ", packet->name);
  printf("short_help=%s ", packet->short_help);
  printf("extra_help=%s ", packet->extra_help);
  printf("type=%d ", packet->type);
  printf("val=%d ", packet->val);
  printf("default_val=%d ", packet->default_val);
  printf("min=%d ", packet->min);
  printf("max=%d ", packet->max);
  printf("strval=%s ", packet->strval);
  printf("default_strval=%s ", packet->default_strval);
  printf("category=%d ", packet->category);
# endif

  assert(i >= 0);

  settable_options[i].name = mystrdup(packet->name);
  settable_options[i].short_help = mystrdup(packet->short_help);
  settable_options[i].extra_help = mystrdup(packet->extra_help);

  settable_options[i].type = packet->type;
  settable_options[i].category = packet->category;

  switch (packet->type) {
  case SSET_BOOL:
    settable_options[i].val = packet->val;
    settable_options[i].min = FALSE;
    settable_options[i].max = TRUE;
    settable_options[i].strval = NULL;
    settable_options[i].default_strval = NULL;
    break;
  case SSET_INT:
    settable_options[i].val = packet->val;
    settable_options[i].min = packet->min;
    settable_options[i].max = packet->max;
    settable_options[i].strval = NULL;
    settable_options[i].default_strval = NULL;
    break;
  case SSET_STRING:
    settable_options[i].strval = mystrdup(packet->strval);
    settable_options[i].default_strval = mystrdup(packet->default_strval);
    break;
  default:
    assert(0);
  }

  /* if we've received all the options, pop up the settings dialog */
  if (i == num_settable_options - 1) {
    popup_settable_options_dialog();
  }
}

/****************************************************************************
  Sell all improvements of the given type in all cities.  If "obsolete_only"
  is specified then only those improvements that are replaced will be sold.

  The "message" string will be filled with a GUI-friendly message about
  what was sold.
****************************************************************************/
void sell_all_improvements(Impr_Type_id impr, bool obsolete_only,
                           char *message, size_t message_sz)
{
  int count = 0, gold = 0;

  if (!can_client_issue_orders()) {
    my_snprintf(message, message_sz, _("You cannot sell improvements."));
    return;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    if (!pcity->common.did_sell && city_got_building(pcity, impr)
        && (!obsolete_only
            || improvement_obsolete(get_player_ptr(), impr)
            || is_building_replaced(pcity, impr))) {
      count++;
      gold += impr_sell_gold(impr);
      city_sell_improvement(pcity, impr);
    }
  } city_list_iterate_end;

  if (count > 0) {
    my_snprintf(message, message_sz, _("Sold %d %s for %d gold."),
                count, get_improvement_name(impr), gold);
  } else {
    my_snprintf(message, message_sz, _("No %s could be sold."),
                get_improvement_name(impr));
  }
}

/****************************************************************************
  Disband all supported units of the given type.  If in_cities_only is
  specified then only units inside our cities will be disbanded.

  The "message" string will be filled with a GUI-friendly message about
  what was sold.
****************************************************************************/
void disband_all_units(Unit_Type_id type, bool in_cities_only,
                       char *message, size_t message_sz)
{
  int count = 0;

  if (!can_client_issue_orders()) {
    /* TRANS: Obscure observer error. */
    my_snprintf(message, message_sz, _("You cannot disband units."));
    return;
  }

  if (unit_type_flag(type, F_UNDISBANDABLE)) {
    my_snprintf(message, message_sz, _("%s cannot be disbanded."),
                unit_name(type));
    return;
  }

  city_list_iterate(get_player_ptr()->cities, pcity) {
    /* Only supported units are disbanded.  Units with no homecity have no
     * cost and are not disbanded. */
    unit_list_iterate(pcity->common.units_supported, punit) {
      city_t *incity = map_get_city(punit->tile);

      if (punit->type == type
          && (!in_cities_only
              || (incity && city_owner(incity) == get_player_ptr()))) {
        count++;
        request_unit_disband(punit);
      }
    } unit_list_iterate_end;
  } city_list_iterate_end;

  if (count > 0) {
    my_snprintf(message, message_sz, _("Disbanded %d %s."),
                count, unit_name(type));
  } else {
    my_snprintf(message, message_sz, _("No %s could be disbanded."),
                unit_name(type));
  }
}
