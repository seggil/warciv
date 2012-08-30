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
#ifndef WC_COMMON_GAME_H
#define WC_COMMON_GAME_H

#include <time.h>       /* time_t */

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#include "shared.h"

#include "connection.h"         /* struct conn_list */
#include "wc_types.h"
#include "improvement.h"        /* Impr_Status */
#include "packets.h"
#include "player.h"

#define OVERFLIGHT_NOTHING  1
#define OVERFLIGHT_FRIGHTEN 2

#define CONTAMINATION_POLLUTION 1
#define CONTAMINATION_FALLOUT   2

/* Different ways the game can end. */
enum game_outcomes {
  GOC_NONE = 0,
  GOC_DRAWN_BY_ENDYEAR,
  GOC_DRAWN_BY_MUTUAL_DESTRUCTION,
  GOC_ENDED_BY_LONE_SURVIVAL,
  GOC_ENDED_BY_SPACESHIP,
  GOC_ENDED_BY_VOTE,
  GOC_ENDED_BY_TEAM_VICTORY,
  GOC_ENDED_BY_ALLIED_VICTORY,

  GOC_NUM_OUTCOMES
};

enum game_types {
  GT_FFA, /* i.e. free for all */
  GT_TEAM,
  GT_DUEL,
  GT_SOLO,

  GT_MIXED, /* everything else */

  GT_NUM_TYPES
};

bool game_type_supports_rating(enum game_types gtype);

struct civ_game {
  /* Main game packets */
  struct packet_game_info info; /* Use send_game_info() */
  struct packet_extgame_info ext_info;
  struct packet_traderoute_info traderoute_info;
  struct packet_ruleset_control ruleset_control;
  struct packet_ruleset_game ruleset_game;

