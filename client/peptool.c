/**********************************************************************
 This file was edited by *pepeto*.
 - warmap
 - pepsettings
*********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "player.h"
#include "registry.h"
#include "support.h"
#include "unit.h"

#include "chatline_g.h"
#include "civclient.h"
#include "control.h"
#include "mapview_g.h"
#include "multiselect.h"
#include "myai.h"
#include "peptool.h"

/* warmap */
struct move_cost_map warmap;

#define AIR_ASSUMES_UNKNOWN_SAFE        TRUE
#define AIR_ASSUMES_FOGGED_SAFE         TRUE

#define MAXARRAYS 10000
#define ARRAYLENGTH 10

struct mappos_array {
  int first_pos;
  int last_pos;
  struct tile *tile[ARRAYLENGTH];
  struct mappos_array *next_array;
};

struct array_pointer {
  struct mappos_array *first_array;
  struct mappos_array *last_array;
};

static struct mappos_array *mappos_arrays[MAXARRAYS];
static struct array_pointer cost_lookup[MAXCOST];
static int array_count;
static int lowest_cost;
static int highest_cost;

bool can_move_here(struct tile *ptile);

/**************************************************************************
...
**************************************************************************/
static void init_queue(void)
{
  int i;
  static bool is_initialized = FALSE;

  if (!is_initialized) {
    for (i = 0; i < MAXARRAYS; i++) {
      mappos_arrays[i] = NULL;
    }
    is_initialized = TRUE;
  }

  for (i = 0; i < MAXCOST; i++) {
    cost_lookup[i].first_array = NULL;
    cost_lookup[i].last_array = NULL;
  }
  array_count = 0;
  lowest_cost = 0;
  highest_cost = 0;
}

/**************************************************************************
...
**************************************************************************/
static struct mappos_array *get_empty_array(void)
{
  struct mappos_array *parray;
  if (!mappos_arrays[array_count])
    mappos_arrays[array_count] = fc_malloc(sizeof(struct mappos_array));
  parray = mappos_arrays[array_count++];
  parray->first_pos = 0;
  parray->last_pos = -1;
  parray->next_array = NULL;
  return parray;
}

/**************************************************************************
...
**************************************************************************/
static void add_to_mapqueue(int cost, struct tile *ptile)
{
  struct mappos_array *our_array;

  assert(cost < MAXCOST && cost >= 0);

  our_array = cost_lookup[cost].last_array;
  if (!our_array) {
    our_array = get_empty_array();
    cost_lookup[cost].first_array = our_array;
    cost_lookup[cost].last_array = our_array;
  } else if (our_array->last_pos == ARRAYLENGTH-1) {
    our_array->next_array = get_empty_array();
    our_array = our_array->next_array;
    cost_lookup[cost].last_array = our_array;
  }

  our_array->tile[++(our_array->last_pos)] = ptile;
  if (cost > highest_cost)
    highest_cost = cost;
  freelog(LOG_DEBUG, "adding cost:%i at %i,%i", cost, ptile->x, ptile->y);
}

/**************************************************************************
...
**************************************************************************/
static struct tile *get_from_mapqueue(void)
{
  struct mappos_array *our_array;
  struct tile *ptile;

  freelog(LOG_DEBUG, "trying get");
  while (lowest_cost < MAXCOST) {
    if (lowest_cost > highest_cost)
      return FALSE;
    our_array = cost_lookup[lowest_cost].first_array;
    if (!our_array) {
      lowest_cost++;
      continue;
    }
    if (our_array->last_pos < our_array->first_pos) {
      if (our_array->next_array) {
	cost_lookup[lowest_cost].first_array = our_array->next_array;
	continue;
      } else {
	cost_lookup[lowest_cost].first_array = NULL;
	lowest_cost++;
	continue;
      }
    }
    ptile = our_array->tile[our_array->first_pos];
    our_array->first_pos++;
    return ptile;
  }
  return NULL;
}

static void init_warmap(struct tile *orig_tile, enum unit_move_type move_type)
{
  if (warmap.size != MAX_MAP_INDEX) {
    warmap.cost = fc_realloc(warmap.cost,
			     MAX_MAP_INDEX * sizeof(*warmap.cost));
    warmap.seacost = fc_realloc(warmap.seacost,
				MAX_MAP_INDEX * sizeof(*warmap.seacost));
    warmap.vector = fc_realloc(warmap.vector,
			       MAX_MAP_INDEX * sizeof(*warmap.vector));
    warmap.size = MAX_MAP_INDEX;
  }

  init_queue();

  switch (move_type) {
  case SEA_MOVING:
    assert(sizeof(*warmap.seacost) == sizeof(char));
    memset(warmap.seacost, MAXCOST, map.xsize * map.ysize);
    WARMAP_SEACOST(orig_tile) = 0;
    break;
  default:
    assert(sizeof(*warmap.cost) == sizeof(char));
    memset(warmap.cost, MAXCOST, map.xsize * map.ysize);
    WARMAP_COST(orig_tile) = 0;
    break;
  }
}  

bool can_move_here(struct tile *ptile)
{
	unit_list_iterate(ptile->units,punit)
	{
		if(!pplayers_allied(game.player_ptr,unit_owner(punit)))
			return FALSE;
	} unit_list_iterate_end;
	return TRUE;
}

