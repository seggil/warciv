
#include "../../config.h"
#include "../../common/city.h"
#include "../../common/traderoute.h"
#include "../../utility/registry.h"

const int city_nbr = 8;
bool is_server = 0;
struct section_file secfile;

static unsigned char used_ids[8192]={0};
void dealloc_id(int id)  /* kludge was in civclient */
{
  used_ids[id/8]&= 0xff ^ (1<<(id%8));
}

void alloc_id(int id)
{
  used_ids[id/8]|= (1<<(id%8));
}

int main()
{
  struct trade_route my_trader[10];
  city_t my_city[city_nbr];

  secfile.filename = "civgame.sav";
  game_load( &secfile );
  my_city[0].common.trade_routes = 0;
  my_city[1].common.trade_routes = 0;
  game_trade_route_add( &my_city[0], &my_city[1]);
  return 0;
}
