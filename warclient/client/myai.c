/**********************************************************************
 This file was edited by *pepeto*.
 - auto-caravans
 - spread
 - patrol
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "city.h"
#include "combat.h"
#include "fcintl.h"
#include "genlist.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "player.h"
#include "unit.h"
#include "shared.h"
#include "support.h"

#include "chatline_g.h"
#include "civclient.h"
#include "clinet.h"
#include "control.h"
#include "mapview_common.h"
#include "mapview_g.h"
#include "menu_g.h"
#include "multiselect.h"
#include "myai.h"
#include "peptool.h"

#define swap(data1,data2,type) {type _data_=data1;data1=data2;data2=_data_;}
#define goto_and_request(punit,ptile)	\
	send_goto_unit_and_calculate_moves_left(punit,ptile);	\
	if(punit->virtual_moves_left>=0)

struct trade_city {
  struct city *pcity;                /* The pointed city */
  int free_slots;                    /* Number of free slots */
  int trade_routes_num;              /* Number of city it can trade with */
  struct trade_route **trade_routes; /* A list of possible trade routes */
  int *distance;                     /* A list of the distances */
};

struct trade_configuration
{
  int free_slots;       /* the total number of free slots */
  int turns;            /* the total number of required turns for a caravan */
  int moves;            /* the total number of required moves for a caravan */
};

struct toggle_city
{
	struct city *pcity;
        int depth;
        enum city_tile_type city_map[CITY_MAP_SIZE][CITY_MAP_SIZE];
};

struct toggle_worker
{
	struct city *pcity;
	struct tile *ptile;
        int x, y;
        bool was_used;
};

static automatic_processus *my_ai_auto_execute;//main automatic processus id for my_ai
static automatic_processus *my_ai_trade_auto_execute;
static automatic_processus *my_ai_wonder_auto_execute;
static automatic_processus *my_ai_patrol_auto_execute;

static struct city_list trade_cities;
static struct trade_route_list trade_plan;
static struct trade_route_list non_ai_trade;
static struct unit_list noners;
static struct unit_list traders;
static struct unit_list helpers;
static struct unit_list patrolers;
static struct unit_list temp;
static bool my_ai_update=FALSE;

static const char *my_ai_activities[]={"Automatic orders","Auto trade route","Auto help wonder","Patrolling"};

struct tile *find_nearest_city(struct unit *punit,bool allies);

struct tile *find_nearest_city(struct unit *punit,bool allies)
{
	if(map_get_city(punit->tile))
		return punit->tile;
	Continent_id cid=punit->tile->continent;
	int bmr=MAXCOST,mr,dist=MAXCOST,d;
	struct tile *btile=NULL;
	bool cond=is_ground_unit(punit);

	players_iterate(pplayer)
	{
		if(!allies&&pplayer!=game.player_ptr)
			continue;
		enum diplstate_type type=pplayer_get_diplstate(pplayer,game.player_ptr)->type;
		if(allies&&!(pplayer==game.player_ptr||type==DS_ALLIANCE||type==DS_TEAM))
			continue;
		city_list_iterate(pplayer->cities,pcity)
		{
			if(cond&&cid!=pcity->tile->continent)
				continue;
			mr=calculate_move_cost(punit,pcity->tile);
			if(mr==bmr)
			{
				d=real_map_distance(punit->tile,pcity->tile);
				if(d<dist)
				{
					bmr=mr;
					dist=d;
				btile=pcity->tile;
				}
			}
			else if(mr<bmr)
			{
				bmr=mr;
				dist=real_map_distance(punit->tile,pcity->tile);
				btile=pcity->tile;
			}
		} city_list_iterate_end;
	} players_iterate_end;
	return btile;
}

/**************************************************************************
 my_ai_trade_route
**************************************************************************/
void update_trade_route(struct trade_route *ptr);
int my_city_num_trade_routes(struct city *pcity,bool all);
int trade_route_value(struct city *pc1,struct city *pc2);
int city_min_trade_route(struct city *pcity,struct trade_route **pptr);
struct trade_route *find_same_trade_route(struct trade_route *ptr);
int calculate_move_trade_cost(struct unit *punit,struct city *pc1,struct city *pc2);
int calculate_best_move_trade_cost(struct trade_route *ptr);
struct trade_route *can_establish_new_trade_route(struct unit *punit,struct city *pc1,struct city *pc2);
struct trade_route *best_city_trade_route(struct unit *punit,struct city *pcity);
struct trade_route *trade_route_list_find_cities(struct trade_route_list *ptrl,struct city *pc1,struct city *pc2);

struct trade_route *trade_route_new(struct unit *punit,struct city *pc1,struct city *pc2,bool planned)
{
	if(!pc1||!pc2||(!planned&&!punit))
		return NULL;
	struct trade_route *ptr=fc_malloc(sizeof(struct trade_route));

	ptr->punit=punit;
	ptr->pc1=pc1;
	ptr->pc2=pc2;
	ptr->ptr0=ptr->ptr1=ptr->ptr2=NULL;
	ptr->planned=planned;
	if(punit)
	{
		ptr->tr_type=TR_NONE;
		update_trade_route(ptr);//for moves_req, turns_req and trade valors
	}
	else//case trade_plan
	{
		ptr->tr_type=TR_NEW;
		ptr->trade=trade_between_cities(pc1,pc2);
	}

        return ptr;
}

void update_trade_route(struct trade_route *ptr)
{
	ptr->trade=trade_between_cities(ptr->pc1,ptr->pc2);
	ptr->moves_req=calculate_best_move_trade_cost(ptr);
	ptr->turns_req=ptr->moves_req>ptr->punit->virtual_moves_left?(2+ptr->moves_req-ptr->punit->virtual_moves_left)/unit_move_rate(ptr->punit):0;
}

int my_city_num_trade_routes(struct city *pcity,bool all)
{
	int num;

	if(all)
		num=trade_route_list_size(&pcity->trade_routes);
	else
		num=0;
	if(pcity->owner==game.player_idx)
		return num+city_num_trade_routes(pcity);
	city_list_iterate(game.player_ptr->cities,ocity)
	{
		if(have_cities_trade_route(pcity,ocity))
			num++;
	} city_list_iterate_end;
	return num;
}

int trade_route_value(struct city *pc1,struct city *pc2)//pc1 is an own city
{
	int i;
	
	for(i=0;i<NUM_TRADEROUTES;i++)
		if(pc1->trade[i]==pc2->id)
			return pc1->trade_value[i];
	return MAXCOST;
}

int city_min_trade_route(struct city *pcity,struct trade_route **pptr)
{
	if(!pcity)
		return MAXCOST;
	int min;

	if(pcity->owner==game.player_idx)
		min=get_city_min_trade_route(pcity,NULL);
	else
	{
		min=MAXCOST;
		city_list_iterate(game.player_ptr->cities,ocity)
		{
			int value=trade_route_value(ocity,pcity);
			if(value<min)
				min=value;
		} city_list_iterate_end;
	}
	
	*pptr=NULL;
	trade_route_list_iterate(pcity->trade_routes,ptr)
	{
		update_trade_route(ptr);
		if(ptr->trade<=min)
		{
			min=ptr->trade;
			*pptr=ptr;
		}
	} trade_route_list_iterate_end;
	return min;
}

struct trade_route *find_same_trade_route(struct trade_route *ptr)
{
	trade_route_list_iterate(ptr->pc1->trade_routes,ctr)
	{
		if(ctr->pc1==ptr->pc2||ctr->pc2==ptr->pc2)
			return ctr;
	} trade_route_list_iterate_end;
	return NULL;
}

int calculate_move_trade_cost(struct unit *punit,struct city *pc1,struct city *pc2)
{
	if(punit->homecity==pc1->id)
		return calculate_move_cost(punit,pc2->tile);
	if(punit->owner!=pc1->owner)
		return MAXCOST;

	struct unit cunit=*punit;
	
	cunit.tile=pc1->tile;
	return calculate_move_cost(punit,pc1->tile)+calculate_move_cost(&cunit,pc2->tile);
}

int calculate_best_move_trade_cost(struct trade_route *ptr)
{
	if(ptr->pc2->owner!=game.player_idx)
		return calculate_move_trade_cost(ptr->punit,ptr->pc1,ptr->pc2);
	
	int mtc1=calculate_move_trade_cost(ptr->punit,ptr->pc1,ptr->pc2),mtc2=calculate_move_trade_cost(ptr->punit,ptr->pc2,ptr->pc1);

	if(mtc2>=mtc1)
		return mtc1;
	swap(ptr->pc1,ptr->pc2,struct city *);
	return mtc2;
}

struct trade_route *can_establish_new_trade_route(struct unit *punit,struct city *pc1,struct city *pc2)
{
	if(!can_cities_trade(pc1,pc2)||pc1==pc2||have_cities_trade_route(pc1,pc2))
		return NULL;

	struct trade_route *ptr=trade_route_new(punit,pc1,pc2,FALSE);
	if(!ptr)
		return NULL;

	ptr->tr_type=TR_NEW;
	if(my_ai_trade_level==LEVEL_ON)
	{
		if(my_city_num_trade_routes(ptr->pc1,FALSE)==NUM_TRADEROUTES)
		{
			ptr->tr_type=TR_REMP;
			if(get_city_min_trade_route(pc1,NULL)>=ptr->trade)
			{
				free(ptr);
				return NULL;
			}
		}
		if(my_city_num_trade_routes(ptr->pc2,FALSE)==NUM_TRADEROUTES)
		{
			ptr->tr_type=TR_REMP;
			if(get_city_min_trade_route(ptr->pc2,NULL)>=ptr->trade)
			{
				free(ptr);
				return NULL;
			}
		}
		return ptr;
	}
	ptr->ptr0=find_same_trade_route(ptr);
	if(ptr->ptr0)
	{
		update_trade_route(ptr->ptr0);
		if(ptr->turns_req<ptr->ptr0->turns_req||(ptr->turns_req==ptr->ptr0->turns_req&&ptr->moves_req>ptr->ptr0->moves_req))
		{
                         ptr->tr_type=ptr->ptr0->tr_type;
                         ptr->planned=ptr->ptr0->planned;
                         return ptr;
		}
		free(ptr);
		return NULL;
	}
	if(my_city_num_trade_routes(ptr->pc1,TRUE)>=NUM_TRADEROUTES)
	{
		if(city_min_trade_route(ptr->pc1,&ptr->ptr1)<ptr->trade)
			ptr->tr_type=TR_REMP;
		else
		{
			free(ptr);
			return NULL;
		}
	}
	if(my_city_num_trade_routes(ptr->pc2,TRUE)>=NUM_TRADEROUTES)
	{
		if(city_min_trade_route(ptr->pc2,&ptr->ptr2)<ptr->trade)
			ptr->tr_type=TR_REMP;
		else
		{
			free(ptr);
			return NULL;
		}
	}
	return ptr;
}

