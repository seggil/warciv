/**********************************************************************
 This file was edited by *pepeto*.
 - filters
 - automatic processus
 - multi-select
 - delayed goto (from warclient)
 - airlift queue (from warclient)
***********************************************************************/
#ifndef _MULTISELECT_H
#define _MULTISELECT_H

#include "shared.h"
#include "unit.h"
#include "unittype.h"

#include "peptool.h"

#define multi_select_iterate(des,punit)	\
{	\
	bool _deselect=des;	\
	struct unit *_punit,*_punit_next_focus=NULL;	\
	bool _cond=(multi_select_size(0)>1);	\
	connection_do_buffer(&aconnection);	\
	unit_list_iterate(*multi_select_get_units_focus(),punit)	\
	{	\
		_punit=punit;	\
		if((_cond&&!unit_satisfies_filter(punit,multi_select_inclusive_filter,multi_select_exclusive_filter))||punit->focus_status==FOCUS_DONE)	\
			continue;

#define multi_select_iterate_end	\
		if(unit_satisfies_filter(_punit,multi_select_inclusive_filter,multi_select_exclusive_filter)&&_punit->focus_status==FOCUS_AVAIL)	\
			_punit_next_focus=_punit;	\
	} unit_list_iterate_end;	\
	connection_do_unbuffer(&aconnection);	\
	if(_deselect||(_cond&&!_punit_next_focus))	\
		multi_select_clear(0);	\
	else if(_cond&&!unit_satisfies_filter(get_unit_in_focus(),multi_select_inclusive_filter,multi_select_exclusive_filter))	\
	{		\
		if(_punit_next_focus)	\
			set_unit_focus(_punit_next_focus);	\
		else	\
			advance_unit_focus();	\
	}	\
}

/* filters RadioItem(All & Off)/CheckItem(all others) */
typedef unsigned int filter;

enum filter_value
{
	FILTER_ALL=1<<0,
	FILTER_NEW=1<<1,
	FILTER_FORTIFIED=1<<2,
	FILTER_SENTRIED=1<<3,
	FILTER_VETERAN=1<<4,
	FILTER_AUTO=1<<5,
	FILTER_IDLE=1<<6,
	FILTER_ABLE_TO_MOVE=1<<7,
	FILTER_MILITARY=1<<8,
	FILTER_OFF=1<<9,
};

#define FILTER_NUM 10

enum place_value
{
	PLACE_SINGLE_UNIT,
	PLACE_ON_TILE,
	PLACE_ON_CONTINENT,
	PLACE_EVERY_WHERE
};

enum utype_value
{
	UTYPE_SAME_TYPE,
	UTYPE_SAME_MOVE_TYPE,
	UTYPE_ALL
};

extern filter multi_select_inclusive_filter,multi_select_exclusive_filter;
extern enum place_value multi_select_place;
extern enum utype_value multi_select_utype;
extern filter delayed_goto_inclusive_filter,delayed_goto_exclusive_filter;
extern enum place_value delayed_goto_place;
extern enum utype_value delayed_goto_utype;

void filter_change(filter *pfilter,enum filter_value value);
bool unit_satisfies_filter(struct unit *punit,filter inclusive_filter,filter exclusive_filter);

/* automatic processus allows to connect functions to signals */

