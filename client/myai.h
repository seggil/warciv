/**********************************************************************
 This file was edited by *pepeto*.
 - auto-caravans
 - patrol
 - spread
***********************************************************************/
#ifndef _MYAI_H
#define _MYAI_H

#include "city.h"
#include "unit.h"
#include "shared.h"

#include "multiselect.h"

//#define WATER_DIST_MAX	12
//#define WATER_DIST_MIN	3
//#define MAX_TRADE_POINTS (2*NUM_TRADEROUTES)
#define MAX_ESTIMATED_TURNS	32

struct my_ai_trade_city;

#define SPECLIST_TAG my_ai_trade_city
#define SPECLIST_TYPE struct my_ai_trade_city
#define SPECLIST_NO_FREE
#define SPECLIST_NO_COPY
#include "speclist.h"
#define my_ai_trade_city_list_iterate(alist,pitem)	\
	TYPED_LIST_ITERATE(struct my_ai_trade_city,alist,pitem)
#define my_ai_trade_city_list_iterate_end LIST_ITERATE_END

struct my_ai_trade_city
{
	struct city *pcity;
	int free_slots;
	struct my_ai_trade_city_list trade_with;
};

/* main functions */
int my_ai_count_activity(enum my_ai_activity activity);
const char *my_ai_unit_activity(struct unit *punit);
char *my_ai_unit_orders(struct unit *punit);
struct unit_list *my_ai_get_units(enum my_ai_activity activity);

void my_ai_goto_nearest_city(struct unit *punit);
void my_ai_caravan(struct unit *punit);
void my_ai_none(struct unit *punit);

void my_ai_orders_free(struct unit *punit);
void my_ai_unit_execute(struct unit *punit);
void my_ai_execute(void);

void my_ai_worklist_event(struct city *pcity);
void my_ai_city_free(struct city *pcity);
void my_ai_help_wonder_city_free(struct city *pcity);

void my_ai_init(void);
void my_ai_free(void);

/* trade functions */
void my_ai_trade_route_alloc(struct trade_route *ptr);
void my_ai_trade_route_alloc_city(struct unit *punit,struct city *pcity);
void my_ai_trade_route_free(struct unit *punit);
void my_ai_trade_route_execute(struct unit *punit);
void my_ai_trade_route_execute_all(void);

struct trade_route *trade_route_new(struct unit *punit,struct city *pc1,struct city *pc2,bool planned);
struct trade_route_list *estimate_non_ai_trade_route(void);
int estimate_non_ai_trade_route_number(struct city *pcity);
void non_ai_trade_change(struct unit *punit,int action);
int count_trade_routes(struct city *pcity);
struct trade_route_list *my_ai_trade_plan_get(void);
struct my_ai_trade_city_list *my_ai_get_trade_cities(void);

void my_ai_add_trade_city(struct city *pcity,bool multi);
void show_cities_in_trade_plan(void);
void recalculate_trade_plan(void);
void clear_my_ai_trade_cities(void);
void show_free_slots_in_trade_plan(void);
void recalculate_trade_plan(void);
void calculate_trade_estimation(void);

/* help wonder functions */
void my_ai_help_wonder_alloc(struct unit *punit,struct help_wonder *phw);
void my_ai_help_wonder_execute(struct unit *punit);
void my_ai_help_wonder_execute_all(void);
void my_ai_help_wonder_free(struct unit *punit);

/* patrol functions */
void my_ai_patrol_alloc(struct unit *punit,struct tile *ptile,bool manalloc);
void my_ai_patrol_execute(struct unit *punit);
void my_ai_patrol_execute_all(void);
void my_ai_patrol_free(struct unit *punit);

/* spread function */
void my_ai_spread_execute(void);

#endif /* _MYAI_H */