struct trade_route *best_city_trade_route(struct unit *punit,struct city *pcity)
{
	if(!pcity)
		return NULL;

	struct trade_route *btr=NULL,*ptr;
	Continent_id continent=punit->tile->continent;

	if(my_ai_trade_external)
	{
		cities_iterate(pc)
		{
			if(pc->tile->continent!=continent)
				continue;
			if(btr)
			{
				ptr=can_establish_new_trade_route(punit,pcity,pc);
				if(ptr)
				{
					if(ptr->tr_type==btr->tr_type)
					{
						if(ptr->trade==btr->trade)
						{
							if(ptr->turns_req<btr->turns_req)
                                                        {
                                                        	if(ptr->moves_req>btr->moves_req)
                                                                	swap(btr,ptr,struct trade_route *);
                                                        }
                                                        else if(ptr->turns_req<btr->turns_req)
								swap(btr,ptr,struct trade_route *);
						}
						else if(ptr->trade>btr->trade)
							swap(btr,ptr,struct trade_route *);
					}
					else if(ptr->tr_type>btr->tr_type)
						swap(btr,ptr,struct trade_route *);	
					free(ptr);
				}
			}
			else
				btr=can_establish_new_trade_route(punit,pcity,pc);
		} cities_iterate_end;
	}
	else
	{
		city_list_iterate(game.player_ptr->cities,pc)
		{
			if(pc->tile->continent!=continent)
				continue;
			if(btr)
			{
				ptr=can_establish_new_trade_route(punit,pcity,pc);
				if(ptr)
				{
					if(ptr->tr_type==btr->tr_type)
					{
						if(ptr->trade==btr->trade)
						{
							if(ptr->turns_req<btr->turns_req)
                                                        {
                                                        	if(ptr->moves_req>btr->moves_req)
                                                                	swap(btr,ptr,struct trade_route *);
                                                        }
                                                        else if(ptr->turns_req<btr->turns_req)
								swap(btr,ptr,struct trade_route *);
						}
						else if(ptr->trade>btr->trade)
							swap(btr,ptr,struct trade_route *);
					}
					else if(ptr->tr_type>btr->tr_type)
						swap(btr,ptr,struct trade_route *);	
					free(ptr);
				}
			}
			else
				btr=can_establish_new_trade_route(punit,pcity,pc);
		} city_list_iterate_end;
	}
	return btr;
}

struct trade_route *trade_route_list_find_cities(struct trade_route_list *ptrl,struct city *pc1,struct city *pc2)
{
	trade_route_list_iterate(*ptrl,ptr)
	{
		if((ptr->pc1==pc1&&ptr->pc2==pc2)||(ptr->pc1==pc2&&ptr->pc2==pc1))
			return ptr;
	} trade_route_list_iterate_end;
	return NULL;
}

/* Toggle tiles to trade code */
struct toggle_city *toggle_city_find(struct toggle_city *pcities, size_t size, struct city *pcity);
struct city *get_city_user(struct tile *pcenter_tile, int *x, int *y, int depth,
                           struct toggle_city *pcities, size_t size);
struct toggle_worker *toggle_worker_new(struct city *pcity, struct tile *ptile, int x, int y);
void toggle_worker_free(struct toggle_worker *ptw);
void recursive_add_cities(struct city *pcity, struct toggle_city *pcities, size_t size, int depth);
void sort_cities(struct toggle_city *pcities, size_t size);
void switch_tiles(struct city *pcity, struct genlist *ptcity, struct toggle_city *pcities, size_t size);
struct genlist *apply_trade(struct city *pcity1, struct city *pcity2);
void release_trade(struct genlist *ptcity);

struct toggle_city *toggle_city_find(struct toggle_city *pcities, size_t size, struct city *pcity)
{
  int i;

  for (i = 0; i < size; i++) {
    if (pcities[i].pcity == pcity) {
      return &pcities[i];
    }
  }

  return NULL;
}

struct city *get_city_user(struct tile *pcenter_tile, int *x, int *y, int depth,
                           struct toggle_city *pcities, size_t size)
{
  struct city *pcity;
  struct toggle_city *ptcity;

  square_iterate(pcenter_tile, CITY_MAP_RADIUS + 1, ptile) {
    if ((pcity = map_get_city(ptile))
        && pcity->owner == game.player_idx
        && map_to_city_map(x, y, pcity, pcenter_tile)
        && pcity->city_map[*x][*y] == C_TILE_WORKER
        && (ptcity = toggle_city_find(pcities, size, pcity))
        && ptcity->depth > depth) {
      return pcity;
    }
  } square_iterate_end;

  return NULL;
}

struct toggle_worker *toggle_worker_new(struct city *pcity, struct tile *ptile, int x, int y)
{
  assert(pcity->city_map[x][y] != C_TILE_UNAVAILABLE);

  struct toggle_worker *ptw = fc_malloc(sizeof(struct toggle_worker));

  ptw->pcity = pcity;
  ptw->ptile = ptile;
  ptw->x = x;
  ptw->y = y;
  ptw->was_used = pcity->city_map[x][y] == C_TILE_WORKER;

  freelog(LOG_VERBOSE, "Apply: Toggling tile (%d, %d) to %s for %s",
          TILE_XY(ptile), ptw->was_used ? "specialist" : "worker", pcity->name);

  if (ptw->was_used) {
    dsend_packet_city_make_specialist(&aconnection, pcity->id, x, y);
    pcity->city_map[x][y] = C_TILE_EMPTY;
    map_city_radius_iterate(ptile, tile1) {
      struct city *pcity2 = map_get_city(tile1);

      if (pcity2 && pcity2 != pcity) {
	int x2, y2;
	assert(map_to_city_map(&x2, &y2, pcity2, ptile));
	pcity2->city_map[x2][y2] = C_TILE_EMPTY;
      }
    } map_city_radius_iterate_end;
  } else {
    dsend_packet_city_make_worker(&aconnection, pcity->id, x, y);
    pcity->city_map[x][y] = C_TILE_WORKER;
    map_city_radius_iterate(ptile, tile1) {
      struct city *pcity2 = map_get_city(tile1);

      if (pcity2 && pcity2 != pcity) {
	int x2, y2;
	assert(map_to_city_map(&x2, &y2, pcity2, ptile));
	pcity2->city_map[x2][y2] = C_TILE_UNAVAILABLE;
      }
    } map_city_radius_iterate_end;
  }

  return ptw;
}

void toggle_worker_free(struct toggle_worker *ptw)
{
  freelog(LOG_VERBOSE, "Release: Toggling tile (%d, %d) to %s for %s",
          TILE_XY(ptw->ptile), ptw->was_used ? "worker" : "specialist", ptw->pcity->name);

  if (ptw->was_used) {
    dsend_packet_city_make_worker(&aconnection, ptw->pcity->id, ptw->x, ptw->y);
  } else {
    dsend_packet_city_make_specialist(&aconnection, ptw->pcity->id, ptw->x, ptw->y);
  }

  free(ptw);
}

void recursive_add_cities(struct city *pcity, struct toggle_city *pcities, size_t size, int depth)
{
  if (!pcity || pcity->owner != game.player_idx) {
    return;
  }

  struct toggle_city *ptcity = toggle_city_find(pcities, size, pcity);
  int i;

  assert(ptcity != NULL);

  if (ptcity->depth > depth) {
    ptcity->depth = depth;
    if (my_ai_establish_trade_route_level > depth) {
      for (i = 0; i < NUM_TRADEROUTES; i++) {
        recursive_add_cities(find_city_by_id(pcity->trade[i]), pcities, size, depth + 1);
      }
    }
  }
}

void sort_cities(struct toggle_city *pcities, size_t size)
{
  int i, j;

  for (i = 0; i < size; i++) {
    for (j = 0; j < i; j++) {
      if (pcities[i].depth < pcities[j].depth) {
        swap(pcities[i], pcities[j], struct toggle_city);
      }
    }
  }
}

void switch_tiles(struct city *pcity, struct genlist *ptcity, struct toggle_city *pcities, size_t csize)
{
  assert(pcity->owner == game.player_idx);

  size_t size = 0, min = MIN(pcity->size, CITY_TILES);
  struct tile *tiles[CITY_TILES];
  struct toggle_city *ttcity = toggle_city_find(pcities, csize, pcity);
  int i, j, x[CITY_TILES], y[CITY_TILES], t[CITY_TILES], trade;

  assert(ttcity != NULL);

  /* Check first the best tiles, ignoring the ones which are not available. */
  city_map_checked_iterate(pcity->tile, cx, cy, ptile) {
    if (ptile == pcity->tile) {
      continue;
    }

    trade = city_get_trade_tile(cx, cy, pcity);
    for (i = 0; i < size; i++) {
      if (trade > t[i] || (trade == t[i] && pcity->city_map[cx][cy] != C_TILE_UNAVAILABLE)) {
        break;
      }
    }

    assert(i < CITY_TILES);
    for (j = size; j > i; j--) {
      tiles[j] = tiles[j - 1];
      x[j] = x[j - 1];
      y[j] = y[j - 1];
      t[j] = t[j - 1];
    }
    tiles[i] = ptile;
    x[i] = cx;
    y[i] = cy;
    t[i] = trade;
    size++;
    assert(size <= CITY_TILES);
  } city_map_checked_iterate_end;

  /* Try to steal tiles from neighbor cities. */
  for (i = 0, j = 0; i < size && j < min; i++) {
    if (!tiles[i]) {
      continue;
    }

    if (pcity->city_map[x[i]][y[i]] == C_TILE_UNAVAILABLE) {
      int cx, cy;
      struct city *pother_city = get_city_user(tiles[i], &cx, &cy, ttcity->depth, pcities, csize);

      if (pother_city != NULL) {
        genlist_insert(ptcity, (void *)toggle_worker_new(pother_city, tiles[i], cx, cy), 0);
        j++;
      } else {
        tiles[i] = NULL;
      }
    } else {
      j++;
    }
  }

  /* Finish the iteration */
  for (; i < size; i++) {
    if (tiles[i] && pcity->city_map[x[i]][y[i]] == C_TILE_WORKER) {
      genlist_insert(ptcity, (void *)toggle_worker_new(pcity, tiles[i], x[i], y[i]), 0);
    }
    tiles[i] = NULL;
  }

  /* Take the best tiles */
  for (i = 0; i < size; i++) {
    if (tiles[i] && pcity->city_map[x[i]][y[i]] != C_TILE_WORKER) {
      genlist_insert(ptcity, (void *)toggle_worker_new(pcity, tiles[i], x[i], y[i]), 0);
    }
  }
}