  /* Common part */
  struct player players[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  struct conn_list *all_connections;  /* including not yet established */
  struct conn_list *est_connections;  /* all established client conns */
  struct conn_list *game_connections; /* involved in game; send map etc */

  /* global_wonders[] may also be (-1), or the id of a city
   * which no longer exists, if the wonder has been destroyed */
  Impr_Status improvements[B_LAST];   /* impr. with equiv_range==World */
  Impr_Type_id palace_building;
  Impr_Type_id land_defend_building;

  /* Server side only */
  struct civ_game_server {
    bool is_new_game;    /* TRUE for games never started */
    int version;         /* Savegame version */
    char id[MAX_ID_LEN]; /* server only */
    char start_units[MAX_LEN_STARTUNIT];
    int dispersion;

    int timeoutint;          /* increase timeout every N turns... */
    int timeoutinc;          /* by this amount ... */
    int timeoutincmult;      /* and multiply timeoutinc by this amount ... */
    int timeoutintinc;       /* and increase timeoutint by this amount */
    int timeoutcounter;      /* timeoutcounter-timeoutint= urns to next inc. */
    int timeoutaddenemymove; /* increase to, when enemy move seen */
    int tcpwritetimeout;
    int netwait;
    time_t last_ping;
    int pingtimeout;
    int pingtime;
    time_t turn_start;

    int diplchance;
    int dipldefchance; /* Read as "diplomat defeat chance". */
    int spyreturnchance;
    int diplbribechance;
    int diplincitechance;
    int citymindist;
    int civilwarsize;
    int contactturns;
    int rapturedelay;
    int aifill;
    int barbarianrate;
    int onsetbarbarian;
    int nbarbarians;
    int occupychance;
    char *startmessage;

    int warminglevel; /* If globalwarming is higher than this number there is
                       * a chance of a warming event. */

    int coolinglevel; /* If nuclearwinter is higher than this number there is
                       * a chance of a cooling event. */

    char save_name[MAX_LEN_NAME];
    int save_nturns;
    int save_compress_level;
    int aqueductloss;
    int killcitizen;
    int razechance;
    bool scorelog;
    int seed;
    bool savepalace;
    bool natural_city_names;
    bool turnblock;
    bool fixedlength;
    bool auto_ai_toggle;
    bool fogofwar;
    bool fogofwar_old;  /* As the fog_of_war bit get changed by setting
                         * the server we need to remember the old setting */

    bool ruleset_loaded;
    int nav; /* AI convenience: tech_req for first non-trireme ferryboat */
    int u_partisan; /* Convenience: tech_req for first Partisan unit */
    char rulesetdir[MAX_LEN_NAME];
    int firepower_factor; /* See README.rulesets */
    int global_init_buildings[MAX_NUM_BUILDING_LIST];

    int ai_goal_government; /* kludge */

    int watchtower_extra_vision;
    int watchtower_vision;
    int allowed_city_names;

    struct civ_game_incite_cost {
      int improvement_factor;
      int unit_factor;
      int total_factor;
    } incite_cost;

    char demography[MAX_LEN_DEMOGRAPHY];

    /* Used by the map editor to control game_save;
     * could be used by the server too */
    struct civ_gamesave_options {
      bool save_random;
      bool save_players;
      bool save_known;       /* Moading will just reveal the squares around
                              * cities and units */
      bool save_starts;      /* Start positions will be auto generated */
      bool save_private_map; /* FoW map; will be created if not saved */
    } save_options;

    int revolution_length; /* 0=> random length, else the fixated length */

    struct civ_game_meta_info {
      bool user_message_set;
      char user_message[256];
    } meta_info;

    struct civ_game_wcdb {
      /* Used to avoid duplicating game end condition checks. */
      enum game_outcomes outcome;

      enum game_types type;
      int id;

      char *termap;
    } wcdb;

    int maxconnections;
    int maxhostconnections;
    int bruteforcethreshold;
    int iterplacementcoefficient;
    int teamplacementtype;
    bool rated;
    bool no_public_links;
    int kicktime;
    int spectatorchat;
    int idlecut;
    bool emptyreset;
    int triremestyle;
    int fracmovestyle;
    int endturn;
    bool revealmap;
    bool civilwar;
  } server;
};

enum game_types game_determine_type(void);

const char *game_type_name(enum game_types type);
const char *game_type_name_orig(enum game_types type);
enum game_types game_get_type_from_string(const char *s);

const char *game_outcome_name(enum game_outcomes outcome);
const char *game_outcome_name_orig(enum game_outcomes outcome);

/* Unused? */
struct lvldat {
  int advspeed;
};

void game_init(void);
void game_free(void);

void ruleset_data_free(void);

int game_next_year(int);
void game_advance_year(void);

int civ_population(struct player *pplayer);
city_t *game_find_city_by_name(const char *name);

struct unit *find_unit_by_id(int id);
city_t *find_city_by_id(int id);
city_t *find_city_by_name_fast(const char *name);

void game_remove_player(struct player *pplayer);
void game_renumber_players(int plrno);

void game_remove_unit(struct unit *punit);
void game_remove_city(city_t *pcity);
void initialize_globals(void);

void translate_data_names(void);

struct player *get_player(int player_id);
bool is_valid_player_id(int player_id);
int get_num_human_and_ai_players(void);

const char *population_to_text(int thousand_citizen);

extern struct civ_game game;
extern bool is_server;

/* NB: When changing defaults marked with "COMPAT", beware
 * unintended side-effects due to compatibilty with
 * non-warclients/servers. In most cases they should not
 * be changed or strange things might happen. */

#define GAME_DEFAULT_BRUTEFORCETHRESHOLD 20
#define GAME_MIN_BRUTEFORCETHRESHOLD     0
#define GAME_MAX_BRUTEFORCETHRESHOLD     MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS

#define GAME_DEFAULT_ITERPLACEMENTCOEFFICIENT 400
#define GAME_MIN_ITERPLACEMENTCOEFFICIENT     100
#define GAME_MAX_ITERPLACEMENTCOEFFICIENT     5000

#define GAME_DEFAULT_TRADEMINDIST 8 /* COMPAT */
#define GAME_MIN_TRADEMINDIST     1
#define GAME_MAX_TRADEMINDIST     999

#define GAME_DEFAULT_CARAVANBONUSSTYLE 1 /* COMPAT */
#define GAME_MIN_CARAVANBONUSSTYLE     0
#define GAME_MAX_CARAVANBONUSSTYLE     1

#define GAME_DEFAULT_TRADEREVENUESTYLE 0 /* COMPAT */
#define GAME_MIN_TRADEREVENUESTYLE     0
#define GAME_MAX_TRADEREVENUESTYLE     2

#define GAME_DEFAULT_TRADEREVENUEPCT 100 /* COMPAT */
#define GAME_MIN_TRADEREVENUEPCT     0
#define GAME_MAX_TRADEREVENUEPCT     200

#define GAME_DEFAULT_MAXTRADEROUTES OLD_NUM_TRADEROUTES /* COMPAT */
#define GAME_MIN_MAXTRADEROUTES     0
#define GAME_MAX_MAXTRADEROUTES     99

#define GAME_DEFAULT_AIRLIFTINGSTYLE 1 /* COMPAT */
#define GAME_MIN_AIRLIFTINGSTYLE     0
#define GAME_MAX_AIRLIFTINGSTYLE     3

#define GAME_DEFAULT_TECHLEAKAGE 0
#define GAME_MIN_TECHLEAKAGE     0
#define GAME_MAX_TECHLEAKAGE     3

#define GAME_DEFAULT_TECHCOSTSTYLE 1
#define GAME_MIN_TECHCOSTSTYLE     0
#define GAME_MAX_TECHCOSTSTYLE     2

#define GAME_DEFAULT_MAXCONNECTIONS 0
#define GAME_MIN_MAXCONNECTIONS     0
#define GAME_MAX_MAXCONNECTIONS     MAX_NUM_CONNECTIONS

#define GAME_DEFAULT_MAXHOSTCONNECTIONS 4
#define GAME_MIN_MAXHOSTCONNECTIONS     0
#define GAME_MAX_MAXHOSTCONNECTIONS     MAX_NUM_CONNECTIONS

#define GAME_DEFAULT_TEAMPLACEMENTTYPE 0
#define GAME_MIN_TEAMPLACEMENTTYPE     0
#define GAME_MAX_TEAMPLACEMENTTYPE     3

#define GAME_DEFAULT_TECHLEAKAGERATE 100 /* COMPAT */
#define GAME_MIN_TECHLEAKAGERATE     0
#define GAME_MAX_TECHLEAKAGERATE     100

#ifdef HAVE_MYSQL
#define GAME_DEFAULT_RATED TRUE
#else
#define GAME_DEFAULT_RATED FALSE
#endif /* HAVE_MYSQL */

#define GAME_DEFAULT_NO_PUBLIC_LINKS TRUE

#define GAME_DEFAULT_KICKTIME 600
#define GAME_MIN_KICKTIME 5
#define GAME_MAX_KICKTIME 86400

#define GAME_DEFAULT_SPECTATORCHAT 0
#define GAME_MIN_SPECTATORCHAT 0
#define GAME_MAX_SPECTATORCHAT 2

#define GAME_DEFAULT_IDLECUT 1800
#define GAME_MIN_IDLECUT 0
#define GAME_MAX_IDLECUT 8639999

#define GAME_DEFAULT_EMPTYRESET TRUE

#define GAME_DEFAULT_TRIREMESTYLE 0
#define GAME_MIN_TRIREMESTYLE 0
#define GAME_MAX_TRIREMESTYLE 1

#define GAME_DEFAULT_FRACMOVESTYLE 1
#define GAME_MIN_FRACMOVESTYLE 0
#define GAME_MAX_FRACMOVESTYLE 1

#define GAME_DEFAULT_ENDTURN 0
#define GAME_MIN_ENDTURN 0
#define GAME_MAX_ENDTURN 999999

#define GAME_DEFAULT_EXPERIMENTALBRIBINGCOST FALSE /* COMPAT */
#define GAME_DEFAULT_IGNORERULESET FALSE
#define GAME_DEFAULT_IMPROVEDAUTOATTACK FALSE
#define GAME_DEFAULT_STACKBRIBING FALSE /* COMPAT */
#define GAME_DEFAULT_GOLDTRADING FALSE  /* COMPAT */
#define GAME_DEFAULT_TECHTRADING FALSE  /* COMPAT */
#define GAME_DEFAULT_CITYTRADING FALSE  /* COMPAT */
#define GAME_DEFAULT_SLOWINVASIONS TRUE
#define GAME_DEFAULT_TEAMPLACEMENT TRUE
#define GAME_DEFAULT_GLOBALWARMINGON FALSE /* COMPAT */
#define GAME_DEFAULT_NUCLEARWINTERON FALSE /* COMPAT */
#define GAME_DEFAULT_KILLSTACK TRUE
#define GAME_DEFAULT_FUTURETECHSSCORE FALSE

#define GAME_DEFAULT_SEED        0
#define GAME_MIN_SEED            0
#define GAME_MAX_SEED            (MAX_UINT32 >> 1)

#define GAME_DEFAULT_GOLD        50
#define GAME_MIN_GOLD            0
#define GAME_MAX_GOLD            50000

#define GAME_DEFAULT_START_UNITS  "cccwwwxxxx"

#define GAME_DEFAULT_DISPERSION  0
#define GAME_MIN_DISPERSION      0
#define GAME_MAX_DISPERSION      10

#define GAME_DEFAULT_TECHLEVEL   0
#define GAME_MIN_TECHLEVEL       0
#define GAME_MAX_TECHLEVEL       100

#define GAME_DEFAULT_UNHAPPYSIZE 4
#define GAME_MIN_UNHAPPYSIZE     1
#define GAME_MAX_UNHAPPYSIZE     6

#define GAME_DEFAULT_ANGRYCITIZEN TRUE

#define GAME_DEFAULT_END_YEAR    2000
#define GAME_MIN_END_YEAR        GAME_START_YEAR
#define GAME_MAX_END_YEAR        5000

#define GAME_DEFAULT_MIN_PLAYERS     1
#define GAME_MIN_MIN_PLAYERS         1
#define GAME_MAX_MIN_PLAYERS         MAX_NUM_PLAYERS

#define GAME_DEFAULT_MAX_PLAYERS     MAX_NUM_PLAYERS
#define GAME_MIN_MAX_PLAYERS         1
#define GAME_MAX_MAX_PLAYERS         MAX_NUM_PLAYERS

#define GAME_DEFAULT_AIFILL          0
#define GAME_MIN_AIFILL              0
#define GAME_MAX_AIFILL              GAME_MAX_MAX_PLAYERS

#define GAME_DEFAULT_RESEARCHCOST          20
#define GAME_MIN_RESEARCHCOST          4
#define GAME_MAX_RESEARCHCOST        100

#define GAME_DEFAULT_DIPLCOST        100
#define GAME_MIN_DIPLCOST            0
#define GAME_MAX_DIPLCOST            100

#define GAME_DEFAULT_FOGOFWAR        TRUE

/* 0 means no national borders.  Performance dropps quickly as the border
 * distance increases (o(n^2) or worse). */
#define GAME_DEFAULT_BORDERS         0
#define GAME_MIN_BORDERS             0
#define GAME_MAX_BORDERS             24

#define GAME_DEFAULT_HAPPYBORDERS    TRUE

#define GAME_DEFAULT_SLOW_INVASIONS  TRUE

#define GAME_DEFAULT_DIPLOMACY       0
#define GAME_MIN_DIPLOMACY           0
#define GAME_MAX_DIPLOMACY           4

#define GAME_DEFAULT_MAXALLIES       0 /* COMPAT */
#define GAME_MIN_MAXALLIES           0
#define GAME_MAX_MAXALLIES           (MAX_NUM_PLAYERS - 1)

#define GAME_DEFAULT_DIPLCHANCE      60
#define GAME_MIN_DIPLCHANCE          1
#define GAME_MAX_DIPLCHANCE          99

#define GAME_DEFAULT_DIPLDEFCHANCE   60
#define GAME_MIN_DIPLDEFCHANCE       1
#define GAME_MAX_DIPLDEFCHANCE       99

#define GAME_DEFAULT_SPYRETURNCHANCE 60
#define GAME_MIN_SPYRETURNCHANCE     1
#define GAME_MAX_SPYRETURNCHANCE     99

#define GAME_DEFAULT_DIPLBRIBECHANCE 60
#define GAME_MIN_DIPLBRIBECHANCE     1
#define GAME_MAX_DIPLBRIBECHANCE     99

#define GAME_DEFAULT_DIPLINCITECHANCE 60
#define GAME_MIN_DIPLINCITECHANCE     1
#define GAME_MAX_DIPLINCITECHANCE     99

#define GAME_DEFAULT_FREECOST        0
#define GAME_MIN_FREECOST            0
#define GAME_MAX_FREECOST            100

#define GAME_DEFAULT_CONQUERCOST     0
#define GAME_MIN_CONQUERCOST         0
#define GAME_MAX_CONQUERCOST         100

#define GAME_DEFAULT_CITYFACTOR      14
#define GAME_MIN_CITYFACTOR          6
#define GAME_MAX_CITYFACTOR          100

#define GAME_DEFAULT_CITYMINDIST     0
#define GAME_MIN_CITYMINDIST         0 /* if 0, ruleset will overwrite this */
#define GAME_MAX_CITYMINDIST         5

#define GAME_DEFAULT_CIVILWARSIZE    10
#define GAME_MIN_CIVILWARSIZE        6
#define GAME_MAX_CIVILWARSIZE        1000

#define GAME_DEFAULT_CONTACTTURNS    20
#define GAME_MIN_CONTACTTURNS        0
#define GAME_MAX_CONTACTTURNS        100

#define GAME_DEFAULT_RAPTUREDELAY    1
#define GAME_MIN_RAPTUREDELAY        1
#define GAME_MAX_RAPTUREDELAY        99 /* 99 practicaly disables rapturing */

#define GAME_DEFAULT_SAVEPALACE      TRUE

#define GAME_DEFAULT_NATURALCITYNAMES TRUE

#define GAME_DEFAULT_FOODBOX         10
#define GAME_MIN_FOODBOX             5
#define GAME_MAX_FOODBOX             30

#define GAME_DEFAULT_AQUEDUCTLOSS    0
#define GAME_MIN_AQUEDUCTLOSS        0
#define GAME_MAX_AQUEDUCTLOSS        100

#define GAME_DEFAULT_KILLCITIZEN     1
#define GAME_MIN_KILLCITIZEN         0
#define GAME_MAX_KILLCITIZEN         15

#define GAME_DEFAULT_TECHPENALTY     0
#define GAME_MIN_TECHPENALTY         0
#define GAME_MAX_TECHPENALTY         100

#define GAME_DEFAULT_RAZECHANCE      20
#define GAME_MIN_RAZECHANCE          0
#define GAME_MAX_RAZECHANCE          100

#define GAME_DEFAULT_CIVSTYLE        2
#define GAME_MIN_CIVSTYLE            1
#define GAME_MAX_CIVSTYLE            2

#define GAME_DEFAULT_SCORELOG        FALSE

#define GAME_DEFAULT_SPACERACE       TRUE

#define GAME_DEFAULT_TURNBLOCK       FALSE

#define GAME_DEFAULT_AUTO_AI_TOGGLE  FALSE

#define GAME_DEFAULT_TIMEOUT         120
#define GAME_DEFAULT_TIMEOUTINT      0
#define GAME_DEFAULT_TIMEOUTINTINC   0
#define GAME_DEFAULT_TIMEOUTINC      0
#define GAME_DEFAULT_TIMEOUTINCMULT  1
#define GAME_DEFAULT_TIMEOUTADDEMOVE 0

#ifndef NDEBUG
#define GAME_MIN_TIMEOUT             30
#else
#define GAME_MIN_TIMEOUT             30
#endif
#define GAME_MAX_TIMEOUT             8639999

#define GAME_DEFAULT_TCPWRITETIMEOUT 30
#define GAME_MIN_TCPWRITETIMEOUT     0
#define GAME_MAX_TCPWRITETIMEOUT     120

#define GAME_DEFAULT_NETWAIT         4
#define GAME_MIN_NETWAIT             0
#define GAME_MAX_NETWAIT             20

#define GAME_DEFAULT_PINGTIME        20
#define GAME_MIN_PINGTIME            1
#define GAME_MAX_PINGTIME            1800

#define GAME_DEFAULT_PINGTIMEOUT     60
#define GAME_MIN_PINGTIMEOUT         10
#define GAME_MAX_PINGTIMEOUT         1800

#define GAME_DEFAULT_NOTRADESIZE     0
#define GAME_MIN_NOTRADESIZE         0
#define GAME_MAX_NOTRADESIZE         49

#define GAME_DEFAULT_FULLTRADESIZE   1
#define GAME_MIN_FULLTRADESIZE       1
#define GAME_MAX_FULLTRADESIZE       50

#define GAME_DEFAULT_BARBARIANRATE   0
#define GAME_MIN_BARBARIANRATE       0
#define GAME_MAX_BARBARIANRATE       6

#define GAME_DEFAULT_ONSETBARBARIAN  (GAME_START_YEAR+ \
                                      ((GAME_DEFAULT_END_YEAR-(GAME_START_YEAR))/3))
