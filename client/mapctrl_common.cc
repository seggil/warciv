/**********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Poject
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
#include <stdlib.h>             /* qsort */

#include "combat.hh"
#include "wc_intl.hh"
#include "log.hh"
#include "support.hh"

#include "agents/agents.hh"
#include "chatline_common.hh"
#include "include/cityrep_g.hh"
#include "civclient.hh"
#include "climisc.hh"
#include "clinet.hh"
#include "agents/cma_core.hh"
#include "control.hh"
#include "wc_intl.hh"
#include "goto.hh"
#include "include/mapctrl_g.hh"
#include "include/mapview_g.hh"
#include "include/menu_g.hh"
#include "multiselect.hh"
#include "options.hh"
#include "tilespec.hh"

#include "mapctrl_common.hh"

/* Selection Rectangle */
static int rect_anchor_x, rect_anchor_y;  /* canvas coordinates for anchor */
static tile_t *rec_canvas_center_tile;
static tile_t *rect_tile;
static int rect_corner_x, rect_corner_y;  /* corner to iterate from */
static int rect_w, rect_h;                /* width, heigth in pixels */

bool rbutton_down = FALSE;
/* 0 false; 1 see it; 2 no visual, drag RMB on only one tile */
int rectangle_selection_state = 0;
tile_t *dist_first_tile = NULL;
tile_t *dist_last_tile = NULL;

/* This changes the behaviour of left mouse
   button in Area Selection mode. */
bool tiles_hilited_cities = FALSE;

/* The mapcanvas clipboard */
static int clipboard = -1;
static bool clipboard_is_unit;

/* Goto with drag and drop. */
bool keyboardless_goto_button_down = FALSE;
bool keyboardless_goto_active = FALSE;
tile_t *keyboardless_goto_start_tile;

/* Update the workers for a city on the map, when the update is received */
city_t *city_workers_display = NULL;

static bool turn_done_state;
static bool is_turn_done_state_valid = FALSE;

/*************************************************************************/

static void clipboard_send_production_packet(city_t *pcity);
static void define_tiles_within_rectangle(void);

/**************************************************************************
 Called when Right Mouse Button is depressed. Record the canvas
 coordinates of the center of the tile, which may be unreal. This
 anchor is not the drawing start point, but is used to calculate
 width, height. Also record the current mapview centering.
**************************************************************************/
void anchor_selection_rectangle(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  tile_to_canvas_pos(&rect_anchor_x, &rect_anchor_y, ptile);
  rect_anchor_x += NORMAL_TILE_WIDTH / 2;
  rect_anchor_y += NORMAL_TILE_HEIGHT / 2;
  /* FIXME: This may be off-by-one. */
  rec_canvas_center_tile = get_center_tile_mapcanvas();
  rect_tile = ptile;
  rect_w = rect_h = 0;
  //printf("%s canvas_x=%d canvas_y=%d\n",__FUNCTION__, canvas_x, canvas_y);
}

/**************************************************************************
 Iterate over the pixel boundaries of the rectangle and pick the tiles
 whose center falls within. Axis pixel incrementation is half tile size to
 accomodate tilesets with varying tile shapes and proportions of X/Y.

 These operations are performed on the tiles:
 -  Make tiles that contain owned cities hilited
    on the map and hilited in the City List Window.

 Later, I'll want to add unit hiliting for mass orders.       -ali
**************************************************************************/
static void define_tiles_within_rectangle(void)
{
  const int W = NORMAL_TILE_WIDTH,   half_W = W / 2;
  const int H = NORMAL_TILE_HEIGHT,  half_H = H / 2;
  const int segments_x = abs(rect_w / half_W);
  const int segments_y = abs(rect_h / half_H);

  /* Iteration direction */
  const int inc_x = (rect_w >= 0 ? half_W : -half_W);
  const int inc_y = (rect_h >= 0 ? half_H : -half_H);

  int x, y, x2, y2, xx, yy;

  bool first = TRUE;
  y = rect_corner_y;
  for (yy = 0; yy <= segments_y; yy++, y += inc_y) {
    x = rect_corner_x;
    for (xx = 0; xx <= segments_x; xx++, x += inc_x) {
      tile_t *ptile;

      /*  For diamond shaped tiles, every other row is indented.
       */
      if ((yy % 2 ^ xx % 2) != 0) {
        continue;
      }

      ptile = canvas_pos_to_tile(x, y);
      if (!ptile) {
        continue;
      }

      /*  "Half-tile" indentation must match, or we'll process
       *  some tiles twice in the case of rectangular shape tiles.
       */
      tile_to_canvas_pos(&x2, &y2, ptile);

      if ((yy % 2) != 0 && ((rect_corner_x % W) ^ abs(x2 % W)) != 0) {
        continue;
      }

      /* Select units */
      if (multi_select_map_selection) {
        unit_list_iterate(ptile->units, punit) {
          if (punit->owner == get_player_idx()) {
            if (first) {
              multi_select_clear(0);
              set_unit_focus(punit);
              first = FALSE;
            } else {
              multi_select_add_unit(punit);
            }
          }
        } unit_list_iterate_end;
        update_menus();
        update_unit_info_label(get_unit_in_focus());
      }

      /*  Tile passed all tests; process it.
       */
      if (ptile->city && ptile->city->common.owner == get_player_idx()) {
        ptile->u.client.hilite = HILITE_CITY;
        tiles_hilited_cities = TRUE;
        update_miscellaneous_menu();
      }
    }
  }

  /* Hilite in City List Window */
  if (tiles_hilited_cities) {
    hilite_cities_from_canvas();      /* cityrep.c */
  }
}