/* Choose the best trade tiles for the cities. */
struct genlist *apply_trade(struct city *pcity1, struct city *pcity2)
{
  size_t size = city_list_size(&game.player_ptr->cities);
  struct toggle_city cities[size];
  int i;
  struct genlist *ptcity = fc_malloc(sizeof(struct genlist));
  genlist_init(ptcity);

  if (my_ai_establish_trade_route_level > 0) {
    i = 0;
    city_list_iterate(game.player_ptr->cities, pcity) {
      cities[i].pcity = pcity;
      cities[i].depth = size;
      memcpy(cities[i].city_map, pcity->city_map, sizeof(pcity->city_map));
      i++;
    } city_list_iterate_end;

    recursive_add_cities(pcity1, cities, size, 1);
    recursive_add_cities(pcity2, cities, size, 1);
    sort_cities(cities, size);
  
    for (i = 0; i < size; i++) {
      if (cities[i].depth < size) {
        switch_tiles(cities[i].pcity, ptcity, cities, size);
      }
    }
    for (i = 0; i < size; i++) {
      memcpy(cities[i].pcity->city_map, cities[i].city_map, sizeof(cities[i].city_map));
    }
  }
  return ptcity;
}

void release_trade(struct genlist *ptcity)
{
  if (!ptcity) {
    return;
  }

  struct toggle_worker *ptw;
  while ((ptw = (struct toggle_worker *)genlist_get(ptcity, 0))) {
    genlist_unlink(ptcity, ptw);
    toggle_worker_free(ptw);
  }
  free(ptcity);
}

void my_ai_trade_route_alloc(struct trade_route *ptr)
{
        char buf[1024];
	struct trade_route *ptr0=ptr->ptr0,*ptr1=ptr->ptr1,*ptr2=ptr->ptr2;
	struct unit *punit=ptr->punit,*punit0=(ptr0?ptr0->punit:NULL),*punit1=(ptr1?ptr1->punit:NULL),*punit2=(ptr2?ptr2->punit:NULL);

	freelog(LOG_VERBOSE,"alloc auto-trade orders for %s (%d/%d)",unit_name(ptr->punit->type),ptr->punit->tile->x,ptr->punit->tile->y);

	punit->my_ai.control=TRUE;
	punit->my_ai.activity=MY_AI_TRADE_ROUTE;
	punit->my_ai.data=(void *)ptr;
	unit_list_append(&traders,punit);
	trade_route_list_unlink(&trade_plan,ptr);
	trade_route_list_append(&ptr->pc1->trade_routes,ptr);
	trade_route_list_append(&ptr->pc2->trade_routes,ptr);
        ptr->ptr0=NULL;
        ptr->ptr1=NULL;
        ptr->ptr2=NULL;
        update_trade_route(ptr);
        my_snprintf(buf,sizeof(buf),_("PepClient: Sending %s %d to establish trade route between %s and %s (%d move%s, %d turn%s)"),
                    unit_type(punit)->name,punit->id,ptr->pc1->name,ptr->pc2->name,ptr->moves_req,ptr->moves_req>1?"s":"",ptr->turns_req,ptr->turns_req>1?"s":"");
	if(my_ai_trade_level>LEVEL_ON)
	{
		if(ptr0&&punit0&&punit0->my_ai.data==ptr0)
		{
			my_ai_trade_route_free(punit0);
			my_ai_caravan(punit0);
		}
		if(my_ai_trade_level==LEVEL_BEST)
		{
			if(ptr1&&punit1&&punit1->my_ai.data==ptr1)
			{
				my_ai_trade_route_free(punit1);
				my_ai_caravan(punit1);
			}
			if(ptr2&&punit2&&punit2->my_ai.data==ptr2)
			{
				my_ai_trade_route_free(punit2);
				my_ai_caravan(punit2);
			}
		}
	}
        /* maybe the route was free */
        if(punit->my_ai.data==ptr)
        {
        	append_output_window(buf);
        }
        update_auto_caravan_menu();
        update_miscellaneous_menu();
	if(draw_city_traderoutes)
		update_map_canvas_visible();
}

void my_ai_trade_route_alloc_city(struct unit *punit,struct city *pcity)
{
	if(!punit||!pcity||!unit_flag(punit,F_TRADE_ROUTE))
		return;

	char buf[256]="\0";
	struct city *hcity=player_find_city_by_id(game.player_ptr,punit->homecity);
	struct trade_route *ptr;

	if(!can_cities_trade(hcity,pcity))
		my_snprintf(buf,sizeof(buf),"PepClient: A trade route is impossible between %s and %s.",hcity->name,pcity->name);
	else if(have_cities_trade_route(hcity,pcity))
		my_snprintf(buf,sizeof(buf),"PepClient: %s and %s have already a trade route.",hcity->name,pcity->name);
	else if((ptr=trade_route_list_find_cities(&trade_plan,hcity,pcity)))
	{
		my_ai_orders_free(punit);
		ptr->punit=punit;
		update_trade_route(ptr);
		my_ai_trade_route_alloc(ptr);
		automatic_processus_event(AUTO_ORDERS,punit);
	}
	else if(trade_route_list_size(&trade_plan)&&!my_ai_trade_manual_trade_route_enable)
		my_snprintf(buf,sizeof(buf),"PepClient: The trade route %s-%s is not planned, the unit cannot execute this order.",hcity->name,pcity->name);
	else
	{
		my_ai_orders_free(punit);
		if(trade_route_list_size(&trade_plan))
			my_snprintf(buf,sizeof(buf),"PepClient: warning: the trade route %s-%s is not planned.",hcity->name,pcity->name);
		my_ai_trade_route_alloc(trade_route_new(punit,hcity,pcity,FALSE));
		automatic_processus_event(AUTO_ORDERS,punit);
	}
	if(buf[0])
		append_output_window(buf);
}

void my_ai_trade_route_execute(struct unit *punit)
{
	if(!my_ai_trade_level)
		return;
	if(!punit->my_ai.control||punit->my_ai.activity!=MY_AI_TRADE_ROUTE)
		return;

	struct trade_route *ptr=(struct trade_route *)punit->my_ai.data;
	struct unit cunit=*punit;

	if(cunit.homecity!=ptr->pc1->id)
	{
		goto_and_request((&cunit),ptr->pc1->tile)
		{
			dsend_packet_unit_change_homecity(&aconnection,punit->id,ptr->pc1->id);
			cunit.tile=ptr->pc1->tile;
			cunit.homecity=ptr->pc1->id;
		}
                else if(my_ai_trade_level==LEVEL_BEST)
                {
                        request_new_unit_activity(punit, ACTIVITY_IDLE);
                }
	}
	if(cunit.homecity==ptr->pc1->id)
	{
		goto_and_request((&cunit),ptr->pc2->tile)
		{
			char buf[256];
                        struct genlist *trade;

                        connection_do_buffer(&aconnection);
                        trade = apply_trade(ptr->pc1, ptr->pc2);
			dsend_packet_unit_establish_trade(&aconnection,punit->id);
                        release_trade(trade);
                        connection_do_unbuffer(&aconnection);
			my_snprintf(buf,sizeof(buf),_("PepClient: %s establishing trade route between %s and %s"),unit_name(punit->type),ptr->pc1->name,ptr->pc2->name);
			append_output_window(buf);
		}
                else if(my_ai_trade_level==LEVEL_BEST)
                {
                        request_new_unit_activity(punit, ACTIVITY_IDLE);
                }
	}
}

void my_ai_trade_route_execute_all(void)
{
	if(!my_ai_enable||!my_ai_trade_level||!unit_list_size(&traders))
		return;
	append_output_window(_("PepClient: Executing all automatic trade route orders..."));
	connection_do_buffer(&aconnection);
	unit_list_iterate(traders,punit)
	{
		my_ai_trade_route_execute(punit);
	} unit_list_iterate_end;
	connection_do_unbuffer(&aconnection);
}

void my_ai_trade_route_free(struct unit *punit)
{
        char buf[1024];
	struct trade_route *ptr=(struct trade_route *)punit->my_ai.data;

	freelog(LOG_VERBOSE,"free auto-trade orders for %s (%d/%d)",unit_name(punit->type),punit->tile->x,punit->tile->y);

	punit->my_ai.activity=MY_AI_NONE;
	punit->my_ai.data=NULL;
	unit_list_unlink(&traders,punit);
	trade_route_list_unlink(&ptr->pc1->trade_routes,ptr);
	trade_route_list_unlink(&ptr->pc2->trade_routes,ptr);
        if(punit->tile!=ptr->pc2->tile)
        {
          my_snprintf(buf,sizeof(buf),_("PepClient: Cancelling trade route between %s and %s"),ptr->pc1->name,ptr->pc2->name);
          append_output_window(buf);
          if(ptr->planned)
            trade_route_list_append(&trade_plan,ptr);
          else
            free(ptr);
        }
	else
		free(ptr);
	if(draw_city_traderoutes)
		update_map_canvas_visible();
}

/* trade planning */
int get_real_trade_route_number(struct city *pcity);
bool cities_will_have_trade(struct city *pcity1, struct city *pcity2);
static struct trade_route *add_trade_route_in_planning
        (struct trade_city *tcities, struct trade_route_list *ptrlist,
         struct trade_configuration *pconf, int c1, int c2);
void recursive_calculate_trade_planning(time_t max_time,
                                        struct trade_city *tcities, int size,
                                        int start_city, int start_trade,
                                        struct trade_route_list *ctlist,
                                        struct trade_configuration *ctconf,
                                        struct trade_route_list *btlist,
                                        struct trade_configuration *btconf);
void calculate_trade_planning(char *buf, size_t buf_len);

/****************************************************************************
  Count the trade routes, but ignore the planned ones.
****************************************************************************/
int get_real_trade_route_number(struct city *pcity)
{
  return my_city_num_trade_routes(pcity, TRUE)
         + estimate_non_ai_trade_route_number(pcity);
}

/****************************************************************************
  Have cities trade route, or will have soon.
****************************************************************************/
bool cities_will_have_trade(struct city *pcity1, struct city *pcity2)
{
  if (have_cities_trade_route(pcity1, pcity2)) {
    return TRUE;
  }

  trade_route_list_iterate(pcity1->trade_routes, ptr) {
    if (ptr->pc1 == pcity2 || ptr->pc2 == pcity2)
      return TRUE;
  } trade_route_list_iterate_end;

  if (my_ai_trade_manual_trade_route_enable) {
    trade_route_list_iterate(non_ai_trade,ptr)
    {
      if ((ptr->pc1 == pcity1 && ptr->pc2 == pcity2)
          || (ptr->pc1 == pcity2 && ptr->pc2 == pcity1)) {
        return TRUE;
      }
    } trade_route_list_iterate_end;
  }

  return FALSE;
}