void generate_warmap(struct unit *punit, enum unit_move_type move_type)
{
  int move_cost;
  bool igter=unit_flag(punit, F_IGTER);
  int maxcost = THRESHOLD * 6 + 2;
  struct tile *ptile;
  unsigned char cost;

  init_warmap(punit->tile, move_type);
  add_to_mapqueue(0, punit->tile);

  if (unit_flag(punit, F_SETTLERS)
      && unit_move_rate(punit)==3)
    maxcost /= 2;

  if(move_type == SEA_MOVING)
	  while ((ptile = get_from_mapqueue())) {
		cost = WARMAP_SEACOST(ptile);
		adjc_dir_iterate(ptile, tile1, dir) {
			if(!is_ocean(tile1->terrain))
				continue;
			move_cost = SINGLE_MOVE;
			move_cost += cost;
			if(can_move_here(tile1) && WARMAP_SEACOST(tile1) > move_cost && move_cost < maxcost) {
			  WARMAP_SEACOST(tile1) = move_cost;
		  if (ptile->move_cost[dir] == MOVE_COST_FOR_VALID_SEA_STEP) {
			add_to_mapqueue(move_cost, tile1);
		  }
		}
	  } adjc_dir_iterate_end;
	}
  else
	  while ((ptile = get_from_mapqueue())) {
		cost = WARMAP_COST(ptile);
	
		adjc_dir_iterate(ptile, tile1, dir) {
		if (WARMAP_COST(tile1) <= cost || !can_move_here(tile1))
		  continue;
	
			if (is_ocean(map_get_terrain(tile1))) {
			  if (ground_unit_transporter_capacity(tile1, game.player_ptr) > 0)
			move_cost = SINGLE_MOVE;
			  else
			continue;
		} else if (is_ocean(ptile->terrain)) {
		  int base_cost = get_tile_type(map_get_terrain(tile1))->movement_cost * SINGLE_MOVE;
		  move_cost = igter ? MOVE_COST_ROAD : MIN(base_cost, unit_move_rate(punit));
			} else if (igter)
		  move_cost = (ptile->move_cost[dir] != 0 ? SINGLE_MOVE : 0);
			else
		  move_cost = MIN(ptile->move_cost[dir], unit_move_rate(punit));
	
			move_cost += cost;
			if (WARMAP_COST(tile1) > move_cost && move_cost < maxcost) {
			  WARMAP_COST(tile1) = move_cost;
			  add_to_mapqueue(move_cost, tile1);
			}
		} adjc_dir_iterate_end;
	  }
}

#define DANGER_MOVE (2 * SINGLE_MOVE + 1)

int calculate_move_cost(struct unit *punit, struct tile *dest_tile)
{
  if(!punit)
    return MAXCOST;
  if (is_air_unit(punit) || is_heli_unit(punit)) {
    return SINGLE_MOVE * real_map_distance(punit->tile, dest_tile);
  }

  warmap.warunit = punit;
  warmap.orig_tile = punit->tile;

    if (is_sailing_unit(punit))
      generate_warmap(punit, SEA_MOVING);
   else
      generate_warmap(punit, LAND_MOVING);

  if (is_sailing_unit(punit))
    return WARMAP_SEACOST(dest_tile);
  else /* ground unit */
    return WARMAP_COST(dest_tile);
}

/* settings */
char *pepsettings_file_name(void);

static const char *peppagenames[]={"Main","Multi-Selection","Delayed Goto","Trade","Wonder","Attack","Defend"};
static int turns=0;
static int load_pepsettings_mode;
static int save_turns;
static int errors_max;

bool my_ai_enable;

bool multi_select_count_all;
bool multi_select_blink_all;
bool multi_select_blink;
bool multi_select_map_selection;
bool spread_airport_cities;
bool spread_allied_cities;

enum my_ai_level my_ai_trade_level;
int my_ai_trade_mode;
bool my_ai_trade_manual_trade_route_enable;
bool my_ai_trade_external;
int my_ai_trade_plan_level;
int my_ai_trade_plan_time_max;
bool my_ai_trade_plan_enable_free_slots;
bool my_ai_trade_plan_recalculate_auto;
bool my_ai_trade_plan_change_homecity;

enum my_ai_level my_ai_wonder_level;

enum my_ai_level my_ai_attack_level;
int my_ai_attack_min;
int my_ai_delayed_attack_delay;
int my_ai_attack_city_power_req;

enum my_ai_level my_ai_defend_level;
int my_ai_defend_city_power_req;