/**************************************************************************
 Called when mouse pointer moves and rectangle is active.
**************************************************************************/
void update_selection_rectangle(int canvas_x, int canvas_y)
{
  const int W = NORMAL_TILE_WIDTH;
  const int H = NORMAL_TILE_HEIGHT;
  const int half_W = W / 2;
  const int half_H = H / 2;
  //static tile_t *rect_tile = NULL;
  int diff_x;
  int diff_y;
  tile_t *center_tile;
  tile_t *ptile;

  //printf("%s ", __FUNCTION__);
  ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  /*  Did mouse pointer move beyond the current tile's
   *  boundaries? Avoid macros; tile may be unreal!
   */
  if (rect_w == 0 && rect_h == 0 && ptile == rect_tile) {
    rectangle_selection_state = 2;
    //printf("2\n");
    rect_tile = ptile;
    return;
  }
  rect_tile = ptile;

  /* Clear previous rectangle. */
  dirty_all();
  flush_dirty();

  /*  Fix canvas coords to the center of the tile.
   */
  tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
  canvas_x += half_W;
  canvas_y += half_H;

  rect_w = rect_anchor_x - canvas_x;  /* width */
  rect_h = rect_anchor_y - canvas_y;  /* height */

  /* FIXME: This may be off-by-one. */
  center_tile = get_center_tile_mapcanvas();
  map_distance_vector(&diff_x, &diff_y, rec_canvas_center_tile, center_tile);

  /*  Adjust width, height if mapview has recentered.*/
  if (diff_x != 0 || diff_y != 0) {
    printf("was recentered ");
    if (is_isometric) {
      rect_w += (diff_x - diff_y) * half_W;
      rect_h += (diff_x + diff_y) * half_H;

      /* Iso wrapping */
      if (abs(rect_w) > map.info.xsize * half_W / 2) {
        int wx = map.info.xsize * half_W;
        int wy = map.info.ysize * half_H; /* gilles: that was xsize */
        if (rect_w > 0 ) {
          rect_w -= wx;
          rect_h -= wy;
        }
        else {
          rect_w += wx;
          rect_h += wy;
        }
      }
    } else { /* not isometric*/
      rect_w += diff_x * W;
      rect_h += diff_y * H;

      /* X wrapping */
      if (abs(rect_w) > map.info.xsize * half_W) {
        int wx = map.info.xsize * W;
        if (rect_w >= 0)
          rect_w -= wx;
        else
          rect_w += wx;
      }
      /* Y wrapping */
      if (abs(rect_h) > map.info.ysize * half_H) {
        int wy = map.info.ysize * H;
        if (rect_w >= 0)
          rect_h -= wy;
        else
          rect_h += wy;
      }
    }
  }

  /* It is currently drawn only to the screen, not backing store */
  if (rect_w == 0 && rect_h == 0) {
    rectangle_selection_state = 2;
    //printf("2\n");
  } else {
    rectangle_selection_state = 1;
    //printf("1\n");
  }
  draw_selection_rectangle(canvas_x, canvas_y, rect_w, rect_h);
  rect_corner_x = canvas_x;
  rect_corner_y = canvas_y;
}

