
#ifdef HAVE_CONFIG_H
#  include "../config.hh"
#endif

#include <assert.h>
#include <ctype.h>
#include "wc_intl.hh"
#include "log.hh"
#include "game_load.hh"
#include "load_save_aux.hh"
#include "rand.hh"
#include "support.hh"
#include "capability.hh"
#include "city.hh"
#include "government.hh"
#include "map.hh"
#include "idex.hh"

#include "aidata.hh"

#include "cityturn.hh"
#include "citytools.hh"
#include "maphand.hh"
#include "plrhand.hh"
#include "meta.hh"
#include "spacerace.hh"
#include "score.hh"
#include "srv_main.hh"
#include "ruleset.hh"
#include "stdinhand.hh"
#include "tradehand.hh"
#include "unittools.hh"

/*
 * This loops over the entire map to load data. It inputs a line of data
 * using the macro SECFILE_LOOKUP_LINE and then loops using the macro
 * SET_XY_CHAR to load each char into the map at (map_x, map_y).  Internal
 * variables ch, map_x, map_y, nat_x, and nat_y are allocated within the
 * macro but definable by the caller.
 *
 * Parameters:
 *   ch: a variable to hold a char (data for a single position)
 *   map_x, map_y: variables for internal map coordinates
 *   nat_x, nat_y: variables for output/native coordinates
 *   SET_XY_CHAR: macro to load the map character at each (map_x, map_y)
 *   SECFILE_LOOKUP_LINE: macro to input the nat_y line for processing
 *
 * Note: some (but not all) of the code this is replacing used to
 * skip over lines that did not exist.  This allowed for
 * backward-compatibility.  We could add another parameter that
 * specified whether it was OK to skip the data, but there's not
 * really much advantage to exiting early in this case.  Instead,
 * we let any map data type to be empty, and just print an
 * informative warning message about it.
 */
#define LOAD_MAP_DATA(ch, nat_y, ptile,                             \
                      SECFILE_LOOKUP_LINE, SET_XY_CHAR)             \
{                                                                   \
  int _nat_x, _nat_y;                                               \
                                                                    \
  bool _warning_printed = FALSE;                                    \
  for (_nat_y = 0; _nat_y < map.info.ysize; _nat_y++) {             \
    const int nat_y = _nat_y;                                       \
    const char *_line = (SECFILE_LOOKUP_LINE);                      \
                                                                    \
    if (!_line || strlen(_line) != map.info.xsize) {                \
      if (!_warning_printed) {                                      \
        /* TRANS: Error message. */                                 \
        freelog(LOG_ERROR, _("The save file contains incomplete "   \
                "map data.  This can happen with old saved "        \
                "games, or it may indicate an invalid saved "       \
                "game file.  Proceed at your own risk."));          \
        if(!_line) {                                                \
          /* TRANS: Error message. */                               \
          freelog(LOG_ERROR, _("Reason: line not found"));          \
        } else {                                                    \
          /* TRANS: Error message. */                               \
          freelog(LOG_ERROR, _("Reason: line too short "            \
                  "(expected %d got %lu"), map.info.xsize,          \
                  (unsigned long) strlen(_line));                   \
        }                                                           \
        /* Do not translate.. */                                    \
        freelog(LOG_ERROR, "secfile_lookup_line='%s'",              \
                #SECFILE_LOOKUP_LINE);                              \
        _warning_printed = TRUE;                                    \
      }                                                             \
      continue;                                                     \
    }                                                               \
    for (_nat_x = 0; _nat_x < map.info.xsize; _nat_x++) {           \
      const char ch = _line[_nat_x];                                \
      tile_t *ptile = native_pos_to_tile(_nat_x, _nat_y);           \
                                                                    \
      (SET_XY_CHAR);                                                \
    }                                                               \
  }                                                                 \
}

const char hex_chars[] = "0123456789abcdef";

/***************************************************************
This returns a binary integer value of the ascii hex char, offset by
the given number of half-bytes. See bin2ascii_hex().
  example: ascii_hex2bin('a', 2) == 0xa00
This is only used in loading games, and it requires some error
checking so it's done as a function.
***************************************************************/
static int ascii_hex2bin(char ch, int halfbyte)
{
  char const *pch;

  if (ch == ' ') {
    /*
     * Sane value. It is unknow if there are savegames out there which
     * need this fix. Savegame.c doesn't write such savegames
     * (anymore) since the inclusion into CVS (2000-08-25).
     */
    return 0;
  }

  pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    die("Unknown hex value: '%c' %d", ch, ch);
  }
  return (pch - hex_chars) << (halfbyte * 4);
}

/****************************************************************************
  Dereferences the terrain character.  See tile_types[].identifier
    example: char2terrain('a') => T_ARCTIC
****************************************************************************/
static Terrain_type_id char2terrain(char ch)
{
  if (ch == UNKNOWN_TERRAIN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  freelog(LOG_DEBUG, _("looking for Terrain identifier '%c' "),ch);
  terrain_type_iterate(id) {
    if (get_tile_type(id)->identifier == ch)
    {
      return id;
    }
    freelog(LOG_DEBUG, _("Terrain identifier '%c' and tiletype '%c'"),ch , get_tile_type(id)->identifier);
  } terrain_type_iterate_end;

  /* TRANS: message for an obscure savegame error. */
  freelog(LOG_FATAL, _("Unknown terrain identifier '%c' in savegame."), ch);
  exit(EXIT_FAILURE);
}

/****************************************************************************
  Returns an order for a character identifier.  See also order2char.
****************************************************************************/
static enum unit_orders char2order(char order)
{
  switch (order) {
  case 'm':
  case 'M':
    return ORDER_MOVE;
  case 'w':
  case 'W':
    return ORDER_FULL_MP;
  case 'a':
  case 'A':
    return ORDER_ACTIVITY;
  }

  /* This can happen if the savegame is invalid. */
  return ORDER_LAST;
}

/****************************************************************************
  Returns a direction for a character identifier.  See also dir2char.
****************************************************************************/
static enum direction8 char2dir(char dir)
{
  /* Numberpad values for the directions. */
  switch (dir) {
  case '1':
    return DIR8_SOUTHWEST;
  case '2':
    return DIR8_SOUTH;
  case '3':
    return DIR8_SOUTHEAST;
  case '4':
    return DIR8_WEST;
  case '6':
    return DIR8_EAST;
  case '7':
    return DIR8_NORTHWEST;
  case '8':
    return DIR8_NORTH;
  case '9':
    return DIR8_NORTHEAST;
  }

  /* This can happen if the savegame is invalid. */
  return (enum direction8)DIR8_LAST;
}

/****************************************************************************
  Returns a character identifier for an activity.  See also char2activity.
  used letter "aefgilmoprstuwxy?"
****************************************************************************/
char activity2char(enum unit_activity activity)
{
  switch (activity) {
  case ACTIVITY_IDLE:
    return 'w';
  case ACTIVITY_POLLUTION:
    return 'p';
  case ACTIVITY_ROAD:
    return 'r';
  case ACTIVITY_MINE:
    return 'm';
  case ACTIVITY_IRRIGATE:
    return 'i';
  case ACTIVITY_FORTIFIED:
    return 'f';
  case ACTIVITY_FORTRESS:
    return 't';
  case ACTIVITY_SENTRY:
    return 's';
  case ACTIVITY_RAILROAD:
    return 'l';
  case ACTIVITY_PILLAGE:
    return 'e';
  case ACTIVITY_GOTO:
    return 'g';
  case ACTIVITY_EXPLORE:
    return 'x';
  case ACTIVITY_TRANSFORM:
    return 'o';
  case ACTIVITY_AIRBASE:
    return 'a';
  case ACTIVITY_FORTIFYING:
    return 'y';
  case ACTIVITY_FALLOUT:
    return 'u';
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_PATROL_UNUSED:
    return '?';
  case ACTIVITY_LAST:
    break;
  }

  assert(0);
  return '?';
}

/****************************************************************************
  Returns an activity for a character identifier.  See also activity2char.
****************************************************************************/
static enum unit_activity char2activity(char activity)
{
  int /* enum unit_activity */ a;

  for (a = 0; a < ACTIVITY_LAST; a++) {
    char achar = activity2char((enum unit_activity)a);

    if (activity == achar || activity == toupper(achar)) {
      return (enum unit_activity)a;
    }
  }

  /* This can happen if the savegame is invalid. */
  return ACTIVITY_LAST;
}

/*
 * Previously (with 1.14.1 and earlier) units had their type saved by ID.
 * This meant any time a unit was added (unless it was added at the end)
 * savegame compatibility would be broken.  Sometime after 1.14.1 this
 * method was changed so the type is saved by name.  However to preserve
 * backwards compatibility we have here a list of unit names from before
 * the change was made.  When loading an old savegame (one that doesn't
 * have the type string) we need to lookup the type into this array
 * to get the "proper" type string.  And when saving a new savegame we
 * insert the "old" type index into the array so that old servers can
 * load the savegame.
 *
 * Note that this list includes the AWACS, which was not in 1.14.1.
 */

/* old (~1.14.1) unit order in default/civ2/history ruleset */
const char* old_default_unit_types[] = {
  "Settlers",   "Engineers",    "Warriors",     "Phalanx",
  "Archers",    "Legion",       "Pikemen",      "Musketeers",
  "Fanatics",   "Partisan",     "Alpine Troops","Riflemen",
  "Marines",    "Paratroopers", "Mech. Inf.",   "Horsemen",
  "Chariot",    "Elephants",    "Crusaders",    "Knights",
  "Dragoons",   "Cavalry",      "Armor",        "Catapult",
  "Cannon",     "Artillery",    "Howitzer",     "Fighter",
  "Bomber",     "Helicopter",   "Stealth Fighter", "Stealth Bomber",
  "Trireme",    "Caravel",      "Galleon",      "Frigate",
  "Ironclad",   "Destroyer",    "Cruiser",      "AEGIS Cruiser",
  "Battleship", "Submarine",    "Carrier",      "Transport",
  "Cruise Missile", "Nuclear",  "Diplomat",     "Spy",
  "Caravan",    "Freight",      "Explorer",     "Barbarian Leader",
  "AWACS"
};
const int old_default_unit_types_size = ARRAY_SIZE(old_default_unit_types);

/* old (1.14.1) improvement order in default ruleset */
const char* old_impr_types[] =
{
  "Airport",            "Aqueduct",             "Bank",
  "Barracks",           "Barracks II",          "Barracks III",
  "Cathedral",          "City Walls",           "Coastal Defense",
  "Colosseum",          "Courthouse",           "Factory",
  "Granary",            "Harbour",              "Hydro Plant",
  "Library",            "Marketplace",          "Mass Transit",
  "Mfg. Plant",         "Nuclear Plant",        "Offshore Platform",
  "Palace",             "Police Station",       "Port Facility",
  "Power Plant",        "Recycling Center",     "Research Lab",
  "SAM Battery",        "SDI Defense",          "Sewer System",
  "Solar Plant",        "Space Component",      "Space Module",
  "Space Structural",   "Stock Exchange",       "Super Highways",
  "Supermarket",        "Temple",               "University",
  "Apollo Program",     "A.Smith's Trading Co.","Colossus",
  "Copernicus' Observatory", "Cure For Cancer", "Darwin's Voyage",
  "Eiffel Tower",       "Great Library",        "Great Wall",
  "Hanging Gardens",    "Hoover Dam",           "Isaac Newton's College",
  "J.S. Bach's Cathedral","King Richard's Crusade", "Leonardo's Workshop",
  "Lighthouse",         "Magellan's Expedition","Manhattan Project",
  "Marco Polo's Embassy","Michelangelo's Chapel","Oracle",
  "Pyramids",           "SETI Program",         "Shakespeare's Theatre",
  "Statue of Liberty",  "Sun Tzu's War Academy","United Nations",
  "Women's Suffrage",   "Coinage"
};
const int old_impr_types_size = ARRAY_SIZE(old_impr_types);

/* old (~1.14.1) techs order in civ2/default ruleset.
 *
 * Note that Theology is called Religion in civ1 ruleset; this is handled
 * as a special case in the code.
 *
 * Nowadays we save A_FUTURE as "A_FUTURE", A_NONE as "A_NONE".
 * A_UNSET as "A_UNSET" - they used to be saved as 198, 0 or -1, 0.
 */
const char* old_default_techs[] =
{
  "A_NONE",
  "Advanced Flight",    "Alphabet",             "Amphibious Warfare",
  "Astronomy",          "Atomic Theory",        "Automobile",
  "Banking",            "Bridge Building",      "Bronze Working",
  "Ceremonial Burial",  "Chemistry",            "Chivalry",
  "Code of Laws",       "Combined Arms",        "Combustion",
  "Communism",          "Computers",            "Conscription",
  "Construction",       "Currency",             "Democracy",
  "Economics",          "Electricity",          "Electronics",
  "Engineering",        "Environmentalism",     "Espionage",
  "Explosives",         "Feudalism",            "Flight",
  "Fundamentalism",     "Fusion Power",         "Genetic Engineering",
  "Guerilla Warfare",   "Gunpowder",            "Horseback Riding",
  "Industrialization",  "Invention",            "Iron Working",
  "Labor Union",        "Laser",                "Leadership",
  "Literacy",           "Machine Tools",        "Magnetism",
  "Map Making",         "Masonry",              "Mass Production",
  "Mathematics",        "Medicine",             "Metallurgy",
  "Miniaturization",    "Mobile Warfare",       "Monarchy",
  "Monotheism",         "Mysticism",            "Navigation",
  "Nuclear Fission",    "Nuclear Power",        "Philosophy",
  "Physics",            "Plastics",             "Polytheism",
  "Pottery",            "Radio",                "Railroad",
  "Recycling",          "Refining",             "Refrigeration",
  "Robotics",           "Rocketry",             "Sanitation",
  "Seafaring",          "Space Flight",         "Stealth",
  "Steam Engine",       "Steel",                "Superconductors",
  "Tactics",            "The Corporation",      "The Republic",
  "The Wheel",          "Theology",             "Theory of Gravity",
  "Trade",              "University",           "Warrior Code",
  "Writing"
};
const int old_default_techs_size = ARRAY_SIZE(old_default_techs);

/* old (~1.14.1) unit order in civ1 ruleset */
const char* old_civ1_unit_types[] = {
  "Settlers",   "Engineers",    "Militia",      "Phalanx",
  "Archers",    "Legion",       "Pikemen",      "Musketeers",
  "Fanatics",   "Partisan",     "Alpine Troops","Riflemen",
  "Marines",    "Paratroopers", "Mech. Inf.",   "Cavalry",
  "Chariot",    "Elephants",    "Crusaders",    "Knights",
  "Dragoons",   "Civ2-Cavalry", "Armor",        "Catapult",
  "Cannon",     "Civ2-Artillery","Artillery",   "Fighter",
  "Bomber",     "Helicopter",   "Stealth Fighter", "Stealth Bomber",
  "Trireme",    "Sail",         "Galleon",      "Frigate",
  "Ironclad",   "Destroyer",    "Cruiser",      "AEGIS Cruiser",
  "Battleship", "Submarine",    "Carrier",      "Transport",
  "Cruise Missile", "Nuclear",  "Diplomat",     "Spy",
  "Caravan",    "Freight",      "Explorer",     "Barbarian Leader"
};
const int old_civ1_unit_types_size = ARRAY_SIZE(old_civ1_unit_types);

/* old (~1.14.1) government order in default, civ1, and history rulesets */
const char* old_default_governments[] =
{
  "Anarchy", "Despotism", "Monarchy", "Communism", "Republic", "Democracy"
};

/* old (~1.14.1) government order in the civ2 ruleset */
const char* old_civ2_governments[] =
{
  "Anarchy", "Despotism", "Monarchy", "Communism", "Fundamentalism",
  "Republic", "Democracy"
};
const int old_civ2_governments_size = ARRAY_SIZE(old_civ2_governments);

#ifndef NDEBUG
/***************************************************************
Unquote a string. The unquoted data is written into dest. If the
unqoted data will be largern than dest_length the function aborts. It
returns the actual length of the unquoted block.
***************************************************************/
static int unquote_block(const char *const quoted_, void *dest,
                         int dest_length)
{
  int i, length, tmp;
  char *endptr;
  const char *quoted = quoted_;

#ifdef NDEBUG
  sscanf(quoted, "%d", &length);
#else
  assert(1 == sscanf(quoted, "%d", &length));
#endif

  assert(length <= dest_length);
  quoted = strchr(quoted, ':');
  assert(quoted != NULL);
  quoted++;

  for (i = 0; i < length; i++) {
    tmp = strtol(quoted, &endptr, 16);
    assert((endptr - quoted) == 2);
    assert(*endptr == ' ');
    assert((tmp & 0xff) == tmp);
    ((unsigned char *) dest)[i] = tmp;
    quoted += 3;
  }
  return length;
}
#endif

/****************************************************************************
  Convert an old-style unit type id into a unit type name.
****************************************************************************/
static const char* old_unit_type_name(int id)
{
  /* before 1.15.0 unit types used to be saved by id */
  if (id < 0) {
    freelog(LOG_ERROR, _("Wrong unit type id value (%d)"), id);
    exit(EXIT_FAILURE);
  }
  /* Different rulesets had different unit names. */
  if (strcmp(game.server.rulesetdir, "civ1") == 0) {
    if (id >= ARRAY_SIZE(old_civ1_unit_types)) {
      freelog(LOG_ERROR, _("Wrong unit type id value (%d)"), id);
      exit(EXIT_FAILURE);
    }
    return old_civ1_unit_types[id];
  } else {
    if (id >= ARRAY_SIZE(old_default_unit_types)) {
      freelog(LOG_ERROR, _("Wrong unit type id value (%d)"), id);
      exit(EXIT_FAILURE);
    }
    return old_default_unit_types[id];
  }
}

/***************************************************************
  Convert old-style improvement type id into improvement type name
***************************************************************/
static const char* old_impr_type_name(int id)
{
  /* before 1.15.0 improvement types used to be saved by id */
  if (id < 0 || id >= ARRAY_SIZE(old_impr_types)) {
    freelog(LOG_ERROR, _("Wrong improvement type id value (%d)"), id);
    exit(EXIT_FAILURE);
  }
  return old_impr_types[id];
}

/***************************************************************
Load the worklist elements specified by path, given the arguments
plrno and wlinx, into the worklist pointed to by pwl.
***************************************************************/
static void worklist_load(struct section_file *file,
                          const char *path, int plrno, int wlinx,
                          struct worklist *pwl)
{
  char efpath[64];
  char idpath[64];
  char namepath[64];
  int i;
  bool end = FALSE;
  const char* name;

  sz_strlcpy(efpath, path);
  sz_strlcat(efpath, ".wlef%d");
  sz_strlcpy(idpath, path);
  sz_strlcat(idpath, ".wlid%d");
  sz_strlcpy(namepath, path);
  sz_strlcat(namepath, ".wlname%d");

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    if (end) {
      pwl->wlefs[i] = WEF_END;
      pwl->wlids[i] = 0;
      (void) section_file_lookup(file, efpath, plrno, wlinx, i);
      (void) section_file_lookup(file, idpath, plrno, wlinx, i);
    } else {
      pwl->wlefs[i] = (worklist_elem_flag)
        secfile_lookup_int_default(file, WEF_END, efpath, plrno, wlinx, i);
      name = secfile_lookup_str_default(file, NULL, namepath, plrno, wlinx, i);

      if (pwl->wlefs[i] == WEF_UNIT) {
        Unit_Type_id type;

        if (!name) {
            /* before 1.15.0 unit types used to be saved by id */
            name = old_unit_type_name(secfile_lookup_int(file, idpath,
                                                         plrno, wlinx, i));
        }

        type = find_unit_type_by_name_orig(name);
        if (type == U_LAST) {
          freelog(LOG_ERROR, _("Unknown unit type '%s' in worklist"),
                  name);
          exit(EXIT_FAILURE);
        }
        pwl->wlids[i] = type;
      } else if (pwl->wlefs[i] == WEF_IMPR) {
        Impr_Type_id type;

        if (!name) {
          name = old_impr_type_name(secfile_lookup_int(file, idpath,
                                                       plrno, wlinx, i));
        }

        type = find_improvement_by_name_orig(name);
        if (type == B_LAST) {
          freelog(LOG_ERROR, _("Unknown improvement type '%s' in worklist"),
                   name);
        }
        pwl->wlids[i] = type;
      }

      if ((pwl->wlefs[i] <= WEF_END) || (pwl->wlefs[i] >= WEF_LAST) ||
          ((pwl->wlefs[i] == WEF_UNIT) && !unit_type_exists(pwl->wlids[i])) ||
          ((pwl->wlefs[i] == WEF_IMPR) && !improvement_exists(pwl->wlids[i]))) {
        pwl->wlefs[i] = WEF_END;
        pwl->wlids[i] = 0;
        end = TRUE;
      }
    }
  }
}