/****************************************************************************
  Add a trade route in the trade planning.
  Returns the added trade route.
****************************************************************************/
struct trade_route *add_trade_route_in_planning
        (struct trade_city *tcities, struct trade_route_list *ptrlist,
         struct trade_configuration *pconf, int c1, int c2)
{
  struct trade_route *ptr;

  ptr = tcities[c1].trade_routes[c2];
  if (!ptr || ptr != tcities[c2].trade_routes[c1]) {
    freelog(LOG_ERROR,
            "Trying to add a bad trade route in the trade planning (%s - %s)",
            tcities[c1].pcity->name, tcities[c2].pcity->name);
    return NULL;
  }

  /* Change cities datas */
  tcities[c1].free_slots--;
  tcities[c1].trade_routes[c2] = NULL;
  tcities[c1].trade_routes_num--;
  tcities[c2].free_slots--;
  tcities[c2].trade_routes[c1] = NULL;
  tcities[c2].trade_routes_num--;

  /* Change configuration */
  trade_route_list_append(ptrlist, ptr);
  pconf->free_slots -= 2;
  if (ptr->moves_req < MAXCOST) {
    pconf->moves += ptr->moves_req;
    pconf->turns += ptr->turns_req;
  } else {
    pconf->turns += tcities[c1].distance[c2];
    pconf->moves += tcities[c1].distance[c2] * SINGLE_MOVE;
  }

  return ptr;
}

/****************************************************************************
  Calculate the trade planning...

  max_time:    maximum time in seconds since 1970.
  tcities:     the list of the trade_city
  size:        the size of this list
  start_city:  begin the calculation by the city with this index.
  start_trade: begin the calculation by this city.
  ctlist:      current trade planning
  ctconf:      current trade configuration
  btlist:      best trade planning
  btconf:      best trade configuration
****************************************************************************/
void recursive_calculate_trade_planning(time_t max_time,
                                        struct trade_city *tcities, int size,
                                        int start_city, int start_trade,
                                        struct trade_route_list *ctlist,
                                        struct trade_configuration *ctconf,
                                        struct trade_route_list *btlist,
                                        struct trade_configuration *btconf)
{
  int i, j;

  for (i = start_city; i < size; i++) {
    if (max_time && time(NULL) > max_time) {
      break;
    }
    if (tcities[i].free_slots <= 0 || tcities[i].trade_routes_num <= 0) {
      continue;
    }

    for (j = i == start_city ? start_trade : i + 1; j < size; j++) {
      if (max_time && time(NULL) > max_time) {
        break;
      }
      if (tcities[j].free_slots <= 0 || !tcities[i].trade_routes[j]) {
        continue;
      }

      /* Plan this trade route */
      struct trade_configuration conf;
      struct trade_route *ptr;

      conf = *ctconf;
      ptr = add_trade_route_in_planning(tcities, ctlist, ctconf, i, j);

      /* Do recursive calculation */
      recursive_calculate_trade_planning(max_time, tcities, size, i, j + 1,
                                         ctlist, ctconf, btlist, btconf);

      /* Reverse */
      trade_route_list_unlink(ctlist, ptr);
      *ctconf = conf;
      tcities[i].free_slots++;
      tcities[i].trade_routes[j] = ptr;
      tcities[i].trade_routes_num++;
      tcities[j].free_slots++;
      tcities[j].trade_routes[i] = ptr;
      tcities[j].trade_routes_num++;
    }
  }

  /* Is the best configuration? */
  if (ctconf->free_slots < btconf->free_slots
      || (ctconf->free_slots == btconf->free_slots
          && (ctconf->turns < btconf->turns
              || (ctconf->turns == btconf->turns
                  && ctconf->moves < btconf->moves)))) {
    trade_route_list_copy(btlist, ctlist);
    *btconf = *ctconf;
  }
}

/****************************************************************************
  Calculate trade planning for the cities in the city list.
  time_limit = 0 means there isn't time limit.
****************************************************************************/
void calculate_trade_planning(char *buf, size_t buf_len)
{
  int i, j, size = city_list_size(&trade_cities), dist;
  struct trade_route_list btrlist, ptrlist;
  struct trade_city tcities[size];
  struct trade_configuration bconf, cconf;
  struct unit *caravan = NULL;

  buf[0] = '\0';
  if (size <= 0) {
    return;
  }

  unit_type_iterate(type) {
    if (unit_type_flag(type, F_TRADE_ROUTE)) {
      caravan = create_unit_virtual(game.player_ptr,
                                    city_list_get(&game.player_ptr->cities, 0),
                                    type, 0);
      break;
    }
  } unit_type_iterate_end;
  assert(caravan);
  caravan->moves_left = 0;

  /* Initialize */
  i = 0;
  trade_route_list_init(&btrlist);
  trade_route_list_init(&ptrlist);
  cconf.free_slots = 0;
  cconf.turns = 0;
  cconf.moves = 0;
  city_list_iterate(trade_cities, pcity) {
    tcities[i].pcity = pcity;
    tcities[i].free_slots = NUM_TRADEROUTES
                            - get_real_trade_route_number(pcity);
    tcities[i].trade_routes_num = 0;
    tcities[i].trade_routes = fc_malloc(size * sizeof(struct trade_route *));
    tcities[i].distance = fc_malloc(size * sizeof(int));

    /* Check if a trade route is possible with the lower index cities */
    for (j = 0; j <= i; j++) {
      if (tcities[i].free_slots > 0 && tcities[j].free_slots > 0
          && can_cities_trade(tcities[i].pcity, tcities[j].pcity)
          && !cities_will_have_trade(tcities[i].pcity, tcities[j].pcity)) {
        struct trade_route *ptr = trade_route_new(NULL, tcities[i].pcity,
                                                  tcities[j].pcity, TRUE);

        dist = real_map_distance(pcity->tile, tcities[j].pcity->tile);
        caravan->homecity = pcity->id;
        caravan->tile = pcity->tile;
        ptr->punit = caravan;
        update_trade_route(ptr);
        ptr->punit = NULL;
        tcities[i].trade_routes[j] = ptr;
        tcities[j].trade_routes[i] = ptr;
        tcities[i].distance[j] = dist;
        tcities[j].distance[i] = dist;
        tcities[i].trade_routes_num++;
        tcities[j].trade_routes_num++;
      } else {
        tcities[i].trade_routes[j] = NULL;
        tcities[j].trade_routes[i] = NULL;
      }
    }

    cconf.free_slots += tcities[i].free_slots;
    i++;
  } city_list_iterate_end;

  /* Improvation of the calculation: allocate first the harder cities */
  do {
    j = -1;
    /* Find the hardest */
    for (i = 0; i < size; i++) {
      if (tcities[i].trade_routes_num > 0
          && tcities[i].trade_routes_num <= tcities[i].free_slots) {
        if (j < 0
            || tcities[i].free_slots - tcities[i].trade_routes_num
              > tcities[j].free_slots - tcities[j].trade_routes_num) {
          j = i;
        }
      }
    }
    if (j >= 0) {
      /* Give it all its trade routes */
      for (i = 0; i < size; i++) {
        if (tcities[j].trade_routes[i] && tcities[i].free_slots > 0) {
          /* Add a trade route in the planning */
          add_trade_route_in_planning(tcities, &ptrlist, &cconf, j, i);

          /*
           * If the city doesn't have free slots anymore,
           * clear its possible trade roures
           */
          if (tcities[i].free_slots == 0) {
            struct trade_route *ptr;
            int k;

            for (k = 0; k < size; k++) {
              if ((ptr = tcities[i].trade_routes[k])) {
                tcities[i].trade_routes[k] = NULL;
                tcities[i].trade_routes_num--;
                tcities[k].trade_routes[i] = NULL;
                tcities[k].trade_routes_num--;
                free(ptr);
              }
            }
            assert(tcities[i].trade_routes_num == 0);
          }
        }
      }
      assert(tcities[j].trade_routes_num == 0);
    }
  } while (j >= 0);

  /* Set the current configuration as best configuration */
  trade_route_list_copy(&btrlist, &ptrlist);
  bconf = cconf;

  /* Do the recursive calculation */
  if (bconf.free_slots > 0) {
    recursive_calculate_trade_planning(my_ai_trade_plan_time_max
                                       ? time(NULL) + my_ai_trade_plan_time_max
                                       : 0, tcities, size, 0, 1,
                                       &ptrlist, &cconf, &btrlist, &bconf);
  }

  /* Free datas */
  destroy_unit_virtual(caravan);
  trade_route_list_free(&ptrlist);
  for (i = 0; i < size; i++) {
    for (j = i; j < size; j++) {
      if (tcities[i].trade_routes[j]) {
        free(tcities[i].trade_routes[j]);
      }
    }
    free(tcities[i].trade_routes);
    free(tcities[i].distance);
  }

  /* Apply */
  if (trade_route_list_size(&btrlist) > 0) {
    trade_route_list_copy(&trade_plan, &btrlist);
    trade_route_list_free(&btrlist);
    my_snprintf(buf, buf_len,
                _("%d new trade routes, with a total of %d moves"),
                trade_route_list_size(&trade_plan), bconf.moves);
  } else {
     my_snprintf(buf, buf_len, _("Didn't find any trade routes to establish"));
  }
}

void show_cities_in_trade_plan(void)
{
	if(city_list_size(&trade_cities)<=0)
		return;

	char buf[1024];
	bool first=TRUE;

	strcpy(buf,_("PepClient: Cities in trade plan:"));
	city_list_iterate(trade_cities,pcity)
	{
		cat_snprintf(buf,sizeof(buf),"%s %s",first?"":",",pcity->name);
		first=FALSE;
	} city_list_iterate_end;
	strcat(buf,".");
	append_output_window(buf);
}

void clear_my_ai_trade_cities(void)
{
	trade_route_list_free(&trade_plan);
	city_list_unlink_all(&trade_cities);
	if(draw_city_traderoutes)
		update_map_canvas_visible();
	append_output_window(_("PepClient: Trade city list cleared."));
	update_auto_caravan_menu();
}

void show_free_slots_in_trade_plan(void)
{
	char buf[1024]="\0";
	int count =0;

	city_list_iterate(trade_cities,pcity)
	{
		int num=count_trade_routes(pcity)+estimate_non_ai_trade_route_number(pcity);
		if(num<NUM_TRADEROUTES)
		{
			cat_snprintf(buf,sizeof(buf),"%s %s (%d)",!count?"":",",pcity->name,NUM_TRADEROUTES-num);
			count+=NUM_TRADEROUTES-num;
		}
	} city_list_iterate_end;
	if(!count)
		append_output_window(_("PepClient: No trade route free slot."));
	else
	{
		char buf2[1024];
		my_snprintf(buf2,sizeof(buf2),_("PepClient: %d trade route free slot%s:%s."),count,count==1?"":"s",buf);
		append_output_window(buf2);
	}
}