/**************************************************************************
 If you want to add a signal:
 - add a value before AUTO_OFF (AUTO_xxx)
 - add a name in the same order in multiselect.c (static const char *ap_events[])
    (N_(xxx))
 - connect the signal in client code with automatic_processus_event(AUTO_xxx,&pointer)
 - connect a callback with automatic_processus_new(...[|AV_TO_FV(AUTO_xxx)],"<main>...",
   0,...,AP_CONNECT(AUTO_xxx,my_callback));
**************************************************************************/
enum automatic_value
{
	AUTO_NEW_YEAR=0,//signal in handle_new_year() (packhand.c)
	AUTO_PRESS_TURN_DONE,//signal in send_turn_done() (civclient.c)
	AUTO_NO_UNIT_SELECTED,//signal in advance_unit_focus() (control.c)
	AUTO_50_TIMEOUT,
	AUTO_80_TIMEOUT,
	AUTO_90_TIMEOUT,
	AUTO_95_TIMEOUT,
	AUTO_5_SECONDS,//signals in ap_timers_update() (multiselect.c)
	AUTO_ORDERS,//signals in myai.c
	AUTO_WAR_DIPLSTATE,//signal in handle_player_info() (packhand.c)
	AUTO_OFF,//no signal
	AUTO_VALUES_NUM
};
#define AUTO_ALL ((1<<AUTO_OFF)-1)//for all filter values
#define AV_TO_FV(av) (1<<(av))//auto value -> filter value
#ifdef DEBUG
	#define AP_CONNECT(value,function) value,function,#function
#else
	#define AP_CONNECT(value,function) value,function
#endif
#define AP_MAIN_CONNECT(function)	\
	AP_CONNECT(AUTO_NEW_YEAR,function),	\
	AP_CONNECT(AUTO_PRESS_TURN_DONE,function),	\
	AP_CONNECT(AUTO_NO_UNIT_SELECTED,function),	\
	AP_CONNECT(AUTO_50_TIMEOUT,function),	\
	AP_CONNECT(AUTO_80_TIMEOUT,function),	\
	AP_CONNECT(AUTO_90_TIMEOUT,function),	\
	AP_CONNECT(AUTO_95_TIMEOUT,function),	\
	AP_CONNECT(AUTO_5_SECONDS,function)

typedef void (*ap_callback)(void *arg1,int arg2);

typedef struct
{
	enum peppage page;//page for setting dialog
	filter default_auto_filter,auto_filter;
	char menu[256],description[256];//path to the filter menu or name for setting dialog
	int data;//value passed on arg2 of each callback;
	ap_callback callback[AUTO_VALUES_NUM];
	void *widget[AUTO_VALUES_NUM];//GtkWidget
} automatic_processus;

#define SPECLIST_TAG automatic_processus
#define SPECLIST_TYPE automatic_processus
#include "speclist.h"
#define automatic_processus_iterate(pitem) \
  TYPED_LIST_ITERATE(automatic_processus,*get_automatic_processus(), pitem)
#define automatic_processus_iterate_end  LIST_ITERATE_END

struct ap_timer
{
	long seconds;
	bool npassed;
};

void ap_timers_init(void);
void ap_timers_update(void);

void auto_filter_change(automatic_processus *pap,enum automatic_value value);
void auto_filter_normalize(automatic_processus *pap);
bool is_auto_value_allowed(automatic_processus *pap,enum automatic_value value);
const char *ap_event_name(enum automatic_value event);
void automatic_processus_event(enum automatic_value event,void *data);
void automatic_processus_init(void);
#define automatic_processus_new(page,default_auto_filter,menu,description,data,...)	\
	real_automatic_processus_new(__FILE__,__LINE__,page,default_auto_filter,menu,description,data,__VA_ARGS__);
void automatic_processus_remove(automatic_processus *pap);
struct automatic_processus_list *get_automatic_processus(void);
automatic_processus *find_automatic_processus_by_name(const char *path);
automatic_processus *real_automatic_processus_new(const char *file,const int line,enum peppage page,filter default_auto_filter,
	const char *menu,const char *description,int data,...);

/* multi-select allows a multi units selection */
#define MULTI_SELECT_NUM 10

struct multi_select
{
	struct unit_list ulist;
	struct unit *punit_focus;
};