/**************************************************************************
  Redraws the selection rectangle after a map flush.
**************************************************************************/
void redraw_selection_rectangle(void)
{
#if 0
  if (rectangle_selection_state) {
    draw_selection_rectangle(rect_corner_x, rect_corner_y, rect_w, rec_h);
  } else
#endif
  if (dist_first_tile) {
    /* Redraw the distance tool */
    redraw_distance_tool();
  }
}

/**************************************************************************
  Update and redraw the distance tool.
**************************************************************************/
void update_distance_tool(int canvas_x, int canvas_y)
{
  if (dist_first_tile) {
    tile_t *ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

    if (ptile != dist_last_tile) {
      dist_last_tile = ptile;
      dirty_all();
      flush_dirty();
      redraw_distance_tool();
    }
  } else {
    /* It's the first tile */
    dist_first_tile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);
    dist_last_tile = dist_first_tile;
    redraw_distance_tool();
  }
}

/**************************************************************************
  Cancel the distance tool.
**************************************************************************/
void cancel_distance_tool(void)
{
  if (!dist_first_tile || !dist_last_tile) {
    return;
  }

  dist_first_tile = NULL;
  dist_last_tile = NULL;

  update_map_canvas_visible(MAP_UPDATE_NORMAL);
}

/**************************************************************************
...
**************************************************************************/
bool is_city_hilited(city_t *pcity)
{
  return pcity != NULL && pcity->common.tile->u.client.hilite == HILITE_CITY;
}

/**************************************************************************
 Remove hiliting from all tiles, but not from rows in the City List window.
**************************************************************************/
void cancel_tile_hiliting(void)
{
  if (tiles_hilited_cities)  {
    tiles_hilited_cities = FALSE;

    whole_map_iterate(ptile) {
      ptile->u.client.hilite = HILITE_NONE;
    } whole_map_iterate_end;

    update_map_canvas_visible(MAP_UPDATE_NORMAL);
    update_miscellaneous_menu();
  }
}

/**************************************************************************
 Action depends on whether the mouse pointer moved more than
 a tile between press/drag and release.
**************************************************************************/
void release_right_button(int canvas_x, int canvas_y)
{
  if (rectangle_selection_state == 1) {
    define_tiles_within_rectangle();
    update_map_canvas_visible(MAP_UPDATE_NORMAL);
    rectangle_selection_state = 0;
  } else if (rectangle_selection_state == 2) {
    cancel_tile_hiliting();
    if (rect_tile->city && rect_tile->city->common.owner == get_player_idx()) {
      rect_tile->u.client.hilite = HILITE_CITY;
      tiles_hilited_cities = TRUE;
      update_map_canvas_visible(MAP_UPDATE_NORMAL);
      toggle_city_hilite(rect_tile->city, TRUE);
    }
    rectangle_selection_state = 0;
  } else {
    recenter_button_pressed(canvas_x, canvas_y);
  }
  rbutton_down = false;
}

/**************************************************************************
 Left Mouse Button in Area Selection mode.
**************************************************************************/
void toggle_tile_hilite(tile_t *ptile)
{
  city_t *pcity = ptile->city;

  if (ptile->u.client.hilite == HILITE_CITY) {
    ptile->u.client.hilite = HILITE_NONE;
    if (pcity) {
      toggle_city_hilite(pcity, FALSE); /* cityrep.c */
    }
  }
  else if (pcity && pcity->common.owner == get_player_idx()) {
    ptile->u.client.hilite = HILITE_CITY;
    tiles_hilited_cities = TRUE;
    toggle_city_hilite(pcity, TRUE); /* in city tab */
  }
  else  {
    return;
  }

  refresh_tile_mapcanvas(ptile, MAP_UPDATE_NORMAL);
}

/**************************************************************************
  The user pressed the overlay-city button (t) while the mouse was at the
  given canvas position.
**************************************************************************/
void key_city_overlay(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    unit_t *punit;
    city_t *pcity = find_city_or_settler_near_tile(ptile, &punit);

    if (pcity) {
      toggle_city_color(pcity);
    } else if (punit) {
      toggle_unit_color(punit);
    }
  }
}