static struct pepsetting static_pepsettings[]=
{
	PSGEN_INT(PAGE_PMAIN,load_pepsettings_mode,N_("PepClient setting load mode"),N_("0: Off\n1: Only static settings\n"
		"2: Only automatic execution values\n3: Static settings and automatic execution values\n4: Only dynamic settings\n"
		"5: Dynamic and static settings \n6: Dynamic settings and automatic execution values\n7: All"),LOAD_PEPSETTINGS_MODE),
	PSGEN_INT(PAGE_PMAIN,errors_max,N_("Maxaimum warning before aborting load"),N_("0: Unlimited"),ERRORS_MAX),
	PSGEN_INT(PAGE_PMAIN,save_turns,N_("Save settings all the x turns"),N_("0: Disable"),SAVE_TURNS),
	PSGEN_BOOL(PAGE_PMAIN,my_ai_enable,N_("Enable automatics orders"),MY_AI_ENABLE),
	PSGEN_BOOL(PAGE_PMAIN,autowakeup_state,N_("Autowakeup sentried units"),AUTOWAKEUP),
	PSGEN_BOOL(PAGE_PMAIN,moveandattack_state,N_("Move and attack mode"),MOVEATTACK),
	PSGEN_INT(PAGE_PMAIN,default_caravan_action,N_("Caravan action upon arrival"),N_("0: Popup dialog\n1: Establish trade route\n"
		"2: Help building wonder\n3: Keep going"),CARAVAN_ACTION),
	PSGEN_INT(PAGE_PMAIN,default_diplomat_action,N_("Diplomat action upon arrival"),N_("0: Popup dialog\n1: Bribe unit\n"
		"2: Sabotage unit (spy)\n3: Establish embassy\n4: Investigate city\n5: Sabotage city\n6: Steal technology\n7: Incite revolt\n"
		"8: Poison city (spy)\n9: Keep going"),DIPLOMAT_ACTION),
	PSGEN_INT(PAGE_MS,multi_select_place,N_("Multi-selection place mode"),N_("0: Single unit\n1: All units on the tile\n"
		"2: All units on the continent\n3: All units"),MS_PLACE),
	PSGEN_INT(PAGE_MS,multi_select_utype,N_("Multi-selection unit type mode"),N_("0: Only units with the same type\n"
		"1: Only units with the same move type\n2: All unit types"),MS_UTYPE),
	PSGEN_BOOL(PAGE_MS,multi_select_count_all,N_("Count all units in the selection, included excluded units"),MS_COUNT_ALL),
	PSGEN_BOOL(PAGE_MS,multi_select_blink_all,N_("Blink all units in the selection, included excluded units"),MS_BLINK_ALL),
	PSGEN_BOOL(PAGE_MS,multi_select_blink,N_("Blink units in the selection"),MS_BLINK),
	PSGEN_BOOL(PAGE_MS,multi_select_map_selection,N_("Allow multi-selection at map"),MS_MAP),
	PSGEN_BOOL(PAGE_MS,spread_airport_cities,N_("Spread only in cities with airport"),SPREAD_AIRPORT),
	PSGEN_BOOL(PAGE_MS,spread_allied_cities,N_("Allow spreading into allied cities"),SPREAD_ALLY),
	PSGEN_FILTER(PAGE_MS,multi_select_inclusive_filter,N_("Multi-selection inclusive filter"),MS_INCLUSIVE),
	PSGEN_FILTER(PAGE_MS,multi_select_exclusive_filter,N_("Multi-selection exclusive filter"),MS_EXCLUSIVE),
	PSGEN_INT(PAGE_DG,unit_limit,N_("Delayed goto unit limit"),N_("0: Unlimited units"),DG_LIMIT),
	PSGEN_INT(PAGE_DG,delayed_goto_place,N_("Delayed goto place mode"),N_("0: Single unit\n1: All units on the tile\n"
		"2: All units on the continent\n3: All units"),DG_PLACE),
	PSGEN_INT(PAGE_DG,delayed_goto_utype,N_("Delayed goto unit type mode"),N_("0: Only units with the same type\n"
		"1: Only units with the same move type\n2: All unit types"),DG_UTYPE),
	PSGEN_FILTER(PAGE_DG,delayed_goto_inclusive_filter,N_("Delayed goto inclusive filter"),DG_INCLUSIVE),
	PSGEN_FILTER(PAGE_DG,delayed_goto_exclusive_filter,N_("Delayed goto exclusive filter"),DG_EXCLUSIVE),
	PSGEN_INT(PAGE_TRADE,my_ai_trade_level,N_("Automatic trade order level"),N_("0: Off\n1: On\n2: Good\n3:Best level"),MY_AI_TRADE_LEVEL),
	PSGEN_INT(PAGE_TRADE,my_ai_trade_mode,N_("Automatic trade mode"),N_("0: Off\n1: Best trade only\n2: Trade planning only\n"
		"3: Trade planning or best trade"),MY_AI_TRADE_MODE),
	PSGEN_BOOL(PAGE_TRADE,my_ai_trade_manual_trade_route_enable,N_("Allow manual trade orders"),MY_AI_TRADE_MANUAL),
	PSGEN_BOOL(PAGE_TRADE,my_ai_trade_external,N_("Allow external trade"),MY_AI_TRADE_EXTERNAL),
	PSGEN_INT(PAGE_TRADE,my_ai_trade_plan_time_max,N_("Maximum time (in seconds) allowed to trade planning calcul"),N_("0: No limit"),
		MY_AI_TRADE_TIME),
	PSGEN_BOOL(PAGE_TRADE,my_ai_trade_plan_recalculate_auto,N_("Automatic trade planning calcul"),MY_AI_TRADE_PLAN_AUTO),
	PSGEN_BOOL(PAGE_TRADE,my_ai_trade_plan_change_homecity,N_("Change homecity if a trade route is faster"),MY_AI_TRADE_PLAN_HOMECITY),
	PSGEN_INT(PAGE_WONDER,my_ai_wonder_level,N_("Automatic help wonder order level"),N_("0: Off\n1: On\n2: Good\n3:Best level"),
		MY_AI_WONDER_LEVEL),
	PSGEN_END
};

struct pepsetting *pepsettings=static_pepsettings;

const char *get_page_name(enum peppage page)
{
	assert(page>=0&&page<PAGE_NUM);

	return peppagenames[page];
}

void init_all_settings(void)
{
	pepsettings_iterate(pset)
	{
		switch(pset->type)
		{
			case TYPE_BOOL:
				*((bool *)pset->data)=pset->def;
				break;
			case TYPE_INT:
				*((int *)pset->data)=pset->def;
				break;
			case TYPE_FILTER:
				*((filter *)pset->data)=pset->def;
				break;
			default:
				break;
		}
	} pepsettings_iterate_end;
}

char *pepsettings_file_name(void)
{
	static char buf[256];
	char *name;

	name=user_home_dir();
	my_snprintf(buf,sizeof(buf),"%s%s%s",name?name:"",name?"/":"",PEPSETTINGS_FILE_NAME);
	return buf;
}