/****************************************************************************
  Loads the units for the given player.
****************************************************************************/
static void load_player_units(player_t *plr, int plrno,
                              struct section_file *file)
{
  int nunits, i, j;
  enum unit_activity activity;
  char *savefile_options = secfile_lookup_str(file, "savefile.options");

  nunits = secfile_lookup_int(file, "player%d.nunits", plrno);
  if (!plr->is_alive && nunits > 0) {
    nunits = 0; /* Some old savegames may be buggy. */
  }

  for (i = 0; i < nunits; i++) {
    unit_t *punit;
    city_t *pcity;
    int nat_x, nat_y;
    const char* type_name;
    Unit_Type_id type;

    type_name = secfile_lookup_str_default(file, NULL,
                                           "player%d.u%d.type_by_name",
                                           plrno, i);
    if (!type_name) {
      /* before 1.15.0 unit types used to be saved by id. */
      int t = secfile_lookup_int(file, "player%d.u%d.type",
                             plrno, i);
      if (t < 0) {
        freelog(LOG_ERROR, _("Wrong player%d.u%d.type value (%d)"),
                plrno, i, t);
        exit(EXIT_FAILURE);
      }
      type_name = old_unit_type_name(t);

    }

    type = find_unit_type_by_name_orig(type_name);
    if (type == U_LAST) {
      freelog(LOG_ERROR, _("Unknown unit type '%s' in player%d section"),
              type_name, plrno);
      exit(EXIT_FAILURE);
    }

    punit = create_unit_virtual(plr, NULL, type,
        secfile_lookup_int(file, "player%d.u%d.veteran", plrno, i));
    punit->id = secfile_lookup_int(file, "player%d.u%d.id", plrno, i);
    alloc_id(punit->id);
    idex_register_unit(punit);

    nat_x = secfile_lookup_int(file, "player%d.u%d.x", plrno, i);
    nat_y = secfile_lookup_int(file, "player%d.u%d.y", plrno, i);
    punit->tile = native_pos_to_tile(nat_x, nat_y);

    /* Avoid warning when loading pre-2.1 saves containing foul status */
    secfile_lookup_bool_default(file, FALSE, "player%d.u%d.foul",
                                plrno, i);

    punit->homecity = secfile_lookup_int(file, "player%d.u%d.homecity",
                                         plrno, i);

    if ((pcity = find_city_by_id(punit->homecity))) {
      unit_list_append(pcity->common.units_supported, punit);
    }

    punit->moves_left
      = secfile_lookup_int(file, "player%d.u%d.moves", plrno, i);
    punit->fuel = secfile_lookup_int(file, "player%d.u%d.fuel", plrno, i);
    activity = (unit_activity)
        secfile_lookup_int(file, "player%d.u%d.activity", plrno, i);
    if (activity == ACTIVITY_PATROL_UNUSED) {
      /* Previously ACTIVITY_PATROL and ACTIVITY_GOTO were used for
       * client-side goto.  Now client-side goto is handled by setting
       * a special flag, and units with orders generally have ACTIVITY_IDLE.
       * Old orders are lost.  Old client-side goto units will still have
       * ACTIVITY_GOTO and will goto the correct position via server goto.
       * Old client-side patrol units lose their patrol routes and are put
       * into idle mode. */
      activity = ACTIVITY_IDLE;
    }
    set_unit_activity(punit, activity);

    /* need to do this to assign/deassign settlers correctly -- Syela
     *
     * was punit->activity=secfile_lookup_int(file,
     *                             "player%d.u%d.activity",plrno, i); */
    punit->activity_count = secfile_lookup_int(file,
                                               "player%d.u%d.activity_count",
                                               plrno, i);
    punit->activity_target = (tile_special_type)
        secfile_lookup_int_default(file, (int) S_NO_SPECIAL,
                                   "player%d.u%d.activity_target", plrno, i);

    punit->done_moving = secfile_lookup_bool_default(file,
        (punit->moves_left == 0), "player%d.u%d.done_moving", plrno, i);

    /* Load the goto information.  Older savegames will not have the
     * "go" field, so we just load the goto destination by default. */
    if (secfile_lookup_bool_default(file, TRUE,
                                    "player%d.u%d.go", plrno, i)) {
      int nat_x = secfile_lookup_int(file, "player%d.u%d.goto_x", plrno, i);
      int nat_y = secfile_lookup_int(file, "player%d.u%d.goto_y", plrno, i);

      punit->goto_tile = native_pos_to_tile(nat_x, nat_y);
    } else {
      punit->goto_tile = NULL;
    }

    punit->ai.control
      = secfile_lookup_bool(file, "player%d.u%d.ai", plrno, i);
    punit->hp = secfile_lookup_int(file, "player%d.u%d.hp", plrno, i);

    punit->ord_map
      = secfile_lookup_int_default(file, 0,
                                   "player%d.u%d.ord_map", plrno, i);
    punit->ord_city
      = secfile_lookup_int_default(file, 0,
                                   "player%d.u%d.ord_city", plrno, i);
    punit->moved
      = secfile_lookup_bool_default(file, FALSE,
                                    "player%d.u%d.moved", plrno, i);
    punit->paradropped
      = secfile_lookup_bool_default(file, FALSE,
                                    "player%d.u%d.paradropped", plrno, i);
    punit->transported_by
      = secfile_lookup_int_default(file, -1, "player%d.u%d.transported_by",
                                   plrno, i);
    /* Initialize upkeep values: these are hopefully initialized
       elsewhere before use (specifically, in city_support(); but
       fixme: check whether always correctly initialized?).
       Below is mainly for units which don't have homecity --
       otherwise these don't get initialized (and AI calculations
       etc may use junk values).
    */

    /* Trade route */
    city_t *pcity1, *pcity2;
    if ((pcity1 = find_city_by_id(secfile_lookup_int_default(file, -1,
                                  "player%d.u%d.trade_route_c1", plrno, i)))
        && (pcity2 = find_city_by_id(secfile_lookup_int_default(file, -1,
                                     "player%d.u%d.trade_route_c2", plrno, i)))) {
      struct trade_route *ptr = game_trade_route_find(pcity1, pcity2);

      if (ptr) {
        ptr->punit = punit;
        punit->ptr = ptr;
        /* This will swap cities if needed. */
        calculate_trade_move_cost(ptr);
      } else {
        freelog(LOG_ERROR, "Wrong trade route for the unit id %d", punit->id);
      }
    }

    /* Air patrol */
    if (can_unit_do_air_patrol(punit)) {
      int x, y;

      x = secfile_lookup_int_default(file, -1, "player%d.u%d.air_patrol_x",
                                     plrno, i);
      y = secfile_lookup_int_default(file, -1, "player%d.u%d.air_patrol_y",
                                     plrno, i);
      if (is_normal_map_pos(x, y)) {
        punit->air_patrol_tile = map_pos_to_tile(x, y);
      }
    }

    /* load the unit orders */
    if (has_capability("orders", savefile_options)) {
      int len = secfile_lookup_int_default(file, 0,
                        "player%d.u%d.orders_length", plrno, i);
      if (len > 0) {
        char *orders_buf, *dir_buf, *act_buf;

        punit->orders.list = (unit_order*)wc_malloc(len * sizeof(*(punit->orders.list)));
        punit->orders.length = len;
        punit->orders.index = secfile_lookup_int_default(file, 0,
                        "player%d.u%d.orders_index", plrno, i);
        punit->orders.repeat = secfile_lookup_bool_default(file, FALSE,
                        "player%d.u%d.orders_repeat", plrno, i);
        punit->orders.vigilant = secfile_lookup_bool_default(file, FALSE,
                        "player%d.u%d.orders_vigilant", plrno, i);

        orders_buf = secfile_lookup_str_default(file, "",
                        "player%d.u%d.orders_list", plrno, i);
        dir_buf = secfile_lookup_str_default(file, "",
                        "player%d.u%d.dir_list", plrno, i);
        act_buf = secfile_lookup_str_default(file, "",
                        "player%d.u%d.activity_list", plrno, i);
        punit->has_orders = TRUE;
        for (j = 0; j < len; j++) {
          struct unit_order *order = &punit->orders.list[j];

          if (orders_buf[j] == '\0' || dir_buf[j] == '\0'
              || act_buf[j] == '\0') {
            freelog(LOG_ERROR, _("Savegame error: invalid unit orders."));
            free_unit_orders(punit);
            break;
          }
          order->order = char2order(orders_buf[j]);
          order->dir = char2dir(dir_buf[j]);
          order->activity = char2activity(act_buf[j]);
          if (order->order == ORDER_LAST
              || (order->order == ORDER_MOVE && order->dir == DIR8_LAST)
              || (order->order == ORDER_ACTIVITY
                  && order->activity == ACTIVITY_LAST)) {
            /* An invalid order.  Just drop the orders for this unit. */
            free(punit->orders.list);
            punit->orders.list = NULL;
            punit->has_orders = FALSE;
            break;
          }
        }
      } else {
        punit->has_orders = FALSE;
        punit->orders.list = NULL;
      }
    } else {
      /* Old-style goto routes get discarded. */
      punit->has_orders = FALSE;
      punit->orders.list = NULL;
    }

    {
      /* Sanity: set the map to known for all tiles within the vision
       * range.
       *
       * FIXME: shouldn't this take into account modifiers like
       * watchtowers? */
      int range = unit_type(punit)->vision_range;

      square_iterate(punit->tile, range, tile1) {
        map_set_known(tile1, plr);
      } square_iterate_end;
    }

    /* allocate the unit's contribution to fog of war */
    if (unit_profits_of_watchtower(punit)
        && map_has_special(punit->tile, S_FORTRESS)) {
      unfog_area(unit_owner(punit), punit->tile,
                 get_watchtower_vision(punit));
    } else {
      unfog_area(unit_owner(punit), punit->tile,
                 unit_type(punit)->vision_range);
    }

    unit_list_append(plr->units, punit);

    unit_list_append(punit->tile->units, punit);
  }
}