void recalculate_trade_plan(void)
{
	char buf[1024], buf2[1024];

	freelog(LOG_VERBOSE,"Calculating automatic trade routes.");
	calculate_trade_planning(buf,sizeof(buf));
        my_snprintf(buf2,sizeof(buf2),_("PepClient: Trade route plan calculation done%s%s."),buf[0]=='\0'?"":": ",buf);
	append_output_window(buf2);
	show_free_slots_in_trade_plan();
	if(draw_city_traderoutes)
		update_map_canvas_visible();
}

void my_ai_add_trade_city(struct city *pcity,bool multi)
{
	if(!pcity)
		return;

	char buf[256]="\0";

	if((city_list_find(&trade_cities,pcity)))
	{
		if(!multi)
		{
			city_list_unlink(&trade_cities,pcity);
			my_snprintf(buf,sizeof(buf),_("PepClient: Remove city %s to trade plan"),pcity->name);
		}
	}
	else
	{
                city_list_append(&trade_cities,pcity);
		my_snprintf(buf,sizeof(buf),_("PepClient: Adding city %s to trade plan"),pcity->name);
	}

	if(buf[0])
	{
		append_output_window(buf);
		update_auto_caravan_menu();
	}
	if(my_ai_trade_plan_recalculate_auto&&!multi)
		recalculate_trade_plan();
}

struct trade_route_list *estimate_non_ai_trade_route(void)
{
	return &non_ai_trade;
}

int estimate_non_ai_trade_route_number(struct city *pcity)
{
	if(!my_ai_trade_manual_trade_route_enable)
		return 0;
	int count=0;

	trade_route_list_iterate(non_ai_trade,ptr)
	{
		if(ptr->pc1==pcity||ptr->pc2==pcity)
			count++;
	} trade_route_list_iterate_end;
	return count;
}

void calculate_trade_estimation(void)
{
        if(!unit_list_size(&traders)&&!trade_route_list_size(&non_ai_trade))
        {
                append_output_window(_("PepClient: No trade routes to estimate."));
                return;
        }

        char buf[1024],ai_trade_buf[MAX_ESTIMATED_TURNS][1024],non_ai_trade_buf[MAX_ESTIMATED_TURNS][1024];
        int i,ai_trade_count[MAX_ESTIMATED_TURNS],non_ai_trade_count[MAX_ESTIMATED_TURNS];
        for(i=0;i<MAX_ESTIMATED_TURNS;i++)
        {
                ai_trade_buf[i][0]='\0';
                ai_trade_count[i]=0;
                non_ai_trade_buf[i][0]='\0';
                non_ai_trade_count[i]=0;
        }
        unit_list_iterate(traders,punit)
        {
                struct trade_route *ptr=(struct trade_route *)punit->my_ai.data;
                update_trade_route(ptr);
                i=ptr->turns_req;
                if(i>=MAX_ESTIMATED_TURNS)
                        continue;
                cat_snprintf(ai_trade_buf[i],sizeof(ai_trade_buf[i]),"%s%s-%s",ai_trade_count[i]?", ":"",ptr->pc1->name,ptr->pc2->name);
                ai_trade_count[i]++;
        } unit_list_iterate_end;
	trade_route_list_iterate(non_ai_trade,ptr)
        {
                update_trade_route(ptr);
                i=ptr->turns_req;
                if(i>=MAX_ESTIMATED_TURNS)
                        continue;
                cat_snprintf(non_ai_trade_buf[i],sizeof(non_ai_trade_buf[i]),"%s%s-%s",non_ai_trade_count[i]?", ":"",ptr->pc1->name,ptr->pc2->name);
                non_ai_trade_count[i]++;
        } trade_route_list_iterate_end;
        append_output_window(_("PepClient: Trade estimation:"));
        for(i=0;i<MAX_ESTIMATED_TURNS;i++)
        {
                if(ai_trade_count[i])
                {
                        snprintf(buf,sizeof(buf),_("PepClient: %d turn%s - %d trade route%s: %s."),
                                 i,i>1?"s":"",ai_trade_count[i],ai_trade_count[i]>1?"s":"",ai_trade_buf[i]);
                        append_output_window(buf);
                }
                if(non_ai_trade_count[i]&&my_ai_trade_manual_trade_route_enable)
                {
                        snprintf(buf,sizeof(buf),_("PepClient: %d turn%s - (%d manual trade route%s: %s)."),
                                 i,i>1?"s":"",non_ai_trade_count[i],non_ai_trade_count[i]>1?"s":"",non_ai_trade_buf[i]);
                        append_output_window(buf);
                }
        }
}

void non_ai_trade_change(struct unit *punit,int action)//0: none, 1: cancel, 2: new, 3: cancel & new
{
	if(action==0)
		return;

	struct trade_route *ptr=NULL;

	trade_route_list_iterate(non_ai_trade,itr)
	{
		if(itr->punit==punit)
		{
			ptr=itr;
			break;
		}
	} trade_route_list_iterate_end;

	if(ptr&&action&1)
	{
		trade_route_list_unlink(&non_ai_trade,ptr);
		free(ptr);
	}
	
	if(action&2&&punit->goto_tile&&punit->goto_tile->city)
		trade_route_list_append(&non_ai_trade,trade_route_new(punit,player_find_city_by_id(game.player_ptr,punit->homecity),
			punit->goto_tile->city,FALSE));

	if(draw_city_traderoutes&&my_ai_trade_manual_trade_route_enable)
		update_map_canvas_visible();
       update_auto_caravan_menu();
}

int count_trade_routes(struct city *pcity)
{
	int count=my_city_num_trade_routes(pcity,TRUE);

	trade_route_list_iterate(trade_plan,ptr)
	{
		if(ptr->pc1==pcity||ptr->pc2==pcity)
			count++;
	} trade_route_list_iterate_end;
	return count;
}

struct trade_route_list *my_ai_trade_plan_get(void)
{
	return &trade_plan;
}


struct city_list *my_ai_get_trade_cities(void)
{
	return &trade_cities;
}

/**************************************************************************
 my_ai_help_wonder
**************************************************************************/
struct help_wonder *help_wonder_new(struct city *pcity,int level,int id);
void help_wonder_destroy(struct help_wonder *phw);
int calculate_shield_cost(struct help_wonder *phw);
void update_help_wonders(struct help_wonder *phw);
struct help_wonder *find_first_wonder(struct unit *punit,struct city *pcity,struct help_wonder_list *pwhlist);
struct help_wonder *best_wonder(struct unit *punit);
struct unit *find_first_helper(struct city *pcity,struct help_wonder_list *phwlist);
bool realloc_help_wonder(struct city *pcity,struct help_wonder_list *dest,struct help_wonder_list *src);
int are_wonders_list_equal(struct help_wonder_list *old,struct help_wonder_list *new);//return 0:yes 1:advance 2:add 3:remove last 4:change

struct help_wonder *help_wonder_new(struct city *pcity,int level,int id)
{
	struct help_wonder *phw=fc_malloc(sizeof(struct help_wonder));

	hw_unit_list_init(&phw->units);
	phw->pcity=pcity;
	phw->level=level;
	phw->id=id;
	return phw;
}

void help_wonder_destroy(struct help_wonder *phw)
{
	hw_unit_list_iterate(phw->units,phw_unit)
	{
		unit_list_append(&noners,phw_unit->punit);
		unit_list_append(&temp,phw_unit->punit);
		my_ai_help_wonder_free(phw_unit->punit);
	} hw_unit_list_iterate_end;
	hw_unit_list_unlink_all(&phw->units);
	free(phw);
}

int calculate_shield_cost(struct help_wonder *phw)
{
	int cost=impr_build_shield_cost(phw->id);

	hw_unit_list_iterate(phw->units,phw_unit)
	{
		cost-=unit_build_shield_cost(phw_unit->punit->type);
	} hw_unit_list_iterate_end;
	return cost;
}

void update_help_wonders(struct help_wonder *phw)
{
	phw->punit0=NULL;
	if(phw->level==-1)
	{
		phw->wonder_cost=calculate_shield_cost(phw)-phw->pcity->shield_stock;
		phw->turns=0;
	}
	else
	{
		phw->wonder_cost=calculate_shield_cost(phw);
		struct worklist *pwl=&phw->pcity->worklist;
		int i;

		phw->turns=city_turns_to_build(phw->pcity,phw->pcity->currently_building,phw->pcity->is_building_unit,TRUE);
		for(i=0;i<phw->level;i++)
			phw->turns+=city_turns_to_build(phw->pcity,pwl->wlids[i],(pwl->wlefs[i]==WEF_UNIT),FALSE);
	}
	struct unit *punit;
	hw_unit_list_iterate(phw->units,phw_unit)
	{
		punit=phw_unit->punit;
		phw_unit->move_req=((calculate_move_cost(punit,phw->pcity->tile)-punit->virtual_moves_left)*SINGLE_MOVE)/unit_move_rate(punit);
	} hw_unit_list_iterate_end;
}

struct help_wonder *find_first_wonder(struct unit *punit,struct city *pcity,struct help_wonder_list *pwhlist)
{
	if(!pwhlist)
		pwhlist=&pcity->help_wonders;
	if(my_ai_wonder_level==LEVEL_ON)
	{
		struct help_wonder *phw=help_wonder_list_get(pwhlist,0);
		if(phw->level!=-1)
			return NULL;
		update_help_wonders(phw);
		return phw;
	}

	int min=0,move_req=((calculate_move_cost(punit,pcity->tile)-punit->virtual_moves_left)*SINGLE_MOVE)/unit_move_rate(punit);

	help_wonder_list_iterate(*pwhlist,phw)
	{
		if(my_ai_wonder_level==LEVEL_BEST)
		{
			if(phw->level==-1)
				min=move_req*pcity->shield_surplus;
			else
				min=0;
		}
		update_help_wonders(phw);
		if(phw->wonder_cost>min)
			return phw;
		else
		{
			hw_unit_list_iterate(phw->units,phw_unit)
			{
				if(phw_unit->move_req>move_req)
				{
					phw->punit0=phw_unit->punit;
					return phw;
				}
			} hw_unit_list_iterate_end;
		}
	} help_wonder_list_iterate_end;
	return NULL;
}