/* load settings */
//this functions return TRUE when a fatal error is found
bool load_city(struct section_file *psf,const char *way,struct city **pcity);
bool load_player(struct section_file *psf,const char *way,struct player **pplayer);
bool load_tile(struct section_file *psf,const char *way,struct tile **ptile);
bool load_unit(struct section_file *psf,const char *way,struct unit **punit);

bool load_city(struct section_file *psf,const char *way,struct city **pcity)
{
	char buf[256];
	int id,x,y;
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	id=secfile_lookup_int_default(psf,-1,buf,"id");
	x=secfile_lookup_int_default(psf,-1,buf,"x");
	y=secfile_lookup_int_default(psf,-1,buf,"y");
	*pcity=find_city_by_id(id);
	if(*pcity&&((*pcity)->tile->x!=x||(*pcity)->tile->y!=y))
		return TRUE;
	return FALSE;
}

bool load_player(struct section_file *psf,const char *way,struct player **pplayer)
{
	char buf[256],name[256];
	int id;
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	id=secfile_lookup_int_default(psf,-1,buf,"id");
	strcpy(name,secfile_lookup_str_default(psf,"\0",buf,"name"));
	if(id<0&&!strcmp(name,"NULL"))
	{
		*pplayer=NULL;
		return FALSE;
	}
	*pplayer=get_player(id);
	if(!(*pplayer)||strcmp((*pplayer)->name,name))
		return TRUE;
	return FALSE;
}

bool load_tile(struct section_file *psf,const char *way,struct tile **ptile)
{
	char buf[256];
	int x,y;
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	x=secfile_lookup_int_default(psf,-1,buf,"x");
	y=secfile_lookup_int_default(psf,-1,buf,"y");
	*ptile=map_pos_to_tile(x,y);
	if(!(*ptile))
		return TRUE;
	return FALSE;
}

bool load_unit(struct section_file *psf,const char *way,struct unit **punit)
{
	char buf[256];
	int id,type;
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	id=secfile_lookup_int_default(psf,-1,buf,"id");
	type=secfile_lookup_int_default(psf,-1,buf,"type");
	*punit=find_unit_by_id(id);
	if(*punit&&(*punit)->type!=type)
		return TRUE;
	return FALSE;
}

