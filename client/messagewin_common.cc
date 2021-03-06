/**********************************************************************
 Freeciv - Copyright (C) 2002 - R. Falke
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
#  include "../config.hh"
#endif

#include <assert.h>
#include <string.h>

#include "wc_intl.hh"
#include "map.hh"
#include "mem.hh"

#include "include/citydlg_g.hh"
#include "civclient.hh"
#include "clinet.hh"
#include "include/mapview_g.hh"
#include "include/messagewin_g.hh"
#include "options.hh"

#include "messagewin_common.hh"

static int frozen_level = 0;
static bool change = FALSE;
static struct message_s *messages = NULL;
static int messages_total = 0;
static int messages_alloc = 0;

/******************************************************************
 Turn off updating of message window
*******************************************************************/
void meswin_freeze(void)
{
  frozen_level++;
}

/******************************************************************
 Turn on updating of message window
*******************************************************************/
void meswin_thaw(void)
{
  frozen_level--;
  if (frozen_level < 0) {
    frozen_level = 0;
  }
  if (frozen_level == 0) {
    update_meswin_dialog();
  }
}

/******************************************************************
 Turn on updating of message window
*******************************************************************/
void meswin_force_thaw(void)
{
  frozen_level = 1;
  meswin_thaw();
}

/**************************************************************************
...
**************************************************************************/
void update_meswin_dialog(void)
{
  if (frozen_level > 0 || !change) {
    return;
  }

  if (!is_meswin_open() && messages_total > 0 &&
      (!get_player_ptr()->ai.control || ai_popup_windows)) {
    popup_meswin_dialog();
    change = FALSE;
    return;
  }

  if (is_meswin_open()) {
    real_update_meswin_dialog();
    change = FALSE;
  }
}

/**************************************************************************
...
**************************************************************************/
void clear_notify_window(void)
{
  int i;

  change = TRUE;
  for (i = 0; i < messages_total; i++) {
    free(messages[i].descr);
    messages[i].descr = NULL;
  }
  messages_total = 0;
  update_meswin_dialog();
}

/**************************************************************************
...
**************************************************************************/
void add_notify_window(char *message, tile_t *ptile,
                       enum event_type event)
{
  const size_t min_msg_len = 50;
  const char *game_prefix1 = "Game: ";
  const char *game_prefix2 = _("Game: ");
  size_t gp_len1 = strlen(game_prefix1);
  size_t gp_len2 = strlen(game_prefix2);
  char *s = (char*)wc_malloc(MAX(strlen(message), min_msg_len) + 1);
  int i, nspc;

  change = TRUE;

  if (messages_total + 2 > messages_alloc) {
    messages_alloc = messages_total + 32;
    messages = static_cast<message_s*>(wc_realloc(messages, messages_alloc * sizeof(*messages)));
  }

  if (strncmp(message, game_prefix1, gp_len1) == 0) {
    strcpy(s, message + gp_len1);
  } else if (strncmp(message, game_prefix2, gp_len2) == 0) {
    strcpy(s, message + gp_len2);
  } else {
    strcpy(s, message);
  }

  nspc = min_msg_len - strlen(s);
  if (nspc > 0) {
    strncat(s, "                                                  ", nspc);
  }

  messages[messages_total].tile = ptile;
  messages[messages_total].event = event;
  messages[messages_total].descr = s;
  messages[messages_total].location_ok = (ptile != NULL);
  messages[messages_total].visited = FALSE;
  messages_total++;

  /*
   * Update the city_ok fields of all messages since the city may have
   * changed owner.
   */
  for (i = 0; i < messages_total; i++) {
    if (messages[i].location_ok) {
      city_t *pcity = map_get_city(messages[i].tile);

      messages[i].city_ok = (pcity && city_owner(pcity) == get_player_ptr());
    } else {
      messages[i].city_ok = FALSE;
    }
  }

  update_meswin_dialog();
}

/**************************************************************************
 Returns the pointer to a message.
**************************************************************************/
struct message_s *get_message(int message_index)
{
  assert(message_index >= 0 && message_index < messages_total);
  return &messages[message_index];
}

/**************************************************************************
 Returns the number of message in the window.
**************************************************************************/
int get_num_messages(void)
{
  return messages_total;
}

/**************************************************************************
 Sets the visited-state of a message
**************************************************************************/
void set_message_visited_state(int message_index, bool state)
{
  messages[message_index].visited = state;
}

/**************************************************************************
 Called from messagewin.c if the user clicks on the popup-city button.
**************************************************************************/
void meswin_popup_city(int message_index)
{
  /* Sometimes the messages might be cleared in between the time
   * that the user clicks a message and the callback is called.
   * In that case, just ignore the click. */
  if (!(0 <= message_index && message_index < messages_total)) {
    return;
  }

  if (messages[message_index].city_ok) {
    tile_t *ptile = messages[message_index].tile;
    city_t *pcity = map_get_city(ptile);

    if (center_when_popup_city) {
      center_tile_mapcanvas(ptile);
    }

    if (pcity && city_owner(pcity) == get_player_ptr()) {
      /* If the event was the city being destroyed, pcity will be NULL
       * and we'd better not try to pop it up.  It's also possible that
       * events will happen on enemy cities; we generally don't want to pop
       * those dialogs up either (although it's hard to be certain).
       *
       * In both cases, it would be better if the popup button weren't
       * highlighted at all - this is left up to the GUI. */
      popup_city_dialog(pcity, FALSE);
    }
  }
}

/**************************************************************************
 Called from messagewin.c if the user clicks on the goto button.
**************************************************************************/
void meswin_goto(int message_index)
{
  if (!(0 <= message_index && message_index < messages_total)) {
    return;
  }

  if (messages[message_index].location_ok) {
    center_tile_mapcanvas(messages[message_index].tile);
  }
}

/**************************************************************************
 Called from messagewin.c if the user double clicks on a message.
**************************************************************************/
void meswin_double_click(int message_index)
{
  if (!(0 <= message_index && message_index < messages_total)) {
    return;
  }

  if (messages[message_index].city_ok
      && is_city_event(messages[message_index].event)) {
    meswin_popup_city(message_index);
  } else if (messages[message_index].location_ok) {
    meswin_goto(message_index);
  }
}