/***************************************************************
Load the worklist elements specified by path, given the arguments
plrno and wlinx, into the worklist pointed to by pwl.
Assumes original save-file format.  Use for backward compatibility.
***************************************************************/
static void worklist_load_old(struct section_file *file,
                              const char *path, int plrno, int wlinx,
                              struct worklist *pwl)
{
  int i, id;
  bool end = FALSE;
  const char* name;

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    if (end) {
      pwl->wlefs[i] = WEF_END;
      pwl->wlids[i] = 0;
      (void) section_file_lookup(file, path, plrno, wlinx, i);
    } else {
      id = secfile_lookup_int_default(file, -1, path, plrno, wlinx, i);

      if ((id < 0) || (id >= 284)) { /* 284 was flag value for end of list */
        pwl->wlefs[i] = WEF_END;
        pwl->wlids[i] = 0;
        end = TRUE;
      } else if (id >= 68) {            /* 68 was offset to unit ids */
        name = old_unit_type_name(id-68);
        pwl->wlefs[i] = WEF_UNIT;
        pwl->wlids[i] = find_unit_type_by_name_orig(name);
        end = !unit_type_exists(pwl->wlids[i]);
      } else {                          /* must be an improvement id */
        name = old_impr_type_name(id);
        pwl->wlefs[i] = WEF_IMPR;
        pwl->wlids[i] = find_improvement_by_name_orig(name);
        end = !improvement_exists(pwl->wlids[i]);
      }
    }
  }

}

/***************************************************************
load the tile map from a savegame file
***************************************************************/
static void map_tiles_load(struct section_file *file)
{
  map.info.topology_id = secfile_lookup_int_default(file, MAP_ORIGINAL_TOPO,
                                                    "map.topology_id");

  /* In some cases we read these before, but not always, and
   * its safe to read them again:
   */
  map.info.xsize = secfile_lookup_int(file, "map.width");
  map.info.ysize = secfile_lookup_int(file, "map.height");

  /* With a FALSE parameter [xy]size are not changed by this call. */
  map_init_topology(FALSE);

  map_allocate();

  /* get the terrain type */
  LOAD_MAP_DATA(ch, line, ptile,
                secfile_lookup_str(file, "map.t%03d", line),
                ptile->terrain = char2terrain(ch));

  assign_continent_numbers(FALSE);

  whole_map_iterate(ptile) {
    map_set_spec_sprite(ptile,
                        secfile_lookup_str_default(file, NULL,
                                                   "map.spec_sprite_%d_%d",
                                                   ptile->nat_x, ptile->nat_y));
  } whole_map_iterate_end;
}

/***************************************************************
load starting positions for the players from a savegame file
Now we don't know how many start positions there are nor how many
should be because rulesets are loaded later. So try to load as
many as they are; there should be at least enough for every
player.  This could be changed/improved in future.
***************************************************************/
static void map_startpos_load(struct section_file *file)
{
  int savegame_start_positions;
  int i, j;
  int nation_id;
  int nat_x, nat_y;

  for (savegame_start_positions = 0;
       secfile_lookup_int_default(file, -1, "map.r%dsx",
                                  savegame_start_positions) != -1;
       savegame_start_positions++) {
    /* Nothing. */
  }


  {
    struct civ_map::civ_map_server::start_position start_positions[savegame_start_positions];

    for (i = j = 0; i < savegame_start_positions; i++) {
      char *nation = secfile_lookup_str_default(file, NULL, "map.r%dsnation",
                                                i);

      if (nation == NULL) {
        /* Starting positions in normal games are saved without nation.
           Just ignore it */
        continue;
      }

      nation_id = find_nation_by_name_orig(nation);
      if (nation_id == NO_NATION_SELECTED && map.server.startpos != 5) {
        freelog(LOG_NORMAL,
                _("Warning: Unknown nation %s for starting position no %d"),
                nation,
                i);
        continue;
      }

      nat_x = secfile_lookup_int(file, "map.r%dsx", i);
      nat_y = secfile_lookup_int(file, "map.r%dsy", i);

      start_positions[j].tile = native_pos_to_tile(nat_x, nat_y);
      start_positions[j].nation = nation_id;
      j++;
    }
    map.server.num_start_positions = j;
    if (map.server.num_start_positions > 0) {
      map.server.start_positions = (civ_map::civ_map_server::start_position*)
          wc_realloc(map.server.start_positions,
                     map.server.num_start_positions
                     * sizeof(civ_map::civ_map_server::start_position));
      for (i = 0; i < j; i++) {
        map.server.start_positions[i] = start_positions[i];
      }
    }
  }


  if (map.server.num_start_positions
      && map.server.num_start_positions < game.info.max_players) {
    freelog(LOG_VERBOSE,
            _("Number of starts (%d) are lower than max_players (%d),"
              " lowering max_players."),
            map.server.num_start_positions, game.info.max_players);
    game.info.max_players = map.server.num_start_positions;
  }
}

/***************************************************************
load a complete map from a savegame file
***************************************************************/
static void map_load(struct section_file *file)
{
  char *savefile_options = secfile_lookup_str(file, "savefile.options");

  /* map_init();
   * This is already called in game_init(), and calling it
   * here stomps on map.huts etc.  --dwp
   */

  map_tiles_load(file);
  if (secfile_lookup_bool_default(file, TRUE, "game.save_starts")) {
    map_startpos_load(file);
  } else {
    map.server.num_start_positions = 0;
  }

  /* get 4-bit segments of 16-bit "special" field. */
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str(file, "map.l%03d", nat_y),
                ptile->special = (tile_special_type)ascii_hex2bin(ch, 0));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str(file, "map.u%03d", nat_y),
                ptile->special = (tile_special_type)(ptile->special | ascii_hex2bin(ch, 1)));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str_default(file, NULL, "map.n%03d", nat_y),
                ptile->special = (tile_special_type)(ptile->special | ascii_hex2bin(ch, 2)));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str_default(file, NULL, "map.f%03d", nat_y),
                ptile->special = (tile_special_type)(ptile->special | ascii_hex2bin(ch, 3)));

  if (secfile_lookup_bool_default(file, TRUE, "game.save_known")) {

    /* get 4-bit segments of the first half of the 32-bit "known" field */
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "map.a%03d", nat_y),
                  ptile->u.server.known = ascii_hex2bin(ch, 0));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "map.b%03d", nat_y),
                  ptile->u.server.known |= ascii_hex2bin(ch, 1));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "map.c%03d", nat_y),
                  ptile->u.server.known |= ascii_hex2bin(ch, 2));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "map.d%03d", nat_y),
                  ptile->u.server.known |= ascii_hex2bin(ch, 3));

    if (has_capability("known32fix", savefile_options)) {
      /* get 4-bit segments of the second half of the 32-bit "known" field */
      LOAD_MAP_DATA(ch, nat_y, ptile,
                    secfile_lookup_str(file, "map.e%03d", nat_y),
                    ptile->u.server.known |= ascii_hex2bin(ch, 4));
      LOAD_MAP_DATA(ch, nat_y, ptile,
                    secfile_lookup_str(file, "map.g%03d", nat_y),
                    ptile->u.server.known |= ascii_hex2bin(ch, 5));
      LOAD_MAP_DATA(ch, nat_y, ptile,
                    secfile_lookup_str(file, "map.h%03d", nat_y),
                    ptile->u.server.known |= ascii_hex2bin(ch, 6));
      LOAD_MAP_DATA(ch, nat_y, ptile,
                    secfile_lookup_str(file, "map.i%03d", nat_y),
                    ptile->u.server.known |= ascii_hex2bin(ch, 7));
    }
  }

  map.server.have_specials = TRUE;
}

/***************************************************************
load the rivers overlay map from a savegame file

(This does not need to be called from map_load(), because
 map_load() loads the rivers overlay along with the rest of
 the specials.  Call this only if you've already called
 map_tiles_load(), and want to overlay rivers defined as
 specials, rather than as terrain types.)
***************************************************************/
static void map_rivers_overlay_load(struct section_file *file)
{
  /* Get the bits of the special flags which contain the river special
     and extract the rivers overlay from them. */
  LOAD_MAP_DATA(ch, line, ptile,
                secfile_lookup_str_default(file, NULL, "map.n%03d", line),
                ptile->special = (tile_special_type)
                    (ptile->special | (ascii_hex2bin(ch, 2) & S_RIVER)));
  map.server.have_rivers_overlay = TRUE;
}

/****************************************************************************
  Convert an old-style government index into a government name.
****************************************************************************/
static const char* old_government_name(int id)
{
  /* before 1.15.0 governments used to be saved by index */
  if (id < 0) {
    freelog(LOG_ERROR, _("Wrong government type id value (%d)"), id);
    exit(EXIT_FAILURE);
  }
  /* Different rulesets had different governments. */
  if (strcmp(game.server.rulesetdir, "civ2") == 0) {
    if (id >= ARRAY_SIZE(old_civ2_governments)) {
      freelog(LOG_ERROR, _("Wrong government type id value (%d)"), id);
      exit(EXIT_FAILURE);
    }
    return old_civ2_governments[id];
  } else {
    if (id >= ARRAY_SIZE(old_default_governments)) {
      freelog(LOG_ERROR, _("Wrong government type id value (%d)"), id);
      exit(EXIT_FAILURE);
    }
    return old_default_governments[id];
  }
}

/****************************************************************************
  Convert an old-style technology id into a tech name.
****************************************************************************/
static const char* old_tech_name(int id)
{
  /* This was 1.14.1 value for A_FUTURE */
  if (id == 198) {
    return "A_FUTURE";
  }

  if (id == -1 || id == 0) {
    return "A_NONE";
  }

  if (id == A_UNSET) {
    return "A_UNSET";
  }

  if (id < 0 || id >= ARRAY_SIZE(old_default_techs)) {
    freelog(LOG_ERROR, _("Wrong tech type id value (%d)"), id);
    exit(EXIT_FAILURE);
  }

  if (strcmp(game.server.rulesetdir, "civ1") == 0 && id == 83) {
    return "Religion";
  }

  return old_default_techs[id];
}

/*****************************************************************************
  Load technology from path_name and if doesn't exist (because savegame
  is too old) load from path.
*****************************************************************************/
static Tech_Type_id load_technology(struct section_file *file,
                                    const char* path, int plrno)
{
  char path_with_name[128];
  const char* name;
  int id;

  my_snprintf(path_with_name, sizeof(path_with_name),
              "%s_name", path);

  name = secfile_lookup_str_default(file, NULL, path_with_name, plrno);
  if (!name) {
    id = secfile_lookup_int_default(file, -1, path, plrno);
    name = old_tech_name(id);
  }

  if (mystrcasecmp(name, "A_FUTURE") == 0) {
    return A_FUTURE;
  }
  if (mystrcasecmp(name, "A_NONE") == 0) {
    return A_NONE;
  }
  if (mystrcasecmp(name, "A_UNSET") == 0) {
    return A_UNSET;
  }
  if (name[0] == '\0') {
    /* it is used by changed_from */
    return -1;
  }

  id = find_tech_by_name_orig(name);
  if (id == A_LAST) {
    freelog(LOG_ERROR, _("Unknown technology (%s)"), name);
    exit(EXIT_FAILURE);
  }
  return id;
}