#define load(type,way,data)	\
	freelog(LOG_DEBUG,"Loading " #type " '%s' at %s: line %d",way,__FILE__,__LINE__);	\
	if(load_##type(&sf,way,&data))	\
	{	\
		freelog(LOG_NORMAL,"Cannot load dynamics settings (%s: line %d, bad " #type " '%s'), aborting...",__FILE__,__LINE__,way);	\
		goto free_datas;	\
	}	\
	if(!(data))	\
	{	\
		freelog(LOG_VERBOSE,"Cannot load '%s' (%s: line %d, " #type " not found)",way,__FILE__,__LINE__);	\
		error_count++;	\
		if(errors_max&&error_count>=errors_max)	\
		{	\
			freelog(LOG_NORMAL,"Cannot load dynamics settings, too much warning (%d/%d), aborting...",error_count,errors_max);	\
			goto free_datas;	\
		}	\
	}	\
	else
 
#define load_owner(type,way,data)	\
	load(type,way,data) if(data->owner!=game.player_idx)	\
	{	\
		freelog(LOG_VERBOSE,"Cannot load '%s' (%s: line %d, " #type " has a bad owner)",way,__FILE__,__LINE__);	\
		data=NULL;	\
	}	\
	else

void load_all_settings(void)
{
	struct section_file sf;
	char *name=pepsettings_file_name(),buf[256];
	int i,num,error_count=0;

	turns=0;

	if(!section_file_load(&sf,name))
	{
		my_snprintf(buf,sizeof(buf),"PepClient: Cannot load setting file %s",name);
		append_output_window(buf);
		return;
	}

	/* static settings */
	if(load_pepsettings_mode&1)
	{
		freelog(LOG_DEBUG,"Loading static settings");
		pepsettings_iterate(pset)
		{
			switch(pset->type)
			{
				case TYPE_BOOL:
					*((bool *)pset->data)=secfile_lookup_bool_default(&sf,*((bool *)pset->data),"static.%s",pset->name);
					break;
				case TYPE_INT:
					*((int *)pset->data)=secfile_lookup_int_default(&sf,*((int *)pset->data),"static.%s",pset->name);
					break;
				case TYPE_FILTER:
					*((filter *)pset->data)=secfile_lookup_int_default(&sf,*((filter *)pset->data),"static.%s",pset->name);
					break;
				default:
					break;
			}
			if(!(load_pepsettings_mode&1))
				break;
		} pepsettings_iterate_end;
	}

	/* automatic processus */
	if(load_pepsettings_mode&2)
	{
		freelog(LOG_DEBUG,"Loading automatic processus");
		automatic_processus *pap;
		num=secfile_lookup_int_default(&sf,-1,"automatic_processus.num");
		for(i=0;i<num;i++)
		{
			strcpy(buf,secfile_lookup_str_default(&sf,"\0","automatic_processus.ap%d.name",i));
			if((pap=find_automatic_processus_by_name(buf)))
			{
				pap->auto_filter=secfile_lookup_int_default(&sf,pap->auto_filter,"automatic_processus.ap%d.filter",i);
				auto_filter_normalize(pap);
			}
			else
				freelog(LOG_ERROR,"No automatic_processus named '%s'",buf);
		}
	}

	/* dynamic settings */
	if(load_pepsettings_mode&4)
	{
		//compatibity test
		bool compatible=TRUE;
		i=secfile_lookup_int_default(&sf,-1,"game_info.xsize");
		if(i!=-1&&i!=map.xsize)
			compatible=FALSE;
		i=secfile_lookup_int_default(&sf,-1,"game_info.ysize");
		if(i!=-1&&i!=map.ysize)
			compatible=FALSE;
		i=secfile_lookup_int_default(&sf,-1,"game_info.topology_id");
		if(i!=-1&&i!=map.topology_id)
			compatible=FALSE;
		num=secfile_lookup_int_default(&sf,-1,"game_info.nplayers");
		if(num!=-1&&num!=game.nplayers)
			compatible=FALSE;
		else
		{
			for(i=0;i<num;i++)
			{
				struct player *pplayer;
				my_snprintf(buf,sizeof(buf),"game_info.player%d",i);
				if(load_player(&sf,buf,&pplayer))
				{
					compatible=FALSE;
					break;
				}
			}
		}

		if(!compatible)
		{
			freelog(LOG_NORMAL,"Dynamic settings were saved for an other game, they cannot be loaded",buf);
			goto end;
		}

		freelog(LOG_DEBUG,"Loading dynamic settings");
		//initialize
		int tairliftunittype=0;
		struct tile_list tairlift;
		struct multi_select tmultiselect[10];
		struct delayed_goto tdelayedgoto[10];
		struct trade_route_list ttraders,ttradeplan;
		struct city_list trallypoint,ttradecities;
		struct hw_unit_list thelpers;
		struct unit_list tpatrolers,tnoners;

		city_list_init(&trallypoint);
		tile_list_init(&tairlift);
		for(i=0;i<=9;i++)
		{
			unit_list_init(&tmultiselect[i].ulist);
			tmultiselect[i].punit_focus=NULL;
			delayed_goto_data_list_init(&tdelayedgoto[i].dglist);
			tdelayedgoto[i].pplayer=NULL;
		}
		trade_route_list_init(&ttraders);
		trade_route_list_init(&ttradeplan);
		city_list_init(&ttradecities);
		hw_unit_list_init(&thelpers);
		unit_list_init(&tpatrolers);
		unit_list_init(&tnoners);

		//rally point
		num=secfile_lookup_int_default(&sf,-1,"dynamic.rally.city_num");
		for(i=0;i<num;i++)
		{
			struct city *pcity;
			struct tile *ptile;
			my_snprintf(buf,sizeof(buf),"dynamic.rally.city%d",i);
			load_owner(city,buf,pcity)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.rally.city%d.tile",i);
				load(tile,buf,ptile)
				{
					struct city *ccity=fc_malloc(sizeof(struct city));
					*ccity=*pcity;
					ccity->rally_point=ptile;
					city_list_append(&trallypoint,ccity);
				}
			}
		}
		//airlift
		num=secfile_lookup_int_default(&sf,-1,"dynamic.airlift.tile_num");
		for(i=0;i<num;i++)
		{
			struct tile *ptile;
			my_snprintf(buf,sizeof(buf),"dynamic.airlift.tile%d",i);
			load(tile,buf,ptile)
			{
				tile_list_append(&tairlift,ptile);
			}
		}
		tairliftunittype=secfile_lookup_int_default(&sf,airliftunittype,"dynamic.airlift.type");
		//multi-select
		for(i=0;i<=9;i++)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit_num",i);
			num=secfile_lookup_int_default(&sf,0,buf);
			int j;
			for(j=0;j<num;j++)
			{
				struct unit *punit;
				my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit%d",i,j);
				load_owner(unit,buf,punit)
				{
					unit_list_append(&tmultiselect[i].ulist,punit);
				}
			}
			if(num)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit_focus",i);
				load_owner(unit,buf,tmultiselect[i].punit_focus);
			}
		}
		//delayed goto
		for(i=0;i<=9;i++)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data_num",i);
			num=secfile_lookup_int_default(&sf,0,buf);
			int j;
			for(j=0;j<num;j++)
			{
				struct delayed_goto_data *pdgd=fc_malloc(sizeof(struct delayed_goto_data));
				my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data%d.%%s",i,j);
				pdgd->id=secfile_lookup_int_default(&sf,0,buf,"id");
				pdgd->type=secfile_lookup_int_default(&sf,0,buf,"type");
				my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data%d.tile",i,j);
				load(tile,buf,pdgd->ptile);
				if(pdgd->id&&pdgd->type&&pdgd->ptile)
					delayed_goto_data_list_append(&tdelayedgoto[i].dglist,pdgd);
				else
					free(pdgd);
			}
			if(num)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.player",i);
				load(player,buf,tdelayedgoto[i].pplayer);
			}
		}
		//my_ai_trade
		num=secfile_lookup_int_default(&sf,-1,"dynamic.trade_route.unit_num");
		for(i=0;i<num;i++)
		{
			struct unit *punit;
			struct city *pc1,*pc2;
			my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d",i);
			load_owner(unit,buf,punit)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d.city1",i);
				load_owner(city,buf,pc1)
				{
					my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d.city2",i);
					load(city,buf,pc2)
					{
						trade_route_list_append(&ttraders,trade_route_new(punit,pc1,pc2,secfile_lookup_bool_default
							(&sf,FALSE,"dynamic.trade_route.unit%d.planned",i)));
					}
				}
			}
		}
		num=secfile_lookup_int_default(&sf,-1,"dynamic.trade_cities.city_num");
		for(i=0;i<num;i++)
		{
			struct city *pcity;
			my_snprintf(buf,sizeof(buf),"dynamic.trade_cities.city%d",i);
			load_owner(city,buf,pcity)
			{
				city_list_append(&ttradecities,pcity);
			}
		}
		num=secfile_lookup_int_default(&sf,-1,"dynamic.trade_plan.tr_num");
		for(i=0;i<num;i++)
		{
			struct city *pc1,*pc2;
			my_snprintf(buf,sizeof(buf),"dynamic.trade_plan.tr%d.city1",i);
			load_owner(city,buf,pc1)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.trade_plan.tr%d.city2",i);
				load(city,buf,pc2)
				{
					trade_route_list_append(&ttradeplan,trade_route_new(NULL,pc1,pc2,TRUE));
				}
			}
		}
		//my_ai_wonder
		num=secfile_lookup_int_default(&sf,-1,"dynamic.help_wonder.unit_num");
		for(i=0;i<num;i++)
		{
			struct unit *punit;
			struct city *pcity;
			struct help_wonder *thw=NULL,*bhw=NULL,*fhw=NULL;
			int id,level;
			my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d",i);
			load_owner(unit,buf,punit)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d.city",i);
				load_owner(city,buf,pcity)
				{
					my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d.%%s",i);
					id=secfile_lookup_int_default(&sf,-1,buf,"wid");
					level=secfile_lookup_int_default(&sf,-1,buf,"level");
					help_wonder_list_iterate(pcity->help_wonders,phw)
					{
						if(phw->id==id)
						{
							if(phw->level==level)
							{
								thw=phw;
								break;
							}
							else
								bhw=phw;
						}
						else
							fhw=phw;
					} help_wonder_list_iterate_end;
					bhw=(thw?thw:(bhw?bhw:fhw));
					if(bhw)
					{
						struct hw_unit *phwu=fc_malloc(sizeof(struct hw_unit));
						phwu->punit=punit;
						phwu->phw=bhw;
						hw_unit_list_append(&thelpers,phwu);
					}
					else
						unit_list_append(&tnoners,punit);
				}
			}
		}
		//my_ai_patrol
		num=secfile_lookup_int_default(&sf,-1,"dynamic.patrol.unit_num");
		for(i=0;i<num;i++)
		{
			struct unit *punit;
			struct tile *ptile;
			my_snprintf(buf,sizeof(buf),"dynamic.patrol.unit%d",i);
			load_owner(unit,buf,punit)
			{
				my_snprintf(buf,sizeof(buf),"dynamic.patrol.unit%d.tile",i);
				load(tile,buf,ptile)
				{
					struct unit *cunit=fc_malloc(sizeof(struct unit));
					*cunit=*punit;
					cunit->my_ai.data=(void *)ptile;
					cunit->my_ai.manalloc=secfile_lookup_bool_default(&sf,FALSE,"dynamic.patrol.unit%d.manalloc",i);
					unit_list_append(&tpatrolers,cunit);
				}
			}
		}
		//my_ai_none
		num=secfile_lookup_int_default(&sf,-1,"dynamic.none.unit_num");
		for(i=0;i<num;i++)
		{
			struct unit *punit;
			my_snprintf(buf,sizeof(buf),"dynamic.none.unit%d",i);
			load_owner(unit,buf,punit)
			{
				unit_list_append(&tnoners,punit);
			}
		}

		//apply
		freelog(LOG_DEBUG,"Apply dynamic settings");
		city_list_iterate(trallypoint,ccity)
		{
			city_set_rally_point(player_find_city_by_id(game.player_ptr,ccity->id),ccity->rally_point);
		} city_list_iterate_end;
		tile_list_iterate(tairlift,ptile)
		{
			add_city_to_auto_airlift_queue(ptile,TRUE);
		} tile_list_iterate_end;
		airliftunittype=tairliftunittype;
		for(i=0;i<=9;i++)
		{
			multi_select_set(i,&tmultiselect[i]);
			delayed_goto_set(i,&tdelayedgoto[i]);
		}
		city_list_iterate(ttradecities,pcity)
		{
			my_ai_add_trade_city(pcity,TRUE);
		} city_list_iterate_end;
		trade_route_list_copy(my_ai_trade_plan_get(),&ttradeplan);
		trade_route_list_iterate(ttraders,ptr)
		{
			my_ai_orders_free(ptr->punit);
			my_ai_trade_route_alloc(ptr);
		} trade_route_list_iterate_end;
		hw_unit_list_iterate(thelpers,phwu)
		{
			my_ai_orders_free(phwu->punit);
			my_ai_help_wonder_alloc(phwu->punit,phwu->phw);
		} hw_unit_list_iterate_end;
		unit_list_iterate(tpatrolers,cunit)
		{
			struct unit *punit=player_find_unit_by_id(game.player_ptr,cunit->id);
			my_ai_orders_free(punit);
			my_ai_patrol_alloc(punit,(struct tile *)cunit->my_ai.data,cunit->my_ai.manalloc);
		} unit_list_iterate_end;
		unit_list_iterate(tnoners,punit)
		{
			if(!punit->my_ai.control)
			{
				unit_list_append(my_ai_get_units(MY_AI_NONE),punit);
				punit->my_ai.control=TRUE;
				punit->my_ai.activity=MY_AI_NONE;
			}
		} unit_list_iterate_end;
		update_unit_info_label(get_unit_in_focus());

