#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <assert.h>
#include <string.h>

#include "capability.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "game.h"

#include "packets.h"

/* in packets_gen.c also */
static unsigned int hash_const(const void *vkey, unsigned int num_buckets)
{
  return 0;
}

static int cmp_const(const void *vkey1, const void *vkey2)
{
  return 0;
}

/* 0 sc */
static struct packet_processing_started *
receive_packet_processing_started_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_processing_started, real_packet);
  printf("sc op=0 PROCESSING_STARTED\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_processing_started(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PROCESSING_STARTED] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PROCESSING_STARTED] = variant;
}

struct packet_processing_started *
receive_packet_processing_started(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_processing_started at the server.");
  }
  ensure_valid_variant_packet_processing_started(pconn);

  switch(pconn->phs.variant[PACKET_PROCESSING_STARTED]) {
    case 100: {
      return receive_packet_processing_started_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 1 sc */
static struct packet_processing_finished *
receive_packet_processing_finished_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_processing_finished, real_packet);
  printf("sc op=1 PROCESSING_FINISHED\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_processing_finished(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PROCESSING_FINISHED] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PROCESSING_FINISHED] = variant;
}

struct packet_processing_finished *
receive_packet_processing_finished(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_processing_finished at the server.");
  }
  ensure_valid_variant_packet_processing_finished(pconn);

  switch(pconn->phs.variant[PACKET_PROCESSING_FINISHED]) {
    case 100: {
      return receive_packet_processing_finished_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 2 sc */
static struct packet_freeze_hint *
receive_packet_freeze_hint_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_freeze_hint, real_packet);
  printf("sc op=2 FREEZE_HINT\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_freeze_hint(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_FREEZE_HINT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_FREEZE_HINT] = variant;
}

struct packet_freeze_hint *
receive_packet_freeze_hint(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_freeze_hint at the server.");
  }
  ensure_valid_variant_packet_freeze_hint(pconn);

  switch(pconn->phs.variant[PACKET_FREEZE_HINT]) {
    case 100: {
      return receive_packet_freeze_hint_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 3 sc */
static struct packet_thaw_hint *
receive_packet_thaw_hint_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_thaw_hint, real_packet);
  printf("sc op=3 THAW_HINT\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_thaw_hint(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_THAW_HINT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_THAW_HINT] = variant;
}

struct packet_thaw_hint *
receive_packet_thaw_hint(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_thaw_hint at the server.");
  }
  ensure_valid_variant_packet_thaw_hint(pconn);

  switch(pconn->phs.variant[PACKET_THAW_HINT]) {
    case 100: {
      return receive_packet_thaw_hint_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 4 cs */
/* 5 sc */
static struct packet_server_join_reply *
receive_packet_server_join_reply_100(
       connection_t *pconn,
       enum packet_type type)
{
  int readin;

  RECEIVE_PACKET_START(packet_server_join_reply, real_packet);
  printf("sc op=5 SERVER_JOIN_REPLY");
  dio_get_bool8(&din, &real_packet->you_can_join);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
  dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
  dio_get_string(&din, real_packet->challenge_file, sizeof(real_packet->challenge_file));

  dio_get_uint8(&din, &readin);
  real_packet->conn_id = readin;
  printf(" you_can_join=%u; message=%s; capability=%s; challenge_file=%s"
         "conn_id=%u\n",
         real_packet->you_can_join, real_packet->message,
         real_packet->capability, real_packet->challenge_file,
         real_packet->conn_id);
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_server_join_reply(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SERVER_JOIN_REPLY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SERVER_JOIN_REPLY] = variant;
}

struct packet_server_join_reply *
receive_packet_server_join_reply(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_server_join_reply at the server.");
  }
  ensure_valid_variant_packet_server_join_reply(pconn);

  switch(pconn->phs.variant[PACKET_SERVER_JOIN_REPLY]) {
    case 100: {
      return receive_packet_server_join_reply_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 6 sc */
#define hash_packet_authentication_req_100 hash_const

#define cmp_packet_authentication_req_100 cmp_const

BV_DEFINE(packet_authentication_req_100_fields, 2);

static struct packet_authentication_req *
receive_packet_authentication_req_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_authentication_req_100_fields fields;
  struct packet_authentication_req *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_authentication_req *clone;
  RECEIVE_PACKET_START(packet_authentication_req, real_packet);
  printf("sc op=6 AUTHENTICATION_REQ");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_authentication_req_100,
                     cmp_packet_authentication_req_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    printf(" type=%u", readin);
    real_packet->type = readin;
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
    printf(" message=%s\n", real_packet->message);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_authentication_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_AUTHENTICATION_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_AUTHENTICATION_REQ] = variant;
}

struct packet_authentication_req *
receive_packet_authentication_req(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_authentication_req at the server.");
  }
  ensure_valid_variant_packet_authentication_req(pconn);

  switch(pconn->phs.variant[PACKET_AUTHENTICATION_REQ]) {
    case 100: {
      return receive_packet_authentication_req_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 7 cs */
/* 8 sc */
static struct packet_server_shutdown *
receive_packet_server_shutdown_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_server_shutdown, real_packet);
  printf("sc op=8 SERVER_SHUTDOWN\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_server_shutdown(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SERVER_SHUTDOWN] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SERVER_SHUTDOWN] = variant;
}

struct packet_server_shutdown *
receive_packet_server_shutdown(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_server_shutdown at the server.");
  }
  ensure_valid_variant_packet_server_shutdown(pconn);

  switch(pconn->phs.variant[PACKET_SERVER_SHUTDOWN]) {
    case 100: {
      return receive_packet_server_shutdown_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 9 sc */
#define hash_packet_nation_unavailable_100 hash_const

#define cmp_packet_nation_unavailable_100 cmp_const

BV_DEFINE(packet_nation_unavailable_100_fields, 1);

static struct packet_nation_unavailable *
receive_packet_nation_unavailable_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_nation_unavailable_100_fields fields;
  struct packet_nation_unavailable *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_nation_unavailable *clone;
  RECEIVE_PACKET_START(packet_nation_unavailable, real_packet);
  printf("sc op=9 NATION_UNAVAILABLE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_nation_unavailable_100,
                     cmp_packet_nation_unavailable_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
    printf(" nation=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_nation_unavailable(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_NATION_UNAVAILABLE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_NATION_UNAVAILABLE] = variant;
}

struct packet_nation_unavailable *
receive_packet_nation_unavailable(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_nation_unavailable at the server.");
  }
  ensure_valid_variant_packet_nation_unavailable(pconn);

  switch(pconn->phs.variant[PACKET_NATION_UNAVAILABLE]) {
    case 100: {
      return receive_packet_nation_unavailable_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 10 cs */
/* 11 sc */
static struct packet_nation_select_ok *
receive_packet_nation_select_ok_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_nation_select_ok, real_packet);
  printf("sc op=11 NATION_SELECT_OK\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_nation_select_ok(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_NATION_SELECT_OK] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_NATION_SELECT_OK] = variant;
}

struct packet_nation_select_ok *
receive_packet_nation_select_ok(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_nation_select_ok at the server.");
  }
  ensure_valid_variant_packet_nation_select_ok(pconn);

  switch(pconn->phs.variant[PACKET_NATION_SELECT_OK]) {
    case 100: {
      return receive_packet_nation_select_ok_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 12 sc */
#define hash_packet_game_state_100 hash_const

#define cmp_packet_game_state_100 cmp_const

BV_DEFINE(packet_game_state_100_fields, 1);

static struct packet_game_state *
receive_packet_game_state_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_game_state_100_fields fields;
  struct packet_game_state *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_game_state *clone;
  int readin;

  RECEIVE_PACKET_START(packet_game_state, real_packet);
  printf("sc op=12 GAME_STATE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_game_state_100,
                     cmp_packet_game_state_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  post_receive_packet_game_state(pconn, real_packet);
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_game_state(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_GAME_STATE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_GAME_STATE] = variant;
}

struct packet_game_state *
receive_packet_game_state(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_game_state at the server.");
  }
  ensure_valid_variant_packet_game_state(pconn);

  switch(pconn->phs.variant[PACKET_GAME_STATE]) {
    case 100: {
      return receive_packet_game_state_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 13 sc */
#define hash_packet_endgame_report_100 hash_const

#define cmp_packet_endgame_report_100 cmp_const

BV_DEFINE(packet_endgame_report_100_fields, 15);

static struct packet_endgame_report *
receive_packet_endgame_report_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_endgame_report_100_fields fields;
  struct packet_endgame_report *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_endgame_report *clone;
  int readin;

  RECEIVE_PACKET_START(packet_endgame_report, real_packet);
  printf("sc op=13 ENDGAME_REPORT\n");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_endgame_report_100,
                     cmp_packet_endgame_report_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_uint8(&din, &readin);
    real_packet->nscores = readin;
    printf("nscores=%u ", readin);
  }

  if (BV_ISSET(fields, 1)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->id[i] = readin;
      printf(" id[%d]=%u", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->score[i] = readin;
      printf(" score[%d]=%u", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 3)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint32(&din, &readin);
      real_packet->pop[i] = readin;
      printf("pop[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 4)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->bnp[i] = readin;
      printf("bnp[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 5)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->mfg[i] = readin;
      printf("mfg[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 6)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->cities[i] = readin;
      printf("cities[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 7)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->techs[i] = readin;
      printf("techs[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->mil_service[i] = readin;
      printf("mil_service[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 9)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->wonders[i] = readin;
      printf("wonders[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 10)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->research[i] = readin;
      printf("research[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 11)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint32(&din, &readin);
      real_packet->landarea[i] = readin;
      printf("landarea[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 12)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint32(&din, &readin);
      real_packet->settledarea[i] = readin;
      printf("setlandarea[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 13)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->literacy[i] = readin;
      printf("literacy[%d]=%u ", i, readin);
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 14)) {
    int i;

    if(real_packet->nscores > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nscores = MAX_NUM_PLAYERS;
    }
    for (i = 0; i < real_packet->nscores; i++) {
      int readin;

      dio_get_uint32(&din, &readin);
      real_packet->spaceship[i] = readin;
      printf("literacy[%d]=%u ", i, readin);
    }
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_endgame_report(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_ENDGAME_REPORT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_ENDGAME_REPORT] = variant;
}

struct packet_endgame_report *
receive_packet_endgame_report(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_endgame_report at the server.");
  }
  ensure_valid_variant_packet_endgame_report(pconn);

  switch(pconn->phs.variant[PACKET_ENDGAME_REPORT]) {
    case 100: {
      return receive_packet_endgame_report_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 14 sc */
static unsigned int hash_packet_tile_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_tile_info *key = (const struct packet_tile_info *) vkey;

  return (((key->x << 8) ^ key->y) % num_buckets);
}

static int cmp_packet_tile_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_tile_info *key1 = (const struct packet_tile_info *) vkey1;
  const struct packet_tile_info *key2 = (const struct packet_tile_info *) vkey2;
  int diff;

  diff = key1->x - key2->x;
  if (diff != 0) {
    return diff;
  }

  diff = key1->y - key2->y;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_tile_info_100_fields, 6);

static struct packet_tile_info *
receive_packet_tile_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_tile_info_100_fields fields;
  struct packet_tile_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_tile_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_tile_info, real_packet);
  printf("sc op=14 TILE_INFO");
  DIO_BV_GET(&din, fields);

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" x=%u y=%u", real_packet->x, readin);

  if (!*hash) {
    *hash = hash_new(hash_packet_tile_info_100,
                     cmp_packet_tile_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int x = real_packet->x;
    int y = real_packet->y;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->x = x;
    real_packet->y = y;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->type = readin;
    printf(" type=%d", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->known = readin;
    printf(" known=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->special = readin;
    printf(" special=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
    printf(" owner=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->continent = readin;
    printf(" continent=%d", readin);
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->spec_sprite, sizeof(real_packet->spec_sprite));
    printf(" spec_sprite=\"%s\"\n", real_packet->spec_sprite);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_tile_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_TILE_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_TILE_INFO] = variant;
}

struct packet_tile_info *
receive_packet_tile_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_tile_info at the server.");
  }
  ensure_valid_variant_packet_tile_info(pconn);

  switch(pconn->phs.variant[PACKET_TILE_INFO]) {
    case 100: {
      return receive_packet_tile_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 15 sc */
#define hash_packet_game_info_100 hash_const

#define cmp_packet_game_info_100 cmp_const

BV_DEFINE(packet_game_info_100_fields, 30);

static struct packet_game_info *
receive_packet_game_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_game_info_100_fields fields;
  struct packet_game_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_game_info *clone;
  RECEIVE_PACKET_START(packet_game_info, real_packet);
  printf("sc op=15 GAME_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_game_info_100,
                     cmp_packet_game_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
    printf(" gold=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->tech = readin;
    printf(" tech=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researchcost = readin;
    printf(" researchcost=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->skill_level = readin;
    printf(" skill_level=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->seconds_to_turndone = readin;
    printf(" seconds_to_turndone=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->timeout = readin;
    printf(" timeout=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn = readin;
    printf(" turn=%d", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->year = readin;
    printf(" year=%d", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->end_year = readin;
    printf(" end_year=%d", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_players = readin;
    printf(" min_players=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->max_players = readin;
    printf(" max_players=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nplayers = readin;
    printf(" nplayers=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->player_idx = readin;
    printf(" player_idx=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->globalwarming = readin;
    printf(" globalwarming=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->heating = readin;
    printf(" heating=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->nuclearwinter = readin;
    printf(" nuclearwinter=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cooling = readin;
    printf(" cooling=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->cityfactor = readin;
    printf(" cityfactor=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->diplcost = readin;
    printf(" diplcost=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->freecost = readin;
    printf(" freecost=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->conquercost = readin;
    printf(" conquercost=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappysize = readin;
    printf(" unhappysize=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->angrycitizen = readin;
    printf(" angrycitizen =%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techpenalty = readin;
    printf(" techpenalty =%u", readin);
  }

  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->foodbox = readin;
    printf(" foodbox =%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->civstyle = readin;
    printf(" civstyle =%u", readin);
  }

  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->diplomacy = readin;
    printf(" diplomacy =%u", readin);
  }

  real_packet->spacerace = BV_ISSET(fields, 27);
  printf(" spacerace =%u", real_packet->spacerace);

  if (BV_ISSET(fields, 28)) {
    printf(" spacerace[]=");
    for (;;) {
      int i;

      dio_get_uint8(&din, &i);
      if(i == 255) {
        break;
      }
      if(i > A_LAST) {
        freelog(LOG_ERROR, "packets_gen.c: WARNING: ignoring intra array diff");
      } else {
        int readin;

        dio_get_uint8(&din, &readin);
        real_packet->global_advances[i] = readin;
        printf(" %u", readin);
      }
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 29)) {
    printf(" global_wonders[]=");
    for (;;) {
      int i;

      dio_get_uint8(&din, &i);
      if(i == 255) {
        break;
      }
      if(i > B_LAST) {
        freelog(LOG_ERROR, "packets_gen.c: WARNING: ignoring intra array diff");
      } else {
        int readin;

        dio_get_uint16(&din, &readin);
        real_packet->global_wonders[i] = readin;
        printf(" %u", readin);
      }
    }
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_game_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_GAME_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_GAME_INFO] = variant;
}

struct packet_game_info *
receive_packet_game_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_game_info at the server.");
  }
  ensure_valid_variant_packet_game_info(pconn);

  switch(pconn->phs.variant[PACKET_GAME_INFO]) {
    case 100: {
      return receive_packet_game_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 16 sc */
#define hash_packet_map_info_100 hash_const

#define cmp_packet_map_info_100 cmp_const

BV_DEFINE(packet_map_info_100_fields, 3);

static struct packet_map_info *
receive_packet_map_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_map_info_100_fields fields;
  struct packet_map_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_map_info *clone;
  RECEIVE_PACKET_START(packet_map_info, real_packet);
  printf("sc op=16 MAP_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_map_info_100,
                     cmp_packet_map_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->xsize = readin;
    printf(" xsize=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->ysize = readin;
    printf(" ysize=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->topology_id = readin;
    printf(" topology_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_map_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_MAP_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_MAP_INFO] = variant;
}

struct packet_map_info *
receive_packet_map_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_map_info at the server.");
  }
  ensure_valid_variant_packet_map_info(pconn);

  switch(pconn->phs.variant[PACKET_MAP_INFO]) {
    case 100: {
      return receive_packet_map_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 17 sc */
#define hash_packet_nuke_tile_info_100 hash_const

#define cmp_packet_nuke_tile_info_100 cmp_const

BV_DEFINE(packet_nuke_tile_info_100_fields, 2);

static struct packet_nuke_tile_info *
receive_packet_nuke_tile_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_nuke_tile_info_100_fields fields;
  struct packet_nuke_tile_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_nuke_tile_info *clone;
  RECEIVE_PACKET_START(packet_nuke_tile_info, real_packet);
  printf("sc op=17 NUKE_TILE_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_nuke_tile_info_100,
                     cmp_packet_nuke_tile_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" y=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_nuke_tile_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_NUKE_TILE_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_NUKE_TILE_INFO] = variant;
}

struct packet_nuke_tile_info *
receive_packet_nuke_tile_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_nuke_tile_info at the server.");
  }
  ensure_valid_variant_packet_nuke_tile_info(pconn);

  switch(pconn->phs.variant[PACKET_NUKE_TILE_INFO]) {
    case 100: {
      return receive_packet_nuke_tile_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 18 sc */
#define hash_packet_chat_msg_100 hash_const

#define cmp_packet_chat_msg_100 cmp_const

BV_DEFINE(packet_chat_msg_100_fields, 5);

static struct packet_chat_msg *
receive_packet_chat_msg_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_chat_msg_100_fields fields;
  struct packet_chat_msg *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_chat_msg *clone;
  RECEIVE_PACKET_START(packet_chat_msg, real_packet);
  printf("sc op=18 CHAT_MSG");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_chat_msg_100,
                     cmp_packet_chat_msg_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
    printf(" message=\"%s\"", real_packet->message);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
     int readin;

     dio_get_uint8(&din, &readin);
     real_packet->y = readin;
     printf(" y=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->event = readin;
    printf(" event=%d", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->conn_id = readin;
    printf(" conn_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  post_receive_packet_chat_msg(pconn, real_packet);
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_chat_msg(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CHAT_MSG] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CHAT_MSG] = variant;
}

struct packet_chat_msg *
receive_packet_chat_msg(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_chat_msg at the server.");
  }
  ensure_valid_variant_packet_chat_msg(pconn);

  switch(pconn->phs.variant[PACKET_CHAT_MSG]) {
    case 100: {
      return receive_packet_chat_msg_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 19 cs */
/* 20 sc */
#define hash_packet_city_remove_100 hash_const

#define cmp_packet_city_remove_100 cmp_const

BV_DEFINE(packet_city_remove_100_fields, 1);

static struct packet_city_remove *
receive_packet_city_remove_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_remove_100_fields fields;
  struct packet_city_remove *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_remove *clone;
  RECEIVE_PACKET_START(packet_city_remove, real_packet);
  printf("sc op=20 CITY_REMOVE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_remove_100,
                     cmp_packet_city_remove_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city_id = readin;
    printf(" city_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_remove(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_REMOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_REMOVE] = variant;
}

struct packet_city_remove *
receive_packet_city_remove(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_remove at the server.");
  }
  ensure_valid_variant_packet_city_remove(pconn);

  switch(pconn->phs.variant[PACKET_CITY_REMOVE]) {
    case 100: {
      return receive_packet_city_remove_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 21 sc */
static unsigned int hash_packet_city_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_city_info *key = (const struct packet_city_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_city_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_city_info *key1 = (const struct packet_city_info *) vkey1;
  const struct packet_city_info *key2 = (const struct packet_city_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_city_info_100_fields, 47);

static struct packet_city_info *
receive_packet_city_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_info_100_fields fields;
  struct packet_city_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_info *clone;
  RECEIVE_PACKET_START(packet_city_info, real_packet);
  printf("sc op=21 CITY_INFO");
  DIO_BV_GET(&din, fields);
  int readin;

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
  printf(" id=%u", readin);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_info_100,
                     cmp_packet_city_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
    printf(" owner=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" y=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
    printf(" size=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int i;

    printf(" people_happy[5]=");
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_happy[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

    printf(" people_content[5]=");
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_content[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

    printf(" people_unhappy[5]=");
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_unhappy[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    printf(" people_angry[5]=");
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_angry[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 9)) {
    int i;

    printf(" specialists[5]=");
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialists[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_prod = readin;
    printf(" food_prod=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_prod = readin;
    printf(" shield_prod=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->trade_prod = readin;
    printf(" trade_prod=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->food_surplus = readin;
    printf(" food_surplus=%d", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->shield_surplus = readin;
    printf(" shield_surplus=%d", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tile_trade = readin;
    printf(" tile_trade=%d", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_stock = readin;
    printf(" food_stock=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_stock = readin;
    printf(" shield_stock=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption = readin;
    printf(" corruption=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int i;

    printf(" trade[]=");
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->trade[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 20)) {
    int i;

    printf(" trade_value[]=");
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trade_value[i] = readin;
      printf("%u", readin);
    }
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->luxury_total = readin;
    printf(" luxury_total=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tax_total = readin;
    printf(" tax_total=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->science_total = readin;
    printf(" science_total=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution = readin;
    printf(" pollution=%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_waste = readin;
    printf(" shield_waste=%u", readin);
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->currently_building = readin;
    printf(" currently_building=%u", readin);
  }
  real_packet->is_building_unit = BV_ISSET(fields, 27);
  printf(" is_building_unit=%u", readin);

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_last_built = readin;
    printf(" turn_last_built=%d", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->changed_from_id = readin;
    printf(" changed_from_id=%u", readin);
  }
  real_packet->changed_from_is_unit = BV_ISSET(fields, 30);
  printf(" changed_from_is_unit=%u", real_packet->changed_from_is_unit);

  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->before_change_shields = readin;
    printf(" before_change_shields=%u", readin);
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->disbanded_shields = readin;
    printf(" disbanded_shields=%u", readin);
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->caravan_shields = readin;
    printf(" caravan_shields=%u", readin);
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
    printf(" last_turns_shield_surplus=%u", readin);
  }
  if (BV_ISSET(fields, 35)) {
    int i;

    dio_get_worklist(&din, &real_packet->worklist);
    printf("worklist={");
    printf(" is_valid=%u", real_packet->worklist.is_valid);
    printf(" name=\"%s\"", real_packet->worklist.name);
    printf(" wlefs[i]=");
    for (i = 0; i < MAX_LEN_WORKLIST; i++)
      printf(" %u", real_packet->worklist.wlefs[i]);
    printf("}");
  }
  if (BV_ISSET(fields, 36)) {
    int i;

    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
    printf("improvements[%d]=[\n", B_LAST+1);
    for (i = 0; i < B_LAST+1; i++) {
      if ( real_packet->improvements[i] == 0 ) {
        printf("@");
      } else {
        printf("%X", real_packet->improvements[i]);
      }
      if ( i % 100 == 99)
        printf("\n");
      else if ((i % 10) == 9)
       printf(" ");
    }
    printf("] ");
  }
  if (BV_ISSET(fields, 37)) {
    int i;

    printf(" city_map[]=");
    for (i = 0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->city_map[i] = readin;
      printf("%u", readin);
    }
  }
  real_packet->did_buy = BV_ISSET(fields, 38);
  printf(" did_buy=%u", real_packet->did_buy);

  real_packet->did_sell = BV_ISSET(fields, 39);
  printf(" did_sell=%u", real_packet->did_sell);

  real_packet->was_happy = BV_ISSET(fields, 40);
  printf(" was_happy=%u", real_packet->was_happy);

  real_packet->airlift = BV_ISSET(fields, 41);
  printf(" airlift=%u", real_packet->airlift);

  real_packet->diplomat_investigate = BV_ISSET(fields, 42);
  printf(" diplomat_investigate=%u", real_packet->diplomat_investigate);

  if (BV_ISSET(fields, 43)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rally_point_x = readin;
    printf(" rally_point_x=%u", readin);
  }
  if (BV_ISSET(fields, 44)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rally_point_y = readin;
    printf(" rally_point_y=%u", readin);
  }
  if (BV_ISSET(fields, 45)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_options = readin;
    printf(" city_options=%u", readin);
  }
  if (BV_ISSET(fields, 46)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_founded = readin;
    printf(" turn_founded=%d\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static unsigned int hash_packet_city_info_101(const void *vkey, unsigned int num_buckets)
{
  const struct packet_city_info *key = (const struct packet_city_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_city_info_101(const void *vkey1, const void *vkey2)
{
  const struct packet_city_info *key1 = (const struct packet_city_info *) vkey1;
  const struct packet_city_info *key2 = (const struct packet_city_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_city_info_101_fields, 45);

static struct packet_city_info *
receive_packet_city_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_info_101_fields fields;
  struct packet_city_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_info *clone;
  RECEIVE_PACKET_START(packet_city_info, real_packet);

  DIO_BV_GET(&din, fields);
  {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->id = readin;
  }

  if (!*hash) {
    *hash = hash_new(hash_packet_city_info_101,
                     cmp_packet_city_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
  }
  if (BV_ISSET(fields, 5)) {
    int i;

    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_happy[i] = readin;
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_content[i] = readin;
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_unhappy[i] = readin;
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_angry[i] = readin;
    }
  }
  if (BV_ISSET(fields, 9)) {
    int i;

    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialists[i] = readin;
    }
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_prod = readin;
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_prod = readin;
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->trade_prod = readin;
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->food_surplus = readin;
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->shield_surplus = readin;
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tile_trade = readin;
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_stock = readin;
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_stock = readin;
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption = readin;
  }
  if (BV_ISSET(fields, 19)) {
    int i;

    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->trade[i] = readin;
    }
  }
  if (BV_ISSET(fields, 20)) {
    int i;

    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trade_value[i] = readin;
    }
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->luxury_total = readin;
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tax_total = readin;
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->science_total = readin;
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution = readin;
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_waste = readin;
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->currently_building = readin;
  }
  real_packet->is_building_unit = BV_ISSET(fields, 27);
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_last_built = readin;
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->changed_from_id = readin;
  }
  real_packet->changed_from_is_unit = BV_ISSET(fields, 30);
  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->before_change_shields = readin;
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->disbanded_shields = readin;
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->caravan_shields = readin;
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
  }
  if (BV_ISSET(fields, 35)) {
    dio_get_worklist(&din, &real_packet->worklist);
  }
  if (BV_ISSET(fields, 36)) {
    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
  }
  if (BV_ISSET(fields, 37)) {
    int i;

    for (i = 0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->city_map[i] = readin;
    }
  }
  real_packet->did_buy = BV_ISSET(fields, 38);
  real_packet->did_sell = BV_ISSET(fields, 39);
  real_packet->was_happy = BV_ISSET(fields, 40);
  real_packet->airlift = BV_ISSET(fields, 41);
  real_packet->diplomat_investigate = BV_ISSET(fields, 42);
  if (BV_ISSET(fields, 43)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_options = readin;
  }
  if (BV_ISSET(fields, 44)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_founded = readin;
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_INFO] != -1) {
    return;
  }

  if((has_capability("extglobalinfo", pconn->capability)
      && has_capability("extglobalinfo", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("extglobalinfo", pconn->capability)
              && has_capability("extglobalinfo", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_CITY_INFO] = variant;
}

struct packet_city_info *
receive_packet_city_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_info at the server.");
  }
  ensure_valid_variant_packet_city_info(pconn);

  switch(pconn->phs.variant[PACKET_CITY_INFO]) {
    case 100: {
      return receive_packet_city_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_city_info_101(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 22 sc */
static unsigned int hash_packet_city_short_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_city_short_info *key = (const struct packet_city_short_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_city_short_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_city_short_info *key1 = (const struct packet_city_short_info *) vkey1;
  const struct packet_city_short_info *key2 = (const struct packet_city_short_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_city_short_info_100_fields, 11);

static struct packet_city_short_info *
receive_packet_city_short_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_short_info_100_fields fields;
  struct packet_city_short_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_short_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_city_short_info, real_packet);
  printf("sc op=22 CITY_SHORT_INFO");
  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
  printf(" id=%u", readin);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_short_info_100,
                     cmp_packet_city_short_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
    printf(" owner=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" y=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\" ", real_packet->name);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
    printf(" size=%u", readin);
  }
  real_packet->happy = BV_ISSET(fields, 5);
  printf(" happy=%u", real_packet->happy);

  real_packet->unhappy = BV_ISSET(fields, 6);
  printf(" unhappy=%u", real_packet->unhappy);

  real_packet->capital = BV_ISSET(fields, 7);
  printf(" capital=%u", real_packet->capital);

  real_packet->walls = BV_ISSET(fields, 8);
  printf(" walls=%u", real_packet->walls);

  real_packet->occupied = BV_ISSET(fields, 9);
  printf(" occupied=%u", real_packet->occupied);

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tile_trade = readin;
    printf(" tile_trade=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_short_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_SHORT_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_SHORT_INFO] = variant;
}

struct packet_city_short_info *
receive_packet_city_short_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_short_info at the server.");
  }
  ensure_valid_variant_packet_city_short_info(pconn);

  switch(pconn->phs.variant[PACKET_CITY_SHORT_INFO]) {
    case 100: {
      return receive_packet_city_short_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 23 cs */
/* 24 cs */
/* 25 cs */
/* 26 cs */
/* 27 cs */
/* 28 cs */
/* 29 cs */
/* 30 cs */
/* 31 cs */
/* 32 cs */
/* 33 cs */
/* 34 sc */
#define hash_packet_city_incite_info_100 hash_const

#define cmp_packet_city_incite_info_100 cmp_const

BV_DEFINE(packet_city_incite_info_100_fields, 2);

static struct packet_city_incite_info *
receive_packet_city_incite_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_incite_info_100_fields fields;
  struct packet_city_incite_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_incite_info *clone;
  RECEIVE_PACKET_START(packet_city_incite_info, real_packet);
  printf("sc op=34 CITY_INCITE_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_incite_info_100,
                     cmp_packet_city_incite_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city_id = readin;
    printf(" city_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cost = readin;
    printf(" cost=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_incite_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_INCITE_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_INCITE_INFO] = variant;
}

struct packet_city_incite_info *
receive_packet_city_incite_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_incite_info at the server.");
  }
  ensure_valid_variant_packet_city_incite_info(pconn);

  switch(pconn->phs.variant[PACKET_CITY_INCITE_INFO]) {
    case 100: {
      return receive_packet_city_incite_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 35 cs */
/* 36 sc */
#define hash_packet_city_name_suggestion_info_100 hash_const

#define cmp_packet_city_name_suggestion_info_100 cmp_const

BV_DEFINE(packet_city_name_suggestion_info_100_fields, 2);

static struct packet_city_name_suggestion_info *
receive_packet_city_name_suggestion_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_name_suggestion_info_100_fields fields;
  struct packet_city_name_suggestion_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_name_suggestion_info *clone;
  RECEIVE_PACKET_START(packet_city_name_suggestion_info, real_packet);
  printf("sc op=36 CITY_NAME_SUGGESTION_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_name_suggestion_info_100,
                     cmp_packet_city_name_suggestion_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->unit_id = readin;
    printf(" unit_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=%s\n", real_packet->name);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_name_suggestion_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO] = variant;
}

struct packet_city_name_suggestion_info *
receive_packet_city_name_suggestion_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_name_suggestion_info at the server.");
  }
  ensure_valid_variant_packet_city_name_suggestion_info(pconn);

  switch(pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_INFO]) {
    case 100: {
      return receive_packet_city_name_suggestion_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 37 sc */
#define hash_packet_city_sabotage_list_100 hash_const

#define cmp_packet_city_sabotage_list_100 cmp_const

BV_DEFINE(packet_city_sabotage_list_100_fields, 3);

static struct packet_city_sabotage_list *
receive_packet_city_sabotage_list_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_city_sabotage_list_100_fields fields;
  struct packet_city_sabotage_list *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_city_sabotage_list *clone;
  RECEIVE_PACKET_START(packet_city_sabotage_list, real_packet);
  printf("sc op=37 CITY_SABOTAGE_LIST");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_city_sabotage_list_100,
                     cmp_packet_city_sabotage_list_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->diplomat_id = readin;
    printf(" diplomat_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city_id = readin;
    printf(" city_id=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
    printf(" improvements[%d]=[", B_LAST);
    for (i=0; i < B_LAST; i++ ) {
      printf("%d", real_packet->improvements[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_city_sabotage_list(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_SABOTAGE_LIST] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_SABOTAGE_LIST] = variant;
}

struct packet_city_sabotage_list *
receive_packet_city_sabotage_list(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_city_sabotage_list at the server.");
  }
  ensure_valid_variant_packet_city_sabotage_list(pconn);

  switch(pconn->phs.variant[PACKET_CITY_SABOTAGE_LIST]) {
    case 100: {
      return receive_packet_city_sabotage_list_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 38 sc */
#define hash_packet_player_remove_100 hash_const

#define cmp_packet_player_remove_100 cmp_const

BV_DEFINE(packet_player_remove_100_fields, 1);

static struct packet_player_remove *
receive_packet_player_remove_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_player_remove_100_fields fields;
  struct packet_player_remove *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_player_remove *clone;
  RECEIVE_PACKET_START(packet_player_remove, real_packet);
  printf("sc op=38 PLAYER_REMOVE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_player_remove_100,
                     cmp_packet_player_remove_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->player_id = readin;
    printf(" player_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_player_remove(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_REMOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_REMOVE] = variant;
}

struct packet_player_remove *
receive_packet_player_remove(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_remove at the server.");
  }
  ensure_valid_variant_packet_player_remove(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_REMOVE]) {
    case 100: {
      return receive_packet_player_remove_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 39 sc */
static unsigned int hash_packet_player_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_player_info *key = (const struct packet_player_info *) vkey;

  return ((key->playerno) % num_buckets);
}

static int cmp_packet_player_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_player_info *key1 = (const struct packet_player_info *) vkey1;
  const struct packet_player_info *key2 = (const struct packet_player_info *) vkey2;
  int diff;

  diff = key1->playerno - key2->playerno;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_player_info_100_fields, 32);

static struct packet_player_info *
receive_packet_player_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_player_info_100_fields fields;
  struct packet_player_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_player_info *clone;
  RECEIVE_PACKET_START(packet_player_info, real_packet);
  printf("sc op=39 PLAYER_INFO");
  DIO_BV_GET(&din, fields);
  {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->playerno = readin;
  }

  if (!*hash) {
    *hash = hash_new(hash_packet_player_info_100,
                     cmp_packet_player_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int playerno = real_packet->playerno;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->playerno = playerno;
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
    printf(" username=\"%s\"", real_packet->username);
  }
  real_packet->is_male = BV_ISSET(fields, 2);
  printf(" is_male=%u", real_packet->is_male);

  if (BV_ISSET(fields, 3)) {
     int readin;

     dio_get_uint8(&din, &readin);
     real_packet->government = readin;
     printf(" government=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
    printf(" target_government=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
    printf(" embassy=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
    printf(" city_style=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
    printf(" nation=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
    printf(" team=%u", readin);
  }
  real_packet->turn_done = BV_ISSET(fields, 9);
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
    printf(" nturns_idle=%d", readin);
  }
  real_packet->is_alive = BV_ISSET(fields, 11);
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
    printf(" reputation=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int i;

    printf(" diplstates[%d+%d]={", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
      printf("[%d %d %d %d] ",
             real_packet->diplstates[i].type,
             real_packet->diplstates[i].turns_left,
             real_packet->diplstates[i].has_reason_to_cancel,
             real_packet->diplstates[i].contact_turns_left);
      if ((i % 10) == 9)
        printf("\n");
    }
    printf("\n");
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
    printf(" gold=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
    printf(" tax=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
    printf(" science=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
    printf(" luxury=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
    printf(" bulbs_last_turn=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
    printf(" bulbs_researched=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
    printf(" techs_researched=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
    printf(" researching=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->researching_cost = readin;
    printf(" researching_cost=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
    printf(" future_tech=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
    printf(" tech_goal=%u", readin);
  }
  real_packet->is_connected = BV_ISSET(fields, 25);
  printf(" is_connected=%u", real_packet->is_connected);

  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
    printf(" revolution_finishes=%u", readin);
  }
  real_packet->ai = BV_ISSET(fields, 27);
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
    printf(" barbarian_type=%u", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
    printf(" gives_shared_vision=%u", readin);
  }
  if (BV_ISSET(fields, 30)) {
    int i;

    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
    printf(" inventions[%d]=[", B_LAST);
    for (i = 0; i < B_LAST; i++ ) {
      printf("%d", real_packet->inventions[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
  }
  if (BV_ISSET(fields, 31)) {
    int i;

    printf(" love[]=");
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
      printf(" team=%u", readin);
    }
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static unsigned int hash_packet_player_info_101(const void *vkey, unsigned int num_buckets)
{
  const struct packet_player_info *key = (const struct packet_player_info *) vkey;

  return ((key->playerno) % num_buckets);
}

static int cmp_packet_player_info_101(const void *vkey1, const void *vkey2)
{
  const struct packet_player_info *key1 = (const struct packet_player_info *) vkey1;
  const struct packet_player_info *key2 = (const struct packet_player_info *) vkey2;
  int diff;

  diff = key1->playerno - key2->playerno;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_player_info_101_fields, 31);

static struct packet_player_info *
receive_packet_player_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_player_info_101_fields fields;
  struct packet_player_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_player_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_player_info, real_packet);

  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->playerno = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_player_info_101,
                     cmp_packet_player_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int playerno = real_packet->playerno;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->playerno = playerno;
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
  }
  real_packet->is_male = BV_ISSET(fields, 2);
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
  }
  real_packet->turn_done = BV_ISSET(fields, 9);
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
  }
  real_packet->is_alive = BV_ISSET(fields, 11);
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
  }
  if (BV_ISSET(fields, 13)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
    }
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
  }
  real_packet->is_connected = BV_ISSET(fields, 24);
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
  }
  real_packet->ai = BV_ISSET(fields, 26);
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
  }
  if (BV_ISSET(fields, 29)) {
    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
  }
  if (BV_ISSET(fields, 30)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
    }
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static unsigned int hash_packet_player_info_102(const void *vkey, unsigned int num_buckets)
{
  const struct packet_player_info *key = (const struct packet_player_info *) vkey;

  return ((key->playerno) % num_buckets);
}

static int cmp_packet_player_info_102(const void *vkey1, const void *vkey2)
{
  const struct packet_player_info *key1 = (const struct packet_player_info *) vkey1;
  const struct packet_player_info *key2 = (const struct packet_player_info *) vkey2;
  int diff;

  diff = key1->playerno - key2->playerno;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_player_info_102_fields, 31);

static struct packet_player_info *
receive_packet_player_info_102(
       connection_t *pconn,
       enum packet_type type)
{
  packet_player_info_102_fields fields;
  struct packet_player_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_player_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_player_info, real_packet);

  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->playerno = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_player_info_102,
                     cmp_packet_player_info_102);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int playerno = real_packet->playerno;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->playerno = playerno;
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  }
  real_packet->is_male = BV_ISSET(fields, 1);
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
  }
  real_packet->turn_done = BV_ISSET(fields, 8);
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
  }
  real_packet->is_alive = BV_ISSET(fields, 10);
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
  }
  if (BV_ISSET(fields, 12)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
    }
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->researching_cost = readin;
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
  }
  real_packet->is_connected = BV_ISSET(fields, 24);
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
  }
  real_packet->ai = BV_ISSET(fields, 26);
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
  }
  if (BV_ISSET(fields, 29)) {
    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
  }
  if (BV_ISSET(fields, 30)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
    }
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static unsigned int hash_packet_player_info_103(const void *vkey, unsigned int num_buckets)
{
  const struct packet_player_info *key = (const struct packet_player_info *) vkey;

  return ((key->playerno) % num_buckets);
}

static int cmp_packet_player_info_103(const void *vkey1, const void *vkey2)
{
  const struct packet_player_info *key1 = (const struct packet_player_info *) vkey1;
  const struct packet_player_info *key2 = (const struct packet_player_info *) vkey2;
  int diff;

  diff = key1->playerno - key2->playerno;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_player_info_103_fields, 30);

static struct packet_player_info *
receive_packet_player_info_103(
       connection_t *pconn,
       enum packet_type type)
{
  packet_player_info_103_fields fields;
  struct packet_player_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_player_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_player_info, real_packet);

  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->playerno = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_player_info_103,
                     cmp_packet_player_info_103);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int playerno = real_packet->playerno;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->playerno = playerno;
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
  }
  real_packet->is_male = BV_ISSET(fields, 1);
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
  }
  real_packet->turn_done = BV_ISSET(fields, 8);
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
  }
  real_packet->is_alive = BV_ISSET(fields, 10);
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
  }
  if (BV_ISSET(fields, 12)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
    }
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
  }
  real_packet->is_connected = BV_ISSET(fields, 23);
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
  }
  real_packet->ai = BV_ISSET(fields, 25);
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
  }
  if (BV_ISSET(fields, 28)) {
    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
  }
  if (BV_ISSET(fields, 29)) {
    int i;

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
    }
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_player_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_INFO] != -1) {
    return;
  }

  if((has_capability("username_info", pconn->capability)
      && has_capability("username_info", our_capability)
     )
     && (has_capability("exttechleakage", pconn->capability)
         && has_capability("exttechleakage", our_capability)
        ))
  {
    variant = 100;
  } else if((has_capability("username_info", pconn->capability)
             && has_capability("username_info", our_capability)
            )
            && !(has_capability("exttechleakage", pconn->capability)
                 && has_capability("exttechleakage", our_capability)
                ))
  {
    variant = 101;
  } else if((has_capability("exttechleakage", pconn->capability)
             && has_capability("exttechleakage", our_capability)
            )
            && !(has_capability("username_info", pconn->capability)
                 && has_capability("username_info", our_capability)
                ))
  {
    variant = 102;
  } else if(!(has_capability("username_info", pconn->capability)
              && has_capability("username_info", our_capability)
             )
             && !(has_capability("exttechleakage", pconn->capability)
                  && has_capability("exttechleakage", our_capability)
                 ))
  {
    variant = 103;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_PLAYER_INFO] = variant;
}

struct packet_player_info *
receive_packet_player_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_player_info at the server.");
  }
  ensure_valid_variant_packet_player_info(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_INFO]) {
    case 100: {
      return receive_packet_player_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_player_info_101(pconn, type);
    }
    case 102: {
      return receive_packet_player_info_102(pconn, type);
    }
    case 103: {
      return receive_packet_player_info_103(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 40 cs */
/* 41 cs */
/* 42 */
/* 43 cs */
/* 44 cs */
/* 45 cs */
/* 46 cs */
/* 47 cs sc */
/* 48 sc */
#define hash_packet_unit_remove_100 hash_const

#define cmp_packet_unit_remove_100 cmp_const

BV_DEFINE(packet_unit_remove_100_fields, 1);

static struct packet_unit_remove *
receive_packet_unit_remove_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_remove_100_fields fields;
  struct packet_unit_remove *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_remove *clone;
  RECEIVE_PACKET_START(packet_unit_remove, real_packet);
  printf("sc op=48 UNIT_REMOVE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_remove_100,
                     cmp_packet_unit_remove_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->unit_id = readin;
    printf(" unit_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_remove(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_REMOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_REMOVE] = variant;
}

struct packet_unit_remove *
receive_packet_unit_remove(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_remove at the server.");
  }
  ensure_valid_variant_packet_unit_remove(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_REMOVE]) {
    case 100: {
      return receive_packet_unit_remove_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 49 sc */
static unsigned int hash_packet_unit_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_unit_info *key = (const struct packet_unit_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_unit_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_unit_info *key1 = (const struct packet_unit_info *) vkey1;
  const struct packet_unit_info *key2 = (const struct packet_unit_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_unit_info_100_fields, 35);

static struct packet_unit_info *
receive_packet_unit_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_info_100_fields fields;
  struct packet_unit_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_unit_info, real_packet);
  printf("sc op=49 UNIT_INFO");
  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
  printf(" id=%u", readin);

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_info_100,
                     cmp_packet_unit_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
    printf(" owner=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" y=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->homecity = readin;
    printf(" homecity=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
    printf(" veteran=%u", readin);
  }
  real_packet->ai = BV_ISSET(fields, 5);
  printf(" ai=%u", real_packet->ai);

  real_packet->paradropped = BV_ISSET(fields, 6);
  printf(" paradropped=%u", real_packet->paradropped);

  real_packet->connecting = BV_ISSET(fields, 7);
  printf(" connecting=%u", real_packet->connecting);

  real_packet->transported = BV_ISSET(fields, 8);
  printf(" transported=%u", real_packet->transported);

  real_packet->done_moving = BV_ISSET(fields, 9);
  printf(" done_moving=%u", real_packet->done_moving);

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
    printf(" type=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
    printf(" team=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movesleft = readin;
    printf(" movesleft=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
    printf(" hp=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
    printf(" fuel=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
    printf(" activity_count=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappiness = readin;
    printf(" unhappiness=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
    printf(" upkeep=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_food = readin;
    printf(" upkeep_food=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_gold = readin;
    printf(" upkeep_gold=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->occupy = readin;
    printf(" occupy=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_x = readin;
    printf(" goto_dest_x=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_y = readin;
    printf(" goto_dest_y=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->air_patrol_x = readin;
    printf(" air_patrol_x=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->air_patrol_y = readin;
    printf(" air_patrol_y=%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
    printf(" activity=%u", readin);
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->activity_target = readin;
    printf(" activity_target=%u", readin);
  }
  real_packet->has_orders = BV_ISSET(fields, 27);
  printf(" has_orders=%u", real_packet->has_orders);

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_length = readin;
    printf(" orders_length=%u", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_index = readin;
    printf(" orders_index=%u", readin);
  }
  real_packet->orders_repeat = BV_ISSET(fields, 30);
  printf(" orders_repeat=%u", real_packet->orders_repeat);

  real_packet->orders_vigilant = BV_ISSET(fields, 31);
  printf(" orders_vigilant=%u", real_packet->orders_vigilant);

  if (BV_ISSET(fields, 32)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    printf(" orders[%u]=", real_packet->orders_length);
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 33)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    printf(" orders_dirs[%u]=", real_packet->orders_length);
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_dirs[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 34)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    printf(" orders_activities[%u]=", real_packet->orders_length);
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_activities[i] = readin;
      printf(" %u", readin);
    }
    printf("\n");
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static unsigned int hash_packet_unit_info_101(const void *vkey, unsigned int num_buckets)
{
  const struct packet_unit_info *key = (const struct packet_unit_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_unit_info_101(const void *vkey1, const void *vkey2)
{
  const struct packet_unit_info *key1 = (const struct packet_unit_info *) vkey1;
  const struct packet_unit_info *key2 = (const struct packet_unit_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_unit_info_101_fields, 33);

static struct packet_unit_info *
receive_packet_unit_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_info_101_fields fields;
  struct packet_unit_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_unit_info, real_packet);

  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_info_101,
                     cmp_packet_unit_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->homecity = readin;
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
  }
  real_packet->ai = BV_ISSET(fields, 5);
  real_packet->paradropped = BV_ISSET(fields, 6);
  real_packet->connecting = BV_ISSET(fields, 7);
  real_packet->transported = BV_ISSET(fields, 8);
  real_packet->done_moving = BV_ISSET(fields, 9);
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movesleft = readin;
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappiness = readin;
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_food = readin;
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_gold = readin;
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->occupy = readin;
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_x = readin;
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_y = readin;
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->activity_target = readin;
  }
  real_packet->has_orders = BV_ISSET(fields, 25);
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_length = readin;
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_index = readin;
  }
  real_packet->orders_repeat = BV_ISSET(fields, 28);
  real_packet->orders_vigilant = BV_ISSET(fields, 29);
  if (BV_ISSET(fields, 30)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders[i] = readin;
    }
  }
  if (BV_ISSET(fields, 31)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_dirs[i] = readin;
    }
  }
  if (BV_ISSET(fields, 32)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_activities[i] = readin;
    }
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_INFO] != -1) {
    return;
  }

  if((has_capability("extglobalinfo", pconn->capability)
      && has_capability("extglobalinfo", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("extglobalinfo", pconn->capability)
              && has_capability("extglobalinfo", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_UNIT_INFO] = variant;
}

struct packet_unit_info *
receive_packet_unit_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_info at the server.");
  }
  ensure_valid_variant_packet_unit_info(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_INFO]) {
    case 100: {
      return receive_packet_unit_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_unit_info_101(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 50 sc */
static unsigned int hash_packet_unit_short_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_unit_short_info *key = (const struct packet_unit_short_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_unit_short_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_unit_short_info *key1 = (const struct packet_unit_short_info *) vkey1;
  const struct packet_unit_short_info *key2 = (const struct packet_unit_short_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_unit_short_info_100_fields, 14);

static struct packet_unit_short_info *
receive_packet_unit_short_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_short_info_100_fields fields;
  struct packet_unit_short_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_short_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_unit_short_info, real_packet);
  printf("sc op=50 UNIT_SHORT_INFO");
  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_short_info_100,
                     cmp_packet_unit_short_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
    printf(" owner=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
    printf(" x=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
    printf(" y=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
    printf(" type=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
    printf(" veteran=%u", readin);
  }
  real_packet->occupied = BV_ISSET(fields, 5);
  printf(" occupied=%u", real_packet->occupied);

  real_packet->goes_out_of_sight = BV_ISSET(fields, 6);
  printf(" goes_out_of_sight=%u", real_packet->goes_out_of_sight);

  real_packet->transported = BV_ISSET(fields, 7);
  printf(" transported=%u", real_packet->transported);

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
    printf(" hp=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
    printf(" activity=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
    printf(" transported_by=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->packet_use = readin;
    printf(" packet_use=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->info_city_id = readin;
    printf(" info_city_id=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->serial_num = readin;
    printf(" serial_num=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_short_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_SHORT_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_SHORT_INFO] = variant;
}

struct packet_unit_short_info *
receive_packet_unit_short_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_short_info at the server.");
  }
  ensure_valid_variant_packet_unit_short_info(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_SHORT_INFO]) {
    case 100: {
      return receive_packet_unit_short_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 51 sc */
#define hash_packet_unit_combat_info_100 hash_const

#define cmp_packet_unit_combat_info_100 cmp_const

BV_DEFINE(packet_unit_combat_info_100_fields, 5);

static struct packet_unit_combat_info *
receive_packet_unit_combat_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_combat_info_100_fields fields;
  struct packet_unit_combat_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_combat_info *clone;
  RECEIVE_PACKET_START(packet_unit_combat_info, real_packet);
  printf("sc op=51 UNIT_COMBAT_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_combat_info_100,
                     cmp_packet_unit_combat_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->attacker_unit_id = readin;
    printf(" attacker_unit_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->defender_unit_id = readin;
    printf(" defender_unit_id=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->attacker_hp = readin;
    printf(" attacker_hp=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defender_hp = readin;
    printf(" defender_hp=%u", readin);
  }
  real_packet->make_winner_veteran = BV_ISSET(fields, 4);
  printf(" make_winner_veteran=%u\n", real_packet->make_winner_veteran);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_combat_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_COMBAT_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_COMBAT_INFO] = variant;
}

struct packet_unit_combat_info *
receive_packet_unit_combat_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_combat_info at the server.");
  }
  ensure_valid_variant_packet_unit_combat_info(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_COMBAT_INFO]) {
    case 100: {
      return receive_packet_unit_combat_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 52 cs */
/* 53 cs */
/* 54 cs */
/* 55 cs */
/* 56 cs */
/* 57 cs */
/* 58 cs */
/* 59 cs */
/* 60 cs */
/* 61 cs */
/* 62 cs */
/* 63 cs */
/* 64 cs */
/* 65 cs */
/* 66 */
/* 67 cs */
/* 68 sc */
#define hash_packet_unit_bribe_info_100 hash_const

#define cmp_packet_unit_bribe_info_100 cmp_const

BV_DEFINE(packet_unit_bribe_info_100_fields, 2);

static struct packet_unit_bribe_info *
receive_packet_unit_bribe_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_bribe_info_100_fields fields;
  struct packet_unit_bribe_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_bribe_info *clone;
  RECEIVE_PACKET_START(packet_unit_bribe_info, real_packet);
  printf("sc op=68 UNIT_BRIBE_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_bribe_info_100,
                     cmp_packet_unit_bribe_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->unit_id = readin;
    printf(" unit_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cost = readin;
    printf(" cost=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_bribe_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_BRIBE_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_BRIBE_INFO] = variant;
}

struct packet_unit_bribe_info *
receive_packet_unit_bribe_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_bribe_info at the server.");
  }
  ensure_valid_variant_packet_unit_bribe_info(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_BRIBE_INFO]) {
    case 100: {
      return receive_packet_unit_bribe_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 69 cs */
/* 70 cs */
/* 71 sc */
#define hash_packet_unit_diplomat_popup_dialog_100 hash_const

#define cmp_packet_unit_diplomat_popup_dialog_100 cmp_const

BV_DEFINE(packet_unit_diplomat_popup_dialog_100_fields, 2);

static struct packet_unit_diplomat_popup_dialog *
receive_packet_unit_diplomat_popup_dialog_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_unit_diplomat_popup_dialog_100_fields fields;
  struct packet_unit_diplomat_popup_dialog *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_unit_diplomat_popup_dialog *clone;
  RECEIVE_PACKET_START(packet_unit_diplomat_popup_dialog, real_packet);
  printf("sc op=71 UNIT_DIPLOMAT_POPUP_DIALOG");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_unit_diplomat_popup_dialog_100,
                     cmp_packet_unit_diplomat_popup_dialog_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->diplomat_id = readin;
    printf(" diplomat_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->target_id = readin;
    printf(" target_id=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_unit_diplomat_popup_dialog(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_DIPLOMAT_POPUP_DIALOG] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_DIPLOMAT_POPUP_DIALOG] = variant;
}

struct packet_unit_diplomat_popup_dialog *
receive_packet_unit_diplomat_popup_dialog(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_unit_diplomat_popup_dialog at the server.");
  }
  ensure_valid_variant_packet_unit_diplomat_popup_dialog(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_DIPLOMAT_POPUP_DIALOG]) {
    case 100: {
      return receive_packet_unit_diplomat_popup_dialog_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 72 cs */
/* 73 cs */
/* 74 sc */
#define hash_packet_diplomacy_init_meeting_100 hash_const

#define cmp_packet_diplomacy_init_meeting_100 cmp_const

BV_DEFINE(packet_diplomacy_init_meeting_100_fields, 2);

static struct packet_diplomacy_init_meeting *
receive_packet_diplomacy_init_meeting_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_diplomacy_init_meeting_100_fields fields;
  struct packet_diplomacy_init_meeting *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_diplomacy_init_meeting *clone;
  RECEIVE_PACKET_START(packet_diplomacy_init_meeting, real_packet);
  printf("sc op=74 DIPLOMACY_INIT_MEETING");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_init_meeting_100,
                     cmp_packet_diplomacy_init_meeting_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->counterpart = readin;
    printf(" counterpart=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->initiated_from = readin;
    printf(" initiated_from=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_diplomacy_init_meeting(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING] = variant;
}

struct packet_diplomacy_init_meeting *
receive_packet_diplomacy_init_meeting(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_init_meeting at the server.");
  }
  ensure_valid_variant_packet_diplomacy_init_meeting(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING]) {
    case 100: {
      return receive_packet_diplomacy_init_meeting_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 75 cs */
/* 76 sc */
#define hash_packet_diplomacy_cancel_meeting_100 hash_const

#define cmp_packet_diplomacy_cancel_meeting_100 cmp_const

BV_DEFINE(packet_diplomacy_cancel_meeting_100_fields, 2);

static struct packet_diplomacy_cancel_meeting *
receive_packet_diplomacy_cancel_meeting_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_diplomacy_cancel_meeting_100_fields fields;
  struct packet_diplomacy_cancel_meeting *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_diplomacy_cancel_meeting *clone;
  RECEIVE_PACKET_START(packet_diplomacy_cancel_meeting, real_packet);
  printf("sc op=76 DIPLOMACY_CANCEL_MEETING");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_cancel_meeting_100,
                     cmp_packet_diplomacy_cancel_meeting_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->counterpart = readin;
    printf(" counterpart=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->initiated_from = readin;
    printf(" initiated_from=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_diplomacy_cancel_meeting(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING] = variant;
}

struct packet_diplomacy_cancel_meeting *
receive_packet_diplomacy_cancel_meeting(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_cancel_meeting at the server.");
  }
  ensure_valid_variant_packet_diplomacy_cancel_meeting(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING]) {
    case 100: {
      return receive_packet_diplomacy_cancel_meeting_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 77 cs */
/* 78 sc */
#define hash_packet_diplomacy_create_clause_100 hash_const

#define cmp_packet_diplomacy_create_clause_100 cmp_const

BV_DEFINE(packet_diplomacy_create_clause_100_fields, 4);

static struct packet_diplomacy_create_clause *
receive_packet_diplomacy_create_clause_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_diplomacy_create_clause_100_fields fields;
  struct packet_diplomacy_create_clause *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_diplomacy_create_clause *clone;
  RECEIVE_PACKET_START(packet_diplomacy_create_clause, real_packet);
  printf("sc op=78 DIPLOMACY_CREATE_CLAUSE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_create_clause_100,
                     cmp_packet_diplomacy_create_clause_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->counterpart = readin;
    printf(" counterpart=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->giver = readin;
    printf(" giver=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
    printf(" type=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_diplomacy_create_clause(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE] = variant;
}

struct packet_diplomacy_create_clause *
receive_packet_diplomacy_create_clause(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_create_clause at the server.");
  }
  ensure_valid_variant_packet_diplomacy_create_clause(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE]) {
    case 100: {
      return receive_packet_diplomacy_create_clause_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 79 cs */
/* 80 sc */
#define hash_packet_diplomacy_remove_clause_100 hash_const

#define cmp_packet_diplomacy_remove_clause_100 cmp_const

BV_DEFINE(packet_diplomacy_remove_clause_100_fields, 4);

static struct packet_diplomacy_remove_clause *
receive_packet_diplomacy_remove_clause_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_diplomacy_remove_clause_100_fields fields;
  struct packet_diplomacy_remove_clause *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_diplomacy_remove_clause *clone;
  RECEIVE_PACKET_START(packet_diplomacy_remove_clause, real_packet);
  printf("sc op=80 DIPLOMACY_REMOVE_CLAUSE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_remove_clause_100,
                     cmp_packet_diplomacy_remove_clause_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->counterpart = readin;
    printf(" counterpart=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->giver = readin;
    printf(" giver=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
    printf(" type=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_diplomacy_remove_clause(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE] = variant;
}

struct packet_diplomacy_remove_clause *
receive_packet_diplomacy_remove_clause(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_remove_clause at the server.");
  }
  ensure_valid_variant_packet_diplomacy_remove_clause(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE]) {
    case 100: {
      return receive_packet_diplomacy_remove_clause_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 81 cs */
/* 82 sc */
#define hash_packet_diplomacy_accept_treaty_100 hash_const

#define cmp_packet_diplomacy_accept_treaty_100 cmp_const

BV_DEFINE(packet_diplomacy_accept_treaty_100_fields, 3);

static struct packet_diplomacy_accept_treaty *
receive_packet_diplomacy_accept_treaty_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_diplomacy_accept_treaty_100_fields fields;
  struct packet_diplomacy_accept_treaty *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_diplomacy_accept_treaty *clone;
  RECEIVE_PACKET_START(packet_diplomacy_accept_treaty, real_packet);
  printf("sc op=82 DIPLOMACY_ACCEPT_TREATY");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_accept_treaty_100,
                     cmp_packet_diplomacy_accept_treaty_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->counterpart = readin;
    printf(" counterpart=%u", readin);
  }
  real_packet->I_accepted = BV_ISSET(fields, 1);
  printf(" I_accepted=%u", real_packet->I_accepted);

  real_packet->other_accepted = BV_ISSET(fields, 2);
  printf(" other_accepted=%u\n", real_packet->other_accepted);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_diplomacy_accept_treaty(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY] = variant;
}

struct packet_diplomacy_accept_treaty *
receive_packet_diplomacy_accept_treaty(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_diplomacy_accept_treaty at the server.");
  }
  ensure_valid_variant_packet_diplomacy_accept_treaty(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY]) {
    case 100: {
      return receive_packet_diplomacy_accept_treaty_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 83 cs */
/* 84 sc */
#define hash_packet_page_msg_100 hash_const

#define cmp_packet_page_msg_100 cmp_const

BV_DEFINE(packet_page_msg_100_fields, 2);

static struct packet_page_msg *
receive_packet_page_msg_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_page_msg_100_fields fields;
  struct packet_page_msg *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_page_msg *clone;
  RECEIVE_PACKET_START(packet_page_msg, real_packet);
  printf("sc op=84 PAGE_MSG");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_page_msg_100,
                     cmp_packet_page_msg_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
    printf(" message=\"%s\"", real_packet->message);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->event = readin;
    printf(" event=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_page_msg(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PAGE_MSG] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PAGE_MSG] = variant;
}

struct packet_page_msg *
receive_packet_page_msg(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_page_msg at the server.");
  }
  ensure_valid_variant_packet_page_msg(pconn);

  switch(pconn->phs.variant[PACKET_PAGE_MSG]) {
    case 100: {
      return receive_packet_page_msg_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 85 cs */
/* 86 sc */
static unsigned int hash_packet_conn_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_conn_info *key = (const struct packet_conn_info *) vkey;

  return ((key->id) % num_buckets);
}

static int cmp_packet_conn_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_conn_info *key1 = (const struct packet_conn_info *) vkey1;
  const struct packet_conn_info *key2 = (const struct packet_conn_info *) vkey2;
  int diff;

  diff = key1->id - key2->id;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_conn_info_100_fields, 8);

static struct packet_conn_info *
receive_packet_conn_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_conn_info_100_fields fields;
  struct packet_conn_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_conn_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_conn_info, real_packet);
  printf("cs op=86 CONN_INFO");
  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->id = readin;
  printf(" id=%u", readin);

  if (!*hash) {
    *hash = hash_new(hash_packet_conn_info_100,
                     cmp_packet_conn_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int id = real_packet->id;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->id = id;
  }

  real_packet->used = BV_ISSET(fields, 0);
  printf(" used=%u", real_packet->used);

  real_packet->established = BV_ISSET(fields, 1);
  printf(" established=%u", real_packet->established);

  real_packet->observer = BV_ISSET(fields, 2);
  printf(" observer=%u", real_packet->observer);

  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->player_num = readin;
    printf(" player_num=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->access_level = readin;
    printf(" access_level=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
    printf(" username=\"%s\"", real_packet->username);
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->addr, sizeof(real_packet->addr));
    printf(" addr=\"%s\"", real_packet->addr);
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
    printf(" capability=\"%s\"\n", real_packet->capability);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_conn_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CONN_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CONN_INFO] = variant;
}

struct packet_conn_info *
receive_packet_conn_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_info at the server.");
  }
  ensure_valid_variant_packet_conn_info(pconn);

  switch(pconn->phs.variant[PACKET_CONN_INFO]) {
    case 100: {
      return receive_packet_conn_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 87 sc */
#define hash_packet_conn_ping_info_100 hash_const

#define cmp_packet_conn_ping_info_100 cmp_const

BV_DEFINE(packet_conn_ping_info_100_fields, 3);

static struct packet_conn_ping_info *
receive_packet_conn_ping_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_conn_ping_info_100_fields fields;
  struct packet_conn_ping_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_conn_ping_info *clone;
  RECEIVE_PACKET_START(packet_conn_ping_info, real_packet);
  printf("sc op=87 CONN_PING_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_conn_ping_info_100,
                     cmp_packet_conn_ping_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->connections = readin;
    printf(" connections=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    if(real_packet->connections > MAX_NUM_CONNECTIONS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_CONNECTIONS;
    }
    printf(" conn_id[%u]=", real_packet->connections);
    for (i = 0; i < real_packet->connections; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->conn_id[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 2)) {

    {
      int i;

      if(real_packet->connections > MAX_NUM_CONNECTIONS) {
        freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
        real_packet->connections = MAX_NUM_CONNECTIONS;
      }
      printf(" ping_time[%u]=", real_packet->connections);
      for (i = 0; i < real_packet->connections; i++) {
        int tmp;

        dio_get_uint32(&din, &tmp);
        real_packet->ping_time[i] = (float)(tmp) / 1000000.0;
        printf(" %u", tmp);
      }
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

#define hash_packet_conn_ping_info_101 hash_const

#define cmp_packet_conn_ping_info_101 cmp_const

BV_DEFINE(packet_conn_ping_info_101_fields, 3);

static struct packet_conn_ping_info *
receive_packet_conn_ping_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_conn_ping_info_101_fields fields;
  struct packet_conn_ping_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_conn_ping_info *clone;
  RECEIVE_PACKET_START(packet_conn_ping_info, real_packet);
  printf("sc op=87 CONN_PING_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_conn_ping_info_101,
                     cmp_packet_conn_ping_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->old_connections = readin;
    printf(" old_connections=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    if(real_packet->connections > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_PLAYERS;
    }
    printf(" old_conn_id[%u]=", real_packet->connections);
    for (i = 0; i < real_packet->connections; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->old_conn_id[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->connections > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_PLAYERS;
    }
    printf("old_ping_time[%u]", real_packet->connections);
    for (i = 0; i < real_packet->connections; i++) {
      int tmp;

      dio_get_uint32(&din, &tmp);
      real_packet->old_ping_time[i] = (float)(tmp) / 1000000.0;
      printf(" %u", tmp);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_conn_ping_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CONN_PING_INFO] != -1) {
    return;
  }

  if((has_capability("conn_ping_info", pconn->capability)
      && has_capability("conn_ping_info", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("conn_ping_info", pconn->capability)
              && has_capability("conn_ping_info", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_CONN_PING_INFO] = variant;
}

struct packet_conn_ping_info *
receive_packet_conn_ping_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_ping_info at the server.");
  }
  ensure_valid_variant_packet_conn_ping_info(pconn);

  switch(pconn->phs.variant[PACKET_CONN_PING_INFO]) {
    case 100: {
      return receive_packet_conn_ping_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_conn_ping_info_101(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 88 sc */
static struct packet_conn_ping *
receive_packet_conn_ping_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_conn_ping, real_packet);
  printf("sc op=88 CONN_PING\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_conn_ping(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CONN_PING] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CONN_PING] = variant;
}

struct packet_conn_ping *
receive_packet_conn_ping(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_conn_ping at the server.");
  }
  ensure_valid_variant_packet_conn_ping(pconn);

  switch(pconn->phs.variant[PACKET_CONN_PING]) {
    case 100: {
      return receive_packet_conn_ping_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 89 cs */
/* 90 sc */
static struct packet_before_new_year *
receive_packet_before_new_year_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_before_new_year, real_packet);
  printf("sc op=90 BEFORE_NEW_YEAR\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_before_new_year(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_BEFORE_NEW_YEAR] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_BEFORE_NEW_YEAR] = variant;
}

struct packet_before_new_year *
receive_packet_before_new_year(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_before_new_year at the server.");
  }
  ensure_valid_variant_packet_before_new_year(pconn);

  switch(pconn->phs.variant[PACKET_BEFORE_NEW_YEAR]) {
    case 100: {
      return receive_packet_before_new_year_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 91 sc */
static struct packet_start_turn *
receive_packet_start_turn_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_start_turn, real_packet);
  printf("sc op=91 START_TURN\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_start_turn(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_START_TURN] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_START_TURN] = variant;
}

struct packet_start_turn *
receive_packet_start_turn(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_start_turn at the server.");
  }
  ensure_valid_variant_packet_start_turn(pconn);

  switch(pconn->phs.variant[PACKET_START_TURN]) {
    case 100: {
      return receive_packet_start_turn_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 92 sc */
#define hash_packet_new_year_100 hash_const

#define cmp_packet_new_year_100 cmp_const

BV_DEFINE(packet_new_year_100_fields, 2);

static struct packet_new_year *
receive_packet_new_year_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_new_year_100_fields fields;
  struct packet_new_year *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_new_year *clone;
  RECEIVE_PACKET_START(packet_new_year, real_packet);
  printf("sc op=92 NEW_YEAR");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_new_year_100,
                     cmp_packet_new_year_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->year = readin;
    printf(" year=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn = readin;
    printf(" turn=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_new_year(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_NEW_YEAR] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_NEW_YEAR] = variant;
}

struct packet_new_year *
receive_packet_new_year(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_new_year at the server.");
  }
  ensure_valid_variant_packet_new_year(pconn);

  switch(pconn->phs.variant[PACKET_NEW_YEAR]) {
    case 100: {
      return receive_packet_new_year_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 93 cs */
/* 94 cs */
/* 95 sc */
static unsigned int hash_packet_spaceship_info_100(const void *vkey, unsigned int num_buckets)
{
  const struct packet_spaceship_info *key = (const struct packet_spaceship_info *) vkey;

  return ((key->player_num) % num_buckets);
}

static int cmp_packet_spaceship_info_100(const void *vkey1, const void *vkey2)
{
  const struct packet_spaceship_info *key1 = (const struct packet_spaceship_info *) vkey1;
  const struct packet_spaceship_info *key2 = (const struct packet_spaceship_info *) vkey2;
  int diff;

  diff = key1->player_num - key2->player_num;
  if (diff != 0) {
    return diff;
  }

  return 0;
}

BV_DEFINE(packet_spaceship_info_100_fields, 17);

static struct packet_spaceship_info *
receive_packet_spaceship_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_spaceship_info_100_fields fields;
  struct packet_spaceship_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_spaceship_info *clone;
  int readin;

  RECEIVE_PACKET_START(packet_spaceship_info, real_packet);
  printf("sc op=95 SPACESHIP_INFO");
  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->player_num = readin;

  if (!*hash) {
    *hash = hash_new(hash_packet_spaceship_info_100,
                     cmp_packet_spaceship_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    int player_num = real_packet->player_num;

    memset(real_packet, 0, sizeof(*real_packet));

    real_packet->player_num = player_num;
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->sship_state = readin;
    printf(" sship_state=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->structurals = readin;
    printf(" structurals=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->components = readin;
    printf(" components=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->modules = readin;
    printf(" modules=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
    printf(" fuel=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->propulsion = readin;
    printf(" propulsion=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->habitation = readin;
    printf(" habitation=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->life_support = readin;
    printf(" life_support=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->solar_panels = readin;
    printf(" solar_panels=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->launch_year = readin;
    printf(" launch_year=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->population = readin;
    printf(" population=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->mass = readin;
    printf(" mass=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int i;

    dio_get_bit_string(&din, real_packet->structure, sizeof(real_packet->structure));
    printf(" structure[%d]=[\n", NUM_SS_STRUCTURALS+1);
    for (i = 0; i < NUM_SS_STRUCTURALS+1; i++) {
      if ( real_packet->structure[i] == 0 ) {
        printf("@");
      } else {
        printf("%X", real_packet->structure[i]);
      }
      if ( i % 100 == 99)
        printf("\n");
      else if ((i % 10) == 9)
       printf(" ");
    }
    printf("] ");
  }
  if (BV_ISSET(fields, 13)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->support_rate = (float)(tmp) / 10000.0;
    printf(" support_rate=%u", tmp);
  }
  if (BV_ISSET(fields, 14)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->energy_rate = (float)(tmp) / 10000.0;
    printf(" energy_rate=%u", tmp);
  }
  if (BV_ISSET(fields, 15)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->success_rate = (float)(tmp) / 10000.0;
    printf(" success_rate=%u", tmp);
  }
  if (BV_ISSET(fields, 16)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->travel_time = (float)(tmp) / 10000.0;
    printf(" travel_time=%u\n", tmp);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_spaceship_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SPACESHIP_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SPACESHIP_INFO] = variant;
}

struct packet_spaceship_info *
receive_packet_spaceship_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_spaceship_info at the server.");
  }
  ensure_valid_variant_packet_spaceship_info(pconn);

  switch(pconn->phs.variant[PACKET_SPACESHIP_INFO]) {
    case 100: {
      return receive_packet_spaceship_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 96 sc */
#define hash_packet_ruleset_unit_100 hash_const

#define cmp_packet_ruleset_unit_100 cmp_const

BV_DEFINE(packet_ruleset_unit_100_fields, 36);

static struct packet_ruleset_unit *
receive_packet_ruleset_unit_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_unit_100_fields fields;
  struct packet_ruleset_unit *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_unit *clone;
  RECEIVE_PACKET_START(packet_ruleset_unit, real_packet);
  printf("sc op=96 RULESET_UNIT");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_unit_100,
                     cmp_packet_ruleset_unit_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->sound_move, sizeof(real_packet->sound_move));
    printf(" sound_move=\"%s\"", real_packet->sound_move);
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->sound_move_alt, sizeof(real_packet->sound_move_alt));
    printf(" sound_move_alt=\"%s\"", real_packet->sound_move_alt);
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->sound_fight, sizeof(real_packet->sound_fight));
    printf(" sound_fight=\"%s\"", real_packet->sound_fight);
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->sound_fight_alt, sizeof(real_packet->sound_fight_alt));
    printf(" sound_fight_alt=\"%s\"", real_packet->sound_fight_alt);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->move_type = readin;
    printf(" move_type=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->build_cost = readin;
    printf(" build_cost=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->pop_cost = readin;
    printf(" pop_cost=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->attack_strength = readin;
    printf(" attack_strength=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defense_strength = readin;
    printf(" defense_strength=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->move_rate = readin;
    printf(" move_rate=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_requirement = readin;
    printf(" tech_requirement=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->impr_requirement = readin;
    printf(" impr_requirement=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->vision_range = readin;
    printf(" vision_range=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->transport_capacity = readin;
    printf(" transport_capacity=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
    printf(" hp=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->firepower = readin;
    printf(" firepower=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->obsoleted_by = readin;
    printf(" obsoleted_by=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
    printf(" fuel=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->happy_cost = readin;
    printf(" happy_cost=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_cost = readin;
    printf(" shield_cost=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_cost = readin;
    printf(" food_cost=%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->gold_cost = readin;
    printf(" gold_cost=%u", readin);
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_range = readin;
    printf(" paratroopers_range=%u", readin);
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_req = readin;
    printf(" paratroopers_mr_req=%u", readin);
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_sub = readin;
    printf(" paratroopers_mr_sub=%u", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int i;

    printf("veteran_name[%u]=", MAX_VET_LEVELS);
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      dio_get_string(&din, real_packet->veteran_name[i], sizeof(real_packet->veteran_name[i]));
      printf(" \"%s\"", real_packet->veteran_name[i]);
    }
  }
  if (BV_ISSET(fields, 30)) {
    int i;

    printf("power_fact[%u]=", MAX_VET_LEVELS);
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int tmp;

      dio_get_uint32(&din, &tmp);
      real_packet->power_fact[i] = (float)(tmp) / 10000.0;
      printf(" %u", tmp);
    }
  }
  if (BV_ISSET(fields, 31)) {
    int i;

    printf("move_bonus[%u]=", MAX_VET_LEVELS);
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->move_bonus[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->bombard_rate = readin;
    printf(" bombard_rate=%u", readin);
  }
  if (BV_ISSET(fields, 33)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
    printf(" team=\"%s\"", real_packet->helptext);
  }
  if (BV_ISSET(fields, 34)) {
    int i;

    DIO_BV_GET(&din, real_packet->flags);
    printf(" flags=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->flags.vec[i]);
    }
  }
  if (BV_ISSET(fields, 35)) {
    int i;

    DIO_BV_GET(&din, real_packet->roles);
    printf(" roles=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->roles.vec[i]);
    }
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_unit(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_UNIT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_UNIT] = variant;
}

struct packet_ruleset_unit *
receive_packet_ruleset_unit(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_unit at the server.");
  }
  ensure_valid_variant_packet_ruleset_unit(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_UNIT]) {
    case 100: {
      return receive_packet_ruleset_unit_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 97 sc */
#define hash_packet_ruleset_game_100 hash_const

#define cmp_packet_ruleset_game_100 cmp_const

BV_DEFINE(packet_ruleset_game_100_fields, 26);

static struct packet_ruleset_game *
receive_packet_ruleset_game_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_game_100_fields fields;
  struct packet_ruleset_game *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_game *clone;
  RECEIVE_PACKET_START(packet_ruleset_game, real_packet);
  printf("sc op=97 RULESET_GAME");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_game_100,
                     cmp_packet_ruleset_game_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int i;

    printf("specialist_name[%u]=", SP_COUNT);
    for (i = 0; i < SP_COUNT; i++) {
      dio_get_string(&din, real_packet->specialist_name[i],
                     sizeof(real_packet->specialist_name[i]));
      printf(" \"%s\"", real_packet->specialist_name[i]);
    }
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    printf("specialist_min_size[%u]=", SP_COUNT);
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialist_min_size[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    printf("specialist_bonus[%u]=", SP_COUNT);
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialist_bonus[i] = readin;
      printf(" %u", readin);
    }
  }
  real_packet->changable_tax = BV_ISSET(fields, 3);
  printf(" changable_tax=%u", real_packet->changable_tax);

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_science = readin;
    printf(" forced_science=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_luxury = readin;
    printf(" forced_luxury=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_gold = readin;
    printf(" forced_gold=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_food = readin;
    printf(" min_city_center_food=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_shield = readin;
    printf(" min_city_center_shield=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_trade = readin;
    printf(" min_city_center_trade=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_dist_bw_cities = readin;
    printf(" min_dist_bw_cities=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->init_vis_radius_sq = readin;
    printf(" init_vis_radius_sq=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hut_overflight = readin;
    printf(" hut_overflight=%u", readin);
  }
  real_packet->pillage_select = BV_ISSET(fields, 13);
  printf(" pillage_select=%u", real_packet->pillage_select);

  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nuke_contamination = readin;
    printf(" nuke_contamination=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int i;

    printf("granary_food_ini[%u]=", MAX_GRANARY_INIS);
    for (i = 0; i < MAX_GRANARY_INIS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->granary_food_ini[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->granary_num_inis = readin;
    printf(" granary_num_inis=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->granary_food_inc = readin;
    printf(" granary_food_inc=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_cost_style = readin;
    printf(" tech_cost_style=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tech_cost_double_year = readin;
    printf(" tech_cost_double_year=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_leakage = readin;
    printf(" tech_leakage=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int i;

    dio_get_tech_list(&din, real_packet->global_init_techs);
    printf("global_init_techs[]=");
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (real_packet->global_init_techs[i] == A_LAST) {
        break;
      } else {
        printf(" %u", real_packet->global_init_techs[i]);
      }
    }
  }
  real_packet->killstack = BV_ISSET(fields, 22);
  printf(" killstack=%u", real_packet->killstack);

  if (BV_ISSET(fields, 23)) {
    int i;

    printf("trireme_loss_chance[i]=");
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trireme_loss_chance[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 24)) {
    int i;

    printf("work_veteran_chance[i]=");
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->work_veteran_chance[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 25)) {
    int i;

    printf("veteran_chance[i]=");
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->veteran_chance[i] = readin;
      printf(" %u", readin);
    }
    printf("\n");
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void
ensure_valid_variant_packet_ruleset_game(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_GAME] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_GAME] = variant;
}

struct packet_ruleset_game *
receive_packet_ruleset_game(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_game at the server.");
  }
  ensure_valid_variant_packet_ruleset_game(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_GAME]) {
    case 100: {
      return receive_packet_ruleset_game_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 98 sc */
#define hash_packet_ruleset_government_ruler_title_100 hash_const

#define cmp_packet_ruleset_government_ruler_title_100 cmp_const

BV_DEFINE(packet_ruleset_government_ruler_title_100_fields, 5);

static struct packet_ruleset_government_ruler_title *
receive_packet_ruleset_government_ruler_title_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_government_ruler_title_100_fields fields;
  struct packet_ruleset_government_ruler_title *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_government_ruler_title *clone;
  RECEIVE_PACKET_START(packet_ruleset_government_ruler_title, real_packet);
  printf("sc op=98 RULESET_GOVERNMENT_RULER_TITLE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_government_ruler_title_100,
                     cmp_packet_ruleset_government_ruler_title_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->gov = readin;
    printf(" gov=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
    printf(" nation=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->male_title, sizeof(real_packet->male_title));
    printf(" male_title=\"%s\"", real_packet->male_title);
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->female_title, sizeof(real_packet->female_title));
    printf(" female_title=\"%s\"\n", real_packet->female_title);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_government_ruler_title(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE] = variant;
}

struct packet_ruleset_government_ruler_title *
receive_packet_ruleset_government_ruler_title(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_government_ruler_title at the server.");
  }
  ensure_valid_variant_packet_ruleset_government_ruler_title(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_GOVERNMENT_RULER_TITLE]) {
    case 100: {
      return receive_packet_ruleset_government_ruler_title_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 99 sc */
#define hash_packet_ruleset_tech_100 hash_const

#define cmp_packet_ruleset_tech_100 cmp_const

BV_DEFINE(packet_ruleset_tech_100_fields, 10);

static struct packet_ruleset_tech *
receive_packet_ruleset_tech_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_tech_100_fields fields;
  struct packet_ruleset_tech *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_tech *clone;
  RECEIVE_PACKET_START(packet_ruleset_tech, real_packet);
  printf("sc op=99 RULESET_TECH");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_tech_100,
                     cmp_packet_ruleset_tech_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    printf("req[2]=");
    for (i = 0; i < 2; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->req[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->root_req = readin;
    printf(" root_req=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
    printf(" flags=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->preset_cost = readin;
    printf(" preset_cost=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->num_reqs = readin;
    printf(" num_reqs=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
    printf(" helptext=\"%s\"", real_packet->helptext);
  }
  if (BV_ISSET(fields, 8)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"\n", real_packet->graphic_alt);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_tech(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_TECH] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_TECH] = variant;
}

struct packet_ruleset_tech *
receive_packet_ruleset_tech(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_tech at the server.");
  }
  ensure_valid_variant_packet_ruleset_tech(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_TECH]) {
    case 100: {
      return receive_packet_ruleset_tech_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 100 sc */
#define hash_packet_ruleset_government_100 hash_const

#define cmp_packet_ruleset_government_100 cmp_const

BV_DEFINE(packet_ruleset_government_100_fields, 45);

static struct packet_ruleset_government *
receive_packet_ruleset_government_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_government_100_fields fields;
  struct packet_ruleset_government *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_government *clone;
  RECEIVE_PACKET_START(packet_ruleset_government, real_packet);
  printf("sc op=100 RULESET_GOVERNMENT");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_government_100,
                     cmp_packet_ruleset_government_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->required_tech = readin;
    printf(" required_tech=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->max_rate = readin;
    printf(" max_rate=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->civil_war = readin;
    printf(" civil_war=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->martial_law_max = readin;
    printf(" martial_law_max=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->martial_law_per = readin;
    printf(" martial_law_per=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->empire_size_mod = readin;
    printf(" empire_size_mod=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->empire_size_inc = readin;
    printf(" empire_size_inc=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rapture_size = readin;
    printf(" rapture_size=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_happy_cost_factor = readin;
    printf(" unit_happy_cost_factor=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_shield_cost_factor = readin;
    printf(" unit_shield_cost_factor=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_food_cost_factor = readin;
    printf(" unit_food_cost_factor=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_gold_cost_factor = readin;
    printf(" unit_gold_cost_factor=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_happy = readin;
    printf(" free_happy=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_shield = readin;
    printf(" free_shield=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_food = readin;
    printf(" free_food=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_gold = readin;
    printf(" free_gold=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_before_penalty = readin;
    printf(" trade_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shields_before_penalty = readin;
    printf(" shields_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_before_penalty = readin;
    printf(" food_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_trade_before_penalty = readin;
    printf(" celeb_trade_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_shields_before_penalty = readin;
    printf(" celeb_shields_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_food_before_penalty = readin;
    printf(" celeb_food_before_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_bonus = readin;
    printf(" trade_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_bonus = readin;
    printf(" shield_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_bonus = readin;
    printf(" food_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_trade_bonus = readin;
    printf(" celeb_trade_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_shield_bonus = readin;
    printf(" celeb_shield_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_food_bonus = readin;
    printf(" celeb_food_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption_level = readin;
    printf(" corruption_level=%u", readin);
  }
  if (BV_ISSET(fields, 30)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fixed_corruption_distance = readin;
    printf(" fixed_corruption_distance=%u", readin);
  }
  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->corruption_distance_factor = readin;
    printf(" corruption_distance_factor=%u", readin);
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->extra_corruption_distance = readin;
    printf(" extra_corruption_distance=%u", readin);
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->corruption_max_distance_cap = readin;
    printf(" corruption_max_distance_cap=%u", readin);
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->waste_level = readin;
    printf(" waste_level=%u", readin);
  }
  if (BV_ISSET(fields, 35)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fixed_waste_distance = readin;
    printf(" fixed_waste_distance=%u", readin);
  }
  if (BV_ISSET(fields, 36)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->waste_distance_factor = readin;
    printf(" waste_distance_factor=%u", readin);
  }
  if (BV_ISSET(fields, 37)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->extra_waste_distance = readin;
    printf(" extra_waste_distance=%u", readin);
  }
  if (BV_ISSET(fields, 38)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->waste_max_distance_cap = readin;
    printf(" waste_max_distance_cap=%u", readin);
  }
  if (BV_ISSET(fields, 39)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->flags = readin;
    printf(" flags=%u", readin);
  }
  if (BV_ISSET(fields, 40)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_ruler_titles = readin;
    printf(" num_ruler_titles=%u", readin);
  }
  if (BV_ISSET(fields, 41)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 42)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 43)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 44)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
    printf(" helptext=\"%s\"\n", real_packet->helptext);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_government(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_GOVERNMENT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_GOVERNMENT] = variant;
}

struct packet_ruleset_government *
receive_packet_ruleset_government(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_government at the server.");
  }
  ensure_valid_variant_packet_ruleset_government(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_GOVERNMENT]) {
    case 100: {
      return receive_packet_ruleset_government_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 101 sc */
#define hash_packet_ruleset_terrain_control_100 hash_const

#define cmp_packet_ruleset_terrain_control_100 cmp_const

BV_DEFINE(packet_ruleset_terrain_control_100_fields, 22);

static struct packet_ruleset_terrain_control *
receive_packet_ruleset_terrain_control_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_terrain_control_100_fields fields;
  struct packet_ruleset_terrain_control *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_terrain_control *clone;
  RECEIVE_PACKET_START(packet_ruleset_terrain_control, real_packet);
  printf("sc op=101 RULESET_TERRAIN_CONTROL");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_terrain_control_100,
                     cmp_packet_ruleset_terrain_control_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  real_packet->may_road = BV_ISSET(fields, 0);
  printf(" may_road=%u", real_packet->may_road);

  real_packet->may_irrigate = BV_ISSET(fields, 1);
  printf(" may_irrigate=%u", real_packet->may_irrigate);

  real_packet->may_mine = BV_ISSET(fields, 2);
  printf(" may_mine=%u", real_packet->may_mine);

  real_packet->may_transform = BV_ISSET(fields, 3);
  printf(" may_transform=%u", real_packet->may_transform);

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->ocean_reclaim_requirement_pct = readin;
    printf(" ocean_reclaim_requirement_pct=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->land_channel_requirement_pct = readin;
    printf(" land_channel_requirement_pct=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->river_move_mode = readin;
    printf(" river_move_mode=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->river_defense_bonus = readin;
    printf(" river_defense_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->river_trade_incr = readin;
    printf(" river_trade_incr=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->river_help_text, sizeof(real_packet->river_help_text));
    printf(" river_help_text=\"%s\"", real_packet->river_help_text);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fortress_defense_bonus = readin;
    printf(" fortress_defense_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->road_superhighway_trade_bonus = readin;
    printf(" road_superhighway_trade_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_food_bonus = readin;
    printf(" rail_food_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_shield_bonus = readin;
    printf(" rail_shield_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_trade_bonus = readin;
    printf(" rail_trade_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->farmland_supermarket_food_bonus = readin;
    printf(" farmland_supermarket_food_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_food_penalty = readin;
    printf(" pollution_food_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_shield_penalty = readin;
    printf(" pollution_shield_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_trade_penalty = readin;
    printf(" pollution_trade_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_food_penalty = readin;
    printf(" fallout_food_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_shield_penalty = readin;
    printf(" fallout_shield_penalty=%u", readin);
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_trade_penalty = readin;
    printf(" fallout_trade_penalty=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_terrain_control(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_TERRAIN_CONTROL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_TERRAIN_CONTROL] = variant;
}

struct packet_ruleset_terrain_control *
receive_packet_ruleset_terrain_control(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_terrain_control at the server.");
  }
  ensure_valid_variant_packet_ruleset_terrain_control(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_TERRAIN_CONTROL]) {
    case 100: {
      return receive_packet_ruleset_terrain_control_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 102 sc */
#define hash_packet_ruleset_nation_100 hash_const

#define cmp_packet_ruleset_nation_100 cmp_const

BV_DEFINE(packet_ruleset_nation_100_fields, 12);

static struct packet_ruleset_nation *
receive_packet_ruleset_nation_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_nation_100_fields fields;
  struct packet_ruleset_nation *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_nation *clone;
  RECEIVE_PACKET_START(packet_ruleset_nation, real_packet);
  printf("sc op=102 RULESET_NATION");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_nation_100,
                     cmp_packet_ruleset_nation_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->name_plural, sizeof(real_packet->name_plural));
    printf(" name_plural=\"%s\"", real_packet->name_plural);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->class_, sizeof(real_packet->class_));
    printf(" class_=\"%s\"", real_packet->class_);
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->legend, sizeof(real_packet->legend));
    printf(" legend=\"%s\"", real_packet->legend);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
    printf(" city_style=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    dio_get_tech_list(&din, real_packet->init_techs);
    printf(" init_techs=");
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (real_packet->init_techs[i] == A_LAST) {
        break;
      } else {
        printf(" %u", real_packet->init_techs[i]);
      }
    }
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->leader_count = readin;
    printf(" leader_count=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int i;

    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
    printf(" leader_name[i]=");
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_string(&din, real_packet->leader_name[i], sizeof(real_packet->leader_name[i]));
      printf(" \"%s\"", real_packet->leader_name[i]);
    }
  }
  if (BV_ISSET(fields, 11)) {
    int i;

    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
    printf("leader_sex[i]=");
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_bool8(&din, &real_packet->leader_sex[i]);
      printf(" %u", real_packet->leader_sex[i]);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_nation(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_NATION] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_NATION] = variant;
}

struct packet_ruleset_nation *
receive_packet_ruleset_nation(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_nation at the server.");
  }
  ensure_valid_variant_packet_ruleset_nation(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_NATION]) {
    case 100: {
      return receive_packet_ruleset_nation_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 103 sc */
#define hash_packet_ruleset_city_100 hash_const

#define cmp_packet_ruleset_city_100 cmp_const

BV_DEFINE(packet_ruleset_city_100_fields, 8);

static struct packet_ruleset_city *
receive_packet_ruleset_city_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_city_100_fields fields;
  struct packet_ruleset_city *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_city *clone;
  RECEIVE_PACKET_START(packet_ruleset_city, real_packet);
  printf("sc op=103 RULESET_CITY");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_city_100,
                     cmp_packet_ruleset_city_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->style_id = readin;
    printf(" style_id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techreq = readin;
    printf(" techreq=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->citizens_graphic,
                   sizeof(real_packet->citizens_graphic));
    printf(" citizens_graphic=\"%s\"", real_packet->citizens_graphic);
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->citizens_graphic_alt,
                   sizeof(real_packet->citizens_graphic_alt));
    printf(" citizens_graphic_alt=\"%s\"", real_packet->citizens_graphic_alt);
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->graphic, sizeof(real_packet->graphic));
    printf(" graphic=\"%s\"", real_packet->graphic);
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->replaced_by = readin;
    printf(" replaced_by=%u", readin);
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_city(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_CITY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_CITY] = variant;
}

struct packet_ruleset_city *
receive_packet_ruleset_city(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_city at the server.");
  }
  ensure_valid_variant_packet_ruleset_city(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_CITY]) {
    case 100: {
      return receive_packet_ruleset_city_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 104 sc */
#define hash_packet_ruleset_building_100 hash_const

#define cmp_packet_ruleset_building_100 cmp_const

BV_DEFINE(packet_ruleset_building_100_fields, 24);

static struct packet_ruleset_building *
receive_packet_ruleset_building_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_building_100_fields fields;
  struct packet_ruleset_building *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_building *clone;
  RECEIVE_PACKET_START(packet_ruleset_building, real_packet);
  printf("sc op=104 RULESET_BUILDING");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_building_100,
                     cmp_packet_ruleset_building_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name,
                   sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->graphic_str,
                   sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_req = readin;
    printf(" tech_req=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->obsolete_by = readin;
    printf(" obsolete_by=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->bldg_req = readin;
    printf(" bldg_req=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->replaced_by = readin;
    printf(" replaced_by=%u", readin);
  }
  real_packet->is_wonder = BV_ISSET(fields, 8);
  printf(" is_wonder=%u", real_packet->is_wonder);

  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->equiv_range = readin;
    printf(" equiv_range=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->build_cost = readin;
    printf(" build_cost=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
    printf(" upkeep=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->sabotage = readin;
    printf(" sabotage=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    dio_get_string(&din, real_packet->soundtag,
                   sizeof(real_packet->soundtag));
    printf(" soundtag=\"%s\"", real_packet->soundtag);
  }
  if (BV_ISSET(fields, 14)) {
    dio_get_string(&din, real_packet->soundtag_alt,
                   sizeof(real_packet->soundtag_alt));
    printf(" soundtag_alt=\"%s\"", real_packet->soundtag_alt);
  }
  if (BV_ISSET(fields, 15)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
    printf(" helptext=\"%s\"", real_packet->helptext);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->terr_gate_count = readin;
    printf(" terr_gate_count=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int i;

    if(real_packet->terr_gate_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->terr_gate_count = 255;
    }
    printf(" terr_gate[i]=");
    for (i = 0; i < real_packet->terr_gate_count; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->terr_gate[i] = readin;
      printf(" %d", readin);
    }
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->spec_gate_count = readin;
    printf(" spec_gate_count=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int i;

    if(real_packet->spec_gate_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->spec_gate_count = 255;
    }
    printf(" spec_gate[i]=");
    for (i = 0; i < real_packet->spec_gate_count; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->spec_gate[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->equiv_dupl_count = readin;
  }
  if (BV_ISSET(fields, 21)) {
    int i;

    if(real_packet->equiv_dupl_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->equiv_dupl_count = 255;
    }
    printf(" equiv_dupl[i]=");
    for (i = 0; i < real_packet->equiv_dupl_count; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->equiv_dupl[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->equiv_repl_count = readin;
    printf(" equiv_repl_count=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int i;

    if(real_packet->equiv_repl_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->equiv_repl_count = 255;
    }
    printf(" equiv_repl[i]=");
    for (i = 0; i < real_packet->equiv_repl_count; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->equiv_repl[i] = readin;
      printf(" %u", readin);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_building(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_BUILDING] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_BUILDING] = variant;
}

struct packet_ruleset_building *
receive_packet_ruleset_building(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_building at the server.");
  }
  ensure_valid_variant_packet_ruleset_building(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_BUILDING]) {
    case 100: {
      return receive_packet_ruleset_building_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 105 sc */
#define hash_packet_ruleset_terrain_100 hash_const

#define cmp_packet_ruleset_terrain_100 cmp_const

BV_DEFINE(packet_ruleset_terrain_100_fields, 38);

static struct packet_ruleset_terrain *
receive_packet_ruleset_terrain_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_terrain_100_fields fields;
  struct packet_ruleset_terrain *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_terrain *clone;
  RECEIVE_PACKET_START(packet_ruleset_terrain, real_packet);
  printf("sc op=105 RULESET_TERRAIN");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_terrain_100,
                     cmp_packet_ruleset_terrain_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    DIO_BV_GET(&din, real_packet->tags);
    printf(" tags=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->tags.vec[i]);
    }
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->terrain_name, sizeof(real_packet->terrain_name));
    printf(" name=\"%s\"", real_packet->terrain_name);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_str,
                   sizeof(real_packet->graphic_str));
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movement_cost = readin;
    printf(" movement_cost=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defense_bonus = readin;
    printf(" defense_bonus=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food = readin;
    printf(" food=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield = readin;
    printf(" shield=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade = readin;
    printf(" trade=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    dio_get_string(&din, real_packet->special_1_name,
                   sizeof(real_packet->special_1_name));
    printf(" special_1_name=\"%s\"", real_packet->special_1_name);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_special_1 = readin;
    printf(" food_special_1=%u", readin);
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_special_1 = readin;
    printf(" shield_special_1=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_special_1 = readin;
    printf(" trade_special_1=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    dio_get_string(&din, real_packet->graphic_str_special_1,
                   sizeof(real_packet->graphic_str_special_1));
    printf(" graphic_str_special_1=\"%s\"", real_packet->graphic_str_special_1);
  }
  if (BV_ISSET(fields, 15)) {
    dio_get_string(&din, real_packet->graphic_alt_special_1, sizeof(real_packet->graphic_alt_special_1));
    printf(" graphic_alt_special_1=\"%s\"", real_packet->graphic_alt_special_1);
  }
  if (BV_ISSET(fields, 16)) {
    dio_get_string(&din, real_packet->special_2_name, sizeof(real_packet->special_2_name));
    printf(" special_2_name=\"%s\"", real_packet->special_2_name);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_special_2 = readin;
    printf(" food_special_2=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_special_2 = readin;
    printf(" shield_special_2=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_special_2 = readin;
    printf(" trade_special_2=%u", readin);
  }
  if (BV_ISSET(fields, 20)) {
    dio_get_string(&din, real_packet->graphic_str_special_2,
                   sizeof(real_packet->graphic_str_special_2));
    printf(" graphic_str_special_2=\"%s\"", real_packet->graphic_str_special_2);
  }
  if (BV_ISSET(fields, 21)) {
    dio_get_string(&din, real_packet->graphic_alt_special_2,
                   sizeof(real_packet->graphic_alt_special_2));
    printf(" graphic_alt_special_2=\"%s\"", real_packet->graphic_alt_special_2);
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->road_trade_incr = readin;
    printf(" road_trade_incr=%u", readin);
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->road_time = readin;
    printf(" road_time=%u", readin);
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->irrigation_result = readin;
    printf(" irrigation_result=%u", readin);
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->irrigation_food_incr = readin;
    printf(" irrigation_food_incr=%u", readin);
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->irrigation_time = readin;
    printf(" irrigation_time=%u", readin);
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->mining_result = readin;
    printf(" mining_result=%u", readin);
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->mining_shield_incr = readin;
    printf(" mining_shield_incr=%u", readin);
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->mining_time = readin;
    printf(" mining_time=%u", readin);
  }
  if (BV_ISSET(fields, 30)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->transform_result = readin;
    printf(" transform_result=%u", readin);
  }
  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->transform_time = readin;
    printf(" transform_time=%u", readin);
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rail_time = readin;
    printf(" rail_time=%u", readin);
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->airbase_time = readin;
    printf(" airbase_time=%u", readin);
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fortress_time = readin;
    printf(" fortress_time=%u", readin);
  }
  if (BV_ISSET(fields, 35)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->clean_pollution_time = readin;
    printf(" clean_pollution_time=%u", readin);
  }
  if (BV_ISSET(fields, 36)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->clean_fallout_time = readin;
    printf(" clean_fallout_time=%u", readin);
  }
  if (BV_ISSET(fields, 37)) {
    dio_get_string(&din, real_packet->helptext,
                   sizeof(real_packet->helptext));
    printf(" helptext=\"%s\"", real_packet->helptext);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_terrain(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_TERRAIN] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_TERRAIN] = variant;
}

struct packet_ruleset_terrain *
receive_packet_ruleset_terrain(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_terrain at the server.");
  }
  ensure_valid_variant_packet_ruleset_terrain(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_TERRAIN]) {
    case 100: {
      return receive_packet_ruleset_terrain_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 106 sc */
#define hash_packet_ruleset_control_100 hash_const

#define cmp_packet_ruleset_control_100 cmp_const

BV_DEFINE(packet_ruleset_control_100_fields, 24);

static struct packet_ruleset_control *
receive_packet_ruleset_control_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_control_100_fields fields;
  struct packet_ruleset_control *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_control *clone;
  RECEIVE_PACKET_START(packet_ruleset_control, real_packet);
  printf("sc op=106 RULESET_CONTROL");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_control_100,
                     cmp_packet_ruleset_control_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->aqueduct_size = readin;
    printf(" aqueduct_size=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->add_to_size_limit = readin;
    printf(" add_to_size_limit=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->notradesize = readin;
    printf(" notradesize=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fulltradesize = readin;
    printf(" fulltradesize=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_unit_types = readin;
    printf(" num_unit_types=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_impr_types = readin;
    printf(" num_impr_types=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_tech_types = readin;
    printf(" num_tech_types=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_cathedral_plus = readin;
    printf(" rtech_cathedral_plus=%u", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_cathedral_minus = readin;
    printf(" rtech_cathedral_minus=%u", readin);
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_colosseum_plus = readin;
    printf(" rtech_colosseum_plus=%u", readin);
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_temple_plus = readin;
    printf(" rtech_temple_plus=%u", readin);
  }
  if (BV_ISSET(fields, 11)) {
    int i;

    dio_get_tech_list(&din, real_packet->rtech_partisan_req);
    printf("rtech_partisan_req[%d]={", MAX_NUM_TECH_LIST);
    for ( i = 0; i < MAX_NUM_TECH_LIST; i++) {
      printf(" %u", real_packet->rtech_partisan_req[i]);
    }
    printf("}");
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government_when_anarchy = readin;
    printf(" government_when_anarchy=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->default_government = readin;
    printf(" default_government=%u", readin);
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government_count = readin;
    printf(" government_count=%u", readin);
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nation_count = readin;
    printf(" nation_count=%u", readin);
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->playable_nation_count = readin;
    printf(" playable_nation_count=%u", readin);
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->style_count = readin;
    printf(" style_count=%u", readin);
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->terrain_count = readin;
    printf(" terrain_count=%u", readin);
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->borders = readin;
    printf(" borders=%u", readin);
  }
  real_packet->happyborders = BV_ISSET(fields, 20);
  printf(" happyborders=%u", real_packet->happyborders);

  real_packet->slow_invasions = BV_ISSET(fields, 21);
  printf(" slow_invasions=%u", real_packet->slow_invasions);

  if (BV_ISSET(fields, 22)) {
    int i;

    for (i = 0; i < MAX_NUM_TEAMS; i++) {
      dio_get_string(&din, real_packet->team_name[i], sizeof(real_packet->team_name[i]));
      printf(" team_name[i]=\"%s\"", (char*)&real_packet->team_name[0]);
    }
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->default_building = readin;
    printf(" default_building=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_control(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_CONTROL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_CONTROL] = variant;
}

struct packet_ruleset_control *
receive_packet_ruleset_control(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_control at the server.");
  }
  ensure_valid_variant_packet_ruleset_control(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_CONTROL]) {
    case 100: {
      return receive_packet_ruleset_control_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 107 cs */
/* 108 cs */
/* 109 sc */
#define hash_packet_single_want_hack_reply_100 hash_const

#define cmp_packet_single_want_hack_reply_100 cmp_const

BV_DEFINE(packet_single_want_hack_reply_100_fields, 1);

static struct packet_single_want_hack_reply *
receive_packet_single_want_hack_reply_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_single_want_hack_reply_100_fields fields;
  struct packet_single_want_hack_reply *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_single_want_hack_reply *clone;
  RECEIVE_PACKET_START(packet_single_want_hack_reply, real_packet);
  printf("sc op=109 SINGLE_WANT_HACK_REPLY");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_single_want_hack_reply_100,
                     cmp_packet_single_want_hack_reply_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  real_packet->you_have_hack = BV_ISSET(fields, 0);
  printf(" you_have_hack=%u\n", real_packet->you_have_hack);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_single_want_hack_reply(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY] = variant;
}

struct packet_single_want_hack_reply *
receive_packet_single_want_hack_reply(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_single_want_hack_reply at the server.");
  }
  ensure_valid_variant_packet_single_want_hack_reply(pconn);

  switch(pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REPLY]) {
    case 100: {
      return receive_packet_single_want_hack_reply_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 110 */
/* 111 sc */
#define hash_packet_game_load_100 hash_const

#define cmp_packet_game_load_100 cmp_const

BV_DEFINE(packet_game_load_100_fields, 9);

static struct packet_game_load *
receive_packet_game_load_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_game_load_100_fields fields;
  struct packet_game_load *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_game_load *clone;
  RECEIVE_PACKET_START(packet_game_load, real_packet);
  printf("sc op=111 GAME_LOAD");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_game_load_100,
                     cmp_packet_game_load_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  real_packet->load_successful = BV_ISSET(fields, 0);
  printf(" load_successful=%u", real_packet->load_successful);

  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nplayers = readin;
    printf(" nplayers=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->load_filename, sizeof(real_packet->load_filename));
    printf(" load_filename=\"%s\"", real_packet->load_filename);
  }
  if (BV_ISSET(fields, 3)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" name[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->name[i], sizeof(real_packet->name[i]));
      printf(" \"%s\"", real_packet->name[i]);
    }
  }
  if (BV_ISSET(fields, 4)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" username[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->username[i], sizeof(real_packet->username[i]));
      printf(" \"%s\"", real_packet->username[i]);
    }
  }
  if (BV_ISSET(fields, 5)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" nation_name[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->nation_name[i], sizeof(real_packet->nation_name[i]));
      printf(" \"%s\"", real_packet->nation_name[i]);
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" nation_flag[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->nation_flag[i], sizeof(real_packet->nation_flag[i]));
      printf(" \"%s\"", real_packet->nation_flag[i]);
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" is_alive[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_alive[i]);
      printf(" %u", real_packet->is_alive[i]);
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
    printf(" is_ai[i]=");
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_ai[i]);
      printf(" %u", real_packet->is_ai[i]);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_game_load(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_GAME_LOAD] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_GAME_LOAD] = variant;
}

struct packet_game_load *
receive_packet_game_load(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_game_load at the server.");
  }
  ensure_valid_variant_packet_game_load(pconn);

  switch(pconn->phs.variant[PACKET_GAME_LOAD]) {
    case 100: {
      return receive_packet_game_load_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 112 sc */
#define hash_packet_options_settable_control_100 hash_const

#define cmp_packet_options_settable_control_100 cmp_const

BV_DEFINE(packet_options_settable_control_100_fields, 3);

static struct packet_options_settable_control *
receive_packet_options_settable_control_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_options_settable_control_100_fields fields;
  struct packet_options_settable_control *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_options_settable_control *clone;
  RECEIVE_PACKET_START(packet_options_settable_control, real_packet);
  printf("sc op=112 OPTIONS_SETTABLE_CONTROL");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_options_settable_control_100,
                     cmp_packet_options_settable_control_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nids = readin;
    printf(" nids=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->ncategories = readin;
    printf(" ncategories=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->ncategories > 256) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->ncategories = 256;
    }
    printf(" category_names=");
    for (i = 0; i < real_packet->ncategories; i++) {
      dio_get_string(&din, real_packet->category_names[i],
                     sizeof(real_packet->category_names[i]));
      printf(" \"%s\"", real_packet->category_names[i]);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_options_settable_control(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL] = variant;
}

struct packet_options_settable_control *
receive_packet_options_settable_control(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_options_settable_control at the server.");
  }
  ensure_valid_variant_packet_options_settable_control(pconn);

  switch(pconn->phs.variant[PACKET_OPTIONS_SETTABLE_CONTROL]) {
    case 100: {
      return receive_packet_options_settable_control_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 113 sc */
#define hash_packet_options_settable_100 hash_const

#define cmp_packet_options_settable_100 cmp_const

BV_DEFINE(packet_options_settable_100_fields, 12);

static struct packet_options_settable *
receive_packet_options_settable_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_options_settable_100_fields fields;
  struct packet_options_settable *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_options_settable *clone;
  RECEIVE_PACKET_START(packet_options_settable, real_packet);
  printf("sc op=113 OPTIONS_SETTABLE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_options_settable_100,
                     cmp_packet_options_settable_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->short_help, sizeof(real_packet->short_help));
    printf(" short_help=\"%s\"", real_packet->short_help);
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->extra_help, sizeof(real_packet->extra_help));
    printf(" extra_help=\"%s\"", real_packet->extra_help);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
    printf(" type=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->val = readin;
    printf(" val=%d", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->default_val = readin;
    printf(" default_val=%d", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->min = readin;
    printf(" min=%d", readin);
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->max = readin;
    printf(" max=%d", readin);
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->strval, sizeof(real_packet->strval));
    printf(" strval=\"%s\"", real_packet->strval);
  }
  if (BV_ISSET(fields, 10)) {
    dio_get_string(&din, real_packet->default_strval, sizeof(real_packet->default_strval));
    printf(" default_strval=\"%s\"", real_packet->default_strval);
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->category = readin;
    printf(" category=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_options_settable(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_OPTIONS_SETTABLE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_OPTIONS_SETTABLE] = variant;
}

struct packet_options_settable *
receive_packet_options_settable(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_options_settable at the server.");
  }
  ensure_valid_variant_packet_options_settable(pconn);

  switch(pconn->phs.variant[PACKET_OPTIONS_SETTABLE]) {
    case 100: {
      return receive_packet_options_settable_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 114 sc */
static struct packet_select_races *
receive_packet_select_races_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_select_races, real_packet);
  printf("sc op=114 SELECT_RACES\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_select_races(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SELECT_RACES] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SELECT_RACES] = variant;
}

struct packet_select_races *
receive_packet_select_races(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_select_races at the server.");
  }
  ensure_valid_variant_packet_select_races(pconn);

  switch(pconn->phs.variant[PACKET_SELECT_RACES]) {
    case 100: {
      return receive_packet_select_races_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 115 */
/* 116 */
/* 117 */
/* 118 */
/* 119 */
/* 120 sc */
#define hash_packet_ruleset_cache_group_100 hash_const

#define cmp_packet_ruleset_cache_group_100 cmp_const

BV_DEFINE(packet_ruleset_cache_group_100_fields, 5);

static struct packet_ruleset_cache_group *
receive_packet_ruleset_cache_group_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_cache_group_100_fields fields;
  struct packet_ruleset_cache_group *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_cache_group *clone;
  RECEIVE_PACKET_START(packet_ruleset_cache_group, real_packet);
  printf("sc op=120 RULESET_CACHE_GROUP");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_cache_group_100,
                     cmp_packet_ruleset_cache_group_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
    printf(" name=\"%s\"", real_packet->name);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_elements = readin;
    printf(" num_elements=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
    printf(" source_buildings[i]=");
    for (i = 0; i < real_packet->num_elements; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->source_buildings[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 3)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
    printf(" ranges[i]=");
    for (i = 0; i < real_packet->num_elements; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->ranges[i] = readin;
      printf(" %u", readin);
    }
  }
  if (BV_ISSET(fields, 4)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
    printf(" survives[i]=");
    for (i = 0; i < real_packet->num_elements; i++) {
      dio_get_bool8(&din, &real_packet->survives[i]);
      printf(" %u", real_packet->survives[i]);
    }
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_cache_group(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_CACHE_GROUP] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_CACHE_GROUP] = variant;
}

struct packet_ruleset_cache_group *
receive_packet_ruleset_cache_group(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_cache_group at the server.");
  }
  ensure_valid_variant_packet_ruleset_cache_group(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_CACHE_GROUP]) {
    case 100: {
      return receive_packet_ruleset_cache_group_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 121 sc */
#define hash_packet_ruleset_cache_effect_100 hash_const

#define cmp_packet_ruleset_cache_effect_100 cmp_const

BV_DEFINE(packet_ruleset_cache_effect_100_fields, 8);

static struct packet_ruleset_cache_effect *
receive_packet_ruleset_cache_effect_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_ruleset_cache_effect_100_fields fields;
  struct packet_ruleset_cache_effect *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_ruleset_cache_effect *clone;
  RECEIVE_PACKET_START(packet_ruleset_cache_effect, real_packet);
  printf("sc op=121 RULESET_CACHE_EFFECT");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_ruleset_cache_effect_100,
                     cmp_packet_ruleset_cache_effect_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
    printf(" id=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->effect_type = readin;
    printf(" effect_type=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->range = readin;
    printf(" range=%u", readin);
  }
  real_packet->survives = BV_ISSET(fields, 3);
  printf(" survives=%u", real_packet->survives);

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->eff_value = readin;
    printf(" eff_value=%u", readin);
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->req_type = readin;
    printf(" req_type=%u", readin);
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->req_value = readin;
    printf(" req_value=%u", readin);
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->group_id = readin;
    printf(" group_id=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_ruleset_cache_effect(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_RULESET_CACHE_EFFECT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_RULESET_CACHE_EFFECT] = variant;
}

struct packet_ruleset_cache_effect *
receive_packet_ruleset_cache_effect(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_ruleset_cache_effect at the server.");
  }
  ensure_valid_variant_packet_ruleset_cache_effect(pconn);

  switch(pconn->phs.variant[PACKET_RULESET_CACHE_EFFECT]) {
    case 100: {
      return receive_packet_ruleset_cache_effect_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 122 sc */
#define hash_packet_traderoute_info_100 hash_const

#define cmp_packet_traderoute_info_100 cmp_const

BV_DEFINE(packet_traderoute_info_100_fields, 5);

static struct packet_traderoute_info *
receive_packet_traderoute_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_traderoute_info_100_fields fields;
  struct packet_traderoute_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_traderoute_info *clone;
  RECEIVE_PACKET_START(packet_traderoute_info, real_packet);
  printf("sc op=122 TRADEROUTE_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_traderoute_info_100,
                     cmp_packet_traderoute_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->trademindist = readin;
    printf(" trademindist=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuepct = readin;
    printf(" traderevenuepct=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuestyle = readin;
    printf(" traderevenuestyle=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->caravanbonusstyle = readin;
    printf(" caravanbonusstyle=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->maxtraderoutes = readin;
    printf(" maxtraderoutes=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

#define hash_packet_traderoute_info_101 hash_const

#define cmp_packet_traderoute_info_101 cmp_const

BV_DEFINE(packet_traderoute_info_101_fields, 4);

static struct packet_traderoute_info *
receive_packet_traderoute_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_traderoute_info_101_fields fields;
  struct packet_traderoute_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_traderoute_info *clone;
  RECEIVE_PACKET_START(packet_traderoute_info, real_packet);
  printf("sc op=122 TRADEROUTE_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_traderoute_info_101,
                     cmp_packet_traderoute_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->trademindist = readin;
    printf(" trademindist=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuepct = readin;
    printf(" traderevenuepct=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuestyle = readin;
    printf(" traderevenuestyle=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->caravanbonusstyle = readin;
    printf(" caravanbonusstyle=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_traderoute_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_TRADEROUTE_INFO] != -1) {
    return;
  }

  if((has_capability("extglobalinfo", pconn->capability)
      && has_capability("extglobalinfo", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("extglobalinfo", pconn->capability)
              && has_capability("extglobalinfo", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_TRADEROUTE_INFO] = variant;
}

struct packet_traderoute_info *
receive_packet_traderoute_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_traderoute_info at the server.");
  }
  ensure_valid_variant_packet_traderoute_info(pconn);

  switch(pconn->phs.variant[PACKET_TRADEROUTE_INFO]) {
    case 100: {
      return receive_packet_traderoute_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_traderoute_info_101(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 123 sc */
#define hash_packet_extgame_info_100 hash_const

#define cmp_packet_extgame_info_100 cmp_const

BV_DEFINE(packet_extgame_info_100_fields, 14);

static struct packet_extgame_info *
receive_packet_extgame_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_extgame_info_100_fields fields;
  struct packet_extgame_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_extgame_info *clone;
  RECEIVE_PACKET_START(packet_extgame_info, real_packet);
  printf("sc op=123 EXTGAME_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_extgame_info_100,
                     cmp_packet_extgame_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  real_packet->futuretechsscore = BV_ISSET(fields, 0);
  printf(" futuretechsscore=%u", real_packet->futuretechsscore);

  real_packet->improvedautoattack = BV_ISSET(fields, 1);
  printf(" improvedautoattack=%u", real_packet->improvedautoattack);

  real_packet->stackbribing = BV_ISSET(fields, 2);
  printf(" stackbribing=%u", real_packet->stackbribing);

  real_packet->experimentalbribingcost = BV_ISSET(fields, 3);
  printf(" experimentalbribingcost=%u", real_packet->experimentalbribingcost);

  real_packet->techtrading = BV_ISSET(fields, 4);
  printf(" techtrading=%u", real_packet->techtrading);

  real_packet->ignoreruleset = BV_ISSET(fields, 5);
  printf(" ignoreruleset=%u", real_packet->ignoreruleset);

  real_packet->goldtrading = BV_ISSET(fields, 6);
  printf(" goldtrading=%u", real_packet->goldtrading);

  real_packet->citytrading = BV_ISSET(fields, 7);
  printf(" citytrading=%u", real_packet->citytrading);

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->airliftingstyle = readin;
    printf(" airliftingstyle=%u", readin);
  }
  real_packet->teamplacement = BV_ISSET(fields, 9);
  printf(" teamplacement=%u", real_packet->teamplacement);

  real_packet->globalwarmingon = BV_ISSET(fields, 10);
  printf(" globalwarmingon=%u", real_packet->globalwarmingon);

  real_packet->nuclearwinteron = BV_ISSET(fields, 11);
  printf(" nuclearwinteron=%u", real_packet->nuclearwinteron);

  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->maxallies = readin;
    printf(" maxallies=%u", readin);
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techleakagerate = readin;
    printf(" techleakagerate=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

#define hash_packet_extgame_info_101 hash_const

#define cmp_packet_extgame_info_101 cmp_const

BV_DEFINE(packet_extgame_info_101_fields, 12);

static struct packet_extgame_info *
receive_packet_extgame_info_101(
       connection_t *pconn,
       enum packet_type type)
{
  packet_extgame_info_101_fields fields;
  struct packet_extgame_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_extgame_info *clone;
  RECEIVE_PACKET_START(packet_extgame_info, real_packet);
  printf("sc op=123 EXTGAME_INFO");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_extgame_info_101,
                     cmp_packet_extgame_info_101);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  real_packet->futuretechsscore = BV_ISSET(fields, 0);
  printf(" futuretechsscore=%u", real_packet->futuretechsscore);

  real_packet->improvedautoattack = BV_ISSET(fields, 1);
  printf(" improvedautoattack=%u", real_packet->improvedautoattack);

  real_packet->stackbribing = BV_ISSET(fields, 2);
  printf(" stackbribing=%u", real_packet->stackbribing);

  real_packet->experimentalbribingcost = BV_ISSET(fields, 3);
  printf(" experimentalbribingcost=%u", real_packet->experimentalbribingcost);

  real_packet->techtrading = BV_ISSET(fields, 4);
  printf(" techtrading =%u", real_packet->techtrading);

  real_packet->ignoreruleset = BV_ISSET(fields, 5);
  printf(" ignoreruleset =%u", real_packet->ignoreruleset);

  real_packet->goldtrading = BV_ISSET(fields, 6);
  printf(" goldtrading =%u", real_packet->goldtrading);

  real_packet->citytrading = BV_ISSET(fields, 7);
  printf(" citytrading =%u", real_packet->citytrading);

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->airliftingstyle = readin;
    printf(" airliftingstyle =%u", readin);
  }
  real_packet->teamplacement = BV_ISSET(fields, 9);
  printf(" teamplacement =%u", real_packet->teamplacement);

  real_packet->globalwarmingon = BV_ISSET(fields, 10);
  printf(" globalwarmingon =%u", real_packet->globalwarmingon);

  real_packet->nuclearwinteron = BV_ISSET(fields, 11);
  printf(" nuclearwinteron =%u\n", real_packet->nuclearwinteron);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_extgame_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_EXTGAME_INFO] != -1) {
    return;
  }

  if((has_capability("exttechleakage", pconn->capability)
      && has_capability("exttechleakage", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("exttechleakage", pconn->capability)
              && has_capability("exttechleakage", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_EXTGAME_INFO] = variant;
}

struct packet_extgame_info *
receive_packet_extgame_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_extgame_info at the server.");
  }
  ensure_valid_variant_packet_extgame_info(pconn);

  switch(pconn->phs.variant[PACKET_EXTGAME_INFO]) {
    case 100: {
      return receive_packet_extgame_info_100(pconn, type);
    }
    case 101: {
      return receive_packet_extgame_info_101(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 124 sc */
#define hash_packet_vote_new_100 hash_const

#define cmp_packet_vote_new_100 cmp_const

BV_DEFINE(packet_vote_new_100_fields, 6);

static struct packet_vote_new *
receive_packet_vote_new_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_vote_new_100_fields fields;
  struct packet_vote_new *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_vote_new *clone;
  RECEIVE_PACKET_START(packet_vote_new, real_packet);
  printf("sc op=124 VOTE_NEW");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_vote_new_100,
                     cmp_packet_vote_new_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
    printf(" vote_no=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->user, sizeof(real_packet->user));
    printf(" user=\"%s\"", real_packet->user);
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->desc, sizeof(real_packet->desc));
    printf(" desc=\"%s\"", real_packet->desc);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->percent_required = readin;
    printf(" percent_required=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
    printf(" flags=%u", readin);
  }
  real_packet->is_poll = BV_ISSET(fields, 5);
  printf(" is_poll=%u\n", real_packet->is_poll);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_vote_new(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_VOTE_NEW] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_VOTE_NEW] = variant;
}

struct packet_vote_new *
receive_packet_vote_new(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_new at the server.");
  }
  ensure_valid_variant_packet_vote_new(pconn);

  switch(pconn->phs.variant[PACKET_VOTE_NEW]) {
    case 100: {
      return receive_packet_vote_new_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 125 sc */
#define hash_packet_vote_update_100 hash_const

#define cmp_packet_vote_update_100 cmp_const

BV_DEFINE(packet_vote_update_100_fields, 5);

static struct packet_vote_update *
receive_packet_vote_update_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_vote_update_100_fields fields;
  struct packet_vote_update *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_vote_update *clone;
  RECEIVE_PACKET_START(packet_vote_update, real_packet);
  printf("sc op-125 VOTE_UPDATE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_vote_update_100,
                     cmp_packet_vote_update_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
    printf(" vote_no=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->yes = readin;
    printf(" yes=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->no = readin;
    printf(" no=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->abstain = readin;
    printf(" abstain=%u", readin);
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_voters = readin;
    printf(" num_voters=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_vote_update(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_VOTE_UPDATE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_VOTE_UPDATE] = variant;
}

struct packet_vote_update *
receive_packet_vote_update(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_update at the server.");
  }
  ensure_valid_variant_packet_vote_update(pconn);

  switch(pconn->phs.variant[PACKET_VOTE_UPDATE]) {
    case 100: {
      return receive_packet_vote_update_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 126 sc */
#define hash_packet_vote_remove_100 hash_const

#define cmp_packet_vote_remove_100 cmp_const

BV_DEFINE(packet_vote_remove_100_fields, 1);

static struct packet_vote_remove *
receive_packet_vote_remove_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_vote_remove_100_fields fields;
  struct packet_vote_remove *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_vote_remove *clone;
  RECEIVE_PACKET_START(packet_vote_remove, real_packet);
  printf("sc op=126 VOTE_REMOVE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_vote_remove_100,
                     cmp_packet_vote_remove_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
    printf(" vote_no=%u", readin);
  }
  printf("\n");

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_vote_remove(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_VOTE_REMOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_VOTE_REMOVE] = variant;
}

struct packet_vote_remove *
receive_packet_vote_remove(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_remove at the server.");
  }
  ensure_valid_variant_packet_vote_remove(pconn);

  switch(pconn->phs.variant[PACKET_VOTE_REMOVE]) {
    case 100: {
      return receive_packet_vote_remove_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 127 sc */
#define hash_packet_vote_resolve_100 hash_const

#define cmp_packet_vote_resolve_100 cmp_const

BV_DEFINE(packet_vote_resolve_100_fields, 2);

static struct packet_vote_resolve *
receive_packet_vote_resolve_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_vote_resolve_100_fields fields;
  struct packet_vote_resolve *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_vote_resolve *clone;
  RECEIVE_PACKET_START(packet_vote_resolve, real_packet);
  printf("sc op=127 VOTE_RESOLVE");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_vote_resolve_100,
                     cmp_packet_vote_resolve_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->vote_no = readin;
    printf(" vote_no=%u", readin);
  }
  real_packet->passed = BV_ISSET(fields, 1);
  printf(" passed=%u", real_packet->passed);

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_vote_resolve(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_VOTE_RESOLVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_VOTE_RESOLVE] = variant;
}

struct packet_vote_resolve *
receive_packet_vote_resolve(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_vote_resolve at the server.");
  }
  ensure_valid_variant_packet_vote_resolve(pconn);

  switch(pconn->phs.variant[PACKET_VOTE_RESOLVE]) {
    case 100: {
      return receive_packet_vote_resolve_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 128 cs */
/* 129 */
/* 130 cs */
/* 131 cs */
/* 132 cs */
/* 133 sc */
#define hash_packet_trade_route_info_100 hash_const

#define cmp_packet_trade_route_info_100 cmp_const

BV_DEFINE(packet_trade_route_info_100_fields, 4);

static struct packet_trade_route_info *
receive_packet_trade_route_info_100(
       connection_t *pconn,
       enum packet_type type)
{
  packet_trade_route_info_100_fields fields;
  struct packet_trade_route_info *old;
  struct hash_table **hash = &pconn->phs.received[type];
  struct packet_trade_route_info *clone;
  RECEIVE_PACKET_START(packet_trade_route_info, real_packet);
  printf("sc op=133 TRADE_ROUTE_INFO\n");
  DIO_BV_GET(&din, fields);

  if (!*hash) {
    *hash = hash_new(hash_packet_trade_route_info_100,
                     cmp_packet_trade_route_info_100);
  }
  old = hash_delete_entry(*hash, real_packet);

  if (old) {
    *real_packet = *old;
  } else {
    memset(real_packet, 0, sizeof(*real_packet));
  }

  if (BV_ISSET(fields, 0)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city1 = readin;
    printf(" city1=%u", readin);
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city2 = readin;
    printf(" city2=%u", readin);
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->unit_id = readin;
    printf(" unit_id=%u", readin);
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->status = readin;
    printf(" status=%u\n", readin);
  } else {
    printf("\n");
  }

  clone = wc_malloc(sizeof(*clone));
  *clone = *real_packet;
  if (old) {
    free(old);
  }
  hash_insert(*hash, clone, clone);

  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_trade_route_info(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_TRADE_ROUTE_INFO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_TRADE_ROUTE_INFO] = variant;
}

struct packet_trade_route_info *
receive_packet_trade_route_info(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_trade_route_info at the server.");
  }
  ensure_valid_variant_packet_trade_route_info(pconn);

  switch(pconn->phs.variant[PACKET_TRADE_ROUTE_INFO]) {
    case 100: {
      return receive_packet_trade_route_info_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 134 */
/* 135 sc */
static struct packet_freeze_client *
receive_packet_freeze_client_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_freeze_client, real_packet);
  printf("sc op=135 FREEZE_CLIENT\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_freeze_client(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_FREEZE_CLIENT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_FREEZE_CLIENT] = variant;
}

struct packet_freeze_client *
receive_packet_freeze_client(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_freeze_client at the server.");
  }
  ensure_valid_variant_packet_freeze_client(pconn);

  switch(pconn->phs.variant[PACKET_FREEZE_CLIENT]) {
    case 100: {
      return receive_packet_freeze_client_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 136 sc */
static struct packet_thaw_client *
receive_packet_thaw_client_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_thaw_client, real_packet);
  printf("sc op=136 THAW_CLIENT\n");
  RECEIVE_PACKET_END(real_packet);
}

static void ensure_valid_variant_packet_thaw_client(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_THAW_CLIENT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_THAW_CLIENT] = variant;
}

struct packet_thaw_client *
receive_packet_thaw_client(
       connection_t *pconn,
       enum packet_type type)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to read data from the closed connection %s",
            conn_description(pconn));
    return NULL;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Receiving packet_thaw_client at the server.");
  }
  ensure_valid_variant_packet_thaw_client(pconn);

  switch(pconn->phs.variant[PACKET_THAW_CLIENT]) {
    case 100: {
      return receive_packet_thaw_client_100(pconn, type);
    }
    default: {
      die("unknown variant");
      return NULL;
    }
  }
}

/* 137 */
/* 138 cs */
/* 139 cs */
/* 140 */
/* 141 cs */
/* 142 cs */
/* 143 */
/* 144 */
/* 145 cs sc */
/* 146 cs sc */
/* 147 */
/* 148 */
/* 149 */
/* 150 cs */