struct help_wonder *best_wonder(struct unit *punit)
{
	Continent_id cid=punit->tile->continent;
	struct help_wonder *bhw=NULL,*phw=NULL;

	city_list_iterate(game.player_ptr->cities,pcity)
	{
		if(pcity->tile->continent!=cid||!help_wonder_list_size(&pcity->help_wonders))
			continue;
		if(bhw)
		{
			phw=find_first_wonder(punit,pcity,NULL);
			if(phw&&phw->turns<bhw->turns)
				bhw=phw;
		}
		else
			bhw=find_first_wonder(punit,pcity,NULL);
	} city_list_iterate_end;
	
	return bhw;
}

struct unit *find_first_helper(struct city *pcity,struct help_wonder_list *phwlist)
{
	struct unit *bunit=NULL;
	int best=MAXCOST,cost;
	help_wonder_list_iterate(*phwlist,phw)
	{
		hw_unit_list_iterate(phw->units,phw_unit)
		{
			cost=calculate_move_cost(phw_unit->punit,pcity->tile);
			if(!cost)
				return phw_unit->punit;
			if(cost<best)
			{
				bunit=phw_unit->punit;
			}
		} hw_unit_list_iterate_end;
	} help_wonder_list_iterate_end;
	return bunit;
}

bool realloc_help_wonder(struct city *pcity,struct help_wonder_list *dest,struct help_wonder_list *src)
{
	struct unit *punit=find_first_helper(pcity,src);
	if(!punit)
		return FALSE;

	struct help_wonder *phw=find_first_wonder(punit,pcity,dest);
	if(!phw)
		return FALSE;

	my_ai_help_wonder_free(punit);
	my_ai_help_wonder_alloc(punit,phw);
	automatic_processus_event(AUTO_ORDERS,punit);
	return TRUE;
}

int are_wonders_list_equal(struct help_wonder_list *old,struct help_wonder_list *new)//return 0:yes 1:advance 2:add 3:remove 4:change
{
	int size1=help_wonder_list_size(old),size2=help_wonder_list_size(new),i;
	struct help_wonder *phw1,*phw2;

	/* are equal ? */
	if(size1==size2)
	{
		for(i=0;i<size1;i++)
		{
			phw1=help_wonder_list_get(old,i);
			phw2=help_wonder_list_get(new,i);
			if(phw1->level!=phw2->level||phw1->id!=phw2->id)
				break;
		}
		if(i==size1)
			return 0;
	}

	/* advance ? */
	bool cond=(size1&&size1==size2+1&&help_wonder_list_get(old,0)->level==-1);
	if(size1==size2||cond)
	{
		for(i=0;i<size2;i++)
		{
			phw1=help_wonder_list_get(old,i+cond);
			phw2=help_wonder_list_get(new,i);
			if(phw1->level!=phw2->level+1||phw1->id!=phw2->id)
				break;
		}
		if(i==size2)
			return 1;
	}

	/* add ? */
	if(size1<size2)
	{
		for(i=0;i<size1;i++)
		{
			phw1=help_wonder_list_get(old,i);
			phw2=help_wonder_list_get(new,i);
			if(phw1->level!=phw2->level||phw1->id!=phw2->id)
				break;
		}
		if(i==size1)
			return 2;
	}

	/* remove ? */
	if(size1>size2)
	{
		for(i=0;i<size2;i++)
		{
			phw1=help_wonder_list_get(old,i);
			phw2=help_wonder_list_get(new,i);
			if(phw1->level!=phw2->level||phw1->id!=phw2->id)
				break;
		}
		if(i==size2)
			return 3;
	}
	
	return 4;
}

void my_ai_help_wonder_alloc(struct unit *punit,struct help_wonder *phw)
{
	struct hw_unit *phw_unit=fc_malloc(sizeof(struct hw_unit));

	freelog(LOG_VERBOSE,"alloc auto-wonders orders for %s (%d/%d)",unit_name(punit->type),punit->tile->x,punit->tile->y);

	phw_unit->punit=punit;
	phw_unit->phw=phw;
	hw_unit_list_append(&phw->units,phw_unit);
	punit->my_ai.control=TRUE;
	punit->my_ai.activity=MY_AI_HELP_WONDER;
	punit->my_ai.data=(void *)phw;
	unit_list_append(&helpers,punit);
	if(my_ai_wonder_level==LEVEL_ON)
		return;
	if(phw->punit0)
	{
		my_ai_orders_free(phw->punit0);
		my_ai_caravan(phw->punit0);
	}
        update_auto_caravan_menu();
        update_miscellaneous_menu();
}

void my_ai_help_wonder_execute(struct unit *punit)
{
	if(!my_ai_wonder_level)
		return;
	if(!punit)
	{
		freelog(LOG_ERROR,"auto_wonders_orders called with a bad unit");
		return;
	}
	if(!punit->my_ai.control||punit->my_ai.activity!=MY_AI_HELP_WONDER)
		return;

	struct help_wonder *phw=(struct help_wonder *)punit->my_ai.data;

	goto_and_request(punit,phw->pcity->tile)
	{
		if(phw->level==-1)
		{
			char buf[256];
			dsend_packet_unit_help_build_wonder(&aconnection,punit->id);
			my_snprintf(buf,sizeof(buf),_("PepClient: %s helping to %s in %s"),unit_name(punit->type),get_improvement_name(phw->id),phw->pcity->name);
			append_output_window(buf);
		}
	}
}

void my_ai_help_wonder_execute_all(void)
{
	if(!my_ai_enable||!my_ai_wonder_level||!unit_list_size(&helpers))
		return;
	append_output_window(_("PepClient: Executing all automatic help wonder orders..."));
	connection_do_buffer(&aconnection);
	unit_list_iterate(helpers,punit)
	{
		my_ai_help_wonder_execute(punit);
	} unit_list_iterate_end;
	connection_do_unbuffer(&aconnection);
}

void my_ai_help_wonder_free(struct unit *punit)
{
	struct help_wonder *phw=(struct help_wonder *)punit->my_ai.data;

	freelog(LOG_VERBOSE,"free auto-wonders orders for %s (%d/%d)",unit_name(punit->type),punit->tile->x,punit->tile->y);

	hw_unit_list_iterate(phw->units,phw_unit)
	{
		if(phw_unit->punit==punit)
		{
			hw_unit_list_unlink(&phw->units,phw_unit);
			free(phw_unit);
			break;
		}
	} hw_unit_list_iterate_end;
	unit_list_unlink(&helpers,punit);
	punit->my_ai.activity=MY_AI_NONE;
	punit->my_ai.data=NULL;
}

/**************************************************************************
 my_ai_patrol
**************************************************************************/
bool must_return_into_city(struct unit *punit);

bool must_return_into_city(struct unit *punit)
{
	return (punit->fuel==1);
}

void my_ai_patrol_alloc(struct unit *punit,struct tile *ptile)
{
        if (!punit || unit_type(punit)->fuel == 0) {
          return;
        }

        char buf[1024];
        struct tile *old_tile=(punit->my_ai.control&&punit->my_ai.activity==MY_AI_PATROL?(struct tile *)punit->my_ai.data:NULL);

        if(old_tile)
        {
                my_ai_orders_free(punit);
                if(ptile==old_tile)
                {
                        my_snprintf(buf,sizeof(buf),_("Warclient: %s %d stopped patrolling (%d, %d)."),
                                    get_unit_type(punit->type)->name,punit->id,ptile->x,ptile->y);
                        append_output_window(buf);
                        return;
                }
        }
	unit_list_append(&patrolers,punit);
	punit->my_ai.control=TRUE;
	punit->my_ai.data=(void *)ptile;
	punit->my_ai.activity=MY_AI_PATROL;
        if(old_tile)
                my_snprintf(buf,sizeof(buf),_("Warclient: %s %d patrolling (%d, %d) instead of (%d, %d)."),
		            get_unit_type(punit->type)->name, punit->id,ptile->x,ptile->y,old_tile->x,old_tile->y);
        else
                my_snprintf(buf,sizeof(buf),_("Warclient: %s %d patrolling (%d, %d)."),
	                    get_unit_type(punit->type)->name,punit->id,ptile->x,ptile->y);
        append_output_window(buf);
	automatic_processus_event(AUTO_ORDERS,punit);
        update_miscellaneous_menu();
}

void my_ai_patrol_execute(struct unit *punit)
{
	if(!punit->my_ai.control||punit->my_ai.activity!=MY_AI_PATROL)
		return;

	struct tile *dest=(struct tile*)punit->my_ai.data;
	int move_cost=calculate_move_cost(punit,dest);

        if(must_return_into_city(punit))
        {
                struct unit cunit=*punit;
                cunit.tile=dest;
                move_cost+=calculate_move_cost(&cunit,find_nearest_city(&cunit,TRUE));
                if(move_cost>punit->moves_left)
                {
                        struct tile *ptile;
                        ptile=find_nearest_city(&cunit,TRUE);
                        if(ptile&&ptile!=find_nearest_city(punit,TRUE))
                                send_goto_unit(punit,ptile);
                        return;
                }
        }
	send_goto_unit(punit,dest);
}

void my_ai_patrol_execute_all(void)
{
	if(!my_ai_enable||!unit_list_size(&patrolers))
		return;
	append_output_window(_("PepClient: Executing patrol orders..."));
	connection_do_buffer(&aconnection);
	unit_list_iterate(patrolers,punit)
	{
		my_ai_patrol_execute(punit);
	} unit_list_iterate_end;
	connection_do_unbuffer(&aconnection);
}

void my_ai_patrol_free(struct unit *punit)
{
	unit_list_unlink(&patrolers,punit);
	punit->my_ai.data=NULL;
	punit->my_ai.activity=MY_AI_NONE;
}

/**************************************************************************
 my_ai_spread
**************************************************************************/
struct scity
{
  int tdv,rdv;
  int tav,rav;
  struct city *pcity;
  struct unit_list ulist;
};

#define SPECLIST_TAG scity
#define SPECLIST_TYPE struct scity
#include "speclist.h"
#define scity_list_iterate(alist,pitem) \
  TYPED_LIST_ITERATE(struct scity,alist,pitem)
#define scity_list_iterate_end LIST_ITERATE_END

struct scity *find_weakest_city(struct scity_list *psclist);
struct unit *find_best_unit(struct unit_list *pulist);

struct scity *find_weakest_city(struct scity_list *psclist)
{
	struct scity *wscity=NULL;

	scity_list_iterate(*psclist,pscity)
	{
		if(!unit_list_size(&pscity->ulist))
			continue;
		if(!wscity||pscity->rdv<wscity->rdv||(pscity->rdv==wscity->rdv&&(pscity->tdv<wscity->tdv||(pscity->tdv==wscity->tdv&&
			(pscity->rav<wscity->rav||(pscity->rav==wscity->rav&&pscity->tav<wscity->rav))))))
			wscity=pscity;
	} scity_list_iterate_end;
	return wscity;
}