/**************************************************************************
  The user pressed the overlay-city button (t) twice.
**************************************************************************/
void key_cities_overlay(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    city_t *pcity = find_city_or_settler_near_tile(ptile, NULL);
    int num = 0;
    bool toggle_on;

    if (client_is_global_observer()) {
      cities_iterate(pcity) {
        if (pcity->u.client.colored) {
          if (++num >= 2) {
            /* No need more */
            break;
          }
        }
      } cities_iterate_end;
    } else {
      city_list_iterate(get_player_ptr()->cities, pcity) {
        if (pcity->u.client.colored) {
          if (++num >= 2) {
            /* No need more */
            break;
          }
        }
      } city_list_iterate_end;
    }

    toggle_on = num == 0 || (num == 1 && pcity && pcity->u.client.colored);

    if (client_is_global_observer()) {
      cities_iterate(pcity) {
        if ((toggle_on ? !pcity->u.client.colored : pcity->u.client.colored)) {
          toggle_city_color(pcity);
        }
      } cities_iterate_end;
    } else {
      city_list_iterate(get_player_ptr()->cities, pcity) {
        if ((toggle_on ? !pcity->u.client.colored : pcity->u.client.colored)) {
          toggle_city_color(pcity);
        }
      } city_list_iterate_end;
    }
  }
}

/**************************************************************************
 Shift-Left-Click on owned city or any visible unit to copy.
**************************************************************************/
void clipboard_copy_production(tile_t *ptile)
{
  char msg[MAX_LEN_MSG];
  city_t *pcity = ptile->city;

  if (!client_is_player()) {
    return;
  }

  if (pcity) {
    if (pcity->common.owner != get_player_idx())  {
      return;
    }
    clipboard = pcity->common.currently_building;
    clipboard_is_unit = pcity->common.is_building_unit;
  } else {
    unit_t *punit = find_visible_unit(ptile);
    if (!punit) {
      return;
    }
    if (!can_player_build_unit_direct(get_player_ptr(), punit->type))  {
      my_snprintf(msg, sizeof(msg),
      _("Game: You don't know how to build %s!"),
        unit_types[punit->type].name);
      append_output_window(msg);
      return;
    }
    clipboard_is_unit = TRUE;
    clipboard = punit->type;
  }
  upgrade_canvas_clipboard();

  my_snprintf(msg, sizeof(msg), _("Game: Copy %s to clipboard."),
    clipboard_is_unit ? unit_types[clipboard].name :
    get_improvement_name(clipboard));
  append_output_window(msg);
}

/**************************************************************************
 If City tiles are hilited, paste into all those cities.
 Otherwise paste into the one city under the mouse pointer.
**************************************************************************/
void clipboard_paste_production(city_t *pcity)
{
  if (!can_client_issue_orders()) {
    return;
  }
  if (clipboard == -1) {
    append_output_window(
    _("Game: Clipboard is empty."));
    return;
  }
  if (!tiles_hilited_cities) {
    if (pcity && pcity->common.owner == get_player_idx()) {
      clipboard_send_production_packet(pcity);
    }
    return;
  }
  else {
    connection_do_buffer(&aconnection);
    city_list_iterate(get_player_ptr()->cities, pcity) {
      if (is_city_hilited(pcity)) {
        clipboard_send_production_packet(pcity);
      }
    } city_list_iterate_end;
    connection_do_unbuffer(&aconnection);
  }
}

/**************************************************************************
...
**************************************************************************/
static void clipboard_send_production_packet(city_t *pcity)
{
  city_cid mycid = city_cid_encode(clipboard_is_unit, clipboard);

  if (mycid == city_cid_encode_from_city(pcity)
      || !city_can_build_impr_or_unit(pcity, mycid)) {
    return;
  }

  dsend_packet_city_change(&aconnection, pcity->common.id, clipboard,
                           clipboard_is_unit);
}