free_datas:
		//free datas
		city_list_free(&trallypoint);
		for(i=0;i<=9;i++)
		{
			unit_list_unlink_all(&tmultiselect[i].ulist);
			delayed_goto_data_list_free(&tdelayedgoto[i].dglist);
		}
		trade_route_list_unlink_all(&ttraders);
		trade_route_list_free(&ttradeplan);
		city_list_unlink_all(&ttradecities);
		hw_unit_list_free(&thelpers);
		unit_list_free(&tpatrolers);
		unit_list_unlink_all(&tnoners);
	}

end:
	section_file_free(&sf);

	append_output_window("PepClient: Settings loaded");
}

/* save settings */
void save_city(struct section_file *psf,const char *way,struct city *pcity);
void save_player(struct section_file *psf,const char *way,struct player *pplayer);
void save_tile(struct section_file *psf,const char *way,struct tile *ptile);
void save_unit(struct section_file *psf,const char *way,struct unit *punit);

void save_city(struct section_file *psf,const char *way,struct city *pcity)
{
	char buf[256];
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	secfile_insert_int(psf,(pcity?pcity->id:-1),buf,"id");
	secfile_insert_int(psf,(pcity?pcity->tile->x:-1),buf,"x");
	secfile_insert_int(psf,(pcity?pcity->tile->y:-1),buf,"y");
}