bool is_unit_in_multi_select(int multi,struct unit *punit);
void multi_select_active_all(int multi);
void multi_select_add_unit(struct unit *punit);
void multi_select_add_or_remove_unit(struct unit *punit);
void multi_select_add_units(struct unit_list *ulist);
void multi_select_blink_update(void);
void multi_select_cat(int dest,int src);
void multi_select_clear(int multi);
void multi_select_clear_all(void);
void multi_select_copy(int dest,int src);
struct multi_select *multi_select_get(int multi);
struct unit_list *multi_select_get_units_focus(void);
void multi_select_init(int multi);
void multi_select_init_all(void);
void multi_select_remove_unit(struct unit *punit);
int multi_select_satisfies_filter(int multi);
void multi_select_set(int multi,struct multi_select *pms);
void multi_select_set_unit(int multi,struct unit *punit);
void multi_select_set_unit_focus(int multi,struct unit *punit);
int multi_select_size(int multi);
void multi_select_wipe_up_unit(struct unit *punit);
Unit_Type_id multi_select_unit_type(int multi);

void multi_select_select(void);

/* delayed goto with many selections and automatic processus */
#define DELAYED_GOTO_NUM 4

struct delayed_goto_data {
	int id;
	int type;
	struct tile *ptile;
};

#define SPECLIST_TAG delayed_goto_data
#define SPECLIST_TYPE struct delayed_goto_data
#include "speclist.h"
#define delayed_goto_data_list_iterate(alist,pitem)	\
	TYPED_LIST_ITERATE(struct delayed_goto_data,alist,pitem)
#define delayed_goto_data_list_iterate_end LIST_ITERATE_END

struct delayed_goto
{
	struct delayed_goto_data_list dglist;
	automatic_processus *pap;
	struct player *pplayer;
};

extern int delayed_para_or_nuke;
extern int unit_limit;
extern int need_tile_for;
	
void delayed_goto_add_unit(int dg,int id,int type,struct tile *ptile);
void delayed_goto_cat(int dest,int src);
void delayed_goto_clear(int dg);
void delayed_goto_clear_all(void);
void delayed_goto_copy(int dest,int src);
struct delayed_goto *delayed_goto_get(int dg);
void delayed_goto_init(int dg);
void delayed_goto_init_all(void);
void delayed_goto_move(int dest,int src);
void delayed_goto_set(int dg,struct delayed_goto *pdg);
int delayed_goto_size(int dg);
struct player *get_tile_player(struct tile *ptile);

void add_unit_to_delayed_goto(struct tile *ptile);
void request_player_execute_delayed_goto(struct player *pplayer,int dg);
void request_unit_execute_delayed_goto(int dg);
void request_execute_delayed_goto(struct tile *ptile,int dg);
void schedule_delayed_airlift(struct tile *ptile);

/* airlift queues with many selections */
#define AIRLIFT_QUEUE_NUM 7
struct airlift_queue
{
	struct tile_list tlist;
	void *widgets[U_LAST+1];//GtkWidget,for menu.c 
	int utype;
};

extern int need_city_for;

void airlift_queue_cat(int dest,int src);
void airlift_queue_clear(int aq);
void airlift_queue_clear_all(void);
void airlift_queue_copy(int dest,int src);
struct airlift_queue *airlift_queue_get(int aq);
void *airlift_queue_get_menu_item(int aq,Unit_Type_id utype);
Unit_Type_id airlift_queue_get_unit_type(int aq);
void airlift_queue_init(int aq);
void airlift_queue_init_all(void);
void airlift_queue_move(int dest,int src);
void airlift_queue_set(int aq,struct airlift_queue *paq);
void airlift_queue_set_menu_item(int aq,Unit_Type_id utype,void *widget);
void airlift_queue_set_unit_type(int aq,Unit_Type_id utype);
void airlift_queue_show(int aq);
int airlift_queue_size(int aq);

void add_city_to_auto_airlift_queue(struct tile *ptile,bool multi);
void request_auto_airlift_source_selection_with_airport(void);
void do_airlift_for(int aq,struct city *pcity);
void do_airlift(struct tile *ptile);

#endif /* _MULTISELECT_H */