/****************************************************************************
  Load all information about player "plrno" into the structure pointed to
  by "plr".

  Old global observers will be loaded as dead player.
****************************************************************************/
static void player_load(player_t *plr, int plrno,
                        struct section_file *file,
                        char** improvement_order,
                        int improvement_order_size,
                        char** technology_order,
                        int technology_order_size)
{
  int i, j, k, x, y, ncities, c_s;
  const char *p;
  const char *name;
  char *savefile_options = secfile_lookup_str(file, "savefile.options");
  struct ai_data *ai;
  struct government *gov;
  int id;
  int target_no;

  server_player_init(plr, TRUE);
  ai = ai_data_get(plr);

  plr->ai.barbarian_type = (barbarian_type)
      secfile_lookup_int_default(file, 0, "player%d.ai.is_barbarian", plrno);
  if (is_barbarian(plr)) game.server.nbarbarians++;

  sz_strlcpy(plr->name, secfile_lookup_str(file, "player%d.name", plrno));
  sz_strlcpy(plr->username,
             secfile_lookup_str_default(file, "", "player%d.username", plrno));

  if (is_barbarian(plr)) {
    plr->nation = game.ruleset_control.nation_count - 1;
  } else {
    /* 1.15 and later versions store nations by name.  Try that first. */
    p = secfile_lookup_str_default(file, NULL, "player%d.nation", plrno);
    if (!p) {
      /*
       * Otherwise read as a pre-1.15 savefile with numeric nation indexes.
       * This random-looking order is from the old nations/ruleset file.
       * Use it to convert old-style nation indices to name strings.
       * The idea is not to be dependent on the order in which nations
       * get read into the registry.
       */
      const char *name_order[] = {
        "roman", "babylonian", "german", "egyptian", "american", "greek",
        "indian", "russian", "zulu", "french", "aztec", "chinese", "english",
        "mongol", "turk", "spanish", "persian", "arab", "carthaginian", "inca",
        "viking", "polish", "hungarian", "danish", "dutch", "swedish",
        "japanese", "portuguese", "finnish", "sioux", "czech", "australian",
        "welsh", "korean", "scottish", "israeli", "argentine", "canadian",
        "ukrainian", "lithuanian", "kenyan", "dunedain", "vietnamese", "thai",
        "mordor", "bavarian", "brazilian", "irish", "cornish", "italian",
        "filipino", "estonian", "latvian", "boer", "silesian", "singaporean",
        "chilean", "catalan", "croatian", "slovenian", "serbian", "barbarian",
      };
      int index = secfile_lookup_int(file, "player%d.race", plrno);

      if (index >= 0 && index < ARRAY_SIZE(name_order)) {
        p = name_order[index];
      } else {
        p = "";
      }
    }
    plr->nation = find_nation_by_name_orig(p);
    if (plr->nation == NO_NATION_SELECTED) {
      freelog(LOG_VERBOSE, "Nation %s (used by %s) isn't available.",
              p, plr->name);
    } else {
      /* Add techs from game and nation, but ignore game.info.tech. */
      init_tech(plr);
      give_initial_techs(plr);
    }
  }

  /* not all players have teams */
  if (section_file_lookup(file, "player%d.team", plrno)) {
    char tmp[MAX_LEN_NAME];

    sz_strlcpy(tmp, secfile_lookup_str(file, "player%d.team", plrno));
    team_add_player(plr, tmp);
    plr->team = team_find_by_name(tmp);
  } else {
    plr->team = TEAM_NONE;
  }

  /* government */
  name = secfile_lookup_str_default(file, NULL, "player%d.government_name",
                                    plrno);
  if (!name) {
    /* old servers used to save government by id */
    id = secfile_lookup_int(file, "player%d.government", plrno);
    name = old_government_name(id);
  }
  gov = find_government_by_name_orig(name);
  if (gov == NULL) {
    freelog(LOG_ERROR, _("Unsupported government found (%s)"), name);
    exit(EXIT_FAILURE);
  }
  plr->government = gov->index;

  /* Target government */
  name = secfile_lookup_str_default(file, NULL,
                                    "player%d.target_government_name",
                                    plrno);
  if (name) {
    gov = find_government_by_name_orig(name);
  } else {
    gov = NULL;
  }
  if (gov) {
    plr->target_government = gov->index;
  } else {
    /* Old servers didn't have this value. */
    plr->target_government = plr->government;
  }

  plr->embassy = secfile_lookup_int(file, "player%d.embassy", plrno);

  p = secfile_lookup_str_default(file, NULL, "player%d.city_style_by_name",
                                 plrno);
  if (!p) {
    char const * old_order[4] = {"European", "Classical", "Tropical", "Asian"};
    c_s = secfile_lookup_int_default(file, 0, "player%d.city_style", plrno);
    if (c_s < 0 || c_s > 3) {
      c_s = 0;
    }
    p = old_order[c_s];
  }
  c_s = get_style_by_name_orig(p);
  if (c_s == -1) {
    freelog(LOG_ERROR, _("Unsupported city style found in player%d section. "
                         "Changed to %s"), plrno, get_city_style_name(0));
    c_s = 0;
  }
  plr->city_style = c_s;

  plr->nturns_idle=0;
  plr->is_male = secfile_lookup_bool_default(file, TRUE, "player%d.is_male", plrno);
  plr->is_alive = secfile_lookup_bool(file, "player%d.is_alive", plrno);
  plr->ai.control = secfile_lookup_bool(file, "player%d.ai.control", plrno);
  for (i = 0; i < MAX_NUM_PLAYERS; i++) {
    plr->ai.love[i]
         = secfile_lookup_int_default(file, 1, "player%d.ai%d.love", plrno, i);
    ai->diplomacy.player_intel[i].spam
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.spam", plrno, i);
    ai->diplomacy.player_intel[i].ally_patience
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.patience", plrno, i);
    ai->diplomacy.player_intel[i].warned_about_space
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.warn_space", plrno, i);
    ai->diplomacy.player_intel[i].asked_about_peace
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.ask_peace", plrno, i);
    ai->diplomacy.player_intel[i].asked_about_alliance
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.ask_alliance", plrno, i);
    ai->diplomacy.player_intel[i].asked_about_ceasefire
         = secfile_lookup_int_default(file, 0, "player%d.ai%d.ask_ceasefire", plrno, i);
  }
  /* Diplomacy target is saved as player number or -1 if none */
  target_no = secfile_lookup_int_default(file, -1,
                                         "player%d.ai.target", plrno);
  ai->diplomacy.target = target_no == -1 ? NULL : &game.players[target_no];
  plr->ai.tech_goal = load_technology(file, "player%d.ai.tech_goal", plrno);
  if (plr->ai.tech_goal == A_NONE) {
    /* Old servers (1.14.1) saved both A_UNSET and A_NONE by 0
     * Here 0 means A_UNSET
     */
    plr->ai.tech_goal = A_UNSET;
  }
  /* Some sane defaults */
  plr->ai.handicap = 0;         /* set later */
  plr->ai.fuzzy = 0;            /* set later */
  plr->ai.expand = 100;         /* set later */
  plr->ai.science_cost = 100;   /* set later */
  plr->ai.skill_level =
    secfile_lookup_int_default(file, game.info.skill_level,
                               "player%d.ai.skill_level", plrno);
  if (plr->ai.control && plr->ai.skill_level==0) {
    plr->ai.skill_level = GAME_OLD_DEFAULT_SKILL_LEVEL;
  }
  if (plr->ai.control) {
    /* Set AI parameters */
    set_ai_level_directer(plr, plr->ai.skill_level);
  }

  plr->economic.gold=secfile_lookup_int(file, "player%d.gold", plrno);
  plr->economic.tax=secfile_lookup_int(file, "player%d.tax", plrno);
  plr->economic.science=secfile_lookup_int(file, "player%d.science", plrno);
  plr->economic.luxury=secfile_lookup_int(file, "player%d.luxury", plrno);

  /* how many future techs were researched already by player */
  plr->future_tech = secfile_lookup_int(file, "player%d.futuretech", plrno);

  /* We use default values for bulbs_researched_before, changed_from
   * and got_tech to preserve backwards-compatibility with save files
   * that didn't store this information. */
  plr->research.bulbs_researched=secfile_lookup_int(file,
                                             "player%d.researched", plrno);
  plr->research.bulbs_researched_before =
          secfile_lookup_int_default(file, 0,
                                     "player%d.researched_before", plrno);
  plr->research.changed_from =
          load_technology(file, "player%d.research_changed_from", plrno);
  plr->got_tech = secfile_lookup_bool_default(file, FALSE,
                                              "player%d.research_got_tech",
                                              plrno);
  plr->research.techs_researched=secfile_lookup_int(file,
                                             "player%d.researchpoints", plrno);
  plr->research.researching =
        load_technology(file, "player%d.researching", plrno);
  if (plr->research.researching == A_NONE) {
    /* Old servers (1.14.1) used to save A_FUTURE by 0
     * This has to be interpreted from context because A_NONE was also
     * saved by 0
     */
    plr->research.researching = A_FUTURE;
  }

  p = secfile_lookup_str_default(file, NULL, "player%d.invs_new", plrno);
  if (!p) {
    /* old savegames */
    p = secfile_lookup_str(file, "player%d.invs", plrno);
    for (k = 0; p[k];  k++) {
      if (p[k] == '1') {
        name = old_tech_name(k);
        id = find_tech_by_name_orig(name);
        if (id != A_LAST) {
          set_invention(plr, id, TECH_KNOWN);
        }
      }
    }
  } else {
    for (k = 0; k < technology_order_size && p[k]; k++) {
      if (p[k] == '1') {
        id = find_tech_by_name_orig(technology_order[k]);
        if (id != A_LAST) {
          set_invention(plr, id, TECH_KNOWN);
        }
      }
    }
  }

  plr->capital = secfile_lookup_bool(file, "player%d.capital", plrno);

  {
    /* The old-style "revolution" value indicates the number of turns until
     * the revolution is complete, or 0 if there is no revolution.  The
     * new-style "revolution_finishes" value indicates the turn in which
     * the revolution will complete (which may be less than the current
     * turn) or -1 if there is no revolution. */
    int revolution = secfile_lookup_int_default(file, 0, "player%d.revolution",
                                                plrno);

    if (revolution == 0) {
      if (plr->government != game.ruleset_control.government_when_anarchy) {
        revolution = -1;
      } else {
        /* some old savegames may be buggy */
        revolution = game.info.turn + 1;
      }
    } else {
      revolution = game.info.turn + revolution;
    }
    plr->revolution_finishes
      = secfile_lookup_int_default(file, revolution,
                                   "player%d.revolution_finishes", plrno);
  }

  update_research(plr);

  plr->reputation=secfile_lookup_int_default(file, GAME_DEFAULT_REPUTATION,
                                             "player%d.reputation", plrno);
  for (i=0; i < game.info.nplayers; i++) {
    plr->diplstates[i].type = (diplstate_type)
      secfile_lookup_int_default(file, DIPLSTATE_WAR,
                                 "player%d.diplstate%d.type", plrno, i);
    plr->diplstates[i].turns_left =
      secfile_lookup_int_default(file, -2,
                                 "player%d.diplstate%d.turns_left", plrno, i);
    plr->diplstates[i].has_reason_to_cancel =
      secfile_lookup_int_default(file, 0,
                                 "player%d.diplstate%d.has_reason_to_cancel",
                                 plrno, i);
    plr->diplstates[i].contact_turns_left =
      secfile_lookup_int_default(file, 0,
                           "player%d.diplstate%d.contact_turns_left", plrno, i);
  }
  /* We don't need this info, but savegames carry it anyway.
     To avoid getting "unused" warnings we touch the values like this. */
  for (i=game.info.nplayers; i<MAX_NUM_PLAYERS+MAX_NUM_BARBARIANS; i++) {
    secfile_lookup_int_default(file, DIPLSTATE_NEUTRAL,
                               "player%d.diplstate%d.type", plrno, i);
    secfile_lookup_int_default(file, 0,
                               "player%d.diplstate%d.turns_left", plrno, i);
    secfile_lookup_int_default(file, 0,
                               "player%d.diplstate%d.has_reason_to_cancel",
                               plrno, i);
    secfile_lookup_int_default(file, 0,
                           "player%d.diplstate%d.contact_turns_left", plrno, i);
  }
  /* Sanity check alliances, prevent allied-with-ally-of-enemy */
  players_iterate(aplayer) {
    if (plr->is_alive
        && aplayer->is_alive
        && pplayers_allied(plr, aplayer)
        && !pplayer_can_ally(plr, aplayer)) {
      freelog(LOG_ERROR, _("Illegal alliance structure detected: "
              "%s's alliance to %s reduced to peace treaty."),
              plr->name, aplayer->name);
      plr->diplstates[aplayer->player_no].type = DIPLSTATE_PEACE;
      aplayer->diplstates[plr->player_no].type = DIPLSTATE_PEACE;
    }
  } players_iterate_end;

  /* Unit statistics. */
  plr->score.units_built =
    secfile_lookup_int_default(file, 0, "player%d.units_built", plrno);
  plr->score.units_killed =
    secfile_lookup_int_default(file, 0, "player%d.units_killed", plrno);
  plr->score.units_lost =
    secfile_lookup_int_default(file, 0, "player%d.units_lost", plrno);

  { /* spacerace */
    struct player_spaceship *ship = &plr->spaceship;
    char prefix[32];
    char *st;

    my_snprintf(prefix, sizeof(prefix), "player%d.spaceship", plrno);
    spaceship_init(ship);
    ship->state = (spaceship_state)secfile_lookup_int(file, "%s.state", prefix);

    if (ship->state != SSHIP_NONE) {
      ship->structurals = secfile_lookup_int(file, "%s.structurals", prefix);
      ship->components = secfile_lookup_int(file, "%s.components", prefix);
      ship->modules = secfile_lookup_int(file, "%s.modules", prefix);
      ship->fuel = secfile_lookup_int(file, "%s.fuel", prefix);
      ship->propulsion = secfile_lookup_int(file, "%s.propulsion", prefix);
      ship->habitation = secfile_lookup_int(file, "%s.habitation", prefix);
      ship->life_support = secfile_lookup_int(file, "%s.life_support", prefix);
      ship->solar_panels = secfile_lookup_int(file, "%s.solar_panels", prefix);

      st = secfile_lookup_str(file, "%s.structure", prefix);
      for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
        if (st[i] == '0') {
          ship->structure[i] = FALSE;
        } else if (st[i] == '1') {
          ship->structure[i] = TRUE;
        } else {
          freelog(LOG_ERROR, "invalid spaceship structure '%c' %d", st[i],
                  st[i]);
          ship->structure[i] = FALSE;
        }
      }
      if (ship->state >= SSHIP_LAUNCHED) {
        ship->launch_year = secfile_lookup_int(file, "%s.launch_year", prefix);
      }
      spaceship_calc_derived(ship);
    }
  }

  ncities = secfile_lookup_int(file, "player%d.ncities", plrno);
  if (!plr->is_alive && ncities > 0) {
    ncities = 0; /* Some old savegames may be buggy. */
  }

  for (i = 0; i < ncities; i++) { /* read the cities */
    city_t *pcity;
    int nat_x = secfile_lookup_int(file, "player%d.c%d.x", plrno, i);
    int nat_y = secfile_lookup_int(file, "player%d.c%d.y", plrno, i);
    tile_t *ptile = native_pos_to_tile(nat_x, nat_y);
    const char* name;
    int id, k;

    pcity = create_city_virtual(plr, ptile,
                      secfile_lookup_str(file, "player%d.c%d.name", plrno, i));

    pcity->common.id=secfile_lookup_int(file, "player%d.c%d.id", plrno, i);
    alloc_id(pcity->common.id);
    idex_register_city(pcity);

    if (section_file_lookup(file, "player%d.c%d.original", plrno, i)) {
      pcity->u.server.original = secfile_lookup_int(file, "player%d.c%d.original",
                                                  plrno, i);
    } else {
      pcity->u.server.original = plrno;
    }
    pcity->common.pop_size = secfile_lookup_int(file, "player%d.c%d.size", plrno, i);

    pcity->u.server.steal = secfile_lookup_int(file, "player%d.c%d.steal", plrno, i);

    specialist_type_iterate(sp) {
      pcity->common.specialists[sp]
        = secfile_lookup_int(file, "player%d.c%d.n%s", plrno, i,
                             game.ruleset_game.specialist_name[sp]);
    } specialist_type_iterate_end;

    /* Trade routes */
    k = secfile_lookup_int_default(file, -1,
                                   "player%d.c%d.wc_trade_route_num", plrno, i);
    if (k >= 0) {
      /* Was saved with warserver datas */
      for (j = 0; j < k; j++) {
        city_t *pother_city;
        struct trade_route *ptr;
        int cid;

        cid = secfile_lookup_int(file, "player%d.c%dw.wc_trade_route%d_city",
                                 plrno, i, j);
        if ((pother_city = find_city_by_id(cid))) {
          /* Mean that the other city is loaded.
           * If it's not the case, do nothing. */
          ptr = game_trade_route_add(pcity, pother_city);
          ptr->status = (trade_route_status)
            secfile_lookup_int(file, "player%d.c%dw.wc_trade_route%d_status",
                               plrno, i, j);
        }
      }
    } else {
      /* Standard warciv */
      for (j = 0; j < OLD_NUM_TRADEROUTES; j++) {
        city_t *pother_city;
        int cid;

        cid = secfile_lookup_int(file, "player%d.c%d.traderoute%d",
                                 plrno, i, j);
        if ((pother_city = find_city_by_id(cid))) {
          /* Mean that the other city is loaded.
           * If it's not the case, do nothing. */
          server_establish_trade_route(pcity, pother_city);
        }
      }
    }

    /* Rally point */
    x = secfile_lookup_int_default(file, -1,
                                   "player%d.c%d.rally_point_x", plrno, i);
    y = secfile_lookup_int_default(file, -1,
                                   "player%d.c%d.rally_point_y", plrno, i);

    if (is_normal_map_pos(x, y)) {
      pcity->common.rally_point = map_pos_to_tile(x, y);
    } else {
      pcity->common.rally_point = NULL;
    }

    /* City Manager parameter */
    if (secfile_lookup_bool_default(file, FALSE,
                                    "player%d.c%d.managed",
                                    plrno, i)) {
      pcity->u.server.managed = TRUE;
      for (j = 0; j < CM_NUM_STATS; j++) {
        pcity->u.server.parameter.minimal_surplus[j] =
          secfile_lookup_int(file, "player%d.c%dw.minimal_surplus%d",
                             plrno, i, j);
        pcity->u.server.parameter.factor[j] =
          secfile_lookup_int(file, "player%d.c%dw.factor%d",
                             plrno, i, j);
      }
      pcity->u.server.parameter.require_happy =
        secfile_lookup_bool(file, "player%d.c%dw.require_happy", plrno, i);
      pcity->u.server.parameter.allow_disorder =
        secfile_lookup_bool(file, "player%d.c%dw.allow_disorder", plrno, i);
      pcity->u.server.parameter.allow_specialists =
        secfile_lookup_bool(file, "player%d.c%dw.allow_specialists", plrno, i);
      pcity->u.server.parameter.happy_factor =
        secfile_lookup_int(file, "player%d.c%dw.happy_factor", plrno, i);
    } else {
      pcity->u.server.managed = FALSE;
    }

    pcity->common.food_stock = secfile_lookup_int(file, "player%d.c%d.food_stock",
                                           plrno, i);
    pcity->common.shield_stock = secfile_lookup_int(file, "player%d.c%d.shield_stock",
                                             plrno, i);
    pcity->common.tile_trade = pcity->common.trade_prod = 0;
    pcity->common.anarchy = secfile_lookup_int(file, "player%d.c%d.anarchy", plrno, i);
    pcity->common.rapture = secfile_lookup_int_default(file, 0, "player%d.c%d.rapture",
                                                plrno, i);
    pcity->common.was_happy = secfile_lookup_bool(file, "player%d.c%d.was_happy",
                                           plrno, i);
    pcity->common.is_building_unit=
      secfile_lookup_bool(file,
                         "player%d.c%d.is_building_unit", plrno, i);
    name = secfile_lookup_str_default(file, NULL,
                                      "player%d.c%d.currently_building_name",
                                      plrno, i);
    if (pcity->common.is_building_unit) {
      if (!name) {
        id = secfile_lookup_int(file, "player%d.c%d.currently_building",
                                plrno, i);
        name = old_unit_type_name(id);
      }
      pcity->common.currently_building = find_unit_type_by_name_orig(name);
    } else {
      if (!name) {
        id = secfile_lookup_int(file, "player%d.c%d.currently_building",
                                plrno, i);
        name = old_impr_type_name(id);
      }
      pcity->common.currently_building = find_improvement_by_name_orig(name);
    }

    if (has_capability("turn_last_built", savefile_options)) {
      pcity->common.turn_last_built = secfile_lookup_int(file,
                                "player%d.c%d.turn_last_built", plrno, i);
    } else {
      /* Before, turn_last_built was stored as a year.  There is no easy
       * way to convert this into a turn value. */
      pcity->common.turn_last_built = 0;
    }
    pcity->common.changed_from_is_unit=
      secfile_lookup_bool_default(file, pcity->common.is_building_unit,
                                 "player%d.c%d.changed_from_is_unit", plrno, i);
    name = secfile_lookup_str_default(file, NULL,
                                      "player%d.c%d.changed_from_name",
                                      plrno, i);
    if (pcity->common.changed_from_is_unit) {
      if (!name) {
        id = secfile_lookup_int(file, "player%d.c%d.changed_from_id",
                                plrno, i);
        name = old_unit_type_name(id);
      }
      pcity->common.changed_from_id = find_unit_type_by_name_orig(name);
    } else {
      if (!name) {
        id = secfile_lookup_int(file, "player%d.c%d.changed_from_id",
                                plrno, i);
        name = old_impr_type_name(id);
      }
      pcity->common.changed_from_id = find_improvement_by_name_orig(name);
    }

    pcity->common.before_change_shields=
      secfile_lookup_int_default(file, pcity->common.shield_stock,
                                 "player%d.c%d.before_change_shields", plrno, i);
    pcity->common.disbanded_shields=
      secfile_lookup_int_default(file, 0,
                                 "player%d.c%d.disbanded_shields", plrno, i);
    pcity->common.caravan_shields=
      secfile_lookup_int_default(file, 0,
                                 "player%d.c%d.caravan_shields", plrno, i);
    pcity->common.last_turns_shield_surplus =
      secfile_lookup_int_default(file, 0,
                                 "player%d.c%d.last_turns_shield_surplus",
                                 plrno, i);

    pcity->u.server.synced = FALSE; /* must re-sync with clients */

    pcity->common.turn_founded =
        secfile_lookup_int_default(file, -2, "player%d.c%d.turn_founded",
                                   plrno, i);

    j = secfile_lookup_int(file, "player%d.c%d.did_buy", plrno, i);
    pcity->common.did_buy = (j != 0);
    if (j == -1 && pcity->common.turn_founded == -2) {
      pcity->common.turn_founded = game.info.turn;
    }

    pcity->common.did_sell =
      secfile_lookup_bool_default(file, FALSE, "player%d.c%d.did_sell", plrno,i);

    pcity->common.airlift = secfile_lookup_bool_default(file, FALSE,
                                        "player%d.c%d.airlift", plrno,i);

    pcity->common.city_options =
      secfile_lookup_int_default(file, CITYOPT_DEFAULT,
                                 "player%d.c%d.options", plrno, i);

    /* Fix for old buggy savegames. */
    if (!has_capability("known32fix", savefile_options)
        && plrno >= 16) {
      map_city_radius_iterate(pcity->common.tile, tile1) {
        map_set_known(tile1, plr);
      } map_city_radius_iterate_end;
    }

    /* adding the cities contribution to fog-of-war */
    map_unfog_pseudo_city_area(&game.players[plrno], pcity->common.tile);

    /* Initialize pcity->common.city_map[][], using set_worker_city() so that
       ptile->worked gets initialized correctly.  The pre-initialisation
       to C_TILE_EMPTY is necessary because set_worker_city() accesses
       the existing value to possibly adjust ptile->worked, so need to
       initialize a non-worked value so ptile->worked (possibly already
       set from neighbouring city) does not get unset for C_TILE_EMPTY
       or C_TILE_UNAVAILABLE here.  -- dwp
    */
    p=secfile_lookup_str(file, "player%d.c%d.workers", plrno, i);
    for(y=0; y<CITY_MAP_SIZE; y++) {
      for(x=0; x<CITY_MAP_SIZE; x++) {
        pcity->common.city_map[x][y] =
            is_valid_city_coords(x, y) ? C_TILE_EMPTY : C_TILE_UNAVAILABLE;
        if (*p == '0') {
          set_worker_city(pcity, x, y,
                          city_map_to_map(pcity, x, y) ?
                          C_TILE_EMPTY : C_TILE_UNAVAILABLE);
        } else if (*p=='1') {
          tile_t *ptile;

          ptile = city_map_to_map(pcity, x, y);

          if (ptile->worked) {
            /* oops, inconsistent savegame; minimal fix: */
            freelog(LOG_VERBOSE, "Inconsistent worked for %s (%d,%d), "
                    "converting to elvis", pcity->common.name, x, y);
            pcity->common.specialists[SP_ELVIS]++;
            set_worker_city(pcity, x, y, C_TILE_UNAVAILABLE);
          } else {
            set_worker_city(pcity, x, y, C_TILE_WORKER);
          }
        } else {
          assert(*p == '2');
          if (is_valid_city_coords(x, y)) {
            set_worker_city(pcity, x, y, C_TILE_UNAVAILABLE);
          }
          assert(pcity->common.city_map[x][y] == C_TILE_UNAVAILABLE);
        }
        p++;
      }
    }

    /* Initialise list of improvements with City- and Building-wide
       equiv_ranges */
    improvement_status_init(pcity->common.improvements,
                            ARRAY_SIZE(pcity->common.improvements));

    p = secfile_lookup_str_default(file, NULL,
                                   "player%d.c%d.improvements_new",
                                   plrno, i);
    if (!p) {
      /* old savegames */
      p = secfile_lookup_str(file, "player%d.c%d.improvements", plrno, i);
      for (k = 0; p[k]; k++) {
        if (p[k] == '1') {
          name = old_impr_type_name(k);
          id = find_improvement_by_name_orig(name);
          if (id != -1) {
            city_add_improvement(pcity, id);
          }
        }
      }
    } else {
      for (k = 0; k < improvement_order_size && p[k]; k++) {
        if (p[k] == '1') {
          id = find_improvement_by_name_orig(improvement_order[k]);
          if (id != -1) {
            city_add_improvement(pcity, id);
          }
        }
      }
    }

    init_worklist(&pcity->common.worklist);
    if (has_capability("worklists2", savefile_options)) {
      worklist_load(file, "player%d.c%d", plrno, i, &pcity->common.worklist);
    } else {
      worklist_load_old(file, "player%d.c%d.worklist%d",
                        plrno, i, &pcity->common.worklist);
    }

    /* FIXME: remove this when the urgency is properly recalculated. */
    pcity->u.server.ai.urgency =
      secfile_lookup_int_default(file, 0, "player%d.c%d.ai.urgency", plrno, i);

    map_set_city(pcity->common.tile, pcity);

    city_list_append(plr->cities, pcity);
  }

  load_player_units(plr, plrno, file);

  /* Toss any existing attribute_block (should not exist) */
  if (plr->attribute_block.data) {
    free(plr->attribute_block.data);
    plr->attribute_block.data = NULL;
  }

  /* This is a big heap of opaque data for the client, check everything! */
  plr->attribute_block.length = secfile_lookup_int_default(
      file, 0, "player%d.attribute_v2_block_length", plrno);

  if (0 > plr->attribute_block.length) {
    freelog(LOG_ERROR, "player%d.attribute_v2_block_length=%d too small",
            plrno,
            plr->attribute_block.length);
    plr->attribute_block.length = 0;
  } else if (MAX_ATTRIBUTE_BLOCK < plr->attribute_block.length) {
    freelog(LOG_ERROR, "player%d.attribute_v2_block_length=%d too big (max %d)",
            plrno,
            plr->attribute_block.length,
            MAX_ATTRIBUTE_BLOCK);
    plr->attribute_block.length = 0;
  } else if (0 < plr->attribute_block.length) {
    int part_nr, parts;
    size_t quoted_length;
    char *quoted;

    plr->attribute_block.data = wc_malloc(plr->attribute_block.length);

    quoted_length = secfile_lookup_int
        (file, "player%d.attribute_v2_block_length_quoted", plrno);
    quoted = (char*)wc_malloc(quoted_length + 1);
    quoted[0] = '\0';

    parts =
        secfile_lookup_int(file, "player%d.attribute_v2_block_parts", plrno);

    for (part_nr = 0; part_nr < parts; part_nr++) {
      char *current = secfile_lookup_str(file,
                                         "player%d.attribute_v2_block_data.part%d",
                                         plrno, part_nr);
      if (!current) {
        freelog(LOG_ERROR, "attribute_v2_block_parts=%d actual=%d",
                parts,
                part_nr);
        break;
      }
      freelog(LOG_DEBUG, "attribute_v2_block_length_quoted=%lu have=%lu part=%lu",
              (unsigned long) quoted_length,
              (unsigned long) strlen(quoted),
              (unsigned long) strlen(current));
      assert(strlen(quoted) + strlen(current) <= quoted_length);
      strcat(quoted, current);
    }
    if (quoted_length != strlen(quoted)) {
      freelog(LOG_FATAL, "attribute_v2_block_length_quoted=%lu actual=%lu",
              (unsigned long) quoted_length,
              (unsigned long) strlen(quoted));
      assert(0);
    }

#ifndef NDEBUG
    unquote_block(quoted,
                  plr->attribute_block.data,
                  plr->attribute_block.length);
#else
    assert(plr->attribute_block.length
           == unquote_block(quoted, plr->attribute_block.data,
                            plr->attribute_block.length));
#endif
    free(quoted);
  }

  plr->is_civil_war_split = secfile_lookup_bool_default(file, FALSE,
      "player%d.is_civil_war_split", plrno);

  if (has_capability("fcdb_save", savefile_options)) {
    plr->wcdb.player_id = secfile_lookup_int_default(file, 0,
        "player%d.fcdb_player_id", plrno);
  }
}