/**************************************************************************
 A newer technology may be available for units.
 Also called from packhand.c.
**************************************************************************/
void upgrade_canvas_clipboard(void)
{
  if (clipboard_is_unit)  {
    int u = can_upgrade_unittype(get_player_ptr(), clipboard);
    if (u != -1)  {
      clipboard = u;
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void release_goto_button(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (keyboardless_goto_active && cursor_state == CURSOR_STATE_GOTO && ptile) {
    if (multi_select_size(0) == 1) {
      do_unit_goto(ptile);
    } else {
      multi_select_iterate(FALSE, punit) {
        send_goto_unit(punit, ptile);
      } multi_select_iterate_end;
    }
    set_cursor_state(NULL, CURSOR_STATE_NONE, ACTIVITY_LAST);
    update_unit_info_label(get_unit_in_focus());
  }
  keyboardless_goto_active = FALSE;
  keyboardless_goto_button_down = FALSE;
  keyboardless_goto_start_tile = NULL;
}

/**************************************************************************
 The goto hover state is only activated when the mouse pointer moves
 beyond the tile where the button was depressed, to avoid mouse typos.
**************************************************************************/
void maybe_activate_keyboardless_goto(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (ptile && get_unit_in_focus()
      && !same_pos(keyboardless_goto_start_tile, ptile)
      && can_client_issue_orders()) {
    keyboardless_goto_active = TRUE;
    request_unit_goto();
  }
}

/**************************************************************************
 Return TRUE iff the turn done button is enabled.
**************************************************************************/
bool get_turn_done_button_state()
{
  if (!is_turn_done_state_valid) {
    update_turn_done_button_state();
  }
  assert(is_turn_done_state_valid);

  return turn_done_state;
}

/**************************************************************************
  Scroll the mapview half a screen in the given direction.  This is a GUI
  direction; i.e., DIR8_NORTH is "up" on the mapview.
**************************************************************************/
void scroll_mapview(enum direction8 gui_dir)
{
  int gui_x = mapview_canvas.gui_x0, gui_y = mapview_canvas.gui_y0;

  if (!can_client_change_view()) {
    return;
  }

  gui_x += DIR_DX[gui_dir] * mapview_canvas.width / 2;
  gui_y += DIR_DY[gui_dir] * mapview_canvas.height / 2;
  set_mapview_origin(gui_x, gui_y);
}

/**************************************************************************
  Do some appropriate action when the "main" mouse button (usually
  left-click) is pressed.  For more sophisticated user control use (or
  write) a different xxx_button_pressed function.
**************************************************************************/
void action_button_pressed(int canvas_x, int canvas_y,
                           enum quickselect_type qtype)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    /* FIXME: Some actions here will need to check can_client_issue_orders.
     * But all we can check is the lowest common requirement. */
    do_map_click(ptile, qtype);
  }
}

/**************************************************************************
  Wakeup sentried units on the tile of the specified location.
**************************************************************************/
void wakeup_button_pressed(int canvas_x, int canvas_y)
{
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_issue_orders() && ptile) {
    wakeup_sentried_units(ptile);
  }
}

/**************************************************************************
  Adjust the position of city workers from the mapview.
**************************************************************************/
void adjust_workers_button_pressed(int canvas_x, int canvas_y)
{
  int city_x, city_y;
  enum city_tile_type worker;
  tile_t *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_issue_orders() && ptile) {
    city_t *pcity = find_city_near_tile(ptile);

    if (pcity && !cma_is_city_under_agent(pcity, NULL)) {
      if (!map_to_city_map(&city_x, &city_y, pcity, ptile)) {
        assert(0);
      }

      worker = get_worker_city(pcity, city_x, city_y);
      if (worker == C_TILE_WORKER) {
        dsend_packet_city_make_specialist(&aconnection, pcity->common.id,
                                          city_x, city_y);
      } else if (worker == C_TILE_EMPTY) {
        dsend_packet_city_make_worker(&aconnection, pcity->common.id,
                                      city_x, city_y);
      } else {
        /* If worker == C_TILE_UNAVAILABLE then we can't use this tile.  No
         * packet is sent and city_workers_display is not updated. */
        return;
      }

      /* When the city info packet is received, update the workers on the
       * map.  This is a bad hack used to selectively update the mapview
       * when we receive the corresponding city packet. */
      city_workers_display = pcity;
    }
  }
}

/**************************************************************************
  Recenter the map on the canvas location, on user request.  Usually this
  is done with a right-click.
**************************************************************************/
void recenter_button_pressed(int canvas_x, int canvas_y)
{
  /* We use the "nearest" tile here so off-map clicks will still work. */
  tile_t *ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    center_tile_mapcanvas(ptile);
  }
}

/**************************************************************************
 Update the turn done button state.
**************************************************************************/
void update_turn_done_button_state()
{
  bool new_state;

  if (!is_turn_done_state_valid) {
    turn_done_state = FALSE;
    is_turn_done_state_valid = TRUE;
    set_turn_done_button_state(turn_done_state);
    freelog(LOG_DEBUG, "setting turn done button state init %d",
            turn_done_state);
  }

  new_state = (can_client_issue_orders()
               && !get_player_ptr()->turn_done && !agents_busy()
               && !turn_done_sent);
  if (new_state == turn_done_state) {
    return;
  }

  freelog(LOG_DEBUG, "setting turn done button state from %d to %d",
          turn_done_state, new_state);
  turn_done_state = new_state;

  set_turn_done_button_state(turn_done_state);

  if (turn_done_state) {
    if (waiting_for_end_turn
        || (get_player_ptr()->ai.control && !ai_manual_turn_done)) {
      send_turn_done();
    } else {
      update_turn_done_button(TRUE);
    }
  }
}