void save_player(struct section_file *psf,const char *way,struct player *pplayer)
{
	char buf[256];
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	secfile_insert_int(psf,(pplayer?pplayer->player_no:-1),buf,"id");
	secfile_insert_str(psf,(pplayer?pplayer->name:"NULL"),buf,"name");
}

void save_tile(struct section_file *psf,const char *way,struct tile *ptile)
{
	char buf[256];
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	secfile_insert_int(psf,(ptile?ptile->x:-1),buf,"x");
	secfile_insert_int(psf,(ptile?ptile->y:-1),buf,"y");
}

void save_unit(struct section_file *psf,const char *way,struct unit *punit)
{
	char buf[256];
	my_snprintf(buf,sizeof(buf),"%s.%%s",way);

	secfile_insert_int(psf,(punit?punit->id:-1),buf,"id");
	secfile_insert_int(psf,(punit?punit->type:-1),buf,"type");
}

void save_all_settings(void)
{
	struct section_file sf;
	char *name=pepsettings_file_name();
	char buf[256];
	int i;

	section_file_init(&sf);
	
	/* static settings */
	pepsettings_iterate(pset)
	{
		switch(pset->type)
		{
			case TYPE_BOOL:
				secfile_insert_bool(&sf,*((bool*)pset->data),"static.%s",pset->name);
				break;
			case TYPE_INT:
				secfile_insert_int(&sf,*((int*)pset->data),"static.%s",pset->name);
				break;
			case TYPE_FILTER:
				secfile_insert_int(&sf,(int)*((filter*)pset->data),"static.%s",pset->name);
				break;
			default:
				break;
		}
	} pepsettings_iterate_end;

	/* automatic processus */
	i=0;
	automatic_processus_iterate(pap)
	{
		if(pap->menu[0]||pap->description[0])
			i++;
	} automatic_processus_iterate_end;
	secfile_insert_int_comment(&sf,i,_("don't modify this !"),"automatic_processus.num");
	i=0;
	automatic_processus_iterate(pap)
	{
		char *name=(pap->menu[0]?pap->menu:pap->description);
		if(!name[0])
			continue;
		my_snprintf(buf,sizeof(buf),"automatic_processus.ap%d.%%s",i);
		secfile_insert_str(&sf,name,buf,"name");
		secfile_insert_int(&sf,pap->auto_filter,buf,"filter");
		i++;
	} automatic_processus_iterate_end;

	/* dynamic settings */
	//some game infos, to test the compatibility
	secfile_insert_int(&sf,map.xsize,"game_info.xsize");
	secfile_insert_int(&sf,map.ysize,"game_info.ysize");
	secfile_insert_int(&sf,map.topology_id,"game_info.topology_id");
	secfile_insert_int(&sf,game.nplayers,"game_info.nplayers");
	i=0;
	players_iterate(pplayer)
	{
		my_snprintf(buf,sizeof(buf),"game_info.player%d",i);
		save_player(&sf,buf,pplayer);
		i++;
	} players_iterate_end;

	//rally point
	i=0;
	city_list_iterate(game.player_ptr->cities,pcity)
	{
		if(pcity->rally_point)
			i++;
	} city_list_iterate_end;
	secfile_insert_int_comment(&sf,i,_("don't modify this !"),"dynamic.rally.city_num");
	i=0;
	city_list_iterate(game.player_ptr->cities,pcity)
	{
		if(!pcity->rally_point)
			continue;
		my_snprintf(buf,sizeof(buf),"dynamic.rally.city%d",i);
		save_city(&sf,buf,pcity);
		my_snprintf(buf,sizeof(buf),"dynamic.rally.city%d.tile",i);
		save_tile(&sf,buf,pcity->rally_point);
		i++;
	} city_list_iterate_end;
	//airlift
	struct tile_list *ptl=get_airlift_queue();
	secfile_insert_int_comment(&sf,tile_list_size(ptl),_("don't modify this !"),"dynamic.airlift.tile_num");
	i=0;
	tile_list_iterate(*ptl,ptile)
	{
		my_snprintf(buf,sizeof(buf),"dynamic.airlift.tile%d",i);
		save_tile(&sf,buf,ptile);
		i++;
	} tile_list_iterate_end;
	secfile_insert_int(&sf,airliftunittype,"dynamic.airlift.type");
	//multi-select
	struct multi_select *pms;
	for(i=0;i<=9;i++)
	{
		pms=multi_select_get(i);
		my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit_num",i);
		secfile_insert_int_comment(&sf,unit_list_size(&pms->ulist),_("don't modify this !"),buf);
		int j=0;
		unit_list_iterate(pms->ulist,punit)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit%d",i,j);
			save_unit(&sf,buf,punit);
			j++;
		} unit_list_iterate_end;
		if(j)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.multiselect%ds.unit_focus",i);
			save_unit(&sf,buf,pms->punit_focus);
		}
	}
	//delayed goto
	struct delayed_goto *pdg;
	for(i=0;i<=9;i++)
	{
		pdg=delayed_goto_get(i);
		my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data_num",i);
		secfile_insert_int_comment(&sf,delayed_goto_data_list_size(&pdg->dglist),_("don't modify this !"),buf);
		int j=0;
		delayed_goto_data_list_iterate(pdg->dglist,pdgd)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data%d.%%s",i,j);
			secfile_insert_int(&sf,pdgd->id,buf,"id");
			secfile_insert_int(&sf,pdgd->type,buf,"type");
			my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.data%d.tile",i,j);
			save_tile(&sf,buf,pdgd->ptile);
			j++;
		} delayed_goto_data_list_iterate_end;
		if(j)
		{
			my_snprintf(buf,sizeof(buf),"dynamic.delayedgoto%ds.player",i);
			save_player(&sf,buf,pdg->pplayer);
		}
	}
	//my_ai_trade
	struct unit_list *pul=my_ai_get_units(MY_AI_TRADE_ROUTE);
	secfile_insert_int_comment(&sf,unit_list_size(pul),_("don't modify this !"),"dynamic.trade_route.unit_num");
	i=0;
	unit_list_iterate(*pul,punit)
	{
		struct trade_route *ptr=(struct trade_route *)punit->my_ai.data;
		my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d",i);
		save_unit(&sf,buf,punit);
		my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d.city1",i);
		save_city(&sf,buf,ptr->pc1);
		my_snprintf(buf,sizeof(buf),"dynamic.trade_route.unit%d.city2",i);
		save_city(&sf,buf,ptr->pc2);
		secfile_insert_bool(&sf,ptr->planned,"dynamic.trade_route.unit%d.planned",i);
		i++;
	} unit_list_iterate_end;
	struct my_ai_trade_city_list *ptcl=my_ai_get_trade_cities();
	secfile_insert_int_comment(&sf,my_ai_trade_city_list_size(ptcl),_("don't modify this !"),"dynamic.trade_cities.city_num");
	i=0;
	my_ai_trade_city_list_iterate(*ptcl,ptc)
	{
		my_snprintf(buf,sizeof(buf),"dynamic.trade_cities.city%d",i);
		save_city(&sf,buf,ptc->pcity);
		i++;
	} my_ai_trade_city_list_iterate_end;
	struct trade_route_list *ptrl=my_ai_trade_plan_get();
	secfile_insert_int_comment(&sf,trade_route_list_size(ptrl),_("don't modify this !"),"dynamic.trade_plan.tr_num");
	i=0;
	trade_route_list_iterate(*ptrl,ptr)
	{
		my_snprintf(buf,sizeof(buf),"dynamic.trade_plan.tr%d.city1",i);
		save_city(&sf,buf,ptr->pc1);
		my_snprintf(buf,sizeof(buf),"dynamic.trade_plan.tr%d.city2",i);
		save_city(&sf,buf,ptr->pc2);
		i++;
	} trade_route_list_iterate_end;
	//my_ai_wonder
	pul=my_ai_get_units(MY_AI_HELP_WONDER);
	secfile_insert_int_comment(&sf,unit_list_size(pul),_("don't modify this !"),"dynamic.help_wonder.unit_num");
	i=0;
	unit_list_iterate(*pul,punit)
	{
		struct help_wonder *phw=(struct help_wonder *)punit->my_ai.data;
		my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d",i);
		save_unit(&sf,buf,punit);
		my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d.city",i);
		save_city(&sf,buf,phw->pcity);
		my_snprintf(buf,sizeof(buf),"dynamic.help_wonder.unit%d.%%s",i);
		secfile_insert_int(&sf,phw->id,buf,"wid");
		secfile_insert_int(&sf,phw->level,buf,"level");
		i++;
	} unit_list_iterate_end;
	//my_ai_patrol
	pul=my_ai_get_units(MY_AI_PATROL);
	secfile_insert_int_comment(&sf,unit_list_size(pul),_("don't modify this !"),"dynamic.patrol.unit_num");
	i=0;
	unit_list_iterate(*pul,punit)
	{
		my_snprintf(buf,sizeof(buf),"dynamic.patrol.unit%d",i);
		save_unit(&sf,buf,punit);
		my_snprintf(buf,sizeof(buf),"dynamic.patrol.unit%d.tile",i);
		save_tile(&sf,buf,(struct tile *)punit->my_ai.data);
		secfile_insert_bool(&sf,punit->my_ai.manalloc,"dynamic.patrol.unit%d.manalloc",i);
		i++;
	} unit_list_iterate_end;
	//my_ai_none
	pul=my_ai_get_units(MY_AI_NONE);
	secfile_insert_int_comment(&sf,unit_list_size(pul),_("don't modify this !"),"dynamic.none.unit_num");
	i=0;
	unit_list_iterate(*pul,punit)
	{
		my_snprintf(buf,sizeof(buf),"dynamic.none.unit%d",i);
		save_unit(&sf,buf,punit);
		i++;
	} unit_list_iterate_end;

	/* save to disk */
	if(!section_file_save(&sf,name,0))
		my_snprintf(buf,sizeof(buf),_("PepClient: Save failed, cannot write to file %s"),name);
	else
		my_snprintf(buf,sizeof(buf),_("PepClient: Saved settings to file %s"),name);

	append_output_window(buf);
	section_file_free(&sf);
}

void autosave_settings(void)
{
	if(!save_turns||client_is_observer())
		return;

	turns++;
	if(turns<save_turns)
		return;
	turns=0;

	save_all_settings();
}