struct unit *find_best_unit(struct unit_list *pulist)
{
	struct unit *bunit=NULL;

	unit_list_iterate(*pulist,punit)
	{
		if(!bunit||unit_type(punit)->defense_strength>unit_type(bunit)->defense_strength||(unit_type(punit)->defense_strength==
			unit_type(bunit)->defense_strength&&unit_type(punit)->attack_strength>unit_type(bunit)->attack_strength))
			bunit=punit;
	} unit_list_iterate_end;
	return bunit;
}

void my_ai_spread_execute(void)
{
	if(!multi_select_satisfies_filter(0))
		return;

	struct scity_list sclist;
	struct scity *pscity=NULL;
	Continent_id cid=get_unit_in_focus()->tile->continent;
	Impr_Type_id airport=find_improvement_by_name_orig("Airport");

	/* get datas */
	scity_list_init(&sclist);
	players_iterate(pplayer)
	{
		if(!spread_allied_cities&&pplayer!=game.player_ptr)
			continue;
		enum diplstate_type type=pplayer_get_diplstate(pplayer,game.player_ptr)->type;
		if(spread_allied_cities&&!(pplayer==game.player_ptr||type==DS_ALLIANCE||type==DS_TEAM))
			continue;
		city_list_iterate(pplayer->cities,pcity)
		{
			if(pcity->tile->continent!=cid||(spread_airport_cities&&!city_got_building(pcity,airport)))
				continue;
			pscity=fc_malloc(sizeof(struct scity));
			pscity->tdv=pscity->rdv=pscity->tav=pscity->rav=0;
			pscity->pcity=pcity;
			unit_list_init(&pscity->ulist);
			scity_list_append(&sclist,pscity);
		} city_list_iterate_end;
	} players_iterate_end;
	if(!scity_list_size(&sclist))
		goto free;
	connection_do_buffer(&aconnection);
	multi_select_iterate(FALSE,punit)
	{
		struct unit_type *type=unit_type(punit);
		struct scity *last=NULL;
		bool multi=FALSE;
		scity_list_iterate(sclist,pscity)
		{
			if(calculate_move_cost(punit,pscity->pcity->tile)>punit->moves_left)
				continue;
			unit_list_append(&pscity->ulist,punit);
			pscity->tdv+=type->defense_strength;
			pscity->tav+=type->attack_strength;
			if(last)
				multi=TRUE;
			last=pscity;
		} scity_list_iterate_end;
		if(!multi&&last)//send if the unit can go to one only city
		{
			send_goto_unit(punit,last->pcity->tile);
			unit_list_unlink(&pscity->ulist,punit);
			last->tdv-=type->defense_strength;
			last->tav-=type->attack_strength;
			last->rdv+=type->defense_strength;
			last->rav+=type->attack_strength;
		}
	} multi_select_iterate_end;

	/* execute */
	while((pscity=find_weakest_city(&sclist)))
	{
		struct unit *punit=find_best_unit(&pscity->ulist);
		if(punit)
		{
			struct unit_type *type=unit_type(punit);
			send_goto_unit(punit,pscity->pcity->tile);
			pscity->rdv+=type->defense_strength;
			pscity->rav+=type->attack_strength;
			scity_list_iterate(sclist,pscity)
			{
				if(!unit_list_find(&pscity->ulist,punit->id))
					continue;
				unit_list_unlink(&pscity->ulist,punit);
				pscity->tdv-=type->defense_strength;
				pscity->tav-=type->attack_strength;
			} scity_list_iterate_end;
		}
	}
	connection_do_unbuffer(&aconnection);

	/* free datas */
free:
	scity_list_iterate(sclist,pscity)
	{
		unit_list_unlink_all(&pscity->ulist);
		free(pscity);
	} scity_list_iterate_end;
	scity_list_unlink_all(&sclist);
}

/**************************************************************************
 my_ai main
**************************************************************************/
struct unit_list *my_ai_get_units(enum my_ai_activity activity)
{
	switch(activity)
	{
		case MY_AI_NONE:
			return &noners;
		case MY_AI_TRADE_ROUTE:
			return &traders;
		case MY_AI_HELP_WONDER:
			return &helpers;
		case MY_AI_PATROL:
			return &patrolers;
		case MY_AI_LAST:
			return NULL;
	}
	return NULL;
}

int my_ai_count_activity(enum my_ai_activity activity)
{
	if(activity==MY_AI_LAST)
		return unit_list_size(&noners)+unit_list_size(&traders)+unit_list_size(&helpers)+unit_list_size(&patrolers);
	return unit_list_size(my_ai_get_units(activity));
}

const char *my_ai_unit_activity(struct unit *punit)
{
	assert(punit->my_ai.activity>=0&&punit->my_ai.activity<MY_AI_LAST);
	return my_ai_activities[punit->my_ai.activity];
}

char *my_ai_unit_orders(struct unit *punit)
{
	static char buf[256];

	if(!punit->my_ai.control)
		return NULL;

	switch(punit->my_ai.activity)
	{
		case MY_AI_NONE:
			buf[0]='\0';
			break;
		case MY_AI_TRADE_ROUTE:
		{
			struct trade_route *ptr=(struct trade_route *)punit->my_ai.data;
                        update_trade_route(ptr);
			my_snprintf(buf,sizeof(buf),"%s-%s (%d move%s, %d turn%s)",ptr->pc1->name,ptr->pc2->name,
                                    ptr->moves_req,ptr->moves_req>1?"s":"",ptr->turns_req,ptr->turns_req>1?"s":"");
			break;
		}
		case MY_AI_HELP_WONDER:
		{
			struct help_wonder *phw=(struct help_wonder *)punit->my_ai.data;
			my_snprintf(buf,sizeof(buf),"%s in %s",get_improvement_name(phw->id),phw->pcity->name);
			break;
		}
		case MY_AI_PATROL:
		{
			struct tile *ptile=(struct tile *)punit->my_ai.data;
			struct city *pcity=map_get_city(ptile);
			if(pcity)
				my_snprintf(buf,sizeof(buf),"%s",pcity->name);
			else
				my_snprintf(buf,sizeof(buf),"(%d, %d)",ptile->x,ptile->y);
			break;
		}
		case MY_AI_LAST:
			return NULL;
	}
	return buf;
}

void my_ai_goto_nearest_city(struct unit *punit)
{
	struct tile *ptile;

	if((ptile=find_nearest_city(punit,FALSE)))
		goto_and_request(punit,ptile)
			request_unit_move_done(punit);
}

void my_ai_caravan(struct unit *punit)
{
	if(!my_ai_trade_level||!unit_flag(punit,F_TRADE_ROUTE))
		goto wonders;

	if(punit->my_ai.control&&punit->my_ai.activity==MY_AI_TRADE_ROUTE)
		my_ai_trade_route_free(punit);

	if(!(my_ai_trade_mode&2)||!trade_route_list_size(&trade_plan))
		goto other_trade;

	{
		struct trade_route *btr=NULL,*htr=NULL,**pptr;
		trade_route_list_iterate(trade_plan,ptr)
		{
			pptr=(!my_ai_trade_plan_change_homecity&&(ptr->pc1->id==punit->homecity||ptr->pc2->id==punit->homecity)?&htr:&btr);
			ptr->punit=punit;
			update_trade_route(ptr);
			if(*pptr)
			{
				if(ptr->turns_req<(*pptr)->turns_req||(ptr->turns_req==(*pptr)->turns_req
                                   &&(ptr->moves_req>(*pptr)->moves_req||(ptr->moves_req==(*pptr)->moves_req&&ptr->trade>(*pptr)->trade))))
				{
        				*pptr=ptr;
				}
			}
			else
				*pptr=ptr;
		} trade_route_list_iterate_end;
		if(htr)
			btr=htr;
		if(my_ai_trade_level==LEVEL_GOOD)
		{
			htr=NULL;
			trade_route_list_iterate(player_find_city_by_id(game.player_ptr,punit->homecity)->trade_routes,ctr)
			{
				if(!ctr->planned)
					continue;

				struct trade_route *ptr=trade_route_new(punit,ctr->pc1,ctr->pc2,TRUE);

				if((ptr->turns_req<ctr->turns_req||(ptr->turns_req==ctr->turns_req&&ptr->moves_req>ctr->moves_req))
                                   &&(!btr||(ptr->turns_req<btr->turns_req||(ptr->turns_req==btr->turns_req&&ptr->moves_req>btr->moves_req))))
				{
					if(htr)
					{
						if(ptr->turns_req<htr->turns_req||(ptr->turns_req==htr->turns_req&&ptr->moves_req>htr->moves_req))
							free(htr);
						else
						{
							free(ptr);
							continue;
						}
					}
					ptr->ptr0=ctr;
					htr=ptr;
				}
				else
					free(ptr);
			} trade_route_list_iterate_end;
			if(htr)
			{
				htr->ptr0->planned=FALSE;
				btr=htr;
			}
		}
		else if(my_ai_trade_level==LEVEL_BEST)
		{
			htr=NULL;
			unit_list_iterate(traders,tunit)
			{
				struct trade_route *utr=(struct trade_route *)punit->my_ai.data;
				if(!utr||!utr->planned)
					continue;

				struct trade_route *ptr=trade_route_new(punit,utr->pc1,utr->pc2,TRUE);

				if((ptr->turns_req<utr->turns_req||(ptr->turns_req==utr->turns_req&&ptr->moves_req>utr->moves_req))
                                   &&(!btr||(ptr->turns_req<btr->turns_req||(ptr->turns_req==btr->turns_req&&ptr->moves_req>btr->moves_req))))
				{
					if(htr)
						free(htr);
					ptr->ptr0=utr;
					htr=ptr;
				}
			} unit_list_iterate_end;
			if(htr)
			{
				htr->ptr0->planned=FALSE;//to don't recalculate trade planning
				btr=htr;
			}
		}
		if(!btr)
			goto other_trade;
		my_ai_orders_free(punit);
		my_ai_trade_route_alloc(btr);
		if(!my_ai_update)
			automatic_processus_event(AUTO_ORDERS,punit);
		return;
	}

other_trade:
	{
		if(!(my_ai_trade_mode&1))
			goto wonders;

		struct trade_route *btr=best_city_trade_route(punit,player_find_city_by_id(game.player_ptr,punit->homecity));

		if(!btr||(btr&&btr->tr_type<TR_NEW))
		{
			struct trade_route *ptr=NULL;
			Continent_id cid=punit->tile->continent;
			city_list_iterate(game.player_ptr->cities,pcity)
			{
				if(pcity->tile->continent!=cid)
					continue;
				if(btr)
				{
					ptr=best_city_trade_route(punit,pcity);
					if(ptr)
					{
						if(ptr->tr_type==btr->tr_type)
						{
							if(ptr->trade==btr->trade)
							{
                                                                if(ptr->turns_req==btr->turns_req)
                                                                {
                                                                        if(ptr->moves_req>btr->moves_req)
                                                                                swap(btr,ptr,struct trade_route *);
                                                                }
								else if(ptr->turns_req<btr->turns_req)
									swap(btr,ptr,struct trade_route *);
							}
							else if(ptr->trade>btr->trade)
								swap(btr,ptr,struct trade_route *);
						}
						else if(ptr->tr_type>btr->tr_type)
							swap(btr,ptr,struct trade_route *);
						free(ptr);
					}
				}
				else
					btr=best_city_trade_route(punit,pcity);
			} city_list_iterate_end;
		}
		if(!btr)
			goto wonders;
		my_ai_orders_free(punit);
		my_ai_trade_route_alloc(btr);
		if(!my_ai_update)
			automatic_processus_event(AUTO_ORDERS,punit);
		return;
	}

wonders:
	if(!my_ai_wonder_level||!unit_flag(punit,F_HELP_WONDER))
		goto goto_city;

	if(punit->my_ai.control&&punit->my_ai.activity==MY_AI_HELP_WONDER)
		my_ai_help_wonder_free(punit);

	{
		struct help_wonder *phw;

		phw=best_wonder(punit);
		if(!phw)
			goto goto_city;
		my_ai_orders_free(punit);
		my_ai_help_wonder_alloc(punit,phw);
		if(!my_ai_update)
			automatic_processus_event(AUTO_ORDERS,punit);
		return;
	}

goto_city:
	{
		my_ai_orders_free(punit);
		punit->my_ai.control=TRUE;
		punit->my_ai.activity=MY_AI_NONE;
		unit_list_append(&noners,punit);
        	my_ai_goto_nearest_city(punit);
                update_miscellaneous_menu();
	}
}