/***************************************************************
...
***************************************************************/
static void savegame_assign_random_nation(player_t *pplayer)
{
  Nation_Type_id i, nation = NO_NATION_SELECTED;
  int n = 0;
  bool used;

  for (i = 0; i < game.ruleset_control.nation_count; i++) {
    used = FALSE;
    players_iterate(pplayer) {
      if (pplayer->nation == i) {
        used = TRUE;
        break;
      }
    } players_iterate_end;
    if (!used) {
      if (myrand(++n) == 0) {
      nation = i;
      }
    }
  }

  if (nation == NO_NATION_SELECTED) {
    die("Cannot attribute any nation for player %s", pplayer->name);
  } else {
    freelog(LOG_VERBOSE, "Player %s's nation changed to %s",
            pplayer->name, get_nation_name_orig(nation));
    pplayer->nation = nation;
    init_tech(pplayer);
    give_initial_techs(pplayer);
  }
}

/**********************************************************************
The private map for fog of war
***********************************************************************/
static void player_map_load(player_t *plr, int plrno,
                            struct section_file *file)
{
  int i;

  if (!plr->is_alive)
    whole_map_iterate(ptile) {
      map_change_seen(ptile, plr, +1);
    } whole_map_iterate_end;

  /* load map if:
     1) it from a fog of war build
     2) fog of war was on (otherwise the private map wasn't saved)
     3) is not from a "unit only" fog of war save file
  */
  if (secfile_lookup_int_default(file, -1, "game.fogofwar") != -1
      && game.server.fogofwar == TRUE
      && secfile_lookup_int_default(file, -1,"player%d.total_ncities", plrno) != -1
      && secfile_lookup_bool_default(file, TRUE, "game.save_private_map")) {
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "player%d.map_t%03d",
                                     plrno, nat_y),
                  map_get_player_tile(ptile, plr)->terrain =
                  char2terrain(ch));

    /* get 4-bit segments of 12-bit "special" field. */
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "player%d.map_l%03d",
                                     plrno, nat_y),
                  map_get_player_tile(ptile, plr)->special =
                      static_cast<tile_special_type>(ascii_hex2bin(ch, 0)));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str(file, "player%d.map_u%03d",
                                     plrno, nat_y),
                  map_get_player_tile(ptile, plr)->special =
                      static_cast<tile_special_type>(
                          map_get_player_tile(ptile, plr)->special
                          | ascii_hex2bin(ch, 1)));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str_default
                  (file, NULL, "player%d.map_n%03d", plrno, nat_y),
                  map_get_player_tile(ptile, plr)->special =
                      static_cast<tile_special_type>(
                          map_get_player_tile(ptile, plr)->special
                          | ascii_hex2bin(ch, 2)));

    /* get 4-bit segments of 16-bit "updated" field */
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str
                  (file, "player%d.map_ua%03d", plrno, nat_y),
                  map_get_player_tile(ptile, plr)->last_updated =
                  ascii_hex2bin(ch, 0));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str
                  (file, "player%d.map_ub%03d", plrno, nat_y),
                  map_get_player_tile(ptile, plr)->last_updated |=
                  ascii_hex2bin(ch, 1));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str
                  (file, "player%d.map_uc%03d", plrno, nat_y),
                  map_get_player_tile(ptile, plr)->last_updated |=
                  ascii_hex2bin(ch, 2));
    LOAD_MAP_DATA(ch, nat_y, ptile,
                  secfile_lookup_str
                  (file, "player%d.map_ud%03d", plrno, nat_y),
                  map_get_player_tile(ptile, plr)->last_updated |=
                  ascii_hex2bin(ch, 3));

    {
      int j;
      struct dumb_city *pdcity;
      i = secfile_lookup_int(file, "player%d.total_ncities", plrno);
      for (j = 0; j < i; j++) {
        int nat_x, nat_y;
        tile_t *ptile;

        nat_x = secfile_lookup_int(file, "player%d.dc%d.x", plrno, j);
        nat_y = secfile_lookup_int(file, "player%d.dc%d.y", plrno, j);
        ptile = native_pos_to_tile(nat_x, nat_y);

        pdcity = (dumb_city*)wc_malloc(sizeof(struct dumb_city));
        pdcity->id = secfile_lookup_int(file, "player%d.dc%d.id", plrno, j);
        sz_strlcpy(pdcity->name, secfile_lookup_str(file, "player%d.dc%d.name", plrno, j));
        pdcity->size = secfile_lookup_int(file, "player%d.dc%d.size", plrno, j);
        pdcity->has_walls = secfile_lookup_bool(file, "player%d.dc%d.has_walls", plrno, j);
        pdcity->occupied = secfile_lookup_bool_default(file, FALSE,
                                        "player%d.dc%d.occupied", plrno, j);
        pdcity->happy = secfile_lookup_bool_default(file, FALSE,
                                        "player%d.dc%d.happy", plrno, j);
        pdcity->unhappy = secfile_lookup_bool_default(file, FALSE,
                                        "player%d.dc%d.unhappy", plrno, j);
        pdcity->owner = secfile_lookup_int(file, "player%d.dc%d.owner", plrno, j);
        map_get_player_tile(ptile, plr)->city = pdcity;
        alloc_id(pdcity->id);
      }
    }

    /* This shouldn't be neccesary if the savegame was consistent, but there
       is a bug in some pre-1.11 savegames. Anyway, it can't hurt */
    whole_map_iterate(ptile) {
      if (map_is_known_and_seen(ptile, plr)) {
        update_player_tile_knowledge(plr, ptile);
        reality_check_city(plr, ptile);
        if (map_get_city(ptile)) {
          update_dumb_city(plr, map_get_city(ptile));
        }
      }
    } whole_map_iterate_end;

  } else {
    /* We have an old savegame or fog of war was turned off; the
       players private knowledge is set to be what he could see
       without fog of war */
    whole_map_iterate(ptile) {
      if (map_is_known(ptile, plr)) {
        city_t *pcity = map_get_city(ptile);
        update_player_tile_last_seen(plr, ptile);
        update_player_tile_knowledge(plr, ptile);
        if (pcity)
          update_dumb_city(plr, pcity);
      }
    } whole_map_iterate_end;
  }
}

