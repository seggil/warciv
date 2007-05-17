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

struct trade_conf
{
	int free_slots;
	int turns;
	int moves;
};

static automatic_processus *my_ai_auto_execute;//main automatic processus id for my_ai
static automatic_processus *my_ai_trade_auto_execute;
static automatic_processus *my_ai_wonder_auto_execute;
static automatic_processus *my_ai_patrol_auto_execute;

static struct my_ai_trade_city_list trade_cities;
static struct trade_route_list trade_plan;
static struct trade_route_list non_ai_trade;
static struct unit_list noners;
static struct unit_list traders;
static struct unit_list helpers;
static struct unit_list patrolers;
static struct unit_list temp;
static struct unit *caravan=NULL;
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

void my_ai_trade_route_alloc(struct trade_route *ptr)
{
        char buf[1024];

	freelog(LOG_VERBOSE,"alloc auto-trade orders for %s (%d/%d)",unit_name(ptr->punit->type),ptr->punit->tile->x,ptr->punit->tile->y);

	ptr->punit->my_ai.control=TRUE;
	ptr->punit->my_ai.activity=MY_AI_TRADE_ROUTE;
	ptr->punit->my_ai.data=(void *)ptr;
	unit_list_append(&traders,ptr->punit);
	trade_route_list_unlink(&trade_plan,ptr);
	trade_route_list_append(&ptr->pc1->trade_routes,ptr);
	trade_route_list_append(&ptr->pc2->trade_routes,ptr);
	if(my_ai_trade_level>LEVEL_ON)
	{
		struct trade_route *ptr1=ptr->ptr1,*ptr2=ptr->ptr2;
		struct unit *punit1=(ptr1?ptr1->punit:NULL),*punit2=(ptr2?ptr2->punit:NULL);
		if(ptr->ptr0)
		{
			struct unit *punit0=ptr->ptr0->punit;
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
        update_trade_route(ptr);
        my_snprintf(buf,sizeof(buf),_("PepClient: Sending %s to establish trade route between %s and %s (%d move%s, %d turn%s)"),
                    unit_type(ptr->punit)->name,ptr->pc1->name,ptr->pc2->name,ptr->moves_req,ptr->moves_req>1?"s":"",ptr->turns_req,ptr->turns_req>1?"s":"");
        append_output_window(buf);
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
		my_snprintf(buf,sizeof(buf),"PepClient: %s and %s have alraedy a trade route.",hcity->name,pcity->name);
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
	update_unit_focus();
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
	}
	if(cunit.homecity==ptr->pc1->id)
	{
		goto_and_request((&cunit),ptr->pc2->tile)
		{
			char buf[256];
			dsend_packet_unit_establish_trade(&aconnection,punit->id);
			my_snprintf(buf,sizeof(buf),_("PepClient: %s establishing trade route between %s and %s"),unit_name(punit->type),ptr->pc1->name,ptr->pc2->name);
			append_output_window(buf);
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
	if(ptr->planned&&punit->tile!=ptr->pc2->tile)
        {
		trade_route_list_append(&trade_plan,ptr);
        	my_snprintf(buf,sizeof(buf),_("PepClient: Cancelling trade route between %s and %s"),ptr->pc1->name,ptr->pc2->name);
                append_output_window(buf);
        }
	else
		free(ptr);
	if(draw_city_traderoutes)
		update_map_canvas_visible();
}

/* trade planning */
static int my_ai_trade_city_sort_id(const void *pc1,const void *pc2);
struct my_ai_trade_city *my_ai_trade_city_list_find_city(struct my_ai_trade_city_list *ptcl,struct city *pcity);
void my_ai_trade_city_list_free(struct my_ai_trade_city_list *ptcl);
bool is_trade_route_planned(struct trade_route_list *ptrl,struct city *pc1,struct city *pc2);
bool can_cities_enter_in_trade_plan(struct city *pc1,struct city *pc2);
void find_best_trade_combination_iterate(time_t time_max,struct trade_route_list *ptrl,struct trade_conf *ptc,struct my_ai_trade_city *pcity,
	int min_id,struct trade_route_list *btsl,struct trade_conf *btc);
void find_best_trade_combination(char *buf, size_t buf_len);

static int my_ai_trade_city_sort_id(const void *pc1,const void *pc2)
{
	return ((*((struct my_ai_trade_city **)pc1))->pcity->id>(*((struct my_ai_trade_city **)pc2))->pcity->id);
}

struct my_ai_trade_city *my_ai_trade_city_list_find_city(struct my_ai_trade_city_list *ptcl,struct city *pcity)
{
	my_ai_trade_city_list_iterate(*ptcl,ptc)
	{
		if(ptc->pcity==pcity)
			return ptc;
	} my_ai_trade_city_list_iterate_end;
	return NULL;
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

void my_ai_trade_city_list_free(struct my_ai_trade_city_list *ptcl)
{
	assert(ptcl);
	my_ai_trade_city_list_iterate(*ptcl,ptc)
	{
		my_ai_trade_city_list_unlink_all(&ptc->trade_with);
		free(ptc);
	} my_ai_trade_city_list_iterate_end;
	my_ai_trade_city_list_unlink_all(ptcl);
}

bool is_trade_route_planned(struct trade_route_list *ptrl,struct city *pc1,struct city *pc2)
{
	trade_route_list_iterate(pc1->trade_routes,ptr)
	{
		if(ptr->pc1==pc2||ptr->pc2==pc2)
			return TRUE;
	} trade_route_list_iterate_end;
	trade_route_list_iterate(*ptrl,ptr)
	{
		if((ptr->pc1==pc1&&ptr->pc2==pc2)||(ptr->pc1==pc2&&ptr->pc2==pc1))
			return TRUE;
	} trade_route_list_iterate_end;
	return FALSE;
}

bool can_cities_enter_in_trade_plan(struct city *pc1,struct city *pc2)
{
	if(!can_cities_trade(pc1,pc2)||have_cities_trade_route(pc1,pc2))
		return FALSE;
	trade_route_list_iterate(pc1->trade_routes,ptr)
	{
		if(ptr->pc1==pc2||ptr->pc2==pc2)
			return FALSE;
	} trade_route_list_iterate_end;
	return TRUE;
}

void find_best_trade_combination_iterate(time_t time_max,struct trade_route_list *ptrl,struct trade_conf *ptc,struct my_ai_trade_city *pcity,
	int min_id,struct trade_route_list *btrl,struct trade_conf *btc)
{
	bool next=FALSE;

	if(pcity->free_slots)
	{
		my_ai_trade_city_list_iterate(pcity->trade_with,tcity)
		{
			if(time(NULL)>=time_max)
				break;
			if(tcity->free_slots&&tcity->pcity->id>min_id)
			{
				struct trade_conf conf=*ptc;
				struct trade_route *ptr=trade_route_new(NULL,pcity->pcity,tcity->pcity,TRUE);
                                int move_cost;

				trade_route_list_append(ptrl,ptr);
				pcity->free_slots--;
				tcity->free_slots--;
				ptc->free_slots-=2;
				caravan->tile=pcity->pcity->tile;
                                move_cost=calculate_move_cost(caravan,tcity->pcity->tile);
				ptc->moves+=move_cost;
                                ptc->turns+=(move_cost+1)/SINGLE_MOVE;

				find_best_trade_combination_iterate(time_max,ptrl,ptc,pcity,tcity->pcity->id,btrl,btc);

				*ptc=conf;
				pcity->free_slots++;
				tcity->free_slots++;
				trade_route_list_unlink(ptrl,ptr);
				free(ptr);
				next=TRUE;
			}
		} my_ai_trade_city_list_iterate_end;
	}

	if(!next)
	{
		my_ai_trade_city_list_iterate(trade_cities,tc)
		{
			if(tc->free_slots&&tc->pcity->id>pcity->pcity->id)
			{
				find_best_trade_combination_iterate(time_max,ptrl,ptc,tc,tc->pcity->id,btrl,btc);
				return;
			}
		} my_ai_trade_city_list_iterate_end;
	}

	if(ptc->free_slots<btc->free_slots||(ptc->free_slots==btc->free_slots
           &&(ptc->turns<btc->turns||(ptc->turns==btc->turns&&ptc->moves<btc->moves))))
	{
		*btc=*ptc;
		trade_route_list_copy(btrl,ptrl);
	}
}

void find_best_trade_combination(char *buf, size_t buf_len)
{
	struct trade_route_list *ptrl;
	struct trade_route_list btrl,ctrl;
	struct trade_conf btc,ctc;

        buf[0]='\0';
	if(!city_list_size(&game.player_ptr->cities))
		return;

	/* initialize */
	ptrl=estimate_non_ai_trade_route();
	trade_route_list_init(&btrl);
	trade_route_list_init(&ctrl);
	ctc.free_slots=0;
	my_ai_trade_city_list_sort(&trade_cities,my_ai_trade_city_sort_id);
	my_ai_trade_city_list_iterate(trade_cities,ptc)
	{
		ptc->free_slots=NUM_TRADEROUTES-my_city_num_trade_routes(ptc->pcity,TRUE)-estimate_non_ai_trade_route_number(ptc->pcity);
		ctc.free_slots+=ptc->free_slots;
		if(ptc->free_slots<0)
		{
			ptc->free_slots=0;
			continue;
		}
		my_ai_trade_city_list_iterate(trade_cities,ttc)
		{
			if(can_cities_enter_in_trade_plan(ptc->pcity,ttc->pcity)&&(!ptrl||!trade_route_list_find_cities(ptrl,ptc->pcity,ttc->pcity)))
				my_ai_trade_city_list_append(&ptc->trade_with,ttc);
		} my_ai_trade_city_list_iterate_end;
	} my_ai_trade_city_list_iterate_end;
	ctc.turns=0;
	ctc.moves=0;

	unit_type_iterate(type)
	{
		if(unit_type_flag(type,F_TRADE_ROUTE))
		{
			caravan=create_unit_virtual(game.player_ptr,city_list_get(&game.player_ptr->cities,0),type,0);
			break;
		}
	} unit_type_iterate_end;
	assert(caravan);

	/* alloc the harder cities */
	struct my_ai_trade_city *pmacity;
	do
	{
		pmacity=NULL;
		my_ai_trade_city_list_iterate(trade_cities,ptc)
		{
			int size=my_ai_trade_city_list_size(&ptc->trade_with);
			if(size&&size<=ptc->free_slots)
			{
		 		if(!pmacity||ptc->free_slots-size>pmacity->free_slots-my_ai_trade_city_list_size(&pmacity->trade_with))
		 		pmacity=ptc;
			}
		 } my_ai_trade_city_list_iterate_end;
		 if(pmacity)
		 {
			my_ai_trade_city_list_iterate(pmacity->trade_with,ttc)
			{
				if(!ttc->free_slots)
					continue;
				trade_route_list_append(&ctrl,trade_route_new(NULL,pmacity->pcity,ttc->pcity,TRUE));
				pmacity->free_slots--;
				ttc->free_slots--;
				ctc.free_slots-=2;
				my_ai_trade_city_list_unlink(&ttc->trade_with,pmacity);
			} my_ai_trade_city_list_iterate_end;
			my_ai_trade_city_list_unlink_all(&pmacity->trade_with);
		}
	} while(pmacity);
	btc=ctc;
	trade_route_list_copy(&btrl,&ctrl);
	
	/* optimize city lists */
	my_ai_trade_city_list_sort(&trade_cities,my_ai_trade_city_sort_id);
	my_ai_trade_city_list_iterate(trade_cities,ptc)
	{
		if(!ptc->free_slots)
		{
			my_ai_trade_city_list_iterate(ptc->trade_with,ttc)
			{
				my_ai_trade_city_list_unlink(&ttc->trade_with,ptc);
			} my_ai_trade_city_list_iterate_end;
			my_ai_trade_city_list_unlink_all(&ptc->trade_with);
		}
		my_ai_trade_city_list_iterate(ptc->trade_with,ttc)
		{
			if(ttc->pcity->id>ptc->pcity->id)
				break;
			my_ai_trade_city_list_unlink(&ptc->trade_with,ttc);
		} my_ai_trade_city_list_iterate_end;
	} my_ai_trade_city_list_iterate_end;

	/* find the best combinaison */
	if(btc.free_slots)
	{
		struct my_ai_trade_city *ptc=my_ai_trade_city_list_get(&trade_cities,0);
		find_best_trade_combination_iterate(time(NULL)+my_ai_trade_plan_time_max,&ctrl,&ctc,ptc,ptc->pcity->id,&btrl,&btc);
	}

	/* apply and free datas */
	if(trade_route_list_size(&btrl))
	{
		trade_route_list_copy(&trade_plan,&btrl);
		trade_route_list_free(&btrl);
                my_snprintf(buf, buf_len, _("%d new trade routes, with a total of %d moves"), trade_route_list_size(&trade_plan), btc.moves);
	}
        else
        	my_snprintf(buf, buf_len, _("Didn't find any trade routes to establish"));
	my_ai_trade_city_list_iterate(trade_cities,ptc)
	{
		my_ai_trade_city_list_unlink_all(&ptc->trade_with);
	} my_ai_trade_city_list_iterate_end;
	trade_route_list_free(&ctrl);
	destroy_unit_virtual(caravan);
	caravan=NULL;
}

void show_cities_in_trade_plan(void)
{
	if(my_ai_trade_city_list_size(&trade_cities)<=0)
		return;

	char buf[1024];
	bool first=TRUE;

	strcpy(buf,_("PepClient: Cities in trade plan:"));
	my_ai_trade_city_list_iterate(trade_cities,ptc)
	{
		cat_snprintf(buf,sizeof(buf),"%s %s",first?"":",",ptc->pcity->name);
		first=FALSE;
	} my_ai_trade_city_list_iterate_end;
	strcat(buf,".");
	append_output_window(buf);
}

void clear_my_ai_trade_cities(void)
{
	trade_route_list_free(&trade_plan);
	my_ai_trade_city_list_free(&trade_cities);
	if(draw_city_traderoutes)
		update_map_canvas_visible();
	append_output_window(_("PepClient: Trade city list cleared."));
	update_menus();
}

void show_free_slots_in_trade_plan(void)
{
	char buf[1024]="\0";
	int count =0;

	my_ai_trade_city_list_iterate(trade_cities,ptc)
	{
		int num=count_trade_routes(ptc->pcity)+estimate_non_ai_trade_route_number(ptc->pcity);
		if(num<NUM_TRADEROUTES)
		{
			cat_snprintf(buf,sizeof(buf),"%s %s (%d)",!count?"":",",ptc->pcity->name,NUM_TRADEROUTES-num);
			count+=NUM_TRADEROUTES-num;
		}
	} my_ai_trade_city_list_iterate_end;
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
	find_best_trade_combination(buf,sizeof(buf));
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
	struct my_ai_trade_city *ptc;

	if((ptc=my_ai_trade_city_list_find_city(&trade_cities,pcity)))
	{
		if(!multi)
		{
			my_ai_trade_city_list_unlink(&trade_cities,ptc);
			my_ai_trade_city_list_unlink_all(&ptc->trade_with);
			free(ptc);
			my_snprintf(buf,sizeof(buf),_("PepClient: Remove city %s to trade plan"),pcity->name);
		}
	}
	else
	{
		ptc=fc_malloc(sizeof(struct my_ai_trade_city));
		ptc->pcity=pcity;
		my_ai_trade_city_list_init(&ptc->trade_with);
		my_ai_trade_city_list_append(&trade_cities,ptc);
		my_snprintf(buf,sizeof(buf),_("PepClient: Adding city %s to trade plan"),pcity->name);
	}

	if(buf[0])
	{
		append_output_window(buf);
		update_menus();
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
       update_menus();
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


struct my_ai_trade_city_list *my_ai_get_trade_cities(void)
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
		if(!map_get_city(punit->tile))
			my_ai_goto_nearest_city(punit);
		else
			my_ai_goto_nearest_city(punit);
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
	if(my_ai_trade_city_list_find_city(&trade_cities,pcity))
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
	my_ai_trade_city_list_init(&trade_cities);
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
	my_ai_trade_city_list_free(&trade_cities);
	trade_route_list_free(&trade_plan);
	trade_route_list_free(&non_ai_trade);
	automatic_processus_remove(my_ai_auto_execute);
	automatic_processus_remove(my_ai_trade_auto_execute);
	automatic_processus_remove(my_ai_wonder_auto_execute);
	automatic_processus_remove(my_ai_patrol_auto_execute);
}