void my_ai_none(struct unit *punit)
{
	if(unit_flag(punit,F_TRADE_ROUTE)||unit_flag(punit,F_HELP_WONDER))
		my_ai_caravan(punit);
	if(punit->my_ai.activity==MY_AI_NONE)
		my_ai_goto_nearest_city(punit);
}

void my_ai_orders_free(struct unit *punit)
{
	if(!punit->my_ai.control)
	{
		if(unit_flag(punit,F_TRADE_ROUTE))
			non_ai_trade_change(punit,1);
		return;
	}
	switch(punit->my_ai.activity)
	{
		case MY_AI_NONE:
			unit_list_unlink(&noners,punit);
			break;
		case MY_AI_TRADE_ROUTE:
			my_ai_trade_route_free(punit);
			break;
		case MY_AI_HELP_WONDER:
			my_ai_help_wonder_free(punit);
			break;
		case MY_AI_PATROL:
			my_ai_patrol_free(punit);
			break;
		default:
			abort();
			break;
	}
	punit->my_ai.control=FALSE;
	punit->ai.control=FALSE;
	request_new_unit_activity(punit,ACTIVITY_IDLE);
        update_auto_caravan_menu();
        update_miscellaneous_menu();
	if(punit==get_unit_in_focus())
		update_unit_info_label(punit);
}

void my_ai_unit_execute(struct unit *punit)
{
	if(!punit->my_ai.control)
		return;
	switch(punit->my_ai.activity)
	{
		case MY_AI_NONE:
			my_ai_none(punit);
			break;
		case MY_AI_TRADE_ROUTE:
			my_ai_trade_route_execute(punit);
			break;
		case MY_AI_HELP_WONDER:
			my_ai_help_wonder_execute(punit);
			break;
		case MY_AI_PATROL:
			my_ai_patrol_execute(punit);
			break;
		default:
			assert(0);
			break;
	}
}

void my_ai_execute(void)
{
	if(!my_ai_enable||!my_ai_count_activity(MY_AI_LAST))
		return;
	append_output_window(_("PepClient: Executing all automatic orders..."));
	my_ai_update=TRUE;
	unit_list_iterate(noners,punit)
	{
		my_ai_none(punit);
	} unit_list_iterate_end;
	my_ai_update=FALSE;
	my_ai_trade_route_execute_all();
	my_ai_help_wonder_execute_all();
	my_ai_patrol_execute_all();
}

void my_ai_worklist_event(struct city *pcity)
{
	int i;
	bool destroy=TRUE;
	struct help_wonder_list hwlist;
	help_wonder_list_init(&hwlist);

	if(!pcity->is_building_unit&&is_wonder(pcity->currently_building))
		help_wonder_list_append(&hwlist,help_wonder_new(pcity,-1,pcity->currently_building));

	for(i=0;pcity->worklist.wlefs[i]!=WEF_END;i++)
		if(pcity->worklist.wlefs[i]==WEF_IMPR&&is_wonder(pcity->worklist.wlids[i]))
			help_wonder_list_append(&hwlist,help_wonder_new(pcity,i,pcity->worklist.wlids[i]));

	unit_list_unlink_all(&temp);
	switch(are_wonders_list_equal(&pcity->help_wonders,&hwlist))
	{
		case 1://advance
			help_wonder_list_iterate(pcity->help_wonders,phw)
			{
				if(phw->level==-1)
				{
					help_wonder_list_unlink(&pcity->help_wonders,phw);
					help_wonder_destroy(phw);
				}
				else
					phw->level--;
			} help_wonder_list_iterate_end;
			break;
		case 2://add
			for(i=help_wonder_list_size(&pcity->help_wonders);i<help_wonder_list_size(&hwlist);i++)
			{
				struct help_wonder *phw=help_wonder_list_get(&hwlist,i);
				help_wonder_list_append(&pcity->help_wonders,help_wonder_new(pcity,phw->level,phw->id));
			}
			break;
		case 3://remove
			for(i=help_wonder_list_size(&hwlist);i<help_wonder_list_size(&pcity->help_wonders);i++)
			{
				struct help_wonder *phw=help_wonder_list_get(&pcity->help_wonders,i);
				help_wonder_list_unlink(&pcity->help_wonders,phw);
				help_wonder_destroy(phw);
			}
			break;
		case 4://change
			if(my_ai_wonder_level==LEVEL_GOOD)
				while(realloc_help_wonder(pcity,&hwlist,&pcity->help_wonders));
			my_ai_help_wonder_city_free(pcity);
			help_wonder_list_iterate(hwlist,phw)
			{
				help_wonder_list_append(&pcity->help_wonders,phw);
			} help_wonder_list_iterate_end;
			destroy=FALSE;
			break;
		default://case 0:yes
			break;
	}

	if(destroy)
	{
		help_wonder_list_iterate(hwlist,phw)
		{
			help_wonder_destroy(phw);
		} help_wonder_list_iterate_end;
	}
	help_wonder_list_unlink_all(&hwlist);

	unit_list_iterate(temp,punit)
	{
		my_ai_caravan(punit);
	} unit_list_iterate_end;
	unit_list_unlink_all(&temp);
}

void my_ai_city_free(struct city *pcity)
{
	trade_route_list_iterate(pcity->trade_routes,ptr)
	{
		my_ai_trade_route_free(ptr->punit);
	} trade_route_list_iterate_end;
	trade_route_list_unlink_all(&pcity->trade_routes);
	my_ai_help_wonder_city_free(pcity);
	if(city_list_find(&trade_cities,pcity))
		my_ai_add_trade_city(pcity,FALSE);
	trade_route_list_iterate(trade_plan,ptr)
	{
		if(ptr->pc1==pcity||ptr->pc2==pcity)
		{
			trade_route_list_unlink(&trade_plan,ptr);
			free(ptr);
		}
	} trade_route_list_iterate_end;
}

void my_ai_help_wonder_city_free(struct city *pcity)
{
	help_wonder_list_iterate(pcity->help_wonders,phw)
	{
		help_wonder_destroy(phw);
	} help_wonder_list_iterate_end;
	help_wonder_list_unlink_all(&pcity->help_wonders);
}

void my_ai_init(void)
{
	unit_list_init(&noners);
	unit_list_init(&traders);
	unit_list_init(&helpers);
	unit_list_init(&patrolers);
	unit_list_init(&temp);
	city_list_init(&trade_cities);
	trade_route_list_init(&trade_plan);
	trade_route_list_init(&non_ai_trade);
	my_ai_auto_execute=automatic_processus_new(PAGE_PMAIN,AV_TO_FV(AUTO_OFF),"","Automatic orders execution",0,AP_MAIN_CONNECT(my_ai_execute),
		AP_CONNECT(AUTO_ORDERS,my_ai_unit_execute),-1);
	my_ai_trade_auto_execute=automatic_processus_new(PAGE_TRADE,AV_TO_FV(AUTO_ORDERS)|AV_TO_FV(AUTO_NEW_YEAR),"","Automatic trade route orders execution",0,
		AP_MAIN_CONNECT(my_ai_trade_route_execute_all),AP_CONNECT(AUTO_ORDERS,my_ai_trade_route_execute),-1);
	my_ai_wonder_auto_execute=automatic_processus_new(PAGE_WONDER,AV_TO_FV(AUTO_ORDERS),"","Automatic help wonder orders execution",0,
		AP_MAIN_CONNECT(my_ai_help_wonder_execute_all),AP_CONNECT(AUTO_ORDERS,my_ai_help_wonder_execute),-1);
	my_ai_patrol_auto_execute=automatic_processus_new(PAGE_PMAIN,AV_TO_FV(AUTO_NEW_YEAR),"","Automatic patrol orders execution",0,
		AP_MAIN_CONNECT(my_ai_patrol_execute_all),AP_CONNECT(AUTO_ORDERS,my_ai_patrol_execute),-1);
}

void my_ai_free(void)
{
        city_list_iterate(game.player_ptr->cities,pcity)
        {
                my_ai_city_free(pcity);
        } city_list_iterate_end;
	unit_list_unlink_all(&noners);
	unit_list_unlink_all(&traders);
	unit_list_unlink_all(&helpers);
	unit_list_unlink_all(&patrolers);
	unit_list_unlink_all(&temp);
	city_list_unlink_all(&trade_cities);
	trade_route_list_free(&trade_plan);
	trade_route_list_free(&non_ai_trade);
	automatic_processus_remove(my_ai_auto_execute);
	automatic_processus_remove(my_ai_trade_auto_execute);
	automatic_processus_remove(my_ai_wonder_auto_execute);
	automatic_processus_remove(my_ai_patrol_auto_execute);
}