/***************************************************************
 For each city and tile, sort unit lists according to
 ord_city and ord_map values.
***************************************************************/
static void apply_unit_ordering(void)
{
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      unit_list_sort_ord_city(pcity->common.units_supported);
    } city_list_iterate_end;
  } players_iterate_end;

  whole_map_iterate(ptile) {
    unit_list_sort_ord_map(ptile->units);
  } whole_map_iterate_end;
}

/***************************************************************
Old savegames have defects...
***************************************************************/
static void check_city(city_t *pcity)
{
  city_map_iterate(x, y) {
    bool res = city_can_work_tile(pcity, x, y);
    switch (pcity->common.city_map[x][y]) {
    case C_TILE_EMPTY:
      if (!res) {
        set_worker_city(pcity, x, y, C_TILE_UNAVAILABLE);
        freelog(LOG_DEBUG, "unavailable tile marked as empty!");
      }
      break;
    case C_TILE_WORKER:
      if (!res) {
        tile_t *ptile;

        pcity->common.specialists[SP_ELVIS]++;
        set_worker_city(pcity, x, y, C_TILE_UNAVAILABLE);
        freelog(LOG_DEBUG, "Worked tile was unavailable!");

        ptile = city_map_to_map(pcity, x, y);

        map_city_radius_iterate(ptile, tile2) {
          city_t *pcity2 = map_get_city(tile2);
          if (pcity2)
            check_city(pcity2);
        } map_city_radius_iterate_end;
      }
      break;
    case C_TILE_UNAVAILABLE:
      if (res) {
        set_worker_city(pcity, x, y, C_TILE_EMPTY);
        freelog(LOG_DEBUG, "Empty tile Marked as unavailable!");
      }
      break;
    }
  } city_map_iterate_end;

  city_refresh(pcity);
}