/**************************************************************************
  Update the goto/patrol line to the given map canvas location.
**************************************************************************/
void update_line(int canvas_x, int canvas_y)
{
  if ((cursor_state == CURSOR_STATE_GOTO
       || cursor_state == CURSOR_STATE_PATROL
       || cursor_state == CURSOR_STATE_CONNECT)
      && draw_goto_line)
  {
    tile_t *ptile;
    tile_t *old_tile;

    ptile = canvas_pos_to_tile(canvas_x, canvas_y);
    if (!ptile) {
      return;
    }

    old_tile = get_line_dest();
    if (!same_pos(old_tile, ptile)) {
      draw_line(ptile);
    }
  }
}

/****************************************************************************
  Update the goto/patrol line to the given overview canvas location.
****************************************************************************/
void overview_update_line(int overview_x, int overview_y)
{
  if ((cursor_state == CURSOR_STATE_GOTO
       || cursor_state == CURSOR_STATE_PATROL
       || cursor_state == CURSOR_STATE_CONNECT)
      && draw_goto_line) {
    tile_t *ptile, *old_tile;
    int x, y;

    overview_to_map_pos(&x, &y, overview_x, overview_y);
    ptile = map_pos_to_tile(x, y);

    old_tile = get_line_dest();
    if (!same_pos(ptile, old_tile)) {
      draw_line(ptile);
    }
  }
}

/**************************************************************************
  Find the focus unit's chance of success at attacking/defending the
  given tile.  Return FALSE if the values cannot be determined (e.g., no
  units on the tile).
**************************************************************************/
bool get_chance_to_win(int *att_chance, int *def_chance, tile_t *ptile)
{
  unit_t *my_unit;
  unit_t *defender;
  unit_t *attacker;

  if (!(my_unit = get_unit_in_focus())
      || !(defender = get_defender(my_unit, ptile))
      || !(attacker = get_attacker(my_unit, ptile))) {
    return FALSE;
  }

  /* chance to win when active unit is attacking the selected unit */
  *att_chance = unit_win_chance(my_unit, defender) * 100;

  /* chance to win when selected unit is attacking the active unit */
  *def_chance = (1.0 - unit_win_chance(attacker, my_unit)) * 100;

  return TRUE;
}

/****************************************************************************
  We sort according to the following logic:

  - Transported units should immediately follow their transporter (note that
    transporting may be recursive).
  - Otherwise we sort by ID (which is what the list is originally sorted by).
****************************************************************************/
static int unit_list_compare(const void *a, const void *b)
{
  const unit_t *punit1 = *(unit_t **)a;
  const unit_t *punit2 = *(unit_t **)b;

  if (punit1->transported_by == punit2->transported_by) {
    /* For units with the same transporter or no transporter: sort by id. */
    /* Perhaps we should sort by name instead? */
    return punit1->id - punit2->id;
  } else if (punit1->transported_by == punit2->id) {
    return 1;
  } else if (punit2->transported_by == punit1->id) {
    return -1;
  } else {
    /* If the transporters aren't the same, put in order by the
     * transporters. */
    const unit_t *ptrans1 = find_unit_by_id(punit1->transported_by);
    const unit_t *ptrans2 = find_unit_by_id(punit2->transported_by);

    if (!ptrans1) {
      ptrans1 = punit1;
    }
    if (!ptrans2) {
      ptrans2 = punit2;
    }

    return unit_list_compare(&ptrans1, &ptrans2);
  }
}

/****************************************************************************
  Fill and sort the list of units on the tile.
****************************************************************************/
void fill_tile_unit_list(tile_t *ptile, unit_t **unit_list)
{
  int i = 0;

  /* First populate the unit list. */
  unit_list_iterate(ptile->units, punit) {
    unit_list[i] = punit;
    i++;
  } unit_list_iterate_end;

  /* Then sort it. */
  qsort(unit_list, i, sizeof(*unit_list), unit_list_compare);
}