#define GAME_MIN_ONSETBARBARIAN      GAME_START_YEAR
#define GAME_MAX_ONSETBARBARIAN      GAME_MAX_END_YEAR

#define GAME_DEFAULT_OCCUPYCHANCE    0
#define GAME_MIN_OCCUPYCHANCE        0
#define GAME_MAX_OCCUPYCHANCE        100

#define GAME_DEFAULT_RULESETDIR      "default"

#define GAME_DEFAULT_SAVE_NAME       "civgame"

#define GAME_DEFAULT_SKILL_LEVEL 3      /* easy */
#define GAME_OLD_DEFAULT_SKILL_LEVEL 5  /* normal; for old save games */

#define GAME_DEFAULT_DEMOGRAPHY      "NASRLPEMOqrb"

#define GAME_DEFAULT_COMPRESS_LEVEL 6    /* if we have compression */
#define GAME_MIN_COMPRESS_LEVEL     0
#define GAME_MAX_COMPRESS_LEVEL     9
#define GAME_NO_COMPRESS_LEVEL      0

#define GAME_DEFAULT_REPUTATION 1000
#define GAME_MAX_REPUTATION 1000
#define GAME_REPUTATION_INCR 2

#define GAME_DEFAULT_WATCHTOWER_VISION 2
#define GAME_MIN_WATCHTOWER_VISION 1
#define GAME_MAX_WATCHTOWER_VISION 3

#define GAME_DEFAULT_WATCHTOWER_EXTRA_VISION 0
#define GAME_MIN_WATCHTOWER_EXTRA_VISION 0
#define GAME_MAX_WATCHTOWER_EXTRA_VISION 2

#define GAME_DEFAULT_ALLOWED_CITY_NAMES 1
#define GAME_MIN_ALLOWED_CITY_NAMES 0
#define GAME_MAX_ALLOWED_CITY_NAMES 3

#define GAME_DEFAULT_REVOLUTION_LENGTH 2
#define GAME_MIN_REVOLUTION_LENGTH 0
#define GAME_MAX_REVOLUTION_LENGTH 10

#define GAME_DEFAULT_REVEALMAP 0

#define GAME_DEFAULT_CIVILWAR 1

#define GAME_START_YEAR -4000

#define specialist_type_iterate(sp)    \
{                                      \
  int sp;                              \
                                       \
  for (sp = 0; sp < SP_COUNT; sp++) {

#define specialist_type_iterate_end    \
  }                                    \
}

#endif  /* WC_COMMON_GAME_H */