/***************************************************************
...
***************************************************************/
void game_load(struct section_file *file)
{
  int i, k, id;
  enum server_states tmp_server_state;
  char *savefile_options;
  const char *string;
  char **improvement_order = NULL;
  int improvement_order_size = 0;
  char **technology_order = NULL;
  int technology_order_size = 0;
  const char *name;

  map.server.generator = 0;
  map.server.is_fixed = TRUE;

  game.server.version = secfile_lookup_int_default(file, 0, "game.version");
  tmp_server_state = (enum server_states)
      secfile_lookup_int_default(file, RUN_GAME_STATE, "game.server_state");

  savefile_options = secfile_lookup_str(file, "savefile.options");
  if (has_capability("improvement_order", savefile_options)) {
    improvement_order = secfile_lookup_str_vec(file, &improvement_order_size,
                                               "savefile.improvement_order");
  }
  if (has_capability("technology_order", savefile_options)) {
    technology_order = secfile_lookup_str_vec(file, &technology_order_size,
                                              "savefile.technology_order");
  }

  /* we require at least version 1.9.0 */
  if (REQUIERED_GAME_VERSION > game.server.version) {
    freelog(LOG_FATAL, _("Savegame too old, at least version 1.9.0 required."));
    exit(EXIT_FAILURE);         /* WTF exit?! XXX */
  }

  set_meta_patches_string(secfile_lookup_str_default(file,
      default_meta_patches_string(), "game.metapatches"));

  game.server.meta_info.user_message_set =
      secfile_lookup_bool_default(file, FALSE, "game.user_metamessage");
  if (game.server.meta_info.user_message_set) {
    set_user_meta_message_string(secfile_lookup_str_default(file,
        default_meta_message_string(), "game.metamessage"));
  } else {
    /* To avoid warnings when loading pre-2.0.10 savegames */
    secfile_lookup_str_default(file, "", "game.metamessage");
  }
  set_meta_topic_string(secfile_lookup_str_default(file,
      default_meta_topic_string(), "game.metatopic"));

  sz_strlcpy(server_arg.metaserver_addr,
             secfile_lookup_str_default(file, DEFAULT_META_SERVER_ADDR,
                                        "game.metaserver"));

  game.info.gold = secfile_lookup_int(file, "game.gold");
  game.info.tech = secfile_lookup_int(file, "game.tech");
  game.info.skill_level = secfile_lookup_int(file, "game.skill_level");
  if (game.info.skill_level == 0) {
    game.info.skill_level = GAME_OLD_DEFAULT_SKILL_LEVEL;
  }

  game.info.timeout = secfile_lookup_int(file, "game.timeout");
  game.server.timeoutint = secfile_lookup_int_default(file,
      GAME_DEFAULT_TIMEOUTINT, "game.timeoutint");
  game.server.timeoutintinc = secfile_lookup_int_default(file,
      GAME_DEFAULT_TIMEOUTINTINC, "game.timeoutintinc");
  game.server.timeoutinc = secfile_lookup_int_default(file,
      GAME_DEFAULT_TIMEOUTINC, "game.timeoutinc");
  game.server.timeoutincmult = secfile_lookup_int_default(file,
      GAME_DEFAULT_TIMEOUTINCMULT, "game.timeoutincmult");
  game.server.timeoutcounter =
      secfile_lookup_int_default(file, 1, "game.timeoutcounter");

  game.server.timeoutaddenemymove = secfile_lookup_int_default(file,
      game.server.timeoutaddenemymove, "game.timeoutaddenemymove");

  game.info.end_year = secfile_lookup_int(file, "game.end_year");
  game.info.researchcost = secfile_lookup_int_default(file, 0, "game.researchcost");
  if (game.info.researchcost == 0) {
    game.info.researchcost = secfile_lookup_int(file, "game.techlevel");
  }

  game.info.year = secfile_lookup_int(file, "game.year");

  if (has_capability("turn", savefile_options)) {
    game.info.turn = secfile_lookup_int(file, "game.turn");
  } else {
    game.info.turn = -2;
  }

  game.info.min_players = secfile_lookup_int(file, "game.min_players");
  game.info.max_players = secfile_lookup_int(file, "game.max_players");
  game.info.nplayers = secfile_lookup_int(file, "game.nplayers");
  game.info.globalwarming = secfile_lookup_int(file, "game.globalwarming");
  game.server.warminglevel = secfile_lookup_int(file, "game.warminglevel");
  game.info.nuclearwinter =
      secfile_lookup_int_default(file, 0, "game.nuclearwinter");
  game.server.coolinglevel = secfile_lookup_int_default(file, 8, "game.coolinglevel");
  game.ruleset_control.notradesize = secfile_lookup_int_default(file, 0, "game.notradesize");
  game.ruleset_control.fulltradesize =
      secfile_lookup_int_default(file, 1, "game.fulltradesize");
  game.info.unhappysize = secfile_lookup_int(file, "game.unhappysize");
  game.info.angrycitizen =
      secfile_lookup_bool_default(file, FALSE, "game.angrycitizen");

  if (game.server.version >= 10100) {
    game.info.cityfactor = secfile_lookup_int(file, "game.cityfactor");
    game.info.diplcost = secfile_lookup_int(file, "game.diplcost");
    game.info.freecost = secfile_lookup_int(file, "game.freecost");
    game.info.conquercost = secfile_lookup_int(file, "game.conquercost");
    game.info.foodbox = secfile_lookup_int(file, "game.foodbox");
    game.info.techpenalty = secfile_lookup_int(file, "game.techpenalty");
    game.server.razechance = secfile_lookup_int(file, "game.razechance");

    /* suppress warnings about unused entries in old savegames: */
    (void) section_file_lookup(file, "game.rail_food");
    (void) section_file_lookup(file, "game.rail_prod");
    (void) section_file_lookup(file, "game.rail_trade");
    (void) section_file_lookup(file, "game.farmfood");
  }
  if (game.server.version >= 10300) {
    game.info.civstyle = secfile_lookup_int_default(file, 0, "game.civstyle");
    game.server.save_nturns = secfile_lookup_int(file, "game.save_nturns");
  }

  game.server.citymindist = secfile_lookup_int_default(file,
      GAME_DEFAULT_CITYMINDIST, "game.citymindist");

  game.server.rapturedelay = secfile_lookup_int_default(file,
      GAME_DEFAULT_RAPTUREDELAY, "game.rapturedelay");

  /* National borders setting. */
  game.ruleset_control.borders = secfile_lookup_int_default(file, 0, "game.borders");
  game.ruleset_control.happyborders = secfile_lookup_bool_default(file, FALSE,
      "game.happyborders");

  /* Diplomacy. */
  game.info.diplomacy = secfile_lookup_int_default(file,
      GAME_DEFAULT_DIPLOMACY, "game.diplomacy");
  game.ext_info.maxallies = secfile_lookup_int_default(file,
      GAME_DEFAULT_MAXALLIES, "game.maxallies");

  if (has_capability("watchtower", savefile_options)) {
    game.server.watchtower_extra_vision =
        secfile_lookup_int_default(file, 0, "game.watchtower_extra_vision");
    game.server.watchtower_vision =
        secfile_lookup_int_default(file, 1, "game.watchtower_vision");
  } else {
    game.server.watchtower_extra_vision = 0;
    game.server.watchtower_vision = 1;
  }

  sz_strlcpy(game.server.save_name, secfile_lookup_str_default(file,
      GAME_DEFAULT_SAVE_NAME, "game.save_name"));

  game.server.aifill = secfile_lookup_int_default(file, 0, "game.aifill");

  game.server.scorelog = secfile_lookup_bool_default(file, FALSE, "game.scorelog");
  sz_strlcpy(game.server.id, secfile_lookup_str_default(file, "", "game.id"));

  game.server.fogofwar = secfile_lookup_bool_default(file, FALSE, "game.fogofwar");
  game.server.fogofwar_old = game.server.fogofwar;

  game.server.civilwarsize = secfile_lookup_int_default(file,
      GAME_DEFAULT_CIVILWARSIZE, "game.civilwarsize");
  game.server.contactturns = secfile_lookup_int_default(file,
      GAME_DEFAULT_CONTACTTURNS, "game.contactturns");

  if (has_capability("diplchance_percent", savefile_options)) {
    game.server.diplchance = secfile_lookup_int_default(file, game.server.diplchance,
                                                 "game.diplchance");
  } else {
    game.server.diplchance = secfile_lookup_int_default(file, 3, /* old default */
                                                 "game.diplchance");
    if (game.server.diplchance < 2) {
      game.server.diplchance = GAME_MAX_DIPLCHANCE;
    } else if (game.server.diplchance > 10) {
      game.server.diplchance = GAME_MIN_DIPLCHANCE;
    } else {
      game.server.diplchance = 100 - (10 * (game.server.diplchance - 1));
    }
  }
  game.server.dipldefchance = secfile_lookup_int_default(file,
      game.server.dipldefchance, "game.dipldefchance");
  game.server.spyreturnchance = secfile_lookup_int_default(file,
      game.server.spyreturnchance, "game.spyreturnchance");
  game.server.diplbribechance = secfile_lookup_int_default(file,
      game.server.diplbribechance, "game.diplbribechance");
  game.server.diplincitechance = secfile_lookup_int_default(file,
      game.server.diplincitechance, "game.diplincitechance");
  game.server.aqueductloss = secfile_lookup_int_default(file, game.server.aqueductloss,
                                                 "game.aqueductloss");
  game.server.killcitizen = secfile_lookup_int_default(file, game.server.killcitizen,
                                                "game.killcitizen");
  game.server.savepalace = secfile_lookup_bool_default(file, game.server.savepalace,
                                                "game.savepalace");
  game.server.turnblock = secfile_lookup_bool_default(file, game.server.turnblock,
                                               "game.turnblock");
  game.server.fixedlength = secfile_lookup_bool_default(file, game.server.fixedlength,
                                                 "game.fixedlength");
  game.server.barbarianrate = secfile_lookup_int_default(file, game.server.barbarianrate,
                                                  "game.barbarians");
  game.server.onsetbarbarian = secfile_lookup_int_default(file, game.server.onsetbarbarian,
                                                   "game.onsetbarbs");
  game.server.revolution_length = secfile_lookup_int_default(file,
      game.server.revolution_length, "game.revolen");

  /* counted in player_load for compatibility with 1.10.0 */
  game.server.nbarbarians = 0;

  game.server.occupychance = secfile_lookup_int_default(file, game.server.occupychance,
                                                 "game.occupychance");
  game.server.seed = secfile_lookup_int_default(file, game.server.seed, "game.randseed");
  game.server.allowed_city_names = secfile_lookup_int_default(file,
      game.server.allowed_city_names, "game.allowed_city_names");

  if (game.info.civstyle == 1) {
    string = "civ1";
  } else {
    string = "default";
    game.info.civstyle = GAME_DEFAULT_CIVSTYLE;
  }

  if (!has_capability("rulesetdir", savefile_options)) {
    char *str2, *str;

    str = secfile_lookup_str_default(file, "default", "game.ruleset.techs");

    if (strcmp("classic",
               secfile_lookup_str_default(file, "default",
                                          "game.ruleset.terrain")) == 0) {
      freelog(LOG_FATAL, _("The savegame uses the classic terrain "
                           "ruleset which is no longer supported."));
      exit(EXIT_FAILURE); /* WTF again exit? XXX */
    }
#define T(x) \
      str2 = secfile_lookup_str_default(file, "default", x); \
      if (strcmp(str, str2) != 0) { \
        freelog(LOG_NORMAL, _("Warning: Different rulesetdirs " \
                              "('%s' and '%s') are no longer supported. " \
                              "Using '%s'."), \
                              str, str2, str); \
      }

    T("game.ruleset.units");
    T("game.ruleset.buildings");
    T("game.ruleset.terrain");
    T("game.ruleset.governments");
    T("game.ruleset.nations");
    T("game.ruleset.cities");
    T("game.ruleset.game");
#undef T

    sz_strlcpy(game.server.rulesetdir, str);
  } else {
    sz_strlcpy(game.server.rulesetdir,
               secfile_lookup_str_default(file, string, "game.rulesetdir"));
  }

  sz_strlcpy(game.server.demography, secfile_lookup_str_default(file,
      GAME_DEFAULT_DEMOGRAPHY, "game.demography"));
  load_user_allow_behavior_state(file);

  game.info.spacerace = secfile_lookup_bool_default(file, game.info.spacerace,
                                               "game.spacerace");

  game.server.auto_ai_toggle = secfile_lookup_bool_default(file, game.server.auto_ai_toggle,
                                                    "game.auto_ai_toggle");

  game.info.heating = 0;
  game.info.cooling = 0;

  if (!game.server.ruleset_loaded) {
    load_rulesets();
  }

  if (game.server.version >= 10300) {
    if (!has_capability("startunits", savefile_options)) {
      int settlers = secfile_lookup_int(file, "game.settlers");
      int explorer = secfile_lookup_int(file, "game.explorer");
      int i;
      for (i = 0; settlers > 0 && i < (MAX_LEN_STARTUNIT - 1);
           i++, settlers--) {
        game.server.start_units[i] = 'c';
      }
      for (; explorer > 0 && i < (MAX_LEN_STARTUNIT - 1); i++, explorer--) {
        game.server.start_units[i] = 'x';
      }
      game.server.start_units[i] = '\0';
    } else {
      sz_strlcpy(game.server.start_units, secfile_lookup_str_default(file,
          GAME_DEFAULT_START_UNITS, "game.start_units"));
    }
    game.server.dispersion = secfile_lookup_int_default(file,
        GAME_DEFAULT_DISPERSION, "game.dispersion");

    /* Warserver settings */
    game.traderoute_info.trademindist = secfile_lookup_int_default(file,
        GAME_DEFAULT_TRADEMINDIST, "game.trademindist");
    game.traderoute_info.traderevenuepct = secfile_lookup_int_default(file,
        GAME_DEFAULT_TRADEREVENUEPCT, "game.traderevenuepct");
    game.traderoute_info.traderevenuestyle = secfile_lookup_int_default(file,
        GAME_DEFAULT_TRADEREVENUESTYLE, "game.traderevenuestyle");
    game.traderoute_info.caravanbonusstyle = secfile_lookup_int_default(file,
        GAME_DEFAULT_CARAVANBONUSSTYLE, "game.caravanbonusstyle");
    game.traderoute_info.maxtraderoutes = secfile_lookup_int_default(file,
        GAME_DEFAULT_MAXTRADEROUTES, "game.maxtraderoutes");
    game.ext_info.futuretechsscore = secfile_lookup_bool_default(file,
        GAME_DEFAULT_FUTURETECHSSCORE, "game.futuretechsscore");
    game.ext_info.improvedautoattack = secfile_lookup_bool_default(file,
        GAME_DEFAULT_IMPROVEDAUTOATTACK, "game.improvedautoattack");
    game.ext_info.stackbribing = secfile_lookup_bool_default(file,
        GAME_DEFAULT_STACKBRIBING, "game.stackbribing");
    game.ext_info.experimentalbribingcost = secfile_lookup_bool_default(file,
        GAME_DEFAULT_EXPERIMENTALBRIBINGCOST, "game.experimentalbribingcost");
    game.ext_info.techtrading = secfile_lookup_bool_default(file,
        GAME_DEFAULT_TECHTRADING, "game.techtrading");
    game.ext_info.goldtrading = secfile_lookup_bool_default(file,
        GAME_DEFAULT_GOLDTRADING, "game.goldtrading");
    game.ext_info.citytrading = secfile_lookup_bool_default(file,
        GAME_DEFAULT_CITYTRADING, "game.citytrading");
    game.ext_info.airliftingstyle = secfile_lookup_int_default(file,
        GAME_DEFAULT_AIRLIFTINGSTYLE, "game.airliftingstyle");
    game.ext_info.teamplacement = secfile_lookup_bool_default(file,
        GAME_DEFAULT_TEAMPLACEMENT, "game.teamplacement");
    game.ext_info.globalwarmingon = secfile_lookup_bool_default(file,
        GAME_DEFAULT_GLOBALWARMINGON, "game.globalwarmingon");
    game.ext_info.nuclearwinteron = secfile_lookup_bool_default(file,
        GAME_DEFAULT_NUCLEARWINTERON, "game.nuclearwinteron");
    game.server.bruteforcethreshold = secfile_lookup_int_default(file,
        GAME_DEFAULT_BRUTEFORCETHRESHOLD, "game.bruteforcethreshold");
    game.server.iterplacementcoefficient = secfile_lookup_int_default(file,
        GAME_DEFAULT_ITERPLACEMENTCOEFFICIENT,
        "game.iterplacementcoefficient");
    game.server.teamplacementtype = secfile_lookup_int_default(file,
        GAME_DEFAULT_TEAMPLACEMENTTYPE, "game.teamplacementtype");
    game.ext_info.techleakagerate = secfile_lookup_int_default(file,
        GAME_DEFAULT_TECHLEAKAGERATE, "game.techleakagerate");
    game.server.triremestyle = secfile_lookup_int_default(file,
        GAME_DEFAULT_TRIREMESTYLE, "game.triremestyle");
    game.server.fracmovestyle = secfile_lookup_int_default(file,
        GAME_DEFAULT_FRACMOVESTYLE, "game.fracmovestyle");
    game.server.endturn = secfile_lookup_int_default(file,
        GAME_DEFAULT_ENDTURN, "game.endturn");
    game.server.revealmap = secfile_lookup_int_default(file,
        GAME_DEFAULT_REVEALMAP, "game.revealmap");
    game.server.civilwar = secfile_lookup_int_default(file,
        GAME_DEFAULT_CIVILWAR, "game.civilwar");

    /* Warserver 'ignoreruleset' settings. */
    game.ext_info.ignoreruleset = secfile_lookup_bool_default(file,
        GAME_DEFAULT_IGNORERULESET, "game.ignoreruleset");
    if (game.ext_info.ignoreruleset) {
      /* Overwrite ruleset settings */
      game.ruleset_control.slow_invasions = secfile_lookup_bool_default(file,
        GAME_DEFAULT_SLOWINVASIONS, "game.slowinvasions");
      game.ruleset_game.killstack = secfile_lookup_bool_default(file,
        GAME_DEFAULT_KILLSTACK, "game.killstack");
      game.ruleset_game.tech_leakage = secfile_lookup_int_default(file,
        GAME_DEFAULT_TECHLEAKAGE, "game.techleakage");
      game.ruleset_game.tech_cost_style = secfile_lookup_int_default(file,
        GAME_DEFAULT_TECHCOSTSTYLE, "game.techcoststyle");
    }

    if (has_capability("wcdb_save", savefile_options)) {
      game.server.wcdb.id = secfile_lookup_int_default(file,
          0, "game.server_wcdb_id");
      game.server.wcdb.type = (game_types)
          secfile_lookup_int_default(file,
                                     0, "game.server_wcdb_type");
    }

    map.info.topology_id = secfile_lookup_int_default(file,
        MAP_ORIGINAL_TOPO, "map.topology_id");
    map.server.size = secfile_lookup_int_default(file,
        MAP_DEFAULT_SIZE, "map.size");
    map.server.riches = secfile_lookup_int(file, "map.riches");
    map.server.huts = secfile_lookup_int(file, "map.huts");
    map.server.generator = secfile_lookup_int(file, "map.generator");
    map.server.seed = secfile_lookup_int(file, "map.seed");
    map.server.landpercent = secfile_lookup_int(file, "map.landpercent");
    map.server.wetness = secfile_lookup_int_default(file,
        MAP_DEFAULT_WETNESS, "map.wetness");
    map.server.steepness = secfile_lookup_int_default(file,
        MAP_DEFAULT_STEEPNESS, "map.steepness");
    map.server.have_huts = secfile_lookup_bool_default(file,
        TRUE, "map.have_huts");
    map.server.temperature = secfile_lookup_int_default(file,
        MAP_DEFAULT_TEMPERATURE, "map.temperature");
    map.server.alltemperate = secfile_lookup_bool_default(file,
        MAP_DEFAULT_ALLTEMPERATE, "map.alltemperate");
    map.server.tinyisles = secfile_lookup_bool_default(file,
        MAP_DEFAULT_TINYISLES, "map.tinyisles");
    map.server.separatepoles = secfile_lookup_bool_default(file,
        MAP_DEFAULT_SEPARATE_POLES, "map.separatepoles");
    map.server.startpos = secfile_lookup_int_default(file, 4, "map.startpos");

    if (has_capability("startoptions", savefile_options)) {
      map.info.xsize = secfile_lookup_int(file, "map.width");
      map.info.ysize = secfile_lookup_int(file, "map.height");
    } else {
      /* old versions saved with these names in PRE_GAME_STATE: */
      map.info.xsize = secfile_lookup_int(file, "map.xsize");
      map.info.ysize = secfile_lookup_int(file, "map.ysize");
    }

    if (tmp_server_state == PRE_GAME_STATE && map.server.generator == 0) {
      /* generator 0 = map done with map editor */
      /* aka a "scenario" */
      if (has_capability("specials", savefile_options)) {
        map_load(file);
        return;
      }
      map_tiles_load(file);
      if (has_capability("riversoverlay", savefile_options)) {
        map_rivers_overlay_load(file);
      }
      if (has_capability("startpos", savefile_options)) {
        map_startpos_load(file);
      }
      return;
    }
  }
  if (tmp_server_state == PRE_GAME_STATE) {
    return;
  }

  /* We check
     1) if the block exists at all.
     2) if it is saved. */
  if (section_file_lookup(file, "random.index_J")
      && secfile_lookup_bool_default(file, TRUE, "game.save_random")) {
    RANDOM_STATE rstate;
    rstate.j = secfile_lookup_int(file, "random.index_J");
    rstate.k = secfile_lookup_int(file, "random.index_K");
    rstate.x = secfile_lookup_int(file, "random.index_X");
    for (i = 0; i < 8; i++) {
      char name[20];
      my_snprintf(name, sizeof(name), "random.table%d", i);
      string = secfile_lookup_str(file, "%s", name);
      sscanf(string, "%8x %8x %8x %8x %8x %8x %8x", &rstate.v[7 * i],
             &rstate.v[7 * i + 1], &rstate.v[7 * i + 2], &rstate.v[7 * i + 3],
             &rstate.v[7 * i + 4], &rstate.v[7 * i + 5], &rstate.v[7 * i + 6]);
    }
    rstate.is_init = TRUE;
    set_myrand_state(rstate);
  } else {
    /* mark it */
    (void) secfile_lookup_bool_default(file, TRUE, "game.save_random");

    /* We're loading a running game without a seed (which is okay, if it's
     * a scenario).  We need to generate the game.server.seed now because it will
     * be needed later during the load. */
    if (tmp_server_state == RUN_GAME_STATE) {
      init_game_seed();
    }
  }


  game.server.is_new_game = !secfile_lookup_bool_default(file,
      TRUE, "game.save_players");

  if (!game.server.is_new_game) {      /* If new game, this is done in srv_main.c */
    /* Initialise lists of improvements with World and Island equiv_ranges */
    improvement_status_init(game.improvements, ARRAY_SIZE(game.improvements));
  }

  map_load(file);

  if (!game.server.is_new_game) {
    /* destroyed wonders: */
    string = secfile_lookup_str_default(file,
        NULL, "game.destroyed_wonders_new");
    if (!string) {
      /* old savegames */
      string = secfile_lookup_str_default(file, "", "game.destroyed_wonders");
      for (k = 0; string[k]; k++) {
        if (string[k] == '1') {
          name = old_impr_type_name(k);
          id = find_improvement_by_name_orig(name);
          if (id != -1) {
            game.info.global_wonders[id] = -1;
          }
        }
      }
    } else {
      for (k = 0; k < improvement_order_size && string[k]; k++) {
        if (string[k] == '1') {
          id = find_improvement_by_name_orig(improvement_order[k]);
          if (id != -1) {
            game.info.global_wonders[id] = -1;
          }
        }
      }
    }

    /* This is done after continents are assigned, but before effects
     * are added. */
    allot_island_improvs();

    for (i = 0; i < game.info.nplayers; i++) {
      player_load(&game.players[i], i, file, improvement_order,
                  improvement_order_size, technology_order,
                  technology_order_size);
    }

    /* Attribute nations to player which their nation wasn't loaded correctly */
    players_iterate(pplayer) {
      if (pplayer->nation == NO_NATION_SELECTED) {
        savegame_assign_random_nation(pplayer);
      }
    } players_iterate_end;

    /* Update all city information.  This must come after all cities are
     * loaded (in player_load) but before player (dumb) cities are loaded
     * (in player_map_load).  Cities are refreshed twice to account for
     * trade routes: the first refresh initializes all cities tile_trade
     * values; the second correctly updates all trade routes. */
    cities_iterate(pcity) {
      generic_city_refresh(pcity, FALSE, NULL);
    } cities_iterate_end;
    cities_iterate(pcity) {
      generic_city_refresh(pcity, FALSE, NULL);
    } cities_iterate_end;

    /* Since the cities must be placed on the map to put them on the
       player map we do this afterwards */
    for (i = 0; i < game.info.nplayers; i++) {
      player_map_load(&game.players[i], i, file);
    }

    /* We do this here since if the did it in player_load, player 1
       would try to unfog (unloaded) player 2's map when player 1's units
       were loaded */
    players_iterate(pplayer) {
      pplayer->really_gives_vision = 0;
      pplayer->gives_shared_vision = 0;
    } players_iterate_end;
    players_iterate(pplayer) {
      char *vision;
      int plrno = pplayer->player_no;

      vision = secfile_lookup_str_default(file,
          NULL, "player%d.gives_shared_vision", plrno);
      if (vision) {
        players_iterate(pplayer2) {
          if (vision[pplayer2->player_no] == '1') {
            give_shared_vision(pplayer, pplayer2);
          }
        } players_iterate_end;
      }
    } players_iterate_end;

    initialize_globals();
    apply_unit_ordering();

    /* Rebuild national borders. */
    map_calculate_borders();

    /* Make sure everything is consistent. */
    players_iterate(pplayer) {
      unit_list_iterate(pplayer->units, punit) {
        if (!can_unit_continue_current_activity(punit)) {
          freelog(LOG_ERROR, "ERROR: Unit doing illegal activity in savegame!");
          punit->activity = ACTIVITY_IDLE;
        }
      } unit_list_iterate_end;

      city_list_iterate(pplayer->cities, pcity) {
        check_city(pcity);
      } city_list_iterate_end;
    } players_iterate_end;
  } else {
    game.info.nplayers = 0;
  }

  if (secfile_lookup_int_default(file, -1, "game.shuffled_player_%d", 0) >= 0) {
    int shuffled_players[game.info.nplayers];

    for (i = 0; i < game.info.nplayers; i++) {
      shuffled_players[i]
          = secfile_lookup_int(file, "game.shuffled_player_%d", i);
    }
    set_shuffled_players(shuffled_players);
  } else {
    /* No shuffled players included, so shuffle them (this may include
     * scenarios). */
    shuffle_players();
  }

  if (!game.server.is_new_game) {
    /* Set active city improvements/wonders and their effects */
    improvements_update_obsolete();
  }

  /* Fix ferrying sanity */
  players_iterate(pplayer) {
    unit_list_iterate_safe(pplayer->units, punit) {
      unit_t *ferry = find_unit_by_id(punit->transported_by);

      if (is_ocean(map_get_terrain(punit->tile))
          && is_ground_unit(punit) && !ferry) {
        freelog(LOG_ERROR, "Removing %s's unferried %s in ocean at (%d, %d)",
                pplayer->name, unit_name(punit->type), TILE_XY(punit->tile));
        bounce_unit(punit, TRUE);
      }
    } unit_list_iterate_safe_end;
  } players_iterate_end;

  /* Fix stacking issues.  We don't rely on the savegame preserving
   * alliance invariants (old savegames often did not) so if there are any
   * unallied units on the same tile we just bounce them. */
  players_iterate(pplayer) {
    players_iterate(aplayer) {
      resolve_unit_stacks(pplayer, aplayer, TRUE);
    } players_iterate_end;
  } players_iterate_end;

  players_iterate(pplayer) {
    calc_civ_score(pplayer);
  } players_iterate_end;

  return;
}


/***************************************************************
...
***************************************************************/
bool game_loadmap(struct section_file *file)
{
  int mapversion;

  freelog(LOG_DEBUG, "Starting Mapload");
  mapversion = secfile_lookup_int_default(file, 0, "game.mapversion");
  freelog(LOG_DEBUG, "mapversion %d", mapversion);

  if (mapversion < 1) {
    freelog(LOG_ERROR, _("Not a mapfile! Loaded nothing."));
    return FALSE;
  }

  map.server.generator = 0;
  map.server.is_fixed = TRUE;

  set_meta_topic_string(secfile_lookup_str_default(file,
      default_meta_topic_string(), "game.metatopic"));
  set_meta_message_string(secfile_lookup_str_default(file,
      default_meta_message_string(), "game.metamessage"));

  sz_strlcpy(game.server.start_units,secfile_lookup_str_default(file,
      "wwwxxxccc", "game.start_units"));
  game.server.dispersion = secfile_lookup_int_default(file, 0, "game.dispersion");
  game.info.max_players = secfile_lookup_int(file, "game.max_players");

  map.info.topology_id = secfile_lookup_int_default(file,
      MAP_ORIGINAL_TOPO, "map.topology_id");
  /* map.seed = secfile_lookup_int_default(file, "map.seed"); */

  map.server.startpos = secfile_lookup_int_default(file, 4, "map.startpos");

  map.info.xsize = secfile_lookup_int(file, "map.width");
  map.info.ysize = secfile_lookup_int(file, "map.height");

  if (!game.server.ruleset_loaded) {
    freelog(LOG_DEBUG, "Starting loading rulesets");
    load_rulesets();
  }
  freelog(LOG_DEBUG, "Starting loading maptiles");
  map.server.have_specials = TRUE;
  map_tiles_load(file);

  freelog(LOG_DEBUG, "Starting loading startpositions");
  if (map.server.startpos == 5) {
    map_startpos_load(file);
    freelog(LOG_DEBUG, "Finished loading startpositions");
  }

  freelog(LOG_DEBUG, "Starting loading specials");

  /* get 4-bit segments of 16-bit "special" field. */
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str(file, "map.l%03d", nat_y),
                ptile->special = (tile_special_type)ascii_hex2bin(ch, 0));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str(file, "map.u%03d", nat_y),
                ptile->special = static_cast<tile_special_type>(
                    ptile->special | ascii_hex2bin(ch, 1)));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str_default(file, NULL, "map.n%03d", nat_y),
                ptile->special = static_cast<tile_special_type>(
                    ptile->special | ascii_hex2bin(ch, 2)));
  LOAD_MAP_DATA(ch, nat_y, ptile,
                secfile_lookup_str_default(file, NULL, "map.f%03d", nat_y),
                ptile->special = static_cast<tile_special_type>(
                    ptile->special | ascii_hex2bin(ch, 3)));

  return TRUE;
}
