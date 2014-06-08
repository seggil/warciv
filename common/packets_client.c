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
# ifdef REPLAY_2
  printf("sc op=0 PROCESSING_STARTED\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=1 PROCESSING_FINISHED\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=2 FREEZE_HINT\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=3 THAW_HINT\n");
# endif
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
static int send_packet_server_join_req_100(
               connection_t *pconn,
               const struct packet_server_join_req *packet)
{
  const struct packet_server_join_req *real_packet = packet;
  SEND_PACKET_START(PACKET_SERVER_JOIN_REQ);

  dio_put_string(&dout, real_packet->username);
  dio_put_string(&dout, real_packet->capability);
  dio_put_string(&dout, real_packet->version_label);
  dio_put_uint32(&dout, real_packet->major_version);
  dio_put_uint32(&dout, real_packet->minor_version);
  dio_put_uint32(&dout, real_packet->patch_version);
# ifdef REPLAY_2
  printf("cs opc=4 SERVER_JOIN_REQ, username=%s, "
         "capability=%s, version_label=%s, "
         "major_version=%u, minor_version=%u, patch_version=%u\n",
         real_packet->username,
         real_packet->capability,
         real_packet->version_label,
         real_packet->major_version,
         real_packet->minor_version,
         real_packet->patch_version);
# endif
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_server_join_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SERVER_JOIN_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SERVER_JOIN_REQ] = variant;
}

int send_packet_server_join_req(
        connection_t *pconn,
        const struct packet_server_join_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_server_join_req from the server.");
  }
  send_ensure_valid_variant_packet_server_join_req(pconn);

  switch(pconn->phs.variant[PACKET_SERVER_JOIN_REQ]) {
    case 100: {
      return send_packet_server_join_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_server_join_req(
         connection_t *pconn,
         const char *username,
         const char *capability,
         const char *version_label,
         int major_version,
         int minor_version,
         int patch_version)
{
  struct packet_server_join_req packet, *real_packet = &packet;

  sz_strlcpy(real_packet->username, username);
  sz_strlcpy(real_packet->capability, capability);
  sz_strlcpy(real_packet->version_label, version_label);
  real_packet->major_version = major_version;
  real_packet->minor_version = minor_version;
  real_packet->patch_version = patch_version;

  return send_packet_server_join_req(pconn, real_packet);
}

/* 5 sc */
static struct packet_server_join_reply *
receive_packet_server_join_reply_100(
       connection_t *pconn,
       enum packet_type type)
{
  int readin;

  RECEIVE_PACKET_START(packet_server_join_reply, real_packet);
# ifdef REPLAY_2
  printf("sc op=5 SERVER_JOIN_REPLY");
# endif
  dio_get_bool8(&din, &real_packet->you_can_join);
  dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
  dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
  dio_get_string(&din, real_packet->challenge_file, sizeof(real_packet->challenge_file));

  dio_get_uint8(&din, &readin);
  real_packet->conn_id = readin;
# ifdef REPLAY_2
  printf(" you_can_join=%u; message=%s; capability=%s; challenge_file=%s"
         "conn_id=%u\n",
         real_packet->you_can_join, real_packet->message,
         real_packet->capability, real_packet->challenge_file,
         real_packet->conn_id);
# endif
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
# ifdef REPLAY_2
  printf("sc op=6 AUTHENTICATION_REQ");
# endif
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
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
    real_packet->type = readin;
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->message, sizeof(real_packet->message));
#   ifdef REPLAY_2
    printf(" message=%s\n", real_packet->message);
  } else {
    printf("\n");
# endif
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
#define hash_packet_authentication_reply_100 hash_const

#define cmp_packet_authentication_reply_100 cmp_const

BV_DEFINE(packet_authentication_reply_100_fields, 1);

static int send_packet_authentication_reply_100(
               connection_t *pconn,
               const struct packet_authentication_reply *packet)
{
  const struct packet_authentication_reply *real_packet = packet;
  packet_authentication_reply_100_fields fields;
  struct packet_authentication_reply *old, *clone;
  bool differ, old_from_hash;
  bool force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_AUTHENTICATION_REPLY];
  int different = 0;

  SEND_PACKET_START(PACKET_AUTHENTICATION_REPLY);
# ifdef REPLAY_2
  printf("cs op=7 AUTHENTICATION_REPLY");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_authentication_reply_100,
                     cmp_packet_authentication_reply_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (strcmp(old->password, real_packet->password) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_string(&dout, real_packet->password);
# ifdef REPLAY_2
    printf(" password=%s\n", real_packet->password);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_authentication_reply(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_AUTHENTICATION_REPLY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_AUTHENTICATION_REPLY] = variant;
}

int send_packet_authentication_reply(
        connection_t *pconn,
        const struct packet_authentication_reply *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_authentication_reply from the server.");
  }
  send_ensure_valid_variant_packet_authentication_reply(pconn);

  switch(pconn->phs.variant[PACKET_AUTHENTICATION_REPLY]) {
    case 100: {
      return send_packet_authentication_reply_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 8 sc */
static struct packet_server_shutdown *
receive_packet_server_shutdown_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_server_shutdown, real_packet);
# ifdef REPLAY_2
  printf("sc op=8 SERVER_SHUTDOWN\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=9 NATION_UNAVAILABLE");
# endif
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
#   ifdef REPLAY_2
    printf(" nation=%u\n", readin);
  } else {
    printf("\n");
# endif
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
#define hash_packet_nation_select_req_100 hash_const

#define cmp_packet_nation_select_req_100 cmp_const

BV_DEFINE(packet_nation_select_req_100_fields, 4);

static int send_packet_nation_select_req_100(
               connection_t *pconn,
               const struct packet_nation_select_req *packet)
{
  const struct packet_nation_select_req *real_packet = packet;
  packet_nation_select_req_100_fields fields;
  struct packet_nation_select_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_NATION_SELECT_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_NATION_SELECT_REQ);
# ifdef REPLAY_2
  printf("cs opc=10 NATION_SELECT_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_nation_select_req_100,
                     cmp_packet_nation_select_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->nation_no != real_packet->nation_no);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->is_male != real_packet->is_male);
  if (differ) {different++;}
  if(packet->is_male) {
    BV_SET(fields, 1);
  }

  differ = (strcmp(old->name, real_packet->name) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  differ = (old->city_style != real_packet->city_style);
  if (differ) {
    different++;
    BV_SET(fields, 3);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->nation_no);
# ifdef REPLAY_2
    printf(" nation_no=%u", real_packet->nation_no);
# endif
  }
  /* field 1 is folded into the header */
  if (BV_ISSET(fields, 2)) {
    dio_put_string(&dout, real_packet->name);
# ifdef REPLAY_2
    printf(" name=%s", real_packet->name);
# endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_put_uint8(&dout, real_packet->city_style);
# ifdef REPLAY_2
    printf(" city_style=%u\n", real_packet->city_style);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_nation_select_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_NATION_SELECT_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_NATION_SELECT_REQ] = variant;
}

int send_packet_nation_select_req(
        connection_t *pconn,
        const struct packet_nation_select_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_nation_select_req from the server.");
  }
  send_ensure_valid_variant_packet_nation_select_req(pconn);

  switch(pconn->phs.variant[PACKET_NATION_SELECT_REQ]) {
    case 100: {
      return send_packet_nation_select_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_nation_select_req(connection_t *pconn,
                                   Nation_Type_id nation_no,
                                   bool is_male,
                                   const char *name,
                                   int city_style)
{
  struct packet_nation_select_req packet, *real_packet = &packet;

  real_packet->nation_no = nation_no;
  real_packet->is_male = is_male;
  sz_strlcpy(real_packet->name, name);
  real_packet->city_style = city_style;

  return send_packet_nation_select_req(pconn, real_packet);
}

/* 11 sc */
static struct packet_nation_select_ok *
receive_packet_nation_select_ok_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_nation_select_ok, real_packet);
# ifdef REPLAY_2
  printf("sc op=11 NATION_SELECT_OK\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=12 GAME_STATE");
# endif
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
#   ifdef REPLAY_2
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=13 ENDGAME_REPORT\n");
# endif
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
#   ifdef REPLAY_2
    printf("nscores=%u ", readin);
#   endif
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
#     ifdef REPLAY_2
      printf(" id[%d]=%u", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf(" score[%d]=%u", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("pop[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("bnp[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("mfg[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("cities[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("techs[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("mil_service[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("wonders[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("research[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("landarea[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("setlandarea[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("literacy[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
#     ifdef REPLAY_2
      printf("literacy[%d]=%u ", i, readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=14 TILE_INFO");
# endif
  DIO_BV_GET(&din, fields);

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" x=%u y=%u", real_packet->x, readin);
#   endif

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
#   ifdef REPLAY_2
    printf(" type=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->known = readin;
#   ifdef REPLAY_2
    printf(" known=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->special = readin;
#   ifdef REPLAY_2
    printf(" special=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->owner = readin;
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->continent = readin;
#   ifdef REPLAY_2
    printf(" continent=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->spec_sprite, sizeof(real_packet->spec_sprite));
#   ifdef REPLAY_2
    printf(" spec_sprite=\"%s\"\n", real_packet->spec_sprite);
  } else {
    printf("\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=15 GAME_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->tech = readin;
#   ifdef REPLAY_2
    printf(" tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researchcost = readin;
#   ifdef REPLAY_2
    printf(" researchcost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->skill_level = readin;
#   ifdef REPLAY_2
    printf(" skill_level=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->seconds_to_turndone = readin;
#   ifdef REPLAY_2
    printf(" seconds_to_turndone=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->timeout = readin;
#   ifdef REPLAY_2
    printf(" timeout=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn = readin;
#   ifdef REPLAY_2
    printf(" turn=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->year = readin;
#   ifdef REPLAY_2
    printf(" year=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->end_year = readin;
#   ifdef REPLAY_2
    printf(" end_year=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_players = readin;
#   ifdef REPLAY_2
    printf(" min_players=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->max_players = readin;
#   ifdef REPLAY_2
    printf(" max_players=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nplayers = readin;
#   ifdef REPLAY_2
    printf(" nplayers=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->player_idx = readin;
#   ifdef REPLAY_2
    printf(" player_idx=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->globalwarming = readin;
#   ifdef REPLAY_2
    printf(" globalwarming=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->heating = readin;
#   ifdef REPLAY_2
    printf(" heating=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->nuclearwinter = readin;
#   ifdef REPLAY_2
    printf(" nuclearwinter=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cooling = readin;
#   ifdef REPLAY_2
    printf(" cooling=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->cityfactor = readin;
#   ifdef REPLAY_2
    printf(" cityfactor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->diplcost = readin;
#   ifdef REPLAY_2
    printf(" diplcost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->freecost = readin;
#   ifdef REPLAY_2
    printf(" freecost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->conquercost = readin;
#   ifdef REPLAY_2
    printf(" conquercost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappysize = readin;
#   ifdef REPLAY_2
    printf(" unhappysize=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->angrycitizen = readin;
#   ifdef REPLAY_2
    printf(" angrycitizen =%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techpenalty = readin;
#   ifdef REPLAY_2
    printf(" techpenalty =%u", readin);
#   endif
  }

  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->foodbox = readin;
#   ifdef REPLAY_2
    printf(" foodbox =%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->civstyle = readin;
#   ifdef REPLAY_2
    printf(" civstyle =%u", readin);
#   endif
  }

  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->diplomacy = readin;
#   ifdef REPLAY_2
    printf(" diplomacy =%u", readin);
#   endif
  }

  real_packet->spacerace = BV_ISSET(fields, 27);
# ifdef REPLAY_2
  printf(" spacerace =%u", real_packet->spacerace);
# endif

  if (BV_ISSET(fields, 28)) {
#   ifdef REPLAY_2
    printf(" spacerace[]=");
#   endif
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
#       ifdef REPLAY_2
        printf(" %u", readin);
#       endif
      }
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
  }
  if (BV_ISSET(fields, 29)) {
#   ifdef REPLAY_2
    printf(" global_wonders[]=");
#   endif
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
#       ifdef REPLAY_2
        printf(" %u", readin);
#       endif
      }
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=16 MAP_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" xsize=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->ysize = readin;
#   ifdef REPLAY_2
    printf(" ysize=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->topology_id = readin;
#   ifdef REPLAY_2
    printf(" topology_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=17 NUKE_TILE_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=18 CHAT_MSG");
# endif
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
#   ifdef REPLAY_2
    printf(" message=\"%s\"", real_packet->message);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->event = readin;
#   ifdef REPLAY_2
    printf(" event=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->conn_id = readin;
#   ifdef REPLAY_2
    printf(" conn_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_chat_msg_req_100 hash_const

#define cmp_packet_chat_msg_req_100 cmp_const

BV_DEFINE(packet_chat_msg_req_100_fields, 1);

static int send_packet_chat_msg_req_100(
               connection_t *pconn,
               const struct packet_chat_msg_req *packet)
{
  const struct packet_chat_msg_req *real_packet = packet;
  packet_chat_msg_req_100_fields fields;
  struct packet_chat_msg_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CHAT_MSG_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_CHAT_MSG_REQ);
# ifdef REPLAY_2
  printf("cs opc=19 CHAT_MSG_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_chat_msg_req_100,
                     cmp_packet_chat_msg_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (strcmp(old->message, real_packet->message) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_string(&dout, real_packet->message);
# ifdef REPLAY_2
    printf(" message=%s\n", real_packet->message);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_chat_msg_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CHAT_MSG_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CHAT_MSG_REQ] = variant;
}

int send_packet_chat_msg_req(
        connection_t *pconn,
        const struct packet_chat_msg_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_chat_msg_req from the server.");
  }
  send_ensure_valid_variant_packet_chat_msg_req(pconn);

  switch(pconn->phs.variant[PACKET_CHAT_MSG_REQ]) {
    case 100: {
      return send_packet_chat_msg_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_chat_msg_req(connection_t *pconn, const char *message)
{
  struct packet_chat_msg_req packet, *real_packet = &packet;

  sz_strlcpy(real_packet->message, message);

  return send_packet_chat_msg_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=20 CITY_REMOVE");
# endif
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
#   ifdef REPLAY_2
    printf(" city_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=21 CITY_INFO");
# endif
  DIO_BV_GET(&din, fields);
  int readin;

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
# ifdef REPLAY_2
  printf(" id=%u", readin);
# endif

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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
#   ifdef REPLAY_2
    printf(" size=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_happy[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_happy[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_content[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_content[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_unhappy[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_unhappy[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_angry[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_angry[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 9)) {
    int i;

#   ifdef REPLAY_2
    printf(" specialists[5]=");
#   endif
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialists[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_prod = readin;
#   ifdef REPLAY_2
    printf(" food_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_prod = readin;
#   ifdef REPLAY_2
    printf(" shield_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->trade_prod = readin;
#   ifdef REPLAY_2
    printf(" trade_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->food_surplus = readin;
#   ifdef REPLAY_2
    printf(" food_surplus=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->shield_surplus = readin;
#   ifdef REPLAY_2
    printf(" shield_surplus=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tile_trade = readin;
#   ifdef REPLAY_2
    printf(" tile_trade=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_stock = readin;
#   ifdef REPLAY_2
    printf(" food_stock=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_stock = readin;
#   ifdef REPLAY_2
    printf(" shield_stock=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption = readin;
#   ifdef REPLAY_2
    printf(" corruption=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int i;

#   ifdef REPLAY_2
    printf(" trade[]=");
#   endif
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->trade[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 20)) {
    int i;

#   ifdef REPLAY_2
    printf(" trade_value[]=");
#   endif
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trade_value[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->luxury_total = readin;
#   ifdef REPLAY_2
    printf(" luxury_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tax_total = readin;
#   ifdef REPLAY_2
    printf(" tax_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->science_total = readin;
#   ifdef REPLAY_2
    printf(" science_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution = readin;
#   ifdef REPLAY_2
    printf(" pollution=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_waste = readin;
#   ifdef REPLAY_2
    printf(" shield_waste=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->currently_building = readin;
#   ifdef REPLAY_2
    printf(" currently_building=%u", readin);
#   endif
  }
  real_packet->is_building_unit = BV_ISSET(fields, 27);
# ifdef REPLAY_2
  printf(" is_building_unit=%u", readin);
# endif

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_last_built = readin;
#   ifdef REPLAY_2
    printf(" turn_last_built=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->changed_from_id = readin;
#   ifdef REPLAY_2
    printf(" changed_from_id=%u", readin);
#   endif
  }
  real_packet->changed_from_is_unit = BV_ISSET(fields, 30);
# ifdef REPLAY_2
  printf(" changed_from_is_unit=%u", real_packet->changed_from_is_unit);
# endif

  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->before_change_shields = readin;
#   ifdef REPLAY_2
    printf(" before_change_shields=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->disbanded_shields = readin;
#   ifdef REPLAY_2
    printf(" disbanded_shields=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->caravan_shields = readin;
#   ifdef REPLAY_2
    printf(" caravan_shields=%u", readin);
#   endif
   }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
#   ifdef REPLAY_2
    printf(" last_turns_shield_surplus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 35)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_worklist(&din, &real_packet->worklist);
#   ifdef REPLAY_2
    printf("worklist={");
    printf(" is_valid=%u", real_packet->worklist.is_valid);
    printf(" name=\"%s\"", real_packet->worklist.name);
    printf(" wlefs[i]=");
    for (i = 0; i < MAX_LEN_WORKLIST; i++)
      printf(" %u", real_packet->worklist.wlefs[i]);
    printf("}");
#   endif
  }
  if (BV_ISSET(fields, 36)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
#   ifdef REPLAY_2
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
#   endif
  }
  if (BV_ISSET(fields, 37)) {
    int i;

#   ifdef REPLAY_2
    printf(" city_map[]=");
#   endif
    for (i = 0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->city_map[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  real_packet->did_buy = BV_ISSET(fields, 38);
# ifdef REPLAY_2
  printf(" did_buy=%u", real_packet->did_buy);
# endif

  real_packet->did_sell = BV_ISSET(fields, 39);
# ifdef REPLAY_2
  printf(" did_sell=%u", real_packet->did_sell);
# endif

  real_packet->was_happy = BV_ISSET(fields, 40);
# ifdef REPLAY_2
  printf(" was_happy=%u", real_packet->was_happy);
# endif

  real_packet->airlift = BV_ISSET(fields, 41);
# ifdef REPLAY_2
  printf(" airlift=%u", real_packet->airlift);
# endif

  real_packet->diplomat_investigate = BV_ISSET(fields, 42);
# ifdef REPLAY_2
  printf(" diplomat_investigate=%u", real_packet->diplomat_investigate);
# endif

  if (BV_ISSET(fields, 43)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rally_point_x = readin;
#   ifdef REPLAY_2
    printf(" rally_point_x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 44)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rally_point_y = readin;
#   ifdef REPLAY_2
    printf(" rally_point_y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 45)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_options = readin;
#   ifdef REPLAY_2
    printf(" city_options=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 46)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_founded = readin;
#   ifdef REPLAY_2
    printf(" turn_founded=%d\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=21 CITY_INFO");
# endif

  DIO_BV_GET(&din, fields);
  {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->id = readin;
# ifdef REPLAY_2
  printf(" id=%u", readin);
# endif
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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
#   ifdef REPLAY_2
    printf(" size=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_happy[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_happy[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_content[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_content[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_unhappy[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
      real_packet->people_unhappy[i] = readin;
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

#   ifdef REPLAY_2
    printf(" people_angry[5]=");
#   endif
    for (i = 0; i < 5; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->people_angry[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 9)) {
    int i;

#   ifdef REPLAY_2
    printf(" specialists[5]=");
#   endif
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialists[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_prod = readin;
#   ifdef REPLAY_2
    printf(" food_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_prod = readin;
#   ifdef REPLAY_2
    printf(" shield_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->trade_prod = readin;
#   ifdef REPLAY_2
    printf(" trade_prod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->food_surplus = readin;
#   ifdef REPLAY_2
    printf(" food_surplus=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->shield_surplus = readin;
#   ifdef REPLAY_2
    printf(" shield_surplus=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tile_trade = readin;
#   ifdef REPLAY_2
    printf(" tile_trade=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->food_stock = readin;
#   ifdef REPLAY_2
    printf(" food_stock=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_stock = readin;
#   ifdef REPLAY_2
    printf(" shield_stock=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption = readin;
#   ifdef REPLAY_2
    printf(" corruption=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int i;

#   ifdef REPLAY_2
    printf(" trade[]=");
#   endif
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->trade[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 20)) {
    int i;

#   ifdef REPLAY_2
    printf(" trade_value[]=");
#   endif
    for (i = 0; i < OLD_NUM_TRADEROUTES; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trade_value[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->luxury_total = readin;
#   ifdef REPLAY_2
    printf(" luxury_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tax_total = readin;
#   ifdef REPLAY_2
    printf(" tax_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->science_total = readin;
#   ifdef REPLAY_2
    printf(" science_total=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution = readin;
#   ifdef REPLAY_2
    printf(" pollution=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->shield_waste = readin;
#   ifdef REPLAY_2
    printf(" shield_waste=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->currently_building = readin;
#   ifdef REPLAY_2
    printf(" currently_building=%u", readin);
#   endif
  }
  real_packet->is_building_unit = BV_ISSET(fields, 27);
# ifdef REPLAY_2
  printf(" is_building_unit=%u", real_packet->is_building_unit );
# endif

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_last_built = readin;
#   ifdef REPLAY_2
    printf(" turn_last_built=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->changed_from_id = readin;
#   ifdef REPLAY_2
    printf(" changed_from_id=%u", readin);
#   endif
  }
  real_packet->changed_from_is_unit = BV_ISSET(fields, 30);
# ifdef REPLAY_2
  printf(" changed_from_is_unit=%u", real_packet->changed_from_is_unit);
# endif

  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->before_change_shields = readin;
#   ifdef REPLAY_2
    printf(" before_change_shields=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->disbanded_shields = readin;
#   ifdef REPLAY_2
    printf(" disbanded_shields=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->caravan_shields = readin;
#   ifdef REPLAY_2
    printf(" caravan_shields=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->last_turns_shield_surplus = readin;
#   ifdef REPLAY_2
    printf(" last_turns_shield_surplus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 35)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_worklist(&din, &real_packet->worklist);
#   ifdef REPLAY_2
    printf("worklist={");
    printf(" is_valid=%u", real_packet->worklist.is_valid);
    printf(" name=\"%s\"", real_packet->worklist.name);
    printf(" wlefs[i]=");
    for (i = 0; i < MAX_LEN_WORKLIST; i++)
      printf(" %u", real_packet->worklist.wlefs[i]);
    printf("}");
#   endif
  }
  if (BV_ISSET(fields, 36)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
#   ifdef REPLAY_2
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
#   endif
  }
  if (BV_ISSET(fields, 37)) {
    int i;

#   ifdef REPLAY_2
    printf(" city_map[]=");
#   endif
    for (i = 0; i < CITY_MAP_SIZE * CITY_MAP_SIZE; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->city_map[i] = readin;
#     ifdef REPLAY_2
      printf("%u", readin);
#     endif
    }
  }
  real_packet->did_buy = BV_ISSET(fields, 38);
# ifdef REPLAY_2
  printf(" did_buy=%u", real_packet->did_buy);
# endif

  real_packet->did_sell = BV_ISSET(fields, 39);
# ifdef REPLAY_2
  printf(" did_sell=%u", real_packet->did_sell);
# endif

  real_packet->was_happy = BV_ISSET(fields, 40);
# ifdef REPLAY_2
  printf(" was_happy=%u", real_packet->was_happy);
# endif

  real_packet->airlift = BV_ISSET(fields, 41);
# ifdef REPLAY_2
  printf(" airlift=%u", real_packet->airlift);
# endif

  real_packet->diplomat_investigate = BV_ISSET(fields, 42);
# ifdef REPLAY_2
  printf(" diplomat_investigate=%u", real_packet->diplomat_investigate);
# endif

  if (BV_ISSET(fields, 43)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_options = readin;
#   ifdef REPLAY_2
    printf(" rally_point_x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 44)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn_founded = readin;
#   ifdef REPLAY_2
    printf(" turn_founded=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=22 CITY_SHORT_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\" ", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->size = readin;
#   ifdef REPLAY_2
    printf(" size=%u", readin);
#   endif
  }
  real_packet->happy = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" happy=%u", real_packet->happy);
# endif

  real_packet->unhappy = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" unhappy=%u", real_packet->unhappy);
# endif

  real_packet->capital = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" capital=%u", real_packet->capital);
# endif

  real_packet->walls = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" walls=%u", real_packet->walls);
# endif

  real_packet->occupied = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" occupied=%u", real_packet->occupied);
# endif

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->tile_trade = readin;
#   ifdef REPLAY_2
    printf(" tile_trade=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_city_sell_100 hash_const

#define cmp_packet_city_sell_100 cmp_const

BV_DEFINE(packet_city_sell_100_fields, 2);

static int send_packet_city_sell_100(
               connection_t *pconn,
               const struct packet_city_sell *packet)
{
  const struct packet_city_sell *real_packet = packet;
  packet_city_sell_100_fields fields;
  struct packet_city_sell *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_SELL];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_SELL);
# ifdef REPLAY_2
  printf("cs opc=23 CITY_SELL");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_sell_100,
                     cmp_packet_city_sell_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->build_id != real_packet->build_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u ", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->build_id);
# ifdef REPLAY_2
    printf(" build_id=%u\n", real_packet->build_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_sell(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_SELL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_SELL] = variant;
}

int send_packet_city_sell(
        connection_t *pconn,
        const struct packet_city_sell *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_sell from the server.");
  }
  send_ensure_valid_variant_packet_city_sell(pconn);

  switch(pconn->phs.variant[PACKET_CITY_SELL]) {
    case 100: {
      return send_packet_city_sell_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_sell(
         connection_t *pconn,
         int city_id, int
         build_id)
{
  struct packet_city_sell packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->build_id = build_id;

  return send_packet_city_sell(pconn, real_packet);
}

/* 24 cs */
#define hash_packet_city_buy_100 hash_const

#define cmp_packet_city_buy_100 cmp_const

BV_DEFINE(packet_city_buy_100_fields, 1);

static int send_packet_city_buy_100(
               connection_t *pconn,
               const struct packet_city_buy *packet)
{
  const struct packet_city_buy *real_packet = packet;
  packet_city_buy_100_fields fields;
  struct packet_city_buy *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_BUY];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_BUY);
# ifdef REPLAY_2
  printf("cs opc=24 CITY_BUY");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_buy_100,
                     cmp_packet_city_buy_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u\n", real_packet->city_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_buy(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_BUY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_BUY] = variant;
}

int send_packet_city_buy(
        connection_t *pconn,
        const struct packet_city_buy *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_buy from the server.");
  }
  send_ensure_valid_variant_packet_city_buy(pconn);

  switch(pconn->phs.variant[PACKET_CITY_BUY]) {
    case 100: {
      return send_packet_city_buy_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_buy(connection_t *pconn, int city_id)
{
  struct packet_city_buy packet, *real_packet = &packet;

  real_packet->city_id = city_id;

  return send_packet_city_buy(pconn, real_packet);
}

/* 25 cs */
#define hash_packet_city_change_100 hash_const

#define cmp_packet_city_change_100 cmp_const

BV_DEFINE(packet_city_change_100_fields, 3);

static int send_packet_city_change_100(
               connection_t *pconn,
               const struct packet_city_change *packet)
{
  const struct packet_city_change *real_packet = packet;
  packet_city_change_100_fields fields;
  struct packet_city_change *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_CHANGE];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_CHANGE);
# ifdef REPLAY_2
  printf("cs opc=25 CITY_CHANGE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_change_100,
                     cmp_packet_city_change_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->build_id != real_packet->build_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->is_build_id_unit_id != real_packet->is_build_id_unit_id);
  if (differ) {different++;}
  if(packet->is_build_id_unit_id) {BV_SET(fields, 2);}

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u ", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->build_id);
# ifdef REPLAY_2
    printf(" build_id=%u\n", real_packet->build_id);
  } else {
    printf("\n");
# endif
  }
  /* field 2 is folded into the header */

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_change(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_CHANGE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_CHANGE] = variant;
}

int send_packet_city_change(
        connection_t *pconn,
        const struct packet_city_change *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_change from the server.");
  }
  send_ensure_valid_variant_packet_city_change(pconn);

  switch(pconn->phs.variant[PACKET_CITY_CHANGE]) {
    case 100: {
      return send_packet_city_change_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_change(
         connection_t *pconn,
         int city_id, int
         build_id, bool is_build_id_unit_id)
{
  struct packet_city_change packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->build_id = build_id;
  real_packet->is_build_id_unit_id = is_build_id_unit_id;

  return send_packet_city_change(pconn, real_packet);
}

/* 26 cs */
#define hash_packet_city_worklist_100 hash_const

#define cmp_packet_city_worklist_100 cmp_const

BV_DEFINE(packet_city_worklist_100_fields, 2);

static int send_packet_city_worklist_100(
               connection_t *pconn,
               const struct packet_city_worklist *packet)
{
  const struct packet_city_worklist *real_packet = packet;
  packet_city_worklist_100_fields fields;
  struct packet_city_worklist *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_WORKLIST];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_WORKLIST);
# ifdef REPLAY_2
  printf("cs opc=26 CITY_WORKLIST");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_worklist_100,
                     cmp_packet_city_worklist_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = !are_worklists_equal(&old->worklist, &real_packet->worklist);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    int length = worklist_length(&real_packet->worklist);
# ifdef REPLAY_2
    int i;
# endif

    dio_put_worklist(&dout, &real_packet->worklist);
# ifdef REPLAY_2
    printf(" worklist={ length=%d", length);
    for (i=0; i < length; i++) {
      printf("(%d %d)",
             real_packet->worklist.wlefs[i],
             real_packet->worklist.wlids[i]);
    }
    printf(" }");
  }
  printf("\n");
# endif

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_worklist(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_WORKLIST] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_WORKLIST] = variant;
}

int send_packet_city_worklist(
        connection_t *pconn,
        const struct packet_city_worklist *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_worklist from the server.");
  }
  send_ensure_valid_variant_packet_city_worklist(pconn);

  switch(pconn->phs.variant[PACKET_CITY_WORKLIST]) {
    case 100: {
      return send_packet_city_worklist_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_worklist(
         connection_t *pconn,
         int city_id, struct
         worklist *worklist)
{
  struct packet_city_worklist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  copy_worklist(&real_packet->worklist, worklist);

  return send_packet_city_worklist(pconn, real_packet);
}

/* 27 cs */
#define hash_packet_city_make_specialist_100 hash_const

#define cmp_packet_city_make_specialist_100 cmp_const

BV_DEFINE(packet_city_make_specialist_100_fields, 3);

static int send_packet_city_make_specialist_100(
               connection_t *pconn,
               const struct packet_city_make_specialist *packet)
{
  const struct packet_city_make_specialist *real_packet = packet;
  packet_city_make_specialist_100_fields fields;
  struct packet_city_make_specialist *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_MAKE_SPECIALIST];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_MAKE_SPECIALIST);
# ifdef REPLAY_2
  printf("cs opc=27 CITY_MAKE_SPECIALIST");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_make_specialist_100,
                     cmp_packet_city_make_specialist_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->worker_x != real_packet->worker_x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->worker_y != real_packet->worker_y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->worker_x);
# ifdef REPLAY_2
    printf(" worker_x=%u", real_packet->worker_x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->worker_y);
# ifdef REPLAY_2
    printf(" worker_y=%u\n", real_packet->worker_y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_make_specialist(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_MAKE_SPECIALIST] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_MAKE_SPECIALIST] = variant;
}

int send_packet_city_make_specialist(
        connection_t *pconn,
        const struct packet_city_make_specialist *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_make_specialist from the server.");
  }
  send_ensure_valid_variant_packet_city_make_specialist(pconn);

  switch(pconn->phs.variant[PACKET_CITY_MAKE_SPECIALIST]) {
    case 100: {
      return send_packet_city_make_specialist_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_make_specialist(
         connection_t *pconn,
         int city_id,
         int worker_x,
         int worker_y)
{
  struct packet_city_make_specialist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->worker_x = worker_x;
  real_packet->worker_y = worker_y;

  return send_packet_city_make_specialist(pconn, real_packet);
}

/* 28 cs */
#define hash_packet_city_make_worker_100 hash_const

#define cmp_packet_city_make_worker_100 cmp_const

BV_DEFINE(packet_city_make_worker_100_fields, 3);

static int send_packet_city_make_worker_100(
               connection_t *pconn,
               const struct packet_city_make_worker *packet)
{
  const struct packet_city_make_worker *real_packet = packet;
  packet_city_make_worker_100_fields fields;
  struct packet_city_make_worker *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_MAKE_WORKER];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_MAKE_WORKER);
# ifdef REPLAY_2
  printf("cs opc=28 CITY_MAKE_WORKER");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_make_worker_100,
                     cmp_packet_city_make_worker_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->worker_x != real_packet->worker_x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->worker_y != real_packet->worker_y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->worker_x);
# ifdef REPLAY_2
    printf(" worker_x=%u", real_packet->worker_x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->worker_y);
# ifdef REPLAY_2
    printf(" worker_y=%u\n", real_packet->worker_y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_make_worker(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_MAKE_WORKER] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_MAKE_WORKER] = variant;
}

int send_packet_city_make_worker(
        connection_t *pconn,
        const struct packet_city_make_worker *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_make_worker from the server.");
  }
  send_ensure_valid_variant_packet_city_make_worker(pconn);

  switch(pconn->phs.variant[PACKET_CITY_MAKE_WORKER]) {
    case 100: {
      return send_packet_city_make_worker_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_make_worker(
         connection_t *pconn,
         int city_id,
         int worker_x,
         int worker_y)
{
  struct packet_city_make_worker packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->worker_x = worker_x;
  real_packet->worker_y = worker_y;

  return send_packet_city_make_worker(pconn, real_packet);
}

/* 29 cs */
#define hash_packet_city_change_specialist_100 hash_const

#define cmp_packet_city_change_specialist_100 cmp_const

BV_DEFINE(packet_city_change_specialist_100_fields, 3);

static int send_packet_city_change_specialist_100(
               connection_t *pconn,
               const struct packet_city_change_specialist *packet)
{
  const struct packet_city_change_specialist *real_packet = packet;
  packet_city_change_specialist_100_fields fields;
  struct packet_city_change_specialist *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_CHANGE_SPECIALIST];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_CHANGE_SPECIALIST);
# ifdef REPLAY_2
  printf("cs opc=29 CITY_CHANGE_SPECIALIST");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_change_specialist_100,
                     cmp_packet_city_change_specialist_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->from != real_packet->from);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->to != real_packet->to);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->from);
# ifdef REPLAY_2
    printf(" from=%u", real_packet->from);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->to);
# ifdef REPLAY_2
    printf(" to=%u\n", real_packet->to);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_change_specialist(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_CHANGE_SPECIALIST] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_CHANGE_SPECIALIST] = variant;
}

int send_packet_city_change_specialist(
        connection_t *pconn,
        const struct packet_city_change_specialist *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_change_specialist from the server.");
  }
  send_ensure_valid_variant_packet_city_change_specialist(pconn);

  switch(pconn->phs.variant[PACKET_CITY_CHANGE_SPECIALIST]) {
    case 100: {
      return send_packet_city_change_specialist_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_change_specialist(connection_t *pconn,
                                        int city_id,
                                        Specialist_type_id from,
                                        Specialist_type_id to)
{
  struct packet_city_change_specialist packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->from = from;
  real_packet->to = to;

  return send_packet_city_change_specialist(pconn, real_packet);
}

/* 30 cs */
#define hash_packet_city_rename_100 hash_const

#define cmp_packet_city_rename_100 cmp_const

BV_DEFINE(packet_city_rename_100_fields, 2);

static int send_packet_city_rename_100(
               connection_t *pconn,
               const struct packet_city_rename *packet)
{
  const struct packet_city_rename *real_packet = packet;
  packet_city_rename_100_fields fields;
  struct packet_city_rename *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_RENAME];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_RENAME);
# ifdef REPLAY_2
  printf("cs opc=30 CITY_RENAME");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_rename_100,
                     cmp_packet_city_rename_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (strcmp(old->name, real_packet->name) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_string(&dout, real_packet->name);
# ifdef REPLAY_2
    printf(" name=%s\n", real_packet->name);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_rename(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_RENAME] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_RENAME] = variant;
}

int send_packet_city_rename(
        connection_t *pconn,
        const struct packet_city_rename *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_rename from the server.");
  }
  send_ensure_valid_variant_packet_city_rename(pconn);

  switch(pconn->phs.variant[PACKET_CITY_RENAME]) {
    case 100: {
      return send_packet_city_rename_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_rename(
         connection_t *pconn,
         int city_id,
         const char *name)
{
  struct packet_city_rename packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  sz_strlcpy(real_packet->name, name);

  return send_packet_city_rename(pconn, real_packet);
}

/* 31 cs */
#define hash_packet_city_options_req_100 hash_const

#define cmp_packet_city_options_req_100 cmp_const

BV_DEFINE(packet_city_options_req_100_fields, 2);

static int send_packet_city_options_req_100(
               connection_t *pconn,
               const struct packet_city_options_req *packet)
{
  const struct packet_city_options_req *real_packet = packet;
  packet_city_options_req_100_fields fields;
  struct packet_city_options_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_OPTIONS_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_OPTIONS_REQ);
# ifdef REPLAY_2
  printf("cs opc=31 CITY_OPTIONS_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_options_req_100,
                     cmp_packet_city_options_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->value != real_packet->value);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u", real_packet->city_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->value);
# ifdef REPLAY_2
    printf(" value=%u\n", real_packet->value);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_options_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_OPTIONS_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_OPTIONS_REQ] = variant;
}

int send_packet_city_options_req(
        connection_t *pconn,
        const struct packet_city_options_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_options_req from the server.");
  }
  send_ensure_valid_variant_packet_city_options_req(pconn);

  switch(pconn->phs.variant[PACKET_CITY_OPTIONS_REQ]) {
    case 100: {
      return send_packet_city_options_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_options_req(
         connection_t *pconn,
         int city_id,
         int value)
{
  struct packet_city_options_req packet, *real_packet = &packet;

  real_packet->city_id = city_id;
  real_packet->value = value;

  return send_packet_city_options_req(pconn, real_packet);
}

/* 32 cs */
#define hash_packet_city_refresh_100 hash_const

#define cmp_packet_city_refresh_100 cmp_const

BV_DEFINE(packet_city_refresh_100_fields, 1);

static int send_packet_city_refresh_100(
               connection_t *pconn,
               const struct packet_city_refresh *packet)
{
  const struct packet_city_refresh *real_packet = packet;
  packet_city_refresh_100_fields fields;
  struct packet_city_refresh *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_REFRESH];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_REFRESH);
# ifdef REPLAY_2
  printf("cs opc=32 CITY_REFRESH");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_refresh_100,
                     cmp_packet_city_refresh_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u\n", real_packet->city_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_refresh(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_REFRESH] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_REFRESH] = variant;
}

int send_packet_city_refresh(
        connection_t *pconn,
        const struct packet_city_refresh *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_refresh from the server.");
  }
  send_ensure_valid_variant_packet_city_refresh(pconn);

  switch(pconn->phs.variant[PACKET_CITY_REFRESH]) {
    case 100: {
      return send_packet_city_refresh_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_refresh(connection_t *pconn, int city_id)
{
  struct packet_city_refresh packet, *real_packet = &packet;

  real_packet->city_id = city_id;

  return send_packet_city_refresh(pconn, real_packet);
}

/* 33 cs */
#define hash_packet_city_incite_inq_100 hash_const

#define cmp_packet_city_incite_inq_100 cmp_const

BV_DEFINE(packet_city_incite_inq_100_fields, 1);

static int send_packet_city_incite_inq_100(
               connection_t *pconn,
               const struct packet_city_incite_inq *packet)
{
  const struct packet_city_incite_inq *real_packet = packet;
  packet_city_incite_inq_100_fields fields;
  struct packet_city_incite_inq *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_INCITE_INQ];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_INCITE_INQ);
# ifdef REPLAY_2
  printf("cs opc=33 CITY_INCITE_INQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_incite_inq_100,
                     cmp_packet_city_incite_inq_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u\n", real_packet->city_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_incite_inq(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_INCITE_INQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_INCITE_INQ] = variant;
}

int send_packet_city_incite_inq(
        connection_t *pconn,
        const struct packet_city_incite_inq *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_incite_inq from the server.");
  }
  send_ensure_valid_variant_packet_city_incite_inq(pconn);

  switch(pconn->phs.variant[PACKET_CITY_INCITE_INQ]) {
    case 100: {
      return send_packet_city_incite_inq_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_incite_inq(connection_t *pconn, int city_id)
{
  struct packet_city_incite_inq packet, *real_packet = &packet;

  real_packet->city_id = city_id;

  return send_packet_city_incite_inq(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=34 CITY_INCITE_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" city_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cost = readin;
#   ifdef REPLAY_2
    printf(" cost=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_city_name_suggestion_req_100 hash_const

#define cmp_packet_city_name_suggestion_req_100 cmp_const

BV_DEFINE(packet_city_name_suggestion_req_100_fields, 1);

static int send_packet_city_name_suggestion_req_100(
               connection_t *pconn,
               const struct packet_city_name_suggestion_req *packet)
{
  const struct packet_city_name_suggestion_req *real_packet = packet;
  packet_city_name_suggestion_req_100_fields fields;
  struct packet_city_name_suggestion_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_NAME_SUGGESTION_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_NAME_SUGGESTION_REQ);
# ifdef REPLAY_2
  printf("cs op=35 CITY_NAME_SUGGESTION_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_name_suggestion_req_100,
                     cmp_packet_city_name_suggestion_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_name_suggestion_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ] = variant;
}

int send_packet_city_name_suggestion_req(
        connection_t *pconn,
        const struct packet_city_name_suggestion_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_name_suggestion_req from the server.");
  }
  send_ensure_valid_variant_packet_city_name_suggestion_req(pconn);

  switch(pconn->phs.variant[PACKET_CITY_NAME_SUGGESTION_REQ]) {
    case 100: {
      return send_packet_city_name_suggestion_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_name_suggestion_req(connection_t *pconn, int unit_id)
{
  struct packet_city_name_suggestion_req packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_city_name_suggestion_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=36 CITY_NAME_SUGGESTION_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" unit_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=%s\n", real_packet->name);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=37 CITY_SABOTAGE_LIST");
# endif
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
#   ifdef REPLAY_2
    printf(" diplomat_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city_id = readin;
#   ifdef REPLAY_2
    printf(" city_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->improvements, sizeof(real_packet->improvements));
#   ifdef REPLAY_2
    printf(" improvements[%d]=[", B_LAST);
    for (i=0; i < B_LAST; i++ ) {
      printf("%d", real_packet->improvements[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=38 PLAYER_REMOVE");
# endif
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
#   ifdef REPLAY_2
    printf(" player_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=39 PLAYER_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
#   ifdef REPLAY_2
    printf(" username=\"%s\"", real_packet->username);
#   endif
  }
  real_packet->is_male = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" is_male=%u", real_packet->is_male);
# endif

  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
#   ifdef REPLAY_2
    printf(" government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
#   ifdef REPLAY_2
    printf(" target_government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
#   ifdef REPLAY_2
    printf(" embassy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
#   ifdef REPLAY_2
    printf(" city_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
#   ifdef REPLAY_2
    printf(" nation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  real_packet->turn_done = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" turn_done=%d", real_packet->turn_done);
# endif

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
#   ifdef REPLAY_2
    printf(" nturns_idle=%d", readin);
#   endif
  }
  real_packet->is_alive = BV_ISSET(fields, 11);
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
#   ifdef REPLAY_2
    printf(" reputation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int i;

#   ifdef REPLAY_2
    printf(" diplstates[%d+%d]={", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
#     ifdef REPLAY_2
      printf("[%d %d %d %d] ",
             real_packet->diplstates[i].type,
             real_packet->diplstates[i].turns_left,
             real_packet->diplstates[i].has_reason_to_cancel,
             real_packet->diplstates[i].contact_turns_left);
      if ((i % 10) == 9)
        printf("\n");
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
#   ifdef REPLAY_2
    printf(" gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
#   ifdef REPLAY_2
    printf(" tax=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
#   ifdef REPLAY_2
    printf(" science=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
#   ifdef REPLAY_2
    printf(" luxury=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
#   ifdef REPLAY_2
    printf(" bulbs_last_turn=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
#   ifdef REPLAY_2
    printf(" bulbs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
#   ifdef REPLAY_2
    printf(" techs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
#   ifdef REPLAY_2
    printf(" researching=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->researching_cost = readin;
#   ifdef REPLAY_2
    printf(" researching_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
#   ifdef REPLAY_2
    printf(" future_tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
#   ifdef REPLAY_2
    printf(" tech_goal=%u", readin);
#   endif
  }
  real_packet->is_connected = BV_ISSET(fields, 25);
# ifdef REPLAY_2
  printf(" is_connected=%u", real_packet->is_connected);
# endif

  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
#   ifdef REPLAY_2
    printf(" revolution_finishes=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 27);
# ifdef REPLAY_2
  printf(" ai=%u", real_packet->ai);
# endif

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
#   ifdef REPLAY_2
    printf(" barbarian_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
#   ifdef REPLAY_2
    printf(" gives_shared_vision=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 30)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
#   ifdef REPLAY_2
    printf(" inventions[%d]=[", B_LAST);
    for (i = 0; i < B_LAST; i++ ) {
      printf("%d", real_packet->inventions[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
#   endif
  }
  if (BV_ISSET(fields, 31)) {
    int i;

#   ifdef REPLAY_2
    printf(" love[]=");
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=39 PLAYER_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
#   ifdef REPLAY_2
    printf(" username=\"%s\"", real_packet->username);
#   endif
  }
  real_packet->is_male = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" is_male=%u", real_packet->is_male);
# endif

  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
#   ifdef REPLAY_2
    printf(" government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
#   ifdef REPLAY_2
    printf(" target_government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
#   ifdef REPLAY_2
    printf(" embassy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
#   ifdef REPLAY_2
    printf(" city_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
#   ifdef REPLAY_2
    printf(" nation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  real_packet->turn_done = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" turn_done=%d", real_packet->turn_done);
# endif

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
#   ifdef REPLAY_2
    printf(" nturns_idle=%d", readin);
#   endif
  }
  real_packet->is_alive = BV_ISSET(fields, 11);
# ifdef REPLAY_2
  printf(" is_alive=%d", real_packet->is_alive);
# endif

  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
#   ifdef REPLAY_2
    printf(" reputation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int i;

#   ifdef REPLAY_2
    printf(" diplstates[%d+%d]={", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
#     ifdef REPLAY_2
      printf("[%d %d %d %d] ",
             real_packet->diplstates[i].type,
             real_packet->diplstates[i].turns_left,
             real_packet->diplstates[i].has_reason_to_cancel,
             real_packet->diplstates[i].contact_turns_left);
      if ((i % 10) == 9)
        printf("\n");
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
#   ifdef REPLAY_2
    printf(" gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
#   ifdef REPLAY_2
    printf(" tax=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
#   ifdef REPLAY_2
    printf(" science=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
#   ifdef REPLAY_2
    printf(" luxury=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
#   ifdef REPLAY_2
    printf(" bulbs_last_turn=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
#   ifdef REPLAY_2
    printf(" bulbs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
#   ifdef REPLAY_2
    printf(" techs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
#   ifdef REPLAY_2
    printf(" researching=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
#   ifdef REPLAY_2
    printf(" future_tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
#   ifdef REPLAY_2
    printf(" tech_goal=%u", readin);
#   endif
  }
  real_packet->is_connected = BV_ISSET(fields, 24);
# ifdef REPLAY_2
  printf(" is_connected=%u", real_packet->is_connected);
# endif

  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
#   ifdef REPLAY_2
    printf(" revolution_finishes=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 26);
# ifdef REPLAY_2
  printf(" ai=%u", real_packet->ai);
# endif

  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
#   ifdef REPLAY_2
    printf(" barbarian_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
#   ifdef REPLAY_2
    printf(" gives_shared_vision=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
#   ifdef REPLAY_2
    printf(" inventions[%d]=[", B_LAST);
    for (i = 0; i < B_LAST; i++ ) {
      printf("%d", real_packet->inventions[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
#   endif
  }
  if (BV_ISSET(fields, 30)) {
    int i;

#   ifdef REPLAY_2
    printf(" love[]=");
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=39 PLAYER_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  real_packet->is_male = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" is_male=%u", real_packet->is_male);
# endif

  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
#   ifdef REPLAY_2
    printf(" government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
#   ifdef REPLAY_2
    printf(" target_government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
#   ifdef REPLAY_2
    printf(" embassy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
#   ifdef REPLAY_2
    printf(" city_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
#   ifdef REPLAY_2
    printf(" nation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  real_packet->turn_done = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" turn_done=%d", real_packet->turn_done);
# endif

  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
#   ifdef REPLAY_2
    printf(" nturns_idle=%d", readin);
#   endif
  }
  real_packet->is_alive = BV_ISSET(fields, 10);
# ifdef REPLAY_2
  printf(" is_alive=%d", real_packet->is_alive);
# endif

  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
#   ifdef REPLAY_2
    printf(" reputation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int i;

#   ifdef REPLAY_2
    printf(" diplstates[%d+%d]={", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
#     ifdef REPLAY_2
      printf("[%d %d %d %d] ",
             real_packet->diplstates[i].type,
             real_packet->diplstates[i].turns_left,
             real_packet->diplstates[i].has_reason_to_cancel,
             real_packet->diplstates[i].contact_turns_left);
      if ((i % 10) == 9)
        printf("\n");
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
#   ifdef REPLAY_2
    printf(" gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
#   ifdef REPLAY_2
    printf(" tax=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
#   ifdef REPLAY_2
    printf(" science=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
#   ifdef REPLAY_2
    printf(" luxury=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
#   ifdef REPLAY_2
    printf(" bulbs_last_turn=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
#   ifdef REPLAY_2
    printf(" bulbs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
#   ifdef REPLAY_2
    printf(" techs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
#   ifdef REPLAY_2
    printf(" researching=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->researching_cost = readin;
#   ifdef REPLAY_2
    printf(" researching_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
#   ifdef REPLAY_2
    printf(" future_tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
#   ifdef REPLAY_2
    printf(" tech_goal=%u", readin);
#   endif
  }
  real_packet->is_connected = BV_ISSET(fields, 24);
# ifdef REPLAY_2
  printf(" is_connected=%u", real_packet->is_connected);
# endif

  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
#   ifdef REPLAY_2
    printf(" revolution_finishes=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 26);
# ifdef REPLAY_2
  printf(" ai=%u", real_packet->ai);
# endif

  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
#   ifdef REPLAY_2
    printf(" barbarian_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
#   ifdef REPLAY_2
    printf(" gives_shared_vision=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
#   ifdef REPLAY_2
    printf(" inventions[%d]=[", B_LAST);
    for (i = 0; i < B_LAST; i++ ) {
      printf("%d", real_packet->inventions[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
#   endif
  }
  if (BV_ISSET(fields, 30)) {
    int i;

#   ifdef REPLAY_2
    printf(" love[]=");
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=39 PLAYER_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  real_packet->is_male = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" is_male=%u", real_packet->is_male);
# endif

  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government = readin;
#   ifdef REPLAY_2
    printf(" government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->target_government = readin;
#   ifdef REPLAY_2
    printf(" target_government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->embassy = readin;
#   ifdef REPLAY_2
    printf(" embassy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
#   ifdef REPLAY_2
    printf(" city_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
#   ifdef REPLAY_2
    printf(" nation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->team = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  real_packet->turn_done = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" turn_done=%d", real_packet->turn_done);
# endif

  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->nturns_idle = readin;
#   ifdef REPLAY_2
    printf(" nturns_idle=%d", readin);
#   endif
  }
  real_packet->is_alive = BV_ISSET(fields, 10);
# ifdef REPLAY_2
  printf(" is_alive=%d", real_packet->is_alive);
# endif

  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->reputation = readin;
#   ifdef REPLAY_2
    printf(" reputation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int i;

#   ifdef REPLAY_2
    printf(" diplstates[%d+%d]={", MAX_NUM_PLAYERS, MAX_NUM_BARBARIANS);
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      dio_get_diplstate(&din, &real_packet->diplstates[i]);
#     ifdef REPLAY_2
      printf("[%d %d %d %d] ",
             real_packet->diplstates[i].type,
             real_packet->diplstates[i].turns_left,
             real_packet->diplstates[i].has_reason_to_cancel,
             real_packet->diplstates[i].contact_turns_left);
      if ((i % 10) == 9)
        printf("\n");
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gold = readin;
#   ifdef REPLAY_2
    printf(" gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tax = readin;
#   ifdef REPLAY_2
    printf(" tax=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->science = readin;
#   ifdef REPLAY_2
    printf(" science=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->luxury = readin;
#   ifdef REPLAY_2
    printf(" luxury=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->bulbs_last_turn = readin;
#   ifdef REPLAY_2
    printf(" bulbs_last_turn=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->bulbs_researched = readin;
#   ifdef REPLAY_2
    printf(" bulbs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->techs_researched = readin;
#   ifdef REPLAY_2
    printf(" techs_researched=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->researching = readin;
#   ifdef REPLAY_2
    printf(" researching=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->future_tech = readin;
#   ifdef REPLAY_2
    printf(" future_tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_goal = readin;
#   ifdef REPLAY_2
    printf(" tech_goal=%u", readin);
#   endif
  }
  real_packet->is_connected = BV_ISSET(fields, 23);
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->revolution_finishes = readin;
#   ifdef REPLAY_2
    printf(" revolution_finishes=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 25);
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->barbarian_type = readin;
#   ifdef REPLAY_2
    printf(" barbarian_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->gives_shared_vision = readin;
#   ifdef REPLAY_2
    printf(" gives_shared_vision=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->inventions, sizeof(real_packet->inventions));
#   ifdef REPLAY_2
    printf(" inventions[%d]=[", B_LAST);
    for (i = 0; i < B_LAST; i++ ) {
      printf("%d", real_packet->inventions[i]);
      if ((i % 10) == 9) printf("\n");
    }
    printf("]\n");
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int i;

#   ifdef REPLAY_2
    printf(" love[]=");
#   endif
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->love[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
static int send_packet_player_turn_done_100(connection_t *pconn)
{
  SEND_PACKET_START(PACKET_PLAYER_TURN_DONE);
  printf("cs opc=40 PLAYER_TURN_DONE\n");
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_turn_done(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_TURN_DONE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_TURN_DONE] = variant;
}

int send_packet_player_turn_done(connection_t *pconn)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_turn_done from the server.");
  }
  send_ensure_valid_variant_packet_player_turn_done(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_TURN_DONE]) {
    case 100: {
      return send_packet_player_turn_done_100(pconn);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 41 cs */
#define hash_packet_player_rates_100 hash_const

#define cmp_packet_player_rates_100 cmp_const

BV_DEFINE(packet_player_rates_100_fields, 3);

static int send_packet_player_rates_100(
               connection_t *pconn,
               const struct packet_player_rates *packet)
{
  const struct packet_player_rates *real_packet = packet;
  packet_player_rates_100_fields fields;
  struct packet_player_rates *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_PLAYER_RATES];
  int different = 0;

  SEND_PACKET_START(PACKET_PLAYER_RATES);
# ifdef REPLAY_2
  printf("cs opc=41 PLAYER_RATES");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_player_rates_100,
                     cmp_packet_player_rates_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->tax != real_packet->tax);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->luxury != real_packet->luxury);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->science != real_packet->science);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->tax);
# ifdef REPLAY_2
    printf(" tax=%u", real_packet->tax);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->luxury);
# ifdef REPLAY_2
    printf(" luxury=%u", real_packet->luxury);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->science);
# ifdef REPLAY_2
    printf(" science=%u\n", real_packet->science);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_rates(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_RATES] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_RATES] = variant;
}

int send_packet_player_rates(
        connection_t *pconn,
        const struct packet_player_rates *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_rates from the server.");
  }
  send_ensure_valid_variant_packet_player_rates(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_RATES]) {
    case 100: {
      return send_packet_player_rates_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_player_rates(
         connection_t *pconn,
         int tax,
         int luxury,
         int science)
{
  struct packet_player_rates packet, *real_packet = &packet;

  real_packet->tax = tax;
  real_packet->luxury = luxury;
  real_packet->science = science;

  return send_packet_player_rates(pconn, real_packet);
}

/* 42 */
/* 43 cs */
#define hash_packet_player_change_government_100 hash_const

#define cmp_packet_player_change_government_100 cmp_const

BV_DEFINE(packet_player_change_government_100_fields, 1);

static int send_packet_player_change_government_100(
               connection_t *pconn,
               const struct packet_player_change_government *packet)
{
  const struct packet_player_change_government *real_packet = packet;
  packet_player_change_government_100_fields fields;
  struct packet_player_change_government *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_PLAYER_CHANGE_GOVERNMENT];
  int different = 0;

  SEND_PACKET_START(PACKET_PLAYER_CHANGE_GOVERNMENT);
# ifdef REPLAY_2
  printf("cs opc=43 PLAYER_CHANGE_GOVERNMENT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_player_change_government_100,
                     cmp_packet_player_change_government_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->government != real_packet->government);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->government);
# ifdef REPLAY_2
    printf(" government=%u\n", real_packet->government);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_change_government(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT] = variant;
}

int send_packet_player_change_government(
               connection_t *pconn,
               const struct packet_player_change_government *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_change_government from the server.");
  }
  send_ensure_valid_variant_packet_player_change_government(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_CHANGE_GOVERNMENT]) {
    case 100: {
      return send_packet_player_change_government_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_player_change_government(
         connection_t *pconn,
         int government)
{
  struct packet_player_change_government packet, *real_packet = &packet;

  real_packet->government = government;

  return send_packet_player_change_government(pconn, real_packet);
}

/* 44 cs */
#define hash_packet_player_research_100 hash_const

#define cmp_packet_player_research_100 cmp_const

BV_DEFINE(packet_player_research_100_fields, 1);

static int send_packet_player_research_100(
               connection_t *pconn,
               const struct packet_player_research *packet)
{
  const struct packet_player_research *real_packet = packet;
  packet_player_research_100_fields fields;
  struct packet_player_research *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_PLAYER_RESEARCH];
  int different = 0;

  SEND_PACKET_START(PACKET_PLAYER_RESEARCH);
# ifdef REPLAY_2
  printf("cs opc=44 PLAYER_RESEARCH");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_player_research_100,
                     cmp_packet_player_research_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->tech != real_packet->tech);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->tech);
# ifdef REPLAY_2
    printf(" tech=%u\n", real_packet->tech);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void ensure_valid_variant_packet_player_research(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_RESEARCH] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_RESEARCH] = variant;
}

int send_packet_player_research(
        connection_t *pconn,
        const struct packet_player_research *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_research from the server.");
  }
  ensure_valid_variant_packet_player_research(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_RESEARCH]) {
    case 100: {
      return send_packet_player_research_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_player_research(connection_t *pconn, int tech)
{
  struct packet_player_research packet, *real_packet = &packet;

  real_packet->tech = tech;

  return send_packet_player_research(pconn, real_packet);
}

/* 45 cs */
#define hash_packet_player_tech_goal_100 hash_const

#define cmp_packet_player_tech_goal_100 cmp_const

BV_DEFINE(packet_player_tech_goal_100_fields, 1);

static int send_packet_player_tech_goal_100(
               connection_t *pconn,
               const struct packet_player_tech_goal *packet)
{
  const struct packet_player_tech_goal *real_packet = packet;
  packet_player_tech_goal_100_fields fields;
  struct packet_player_tech_goal *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_PLAYER_TECH_GOAL];
  int different = 0;

  SEND_PACKET_START(PACKET_PLAYER_TECH_GOAL);
# ifdef REPLAY_2
  printf("cs opc=45 PLAYER_TECH_GOAL");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_player_tech_goal_100,
                     cmp_packet_player_tech_goal_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->tech != real_packet->tech);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->tech);
# ifdef REPLAY_2
    printf("tech=%u\n", real_packet->tech);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_tech_goal(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_TECH_GOAL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_TECH_GOAL] = variant;
}

int send_packet_player_tech_goal(
        connection_t *pconn,
        const struct packet_player_tech_goal *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_tech_goal from the server.");
  }
  send_ensure_valid_variant_packet_player_tech_goal(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_TECH_GOAL]) {
    case 100: {
      return send_packet_player_tech_goal_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_player_tech_goal(connection_t *pconn, int tech)
{
  struct packet_player_tech_goal packet, *real_packet = &packet;

  real_packet->tech = tech;

  return send_packet_player_tech_goal(pconn, real_packet);
}

/* 46 cs */
static int send_packet_player_attribute_block_100(connection_t *pconn)
{
  SEND_PACKET_START(PACKET_PLAYER_ATTRIBUTE_BLOCK);
# ifdef REPLAY_2
  printf("cs opc=46 PLAYER_ATTRIBUTE_BLOCK\n");
# endif
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_attribute_block(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK] = variant;
}

int send_packet_player_attribute_block(connection_t *pconn)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_attribute_block from the server.");
  }
  send_ensure_valid_variant_packet_player_attribute_block(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_ATTRIBUTE_BLOCK]) {
    case 100: {
      return send_packet_player_attribute_block_100(pconn);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

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
# ifdef REPLAY_2
  printf("sc op=48 UNIT_REMOVE");
# endif
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
#   ifdef REPLAY_2
    printf(" unit_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=49 UNIT_INFO");
# endif
  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
# ifdef REPLAY_2
  printf(" id=%u", readin);
# endif

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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->homecity = readin;
#   ifdef REPLAY_2
    printf(" homecity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
#   ifdef REPLAY_2
    printf(" veteran=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" ai=%u", real_packet->ai);
# endif

  real_packet->paradropped = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" paradropped=%u", real_packet->paradropped);
# endif

  real_packet->connecting = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" connecting=%u", real_packet->connecting);
# endif

  real_packet->transported = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" transported=%u", real_packet->transported);
# endif

  real_packet->done_moving = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" done_moving=%u", real_packet->done_moving);
# endif

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movesleft = readin;
#   ifdef REPLAY_2
    printf(" movesleft=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
#   ifdef REPLAY_2
    printf(" hp=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
#   ifdef REPLAY_2
    printf(" fuel=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
#   ifdef REPLAY_2
    printf(" activity_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappiness = readin;
#   ifdef REPLAY_2
    printf(" unhappiness=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
#   ifdef REPLAY_2
    printf(" upkeep=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_food = readin;
#   ifdef REPLAY_2
    printf(" upkeep_food=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_gold = readin;
#   ifdef REPLAY_2
    printf(" upkeep_gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->occupy = readin;
#   ifdef REPLAY_2
    printf(" occupy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_x = readin;
#   ifdef REPLAY_2
    printf(" goto_dest_x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_y = readin;
#   ifdef REPLAY_2
    printf(" goto_dest_y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->air_patrol_x = readin;
#   ifdef REPLAY_2
    printf(" air_patrol_x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->air_patrol_y = readin;
#   ifdef REPLAY_2
    printf(" air_patrol_y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
#   ifdef REPLAY_2
    printf(" activity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->activity_target = readin;
#   ifdef REPLAY_2
    printf(" activity_target=%u", readin);
#   endif
  }
  real_packet->has_orders = BV_ISSET(fields, 27);
# ifdef REPLAY_2
  printf(" has_orders=%u", real_packet->has_orders);
# endif

  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_length = readin;
#   ifdef REPLAY_2
    printf(" orders_length=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_index = readin;
#   ifdef REPLAY_2
    printf(" orders_index=%u", readin);
#   endif
  }
  real_packet->orders_repeat = BV_ISSET(fields, 30);
# ifdef REPLAY_2
  printf(" orders_repeat=%u", real_packet->orders_repeat);
# endif

  real_packet->orders_vigilant = BV_ISSET(fields, 31);
# ifdef REPLAY_2
  printf(" orders_vigilant=%u", real_packet->orders_vigilant);
# endif

  if (BV_ISSET(fields, 32)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
#   ifdef REPLAY_2
    printf(" orders[%u]=", real_packet->orders_length);
#   endif
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 33)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
#   ifdef REPLAY_2
    printf(" orders_dirs[%u]=", real_packet->orders_length);
#   endif
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_dirs[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 34)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
#   ifdef REPLAY_2
    printf(" orders_activities[%u]=", real_packet->orders_length);
#   endif
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_activities[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=49 UNIT_INFO");
# endif
  DIO_BV_GET(&din, fields);

  dio_get_uint16(&din, &readin);
  real_packet->id = readin;
# ifdef REPLAY_2
  printf(" id=%u", readin);
# endif

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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->homecity = readin;
#   ifdef REPLAY_2
    printf(" homecity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
#   ifdef REPLAY_2
    printf(" veteran=%u", readin);
#   endif
  }
  real_packet->ai = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" ai=%u", real_packet->ai);
# endif

  real_packet->paradropped = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" paradropped=%u", real_packet->paradropped);
# endif

  real_packet->connecting = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" connecting=%u", real_packet->connecting);
# endif

  real_packet->transported = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" transported=%u", real_packet->transported);
# endif

  real_packet->done_moving = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" done_moving=%u", real_packet->done_moving);
# endif

  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
#   ifdef REPLAY_2
    printf(" team=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movesleft = readin;
#   ifdef REPLAY_2
    printf(" movesleft=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
#   ifdef REPLAY_2
    printf(" hp=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
#   ifdef REPLAY_2
    printf(" fuel=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity_count = readin;
#   ifdef REPLAY_2
    printf(" activity_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unhappiness = readin;
#   ifdef REPLAY_2
    printf(" unhappiness=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
#   ifdef REPLAY_2
    printf(" upkeep=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_food = readin;
#   ifdef REPLAY_2
    printf(" upkeep_food=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep_gold = readin;
#   ifdef REPLAY_2
    printf(" upkeep_gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->occupy = readin;
#   ifdef REPLAY_2
    printf(" occupy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_x = readin;
#   ifdef REPLAY_2
    printf(" goto_dest_x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->goto_dest_y = readin;
#   ifdef REPLAY_2
    printf(" goto_dest_y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
#   ifdef REPLAY_2
    printf(" activity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->activity_target = readin;
#   ifdef REPLAY_2
    printf(" activity_target=%u", readin);
#   endif
  }
  real_packet->has_orders = BV_ISSET(fields, 25);
# ifdef REPLAY_2
  printf(" has_orders=%u", real_packet->has_orders);
# endif

  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_length = readin;
#   ifdef REPLAY_2
    printf(" orders_length=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->orders_index = readin;
#   ifdef REPLAY_2
    printf(" orders_index=%u", readin);
#   endif
  }
  real_packet->orders_repeat = BV_ISSET(fields, 28);
# ifdef REPLAY_2
  printf(" orders_repeat=%u", real_packet->orders_repeat);
# endif

  real_packet->orders_vigilant = BV_ISSET(fields, 29);
# ifdef REPLAY_2
  printf(" orders_vigilant=%u", real_packet->orders_vigilant);
# endif

  if (BV_ISSET(fields, 30)) {
    int i;

#   ifdef REPLAY_2
    printf(" orders[%u]=", real_packet->orders_length);
#   endif
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 31)) {
    int i;

#   ifdef REPLAY_2
    printf(" orders_dirs[%u]=", real_packet->orders_length);
#   endif
    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_dirs[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 32)) {
    int i;

    if(real_packet->orders_length > MAX_LEN_ROUTE) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->orders_length = MAX_LEN_ROUTE;
    }
#   ifdef REPLAY_2
    printf(" orders_activities[%u]=", real_packet->orders_length);
#   endif
    for (i = 0; i < real_packet->orders_length; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->orders_activities[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=50 UNIT_SHORT_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" owner=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->x = readin;
#   ifdef REPLAY_2
    printf(" x=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->y = readin;
#   ifdef REPLAY_2
    printf(" y=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->veteran = readin;
#   ifdef REPLAY_2
    printf(" veteran=%u", readin);
#   endif
  }
  real_packet->occupied = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" occupied=%u", real_packet->occupied);
# endif

  real_packet->goes_out_of_sight = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" goes_out_of_sight=%u", real_packet->goes_out_of_sight);
# endif

  real_packet->transported = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" transported=%u", real_packet->transported);
# endif

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
#   ifdef REPLAY_2
    printf(" hp=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->activity = readin;
#   ifdef REPLAY_2
    printf(" activity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->transported_by = readin;
#   ifdef REPLAY_2
    printf(" transported_by=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->packet_use = readin;
#   ifdef REPLAY_2
    printf(" packet_use=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->info_city_id = readin;
#   ifdef REPLAY_2
    printf(" info_city_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->serial_num = readin;
#   ifdef REPLAY_2
    printf(" serial_num=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=51 UNIT_COMBAT_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" attacker_unit_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->defender_unit_id = readin;
#   ifdef REPLAY_2
    printf(" defender_unit_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->attacker_hp = readin;
#   ifdef REPLAY_2
    printf(" attacker_hp=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defender_hp = readin;
#   ifdef REPLAY_2
    printf(" defender_hp=%u", readin);
#   endif
  }
  real_packet->make_winner_veteran = BV_ISSET(fields, 4);
# ifdef REPLAY_2
  printf(" make_winner_veteran=%u\n", real_packet->make_winner_veteran);
# endif

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
#define hash_packet_unit_move_100 hash_const

#define cmp_packet_unit_move_100 cmp_const

BV_DEFINE(packet_unit_move_100_fields, 3);

static int send_packet_unit_move_100(
               connection_t *pconn,
               const struct packet_unit_move *packet)
{
  const struct packet_unit_move *real_packet = packet;
  packet_unit_move_100_fields fields;
  struct packet_unit_move *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_MOVE];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_MOVE);
# ifdef REPLAY_2
  printf("cs opc=52 UNIT_MOVE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_move_100,
                     cmp_packet_unit_move_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->x != real_packet->x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->y != real_packet->y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->x);
# ifdef REPLAY_2
    printf(" x=%u", real_packet->x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->y);
# ifdef REPLAY_2
    printf(" y=%u\n", real_packet->y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_move(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_MOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_MOVE] = variant;
}

int send_packet_unit_move(
        connection_t *pconn,
        const struct packet_unit_move *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_move from the server.");
  }
  send_ensure_valid_variant_packet_unit_move(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_MOVE]) {
    case 100: {
      return send_packet_unit_move_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_move(
         connection_t *pconn,
         int unit_id,
         int x,
         int y)
{
  struct packet_unit_move packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;

  return send_packet_unit_move(pconn, real_packet);
}

/* 53 cs */
#define hash_packet_unit_build_city_100 hash_const

#define cmp_packet_unit_build_city_100 cmp_const

BV_DEFINE(packet_unit_build_city_100_fields, 2);

static int send_packet_unit_build_city_100(
               connection_t *pconn,
               const struct packet_unit_build_city *packet)
{
  const struct packet_unit_build_city *real_packet = packet;
  packet_unit_build_city_100_fields fields;
  struct packet_unit_build_city *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_BUILD_CITY];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_BUILD_CITY);
# ifdef REPLAY_2
  printf("cs opc=53 UNIT_BUILD_CITY");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_build_city_100,
                     cmp_packet_unit_build_city_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (strcmp(old->name, real_packet->name) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_string(&dout, real_packet->name);
# ifdef REPLAY_2
    printf(" name=%s\n", real_packet->name);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_build_city(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_BUILD_CITY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_BUILD_CITY] = variant;
}

int send_packet_unit_build_city(
        connection_t *pconn,
        const struct packet_unit_build_city *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_build_city from the server.");
  }
  send_ensure_valid_variant_packet_unit_build_city(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_BUILD_CITY]) {
    case 100: {
      return send_packet_unit_build_city_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_build_city(
         connection_t *pconn,
         int unit_id,
         const char *name)
{
  struct packet_unit_build_city packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  sz_strlcpy(real_packet->name, name);

  return send_packet_unit_build_city(pconn, real_packet);
}

/* 54 cs */
#define hash_packet_unit_disband_100 hash_const

#define cmp_packet_unit_disband_100 cmp_const

BV_DEFINE(packet_unit_disband_100_fields, 1);

static int send_packet_unit_disband_100(
               connection_t *pconn,
               const struct packet_unit_disband *packet)
{
  const struct packet_unit_disband *real_packet = packet;
  packet_unit_disband_100_fields fields;
  struct packet_unit_disband *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_DISBAND];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_DISBAND);
# ifdef REPLAY_2
  printf("cs opc=54 UNIT_DISBAND");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_disband_100,
                     cmp_packet_unit_disband_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_disband(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_DISBAND] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_DISBAND] = variant;
}

int send_packet_unit_disband(
        connection_t *pconn,
        const struct packet_unit_disband *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_disband from the server.");
  }
  send_ensure_valid_variant_packet_unit_disband(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_DISBAND]) {
    case 100: {
      return send_packet_unit_disband_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_disband(connection_t *pconn, int unit_id)
{
  struct packet_unit_disband packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_disband(pconn, real_packet);
}

/* 55 cs */
#define hash_packet_unit_change_homecity_100 hash_const

#define cmp_packet_unit_change_homecity_100 cmp_const

BV_DEFINE(packet_unit_change_homecity_100_fields, 2);

static int send_packet_unit_change_homecity_100(
               connection_t *pconn,
               const struct packet_unit_change_homecity *packet)
{
  const struct packet_unit_change_homecity *real_packet = packet;
  packet_unit_change_homecity_100_fields fields;
  struct packet_unit_change_homecity *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_CHANGE_HOMECITY];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_CHANGE_HOMECITY);
# ifdef REPLAY_2
  printf("cs opc=55 UNIT_CHANGE_HOMECITY");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_change_homecity_100,
                     cmp_packet_unit_change_homecity_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u\n", real_packet->city_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_change_homecity(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_CHANGE_HOMECITY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_CHANGE_HOMECITY] = variant;
}

int send_packet_unit_change_homecity(
        connection_t *pconn,
        const struct packet_unit_change_homecity *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_change_homecity from the server.");
  }
  send_ensure_valid_variant_packet_unit_change_homecity(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_CHANGE_HOMECITY]) {
    case 100: {
      return send_packet_unit_change_homecity_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_change_homecity(
         connection_t *pconn,
         int unit_id,
         int city_id)
{
  struct packet_unit_change_homecity packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->city_id = city_id;

  return send_packet_unit_change_homecity(pconn, real_packet);
}

/* 56 cs */
#define hash_packet_unit_establish_trade_100 hash_const

#define cmp_packet_unit_establish_trade_100 cmp_const

BV_DEFINE(packet_unit_establish_trade_100_fields, 1);

static int send_packet_unit_establish_trade_100(
               connection_t *pconn,
               const struct packet_unit_establish_trade *packet)
{
  const struct packet_unit_establish_trade *real_packet = packet;
  packet_unit_establish_trade_100_fields fields;
  struct packet_unit_establish_trade *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_ESTABLISH_TRADE];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_ESTABLISH_TRADE);
# ifdef REPLAY_2
  printf("cs opc=56 UNIT_ESTABLISH_TRADE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_establish_trade_100,
                     cmp_packet_unit_establish_trade_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_establish_trade(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_ESTABLISH_TRADE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_ESTABLISH_TRADE] = variant;
}

int send_packet_unit_establish_trade(
        connection_t *pconn,
        const struct packet_unit_establish_trade *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_establish_trade from the server.");
  }
  send_ensure_valid_variant_packet_unit_establish_trade(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_ESTABLISH_TRADE]) {
    case 100: {
      return send_packet_unit_establish_trade_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_establish_trade(connection_t *pconn, int unit_id)
{
  struct packet_unit_establish_trade packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_establish_trade(pconn, real_packet);
}

/* 57 cs */
#define hash_packet_unit_help_build_wonder_100 hash_const

#define cmp_packet_unit_help_build_wonder_100 cmp_const

BV_DEFINE(packet_unit_help_build_wonder_100_fields, 1);

static int send_packet_unit_help_build_wonder_100(
               connection_t *pconn,
               const struct packet_unit_help_build_wonder *packet)
{
  const struct packet_unit_help_build_wonder *real_packet = packet;
  packet_unit_help_build_wonder_100_fields fields;
  struct packet_unit_help_build_wonder *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_HELP_BUILD_WONDER];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_HELP_BUILD_WONDER);
# ifdef REPLAY_2
  printf("cs opc=57 UNIT_HELP_BUILD_WONDER");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_help_build_wonder_100,
                     cmp_packet_unit_help_build_wonder_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_help_build_wonder(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER] = variant;
}

int send_packet_unit_help_build_wonder(
        connection_t *pconn,
        const struct packet_unit_help_build_wonder *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_help_build_wonder from the server.");
  }
  send_ensure_valid_variant_packet_unit_help_build_wonder(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_HELP_BUILD_WONDER]) {
    case 100: {
      return send_packet_unit_help_build_wonder_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_help_build_wonder(connection_t *pconn, int unit_id)
{
  struct packet_unit_help_build_wonder packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_help_build_wonder(pconn, real_packet);
}

/* 58 cs */
#define hash_packet_unit_goto_100 hash_const

#define cmp_packet_unit_goto_100 cmp_const

BV_DEFINE(packet_unit_goto_100_fields, 3);

static int send_packet_unit_goto_100(
               connection_t *pconn,
               const struct packet_unit_goto *packet)
{
  const struct packet_unit_goto *real_packet = packet;
  packet_unit_goto_100_fields fields;
  struct packet_unit_goto *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_GOTO];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_GOTO);
# ifdef REPLAY_2
  printf("cs opc=58 UNIT_GOTO");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_goto_100,
                     cmp_packet_unit_goto_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->x != real_packet->x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->y != real_packet->y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->x);
# ifdef REPLAY_2
    printf(" x=%u", real_packet->x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->y);
# ifdef REPLAY_2
    printf(" y=%u\n", real_packet->y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_goto(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_GOTO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_GOTO] = variant;
}

int send_packet_unit_goto(
        connection_t *pconn,
        const struct packet_unit_goto *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_goto from the server.");
  }
  send_ensure_valid_variant_packet_unit_goto(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_GOTO]) {
    case 100: {
      return send_packet_unit_goto_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_goto(
         connection_t *pconn,
         int unit_id,
         int x, int y)
{
  struct packet_unit_goto packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;

  return send_packet_unit_goto(pconn, real_packet);
}

/* 59 cs */
#define hash_packet_unit_orders_100 hash_const

#define cmp_packet_unit_orders_100 cmp_const

BV_DEFINE(packet_unit_orders_100_fields, 9);

static int send_packet_unit_orders_100(
               connection_t *pconn,
               const struct packet_unit_orders *packet)
{
  const struct packet_unit_orders *real_packet = packet;
  packet_unit_orders_100_fields fields;
  struct packet_unit_orders *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_ORDERS];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_ORDERS);
# ifdef REPLAY_2
  printf("cs opc=59 UNIT_ORDER");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_orders_100,
                     cmp_packet_unit_orders_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->length != real_packet->length);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->repeat != real_packet->repeat);
  if (differ) {
    different++;
  }
  if (packet->repeat) {
    BV_SET(fields, 2);
  }

  differ = (old->vigilant != real_packet->vigilant);
  if (differ) {
    different++;
  }
  if (packet->vigilant) {
    BV_SET(fields, 3);
  }

  {
    differ = (old->length != real_packet->length);
    if(!differ) {
      int i;
      for (i = 0; i < real_packet->length; i++) {
        if (old->orders[i] != real_packet->orders[i]) {
          differ = TRUE;
          break;
        }
      }
    }
  }
  if (differ) {
    different++;
    BV_SET(fields, 4);
  }

  {
    differ = (old->length != real_packet->length);
    if(!differ) {
      int i;
      for (i = 0; i < real_packet->length; i++) {
        if (old->dir[i] != real_packet->dir[i]) {
          differ = TRUE;
          break;
        }
      }
    }
  }
  if (differ) {
    different++;
    BV_SET(fields, 5);
  }

  {
    differ = (old->length != real_packet->length);
    if(!differ) {
      int i;
      for (i = 0; i < real_packet->length; i++) {
        if (old->activity[i] != real_packet->activity[i]) {
          differ = TRUE;
          break;
        }
      }
    }
  }
  if (differ) {
    different++;
    BV_SET(fields, 6);
  }

  differ = (old->dest_x != real_packet->dest_x);
  if (differ) {
    different++;
    BV_SET(fields, 7);
  }

  differ = (old->dest_y != real_packet->dest_y);
  if (differ) {
    different++;
    BV_SET(fields, 8);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->length);
  }
  /* field 2 is folded into the header */
  /* field 3 is folded into the header */
  if (BV_ISSET(fields, 4)) {
    int i;

# ifdef REPLAY_2
    printf(" orders[i]=");
# endif
    for (i = 0; i < real_packet->length; i++) {
      dio_put_uint8(&dout, real_packet->orders[i]);
# ifdef REPLAY_2
      printf("%0X", real_packet->orders[i]);
# endif
    }
  }
  if (BV_ISSET(fields, 5)) {
    int i;

# ifdef REPLAY_2
    printf(" dir[i]=");
# endif
    for (i = 0; i < real_packet->length; i++) {
      dio_put_uint8(&dout, real_packet->dir[i]);
# ifdef REPLAY_2
      printf("%0X", real_packet->dir[i]);
# endif
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

# ifdef REPLAY_2
    printf(" activity[i]=");
# endif
    for (i = 0; i < real_packet->length; i++) {
      dio_put_uint8(&dout, real_packet->activity[i]);
# ifdef REPLAY_2
      printf("%0X", real_packet->activity[i]);
# endif
    }
  }
  if (BV_ISSET(fields, 7)) {
    dio_put_uint8(&dout, real_packet->dest_x);
# ifdef REPLAY_2
    printf(" dest_x=%u", real_packet->dest_x);
# endif
  }
  if (BV_ISSET(fields, 8)) {
    dio_put_uint8(&dout, real_packet->dest_y);
# ifdef REPLAY_2
    printf(" dest_y=%u\n", real_packet->dest_y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_orders(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_ORDERS] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_ORDERS] = variant;
}

int send_packet_unit_orders(
        connection_t *pconn,
        const struct packet_unit_orders *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_orders from the server.");
  }
  send_ensure_valid_variant_packet_unit_orders(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_ORDERS]) {
    case 100: {
      return send_packet_unit_orders_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 60 cs */
#define hash_packet_unit_auto_100 hash_const

#define cmp_packet_unit_auto_100 cmp_const

BV_DEFINE(packet_unit_auto_100_fields, 1);

static int send_packet_unit_auto_100(
               connection_t *pconn,
               const struct packet_unit_auto *packet)
{
  const struct packet_unit_auto *real_packet = packet;
  packet_unit_auto_100_fields fields;
  struct packet_unit_auto *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_AUTO];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_AUTO);
# ifdef REPLAY_2
  printf("cs opc=60 UNIT_AUTO");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_auto_100,
                     cmp_packet_unit_auto_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_auto(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_AUTO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_AUTO] = variant;
}

int send_packet_unit_auto(
        connection_t *pconn,
        const struct packet_unit_auto *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_auto from the server.");
  }
  send_ensure_valid_variant_packet_unit_auto(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_AUTO]) {
    case 100: {
      return send_packet_unit_auto_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_auto(connection_t *pconn, int unit_id)
{
  struct packet_unit_auto packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_auto(pconn, real_packet);
}

/* 61 cs */
#define hash_packet_unit_unload_100 hash_const

#define cmp_packet_unit_unload_100 cmp_const

BV_DEFINE(packet_unit_unload_100_fields, 2);

static int send_packet_unit_unload_100(
               connection_t *pconn,
               const struct packet_unit_unload *packet)
{
  const struct packet_unit_unload *real_packet = packet;
  packet_unit_unload_100_fields fields;
  struct packet_unit_unload *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_UNLOAD];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_UNLOAD);
# ifdef REPLAY_2
  printf("cs opc=61 UNIT_UNLOAD");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_unload_100,
                     cmp_packet_unit_unload_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->cargo_id != real_packet->cargo_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->transporter_id != real_packet->transporter_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->cargo_id);
# ifdef REPLAY_2
    printf(" cargo_id=%u", real_packet->cargo_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->transporter_id);
# ifdef REPLAY_2
    printf(" transporter_id=%u\n", real_packet->transporter_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_unload(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_UNLOAD] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_UNLOAD] = variant;
}

int send_packet_unit_unload(
        connection_t *pconn,
        const struct packet_unit_unload *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_unload from the server.");
  }
  send_ensure_valid_variant_packet_unit_unload(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_UNLOAD]) {
    case 100: {
      return send_packet_unit_unload_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_unload(
         connection_t *pconn,
         int cargo_id,
         int transporter_id)
{
  struct packet_unit_unload packet, *real_packet = &packet;

  real_packet->cargo_id = cargo_id;
  real_packet->transporter_id = transporter_id;

  return send_packet_unit_unload(pconn, real_packet);
}

/* 62 cs */
#define hash_packet_unit_upgrade_100 hash_const

#define cmp_packet_unit_upgrade_100 cmp_const

BV_DEFINE(packet_unit_upgrade_100_fields, 1);

static int send_packet_unit_upgrade_100(
               connection_t *pconn,
               const struct packet_unit_upgrade *packet)
{
  const struct packet_unit_upgrade *real_packet = packet;
  packet_unit_upgrade_100_fields fields;
  struct packet_unit_upgrade *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_UPGRADE];
  int different = 0;
  SEND_PACKET_START(PACKET_UNIT_UPGRADE);
# ifdef REPLAY_2
  printf("cs opc=62 UNIT_UPGRADE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_upgrade_100,
                     cmp_packet_unit_upgrade_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_upgrade(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_UPGRADE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_UPGRADE] = variant;
}

int send_packet_unit_upgrade(
        connection_t *pconn,
        const struct packet_unit_upgrade *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_upgrade from the server.");
  }
  send_ensure_valid_variant_packet_unit_upgrade(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_UPGRADE]) {
    case 100: {
      return send_packet_unit_upgrade_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_upgrade(connection_t *pconn, int unit_id)
{
  struct packet_unit_upgrade packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_upgrade(pconn, real_packet);
}

/* 63 cs */
#define hash_packet_unit_nuke_100 hash_const

#define cmp_packet_unit_nuke_100 cmp_const

BV_DEFINE(packet_unit_nuke_100_fields, 1);

static int send_packet_unit_nuke_100(
               connection_t *pconn,
               const struct packet_unit_nuke *packet)
{
  const struct packet_unit_nuke *real_packet = packet;
  packet_unit_nuke_100_fields fields;
  struct packet_unit_nuke *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_NUKE];
  int different = 0;
  SEND_PACKET_START(PACKET_UNIT_NUKE);
# ifdef REPLAY_2
  printf("cs opc=63 UNIT_NUKE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_nuke_100,
                     cmp_packet_unit_nuke_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_nuke(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_NUKE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_NUKE] = variant;
}

int send_packet_unit_nuke(
        connection_t *pconn,
        const struct packet_unit_nuke *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_nuke from the server.");
  }
  send_ensure_valid_variant_packet_unit_nuke(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_NUKE]) {
    case 100: {
      return send_packet_unit_nuke_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_nuke(connection_t *pconn, int unit_id)
{
  struct packet_unit_nuke packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_nuke(pconn, real_packet);
}

/* 64 cs */
#define hash_packet_unit_paradrop_to_100 hash_const

#define cmp_packet_unit_paradrop_to_100 cmp_const

BV_DEFINE(packet_unit_paradrop_to_100_fields, 3);

static int send_packet_unit_paradrop_to_100(
               connection_t *pconn,
               const struct packet_unit_paradrop_to *packet)
{
  const struct packet_unit_paradrop_to *real_packet = packet;
  packet_unit_paradrop_to_100_fields fields;
  struct packet_unit_paradrop_to *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_PARADROP_TO];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_PARADROP_TO);
# ifdef REPLAY_2
  printf("cs opc=64 UNIT_PARADROP_TO");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_paradrop_to_100,
                     cmp_packet_unit_paradrop_to_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->x != real_packet->x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->y != real_packet->y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->x);
# ifdef REPLAY_2
    printf(" x=%u", real_packet->x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->y);
# ifdef REPLAY_2
    printf(" y=%u\n", real_packet->y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_paradrop_to(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_PARADROP_TO] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_PARADROP_TO] = variant;
}

int send_packet_unit_paradrop_to(
        connection_t *pconn,
        const struct packet_unit_paradrop_to *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_paradrop_to from the server.");
  }
  send_ensure_valid_variant_packet_unit_paradrop_to(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_PARADROP_TO]) {
    case 100: {
      return send_packet_unit_paradrop_to_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_paradrop_to(
         connection_t *pconn,
         int unit_id,
         int x, int y)
{
  struct packet_unit_paradrop_to packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->x = x;
  real_packet->y = y;

  return send_packet_unit_paradrop_to(pconn, real_packet);
}

/* 65 cs */
#define hash_packet_unit_airlift_100 hash_const

#define cmp_packet_unit_airlift_100 cmp_const

BV_DEFINE(packet_unit_airlift_100_fields, 2);

static int send_packet_unit_airlift_100(
               connection_t *pconn,
               const struct packet_unit_airlift *packet)
{
  const struct packet_unit_airlift *real_packet = packet;
  packet_unit_airlift_100_fields fields;
  struct packet_unit_airlift *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_AIRLIFT];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_AIRLIFT);
# ifdef REPLAY_2
  printf("cs opc=65 UNIT_AIRLIFT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_airlift_100,
                     cmp_packet_unit_airlift_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->city_id != real_packet->city_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->city_id);
# ifdef REPLAY_2
    printf(" city_id=%u\n", real_packet->city_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_airlift(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_AIRLIFT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_AIRLIFT] = variant;
}

int send_packet_unit_airlift(
        connection_t *pconn,
        const struct packet_unit_airlift *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_airlift from the server.");
  }
  send_ensure_valid_variant_packet_unit_airlift(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_AIRLIFT]) {
    case 100: {
      return send_packet_unit_airlift_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_airlift(
         connection_t *pconn,
         int unit_id,
         int city_id)
{
  struct packet_unit_airlift packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->city_id = city_id;

  return send_packet_unit_airlift(pconn, real_packet);
}

/* 66 */
/* 67 cs */
#define hash_packet_unit_bribe_inq_100 hash_const

#define cmp_packet_unit_bribe_inq_100 cmp_const

BV_DEFINE(packet_unit_bribe_inq_100_fields, 1);

static int send_packet_unit_bribe_inq_100(
               connection_t *pconn,
               const struct packet_unit_bribe_inq *packet)
{
  const struct packet_unit_bribe_inq *real_packet = packet;
  packet_unit_bribe_inq_100_fields fields;
  struct packet_unit_bribe_inq *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_BRIBE_INQ];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_BRIBE_INQ);
# ifdef REPLAY_2
  printf("cs opc=67 UNIT_BRIBE_INQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_bribe_inq_100,
                     cmp_packet_unit_bribe_inq_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u\n", real_packet->unit_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_bribe_inq(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_BRIBE_INQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_BRIBE_INQ] = variant;
}

int send_packet_unit_bribe_inq(
        connection_t *pconn,
        const struct packet_unit_bribe_inq *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_bribe_inq from the server.");
  }
  send_ensure_valid_variant_packet_unit_bribe_inq(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_BRIBE_INQ]) {
    case 100: {
      return send_packet_unit_bribe_inq_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_bribe_inq(connection_t *pconn, int unit_id)
{
  struct packet_unit_bribe_inq packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;

  return send_packet_unit_bribe_inq(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=68 UNIT_BRIBE_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" unit_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->cost = readin;
#   ifdef REPLAY_2
    printf(" cost=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_unit_type_upgrade_100 hash_const

#define cmp_packet_unit_type_upgrade_100 cmp_const

BV_DEFINE(packet_unit_type_upgrade_100_fields, 1);

static int send_packet_unit_type_upgrade_100(
               connection_t *pconn,
               const struct packet_unit_type_upgrade *packet)
{
  const struct packet_unit_type_upgrade *real_packet = packet;
  packet_unit_type_upgrade_100_fields fields;
  struct packet_unit_type_upgrade *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_TYPE_UPGRADE];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_TYPE_UPGRADE);
# ifdef REPLAY_2
  printf("cs opc=69 UNIT_TYPE_UPGRADE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_type_upgrade_100,
                     cmp_packet_unit_type_upgrade_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->type != real_packet->type);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->type);
# ifdef REPLAY_2
    printf(" type=%u\n", real_packet->type);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_type_upgrade(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_TYPE_UPGRADE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_TYPE_UPGRADE] = variant;
}

int send_packet_unit_type_upgrade(
        connection_t *pconn,
        const struct packet_unit_type_upgrade *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_type_upgrade from the server.");
  }
  send_ensure_valid_variant_packet_unit_type_upgrade(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_TYPE_UPGRADE]) {
    case 100: {
      return send_packet_unit_type_upgrade_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_type_upgrade(connection_t *pconn, Unit_Type_id type)
{
  struct packet_unit_type_upgrade packet, *real_packet = &packet;

  real_packet->type = type;

  return send_packet_unit_type_upgrade(pconn, real_packet);
}

/* 70 cs */
#define hash_packet_unit_diplomat_action_100 hash_const

#define cmp_packet_unit_diplomat_action_100 cmp_const

BV_DEFINE(packet_unit_diplomat_action_100_fields, 4);

static int send_packet_unit_diplomat_action_100(
               connection_t *pconn,
               const struct packet_unit_diplomat_action *packet)
{
  const struct packet_unit_diplomat_action *real_packet = packet;
  packet_unit_diplomat_action_100_fields fields;
  struct packet_unit_diplomat_action *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_DIPLOMAT_ACTION];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_DIPLOMAT_ACTION);
# ifdef REPLAY_2
  printf("cs opc=70 UNIT_DIPLOMAT_ACTION");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_diplomat_action_100,
                     cmp_packet_unit_diplomat_action_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->diplomat_id != real_packet->diplomat_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->action_type != real_packet->action_type);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->target_id != real_packet->target_id);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  differ = (old->value != real_packet->value);
  if (differ) {
    different++;
    BV_SET(fields, 3);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->diplomat_id);
# ifdef REPLAY_2
    printf(" diplomat_id=%u", real_packet->diplomat_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->action_type);
# ifdef REPLAY_2
    printf(" action_type=%u", real_packet->action_type);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint16(&dout, real_packet->target_id);
# ifdef REPLAY_2
    printf(" target_id=%u", real_packet->target_id);
# endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_put_sint16(&dout, real_packet->value);
# ifdef REPLAY_2
    printf(" value=%d\n", real_packet->value);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_diplomat_action(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION] = variant;
}

int send_packet_unit_diplomat_action(
        connection_t *pconn,
        const struct packet_unit_diplomat_action *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_diplomat_action from the server.");
  }
  send_ensure_valid_variant_packet_unit_diplomat_action(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_DIPLOMAT_ACTION]) {
    case 100: {
      return send_packet_unit_diplomat_action_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_diplomat_action(
         connection_t *pconn,
         int diplomat_id,
         enum diplomat_actions action_type,
         int target_id,
         int value)
{
  struct packet_unit_diplomat_action packet, *real_packet = &packet;

  real_packet->diplomat_id = diplomat_id;
  real_packet->action_type = action_type;
  real_packet->target_id = target_id;
  real_packet->value = value;

  return send_packet_unit_diplomat_action(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=71 UNIT_DIPLOMAT_POPUP_DIALOG");
# endif
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
#   ifdef REPLAY_2
    printf(" diplomat_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->target_id = readin;
#   ifdef REPLAY_2
    printf(" target_id=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_unit_change_activity_100 hash_const

#define cmp_packet_unit_change_activity_100 cmp_const

BV_DEFINE(packet_unit_change_activity_100_fields, 3);

static int send_packet_unit_change_activity_100(
               connection_t *pconn,
               const struct packet_unit_change_activity *packet)
{
  const struct packet_unit_change_activity *real_packet = packet;
  packet_unit_change_activity_100_fields fields;
  struct packet_unit_change_activity *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_CHANGE_ACTIVITY];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_CHANGE_ACTIVITY);
# ifdef REPLAY_2
  printf("cs opc=72 UNIT_CHANGE_ACTIVITY");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_change_activity_100,
                     cmp_packet_unit_change_activity_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->activity != real_packet->activity);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->activity_target != real_packet->activity_target);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->activity);
# ifdef REPLAY_2
    printf(" activity=%u", real_packet->activity);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint16(&dout, real_packet->activity_target);
# ifdef REPLAY_2
    printf(" activity_target=%u\n", real_packet->activity_target);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_change_activity(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY] = variant;
}

int send_packet_unit_change_activity(
        connection_t *pconn,
        const struct packet_unit_change_activity *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_change_activity from the server.");
  }
  send_ensure_valid_variant_packet_unit_change_activity(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_CHANGE_ACTIVITY]) {
    case 100: {
      return send_packet_unit_change_activity_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_change_activity(
         connection_t *pconn,
         int unit_id,
         enum unit_activity activity,
         enum tile_special_type activity_target)
{
  struct packet_unit_change_activity packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->activity = activity;
  real_packet->activity_target = activity_target;

  return send_packet_unit_change_activity(pconn, real_packet);
}

/* 73 cs */
#define hash_packet_diplomacy_init_meeting_req_100 hash_const

#define cmp_packet_diplomacy_init_meeting_req_100 cmp_const

BV_DEFINE(packet_diplomacy_init_meeting_req_100_fields, 1);

static int send_packet_diplomacy_init_meeting_req_100(
               connection_t *pconn,
               const struct packet_diplomacy_init_meeting_req *packet)
{
  const struct packet_diplomacy_init_meeting_req *real_packet = packet;
  packet_diplomacy_init_meeting_req_100_fields fields;
  struct packet_diplomacy_init_meeting_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_INIT_MEETING_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_INIT_MEETING_REQ);
# ifdef REPLAY_2
  printf("cs opc=73 DIPLOMACY_INIT_MEETING_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_init_meeting_req_100,
                     cmp_packet_diplomacy_init_meeting_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->counterpart != real_packet->counterpart);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->counterpart);
# ifdef REPLAY_2
    printf(" counterpart=%u\n", real_packet->counterpart);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_init_meeting_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ] = variant;
}

int send_packet_diplomacy_init_meeting_req(
        connection_t *pconn,
        const struct packet_diplomacy_init_meeting_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_init_meeting_req from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_init_meeting_req(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_INIT_MEETING_REQ]) {
    case 100: {
      return send_packet_diplomacy_init_meeting_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_init_meeting_req(
         connection_t *pconn,
         int counterpart)
{
  struct packet_diplomacy_init_meeting_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;

  return send_packet_diplomacy_init_meeting_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=74 DIPLOMACY_INIT_MEETING");
# endif
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
#   ifdef REPLAY_2
    printf(" counterpart=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->initiated_from = readin;
#   ifdef REPLAY_2
    printf(" initiated_from=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_diplomacy_cancel_meeting_req_100 hash_const

#define cmp_packet_diplomacy_cancel_meeting_req_100 cmp_const

BV_DEFINE(packet_diplomacy_cancel_meeting_req_100_fields, 1);

static int send_packet_diplomacy_cancel_meeting_req_100(
               connection_t *pconn,
               const struct packet_diplomacy_cancel_meeting_req *packet)
{
  const struct packet_diplomacy_cancel_meeting_req *real_packet = packet;
  packet_diplomacy_cancel_meeting_req_100_fields fields;
  struct packet_diplomacy_cancel_meeting_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_CANCEL_MEETING_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_CANCEL_MEETING_REQ);
# ifdef REPLAY_2
  printf("cs opc=75 DIPLOMACY_CANCEL_MEETING_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_cancel_meeting_req_100,
                     cmp_packet_diplomacy_cancel_meeting_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->counterpart != real_packet->counterpart);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->counterpart);
# ifdef REPLAY_2
    printf(" counterpart=%u\n", real_packet->counterpart);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_cancel_meeting_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ] = variant;
}

int send_packet_diplomacy_cancel_meeting_req(
        connection_t *pconn,
        const struct packet_diplomacy_cancel_meeting_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_cancel_meeting_req from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_cancel_meeting_req(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_MEETING_REQ]) {
    case 100: {
      return send_packet_diplomacy_cancel_meeting_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_cancel_meeting_req(
         connection_t *pconn,
         int counterpart)
{
  struct packet_diplomacy_cancel_meeting_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;

  return send_packet_diplomacy_cancel_meeting_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=76 DIPLOMACY_CANCEL_MEETING");
# endif
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
#   ifdef REPLAY_2
    printf(" counterpart=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->initiated_from = readin;
#   ifdef REPLAY_2
    printf(" initiated_from=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_diplomacy_create_clause_req_100 hash_const

#define cmp_packet_diplomacy_create_clause_req_100 cmp_const

BV_DEFINE(packet_diplomacy_create_clause_req_100_fields, 4);

static int send_packet_diplomacy_create_clause_req_100(
               connection_t *pconn,
               const struct packet_diplomacy_create_clause_req *packet)
{
  const struct packet_diplomacy_create_clause_req *real_packet = packet;
  packet_diplomacy_create_clause_req_100_fields fields;
  struct packet_diplomacy_create_clause_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_CREATE_CLAUSE_REQ);
# ifdef REPLAY_2
  printf("cs opc=77 DIPLOMACY_CREATE_CLAUSE_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_create_clause_req_100,
                     cmp_packet_diplomacy_create_clause_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->counterpart != real_packet->counterpart);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->giver != real_packet->giver);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->type != real_packet->type);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  differ = (old->value != real_packet->value);
  if (differ) {
    different++;
    BV_SET(fields, 3);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->counterpart);
# ifdef REPLAY_2
    printf(" counterpart=%u", real_packet->counterpart);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->giver);
# ifdef REPLAY_2
    printf(" giver=%u", real_packet->giver);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->type);
# ifdef REPLAY_2
    printf(" type=%u", real_packet->type);
# endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_put_uint32(&dout, real_packet->value);
# ifdef REPLAY_2
    printf(" value=%u\n", real_packet->value);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_create_clause_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ] = variant;
}

int send_packet_diplomacy_create_clause_req(
        connection_t *pconn,
        const struct packet_diplomacy_create_clause_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_create_clause_req from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_create_clause_req(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_CREATE_CLAUSE_REQ]) {
    case 100: {
      return send_packet_diplomacy_create_clause_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_create_clause_req(
         connection_t *pconn,
         int counterpart,
         int giver,
         enum clause_type type,
         int value)
{
  struct packet_diplomacy_create_clause_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;

  return send_packet_diplomacy_create_clause_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=78 DIPLOMACY_CREATE_CLAUSE");
# endif
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
#   ifdef REPLAY_2
    printf(" counterpart=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->giver = readin;
#   ifdef REPLAY_2
    printf(" giver=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
#   ifdef REPLAY_2
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_diplomacy_remove_clause_req_100 hash_const

#define cmp_packet_diplomacy_remove_clause_req_100 cmp_const

BV_DEFINE(packet_diplomacy_remove_clause_req_100_fields, 4);

static int send_packet_diplomacy_remove_clause_req_100(
               connection_t *pconn,
               const struct packet_diplomacy_remove_clause_req *packet)
{
  const struct packet_diplomacy_remove_clause_req *real_packet = packet;
  packet_diplomacy_remove_clause_req_100_fields fields;
  struct packet_diplomacy_remove_clause_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ);
# ifdef REPLAY_2
  printf("cs opc=79 DIPLOMACY_REMOVE_CLAUSE_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_remove_clause_req_100,
                     cmp_packet_diplomacy_remove_clause_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->counterpart != real_packet->counterpart);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->giver != real_packet->giver);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->type != real_packet->type);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  differ = (old->value != real_packet->value);
  if (differ) {
    different++;
    BV_SET(fields, 3);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->counterpart);
# ifdef REPLAY_2
    printf(" counterpart=%u", real_packet->counterpart);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->giver);
# ifdef REPLAY_2
    printf(" giver=%u", real_packet->giver);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->type);
# ifdef REPLAY_2
    printf(" type=%u", real_packet->type);
# endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_put_uint32(&dout, real_packet->value);
# ifdef REPLAY_2
    printf(" value=%u\n", real_packet->value);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_remove_clause_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ] = variant;
}

int send_packet_diplomacy_remove_clause_req(
        connection_t *pconn,
        const struct packet_diplomacy_remove_clause_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_remove_clause_req from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_remove_clause_req(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ]) {
    case 100: {
      return send_packet_diplomacy_remove_clause_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_remove_clause_req(
         connection_t *pconn,
         int counterpart,
         int giver,
         enum clause_type type,
         int value)
{
  struct packet_diplomacy_remove_clause_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;
  real_packet->giver = giver;
  real_packet->type = type;
  real_packet->value = value;

  return send_packet_diplomacy_remove_clause_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=80 DIPLOMACY_REMOVE_CLAUSE");
# endif
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
#   ifdef REPLAY_2
    printf(" counterpart=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->giver = readin;
#   ifdef REPLAY_2
    printf(" giver=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->value = readin;
#   ifdef REPLAY_2
    printf(" value=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_diplomacy_accept_treaty_req_100 hash_const

#define cmp_packet_diplomacy_accept_treaty_req_100 cmp_const

BV_DEFINE(packet_diplomacy_accept_treaty_req_100_fields, 1);

static int send_packet_diplomacy_accept_treaty_req_100(
               connection_t *pconn,
               const struct packet_diplomacy_accept_treaty_req *packet)
{
  const struct packet_diplomacy_accept_treaty_req *real_packet = packet;
  packet_diplomacy_accept_treaty_req_100_fields fields;
  struct packet_diplomacy_accept_treaty_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_ACCEPT_TREATY_REQ);
# ifdef REPLAY_2
  printf("cs op=81 DIPLOMACY_ACCEPT_TREATY_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_accept_treaty_req_100,
                     cmp_packet_diplomacy_accept_treaty_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->counterpart != real_packet->counterpart);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->counterpart);
# ifdef REPLAY_2
    printf(" counterpart=%u\n", real_packet->counterpart);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_accept_treaty_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ] = variant;
}

int send_packet_diplomacy_accept_treaty_req(
        connection_t *pconn,
        const struct packet_diplomacy_accept_treaty_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_accept_treaty_req from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_accept_treaty_req(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_ACCEPT_TREATY_REQ]) {
    case 100: {
      return send_packet_diplomacy_accept_treaty_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_accept_treaty_req(
         connection_t *pconn,
         int counterpart)
{
  struct packet_diplomacy_accept_treaty_req packet, *real_packet = &packet;

  real_packet->counterpart = counterpart;

  return send_packet_diplomacy_accept_treaty_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=82 DIPLOMACY_ACCEPT_TREATY");
# endif
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
#   ifdef REPLAY_2
    printf(" counterpart=%u", readin);
#   endif
  }
  real_packet->I_accepted = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" I_accepted=%u", real_packet->I_accepted);
# endif

  real_packet->other_accepted = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" other_accepted=%u\n", real_packet->other_accepted);
# endif

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
#define hash_packet_diplomacy_cancel_pact_100 hash_const

#define cmp_packet_diplomacy_cancel_pact_100 cmp_const

BV_DEFINE(packet_diplomacy_cancel_pact_100_fields, 2);

static int send_packet_diplomacy_cancel_pact_100(
               connection_t *pconn,
               const struct packet_diplomacy_cancel_pact *packet)
{
  const struct packet_diplomacy_cancel_pact *real_packet = packet;
  packet_diplomacy_cancel_pact_100_fields fields;
  struct packet_diplomacy_cancel_pact *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_DIPLOMACY_CANCEL_PACT];
  int different = 0;

  SEND_PACKET_START(PACKET_DIPLOMACY_CANCEL_PACT);
# ifdef REPLAY_2
  printf("cs opc=83 DIPLOMACY_CANCEL_PACT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_diplomacy_cancel_pact_100,
                     cmp_packet_diplomacy_cancel_pact_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->other_player_id != real_packet->other_player_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->clause != real_packet->clause);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->other_player_id);
# ifdef REPLAY_2
    printf(" other_player_id=%u", real_packet->other_player_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->clause);
# ifdef REPLAY_2
    printf(" clause=%u\n", real_packet->clause);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_diplomacy_cancel_pact(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT] = variant;
}

int send_packet_diplomacy_cancel_pact(
        connection_t *pconn,
        const struct packet_diplomacy_cancel_pact *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_diplomacy_cancel_pact from the server.");
  }
  send_ensure_valid_variant_packet_diplomacy_cancel_pact(pconn);

  switch(pconn->phs.variant[PACKET_DIPLOMACY_CANCEL_PACT]) {
    case 100: {
      return send_packet_diplomacy_cancel_pact_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_diplomacy_cancel_pact(
         connection_t *pconn,
         int other_player_id,
         enum clause_type clause)
{
  struct packet_diplomacy_cancel_pact packet, *real_packet = &packet;

  real_packet->other_player_id = other_player_id;
  real_packet->clause = clause;

  return send_packet_diplomacy_cancel_pact(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=84 PAGE_MSG");
# endif
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
#   ifdef REPLAY_2
    printf(" message=\"%s\"", real_packet->message);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->event = readin;
#   ifdef REPLAY_2
    printf(" event=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
#define hash_packet_report_req_100 hash_const

#define cmp_packet_report_req_100 cmp_const

BV_DEFINE(packet_report_req_100_fields, 1);

static int send_packet_report_req_100(
               connection_t *pconn,
               const struct packet_report_req *packet)
{
  const struct packet_report_req *real_packet = packet;
  packet_report_req_100_fields fields;
  struct packet_report_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_REPORT_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_REPORT_REQ);
# ifdef REPLAY_2
  printf("cs opc=85 REPORT_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_report_req_100,
                     cmp_packet_report_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->type != real_packet->type);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->type);
# ifdef REPLAY_2
    printf(" type=%u\n", real_packet->type);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_report_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_REPORT_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_REPORT_REQ] = variant;
}

int send_packet_report_req(
        connection_t *pconn,
        const struct packet_report_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_report_req from the server.");
  }
  send_ensure_valid_variant_packet_report_req(pconn);

  switch(pconn->phs.variant[PACKET_REPORT_REQ]) {
    case 100: {
      return send_packet_report_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_report_req(connection_t *pconn, enum report_type type)
{
  struct packet_report_req packet, *real_packet = &packet;

  real_packet->type = type;

  return send_packet_report_req(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("cs op=86 CONN_INFO");
# endif
  DIO_BV_GET(&din, fields);

  dio_get_uint8(&din, &readin);
  real_packet->id = readin;
# ifdef REPLAY_2
  printf(" id=%u", readin);
# endif

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
# ifdef REPLAY_2
  printf(" used=%u", real_packet->used);
# endif

  real_packet->established = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" established=%u", real_packet->established);
# endif

  real_packet->observer = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" observer=%u", real_packet->observer);
# endif

  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->player_num = readin;
#   ifdef REPLAY_2
    printf(" player_num=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->access_level = readin;
#   ifdef REPLAY_2
    printf(" access_level=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->username, sizeof(real_packet->username));
#   ifdef REPLAY_2
    printf(" username=\"%s\"", real_packet->username);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->addr, sizeof(real_packet->addr));
#   ifdef REPLAY_2
    printf(" addr=\"%s\"", real_packet->addr);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->capability, sizeof(real_packet->capability));
#   ifdef REPLAY_2
    printf(" capability=\"%s\"\n", real_packet->capability);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=87 CONN_PING_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" connections=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    if(real_packet->connections > MAX_NUM_CONNECTIONS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_CONNECTIONS;
    }
#   ifdef REPLAY_2
    printf(" conn_id[%u]=", real_packet->connections);
#   endif
    for (i = 0; i < real_packet->connections; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->conn_id[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 2)) {

    {
      int i;

      if(real_packet->connections > MAX_NUM_CONNECTIONS) {
        freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
        real_packet->connections = MAX_NUM_CONNECTIONS;
      }
#     ifdef REPLAY_2
      printf(" ping_time[%u]=", real_packet->connections);
#     endif
      for (i = 0; i < real_packet->connections; i++) {
        int tmp;

        dio_get_uint32(&din, &tmp);
        real_packet->ping_time[i] = (float)(tmp) / 1000000.0;
#       ifdef REPLAY_2
        printf(" %u", tmp);
#       endif
      }
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=87 CONN_PING_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" old_connections=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int i;

    if(real_packet->connections > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" old_conn_id[%u]=", real_packet->connections);
#   endif
    for (i = 0; i < real_packet->connections; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->old_conn_id[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->connections > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->connections = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf("old_ping_time[%u]", real_packet->connections);
#   endif
    for (i = 0; i < real_packet->connections; i++) {
      int tmp;

      dio_get_uint32(&din, &tmp);
      real_packet->old_ping_time[i] = (float)(tmp) / 1000000.0;
#     ifdef REPLAY_2
      printf(" %u", tmp);
#     endif
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=88 CONN_PING\n");
# endif
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
static int send_packet_conn_pong_100(connection_t *pconn)
{
  SEND_PACKET_START(PACKET_CONN_PONG);
# ifdef REPLAY_2
  printf("cs opc=89 CONN_PONG\n");
# endif
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_conn_pong(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CONN_PONG] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CONN_PONG] = variant;
}

int send_packet_conn_pong(connection_t *pconn)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_conn_pong from the server.");
  }
  send_ensure_valid_variant_packet_conn_pong(pconn);

  switch(pconn->phs.variant[PACKET_CONN_PONG]) {
    case 100: {
      return send_packet_conn_pong_100(pconn);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 90 sc */
static struct packet_before_new_year *
receive_packet_before_new_year_100(
       connection_t *pconn,
       enum packet_type type)
{
  RECEIVE_PACKET_START(packet_before_new_year, real_packet);
# ifdef REPLAY_2
  printf("sc op=90 BEFORE_NEW_YEAR\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=91 START_TURN\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=92 NEW_YEAR");
# endif
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
#   ifdef REPLAY_2
    printf(" year=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->turn = readin;
#   ifdef REPLAY_2
    printf(" turn=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
static int send_packet_spaceship_launch_100(connection_t *pconn)
{
  SEND_PACKET_START(PACKET_SPACESHIP_LAUNCH);
# ifdef REPLAY_2
  printf("cs op=93 SPACESHIP_LAUNCH\n");
# endif
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_spaceship_launch(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SPACESHIP_LAUNCH] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SPACESHIP_LAUNCH] = variant;
}

int send_packet_spaceship_launch(connection_t *pconn)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_spaceship_launch from the server.");
  }
  send_ensure_valid_variant_packet_spaceship_launch(pconn);

  switch(pconn->phs.variant[PACKET_SPACESHIP_LAUNCH]) {
    case 100: {
      return send_packet_spaceship_launch_100(pconn);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 94 cs */
#define hash_packet_spaceship_place_100 hash_const

#define cmp_packet_spaceship_place_100 cmp_const

BV_DEFINE(packet_spaceship_place_100_fields, 2);

static int send_packet_spaceship_place_100(
               connection_t *pconn,
               const struct packet_spaceship_place *packet)
{
  const struct packet_spaceship_place *real_packet = packet;
  packet_spaceship_place_100_fields fields;
  struct packet_spaceship_place *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_SPACESHIP_PLACE];
  int different = 0;

  SEND_PACKET_START(PACKET_SPACESHIP_PLACE);
# ifdef REPLAY_2
  printf("cs opc=94 SPACESHIP_PLACE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_spaceship_place_100,
                     cmp_packet_spaceship_place_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->type != real_packet->type);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->num != real_packet->num);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->type);
# ifdef REPLAY_2
    printf(" type=%u", real_packet->type);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->num);
# ifdef REPLAY_2
    printf(" num=%u\n", real_packet->num);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_spaceship_place(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SPACESHIP_PLACE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_SPACESHIP_PLACE] = variant;
}

int send_packet_spaceship_place(
        connection_t *pconn,
        const struct packet_spaceship_place *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_spaceship_place from the server.");
  }
  send_ensure_valid_variant_packet_spaceship_place(pconn);

  switch(pconn->phs.variant[PACKET_SPACESHIP_PLACE]) {
    case 100: {
      return send_packet_spaceship_place_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_spaceship_place(
         connection_t *pconn,
         enum spaceship_place_type type,
         int num)
{
  struct packet_spaceship_place packet, *real_packet = &packet;

  real_packet->type = type;
  real_packet->num = num;

  return send_packet_spaceship_place(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=95 SPACESHIP_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" sship_state=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->structurals = readin;
#   ifdef REPLAY_2
    printf(" structurals=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->components = readin;
#   ifdef REPLAY_2
    printf(" components=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->modules = readin;
#   ifdef REPLAY_2
    printf(" modules=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
#   ifdef REPLAY_2
    printf(" fuel=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->propulsion = readin;
#   ifdef REPLAY_2
    printf(" propulsion=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->habitation = readin;
#   ifdef REPLAY_2
    printf(" habitation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->life_support = readin;
#   ifdef REPLAY_2
    printf(" life_support=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->solar_panels = readin;
#   ifdef REPLAY_2
    printf(" solar_panels=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->launch_year = readin;
#   ifdef REPLAY_2
    printf(" launch_year=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->population = readin;
#   ifdef REPLAY_2
    printf(" population=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->mass = readin;
#   ifdef REPLAY_2
    printf(" mass=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_bit_string(&din, real_packet->structure, sizeof(real_packet->structure));
#   ifdef REPLAY_2
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
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->support_rate = (float)(tmp) / 10000.0;
#   ifdef REPLAY_2
    printf(" support_rate=%u", tmp);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->energy_rate = (float)(tmp) / 10000.0;
#   ifdef REPLAY_2
    printf(" energy_rate=%u", tmp);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->success_rate = (float)(tmp) / 10000.0;
#   ifdef REPLAY_2
    printf(" success_rate=%u", tmp);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int tmp;

    dio_get_uint32(&din, &tmp);
    real_packet->travel_time = (float)(tmp) / 10000.0;
#   ifdef REPLAY_2
    printf(" travel_time=%u\n", tmp);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=96 RULESET_UNIT");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->sound_move, sizeof(real_packet->sound_move));
#   ifdef REPLAY_2
    printf(" sound_move=\"%s\"", real_packet->sound_move);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->sound_move_alt, sizeof(real_packet->sound_move_alt));
#   ifdef REPLAY_2
    printf(" sound_move_alt=\"%s\"", real_packet->sound_move_alt);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->sound_fight, sizeof(real_packet->sound_fight));
#   ifdef REPLAY_2
    printf(" sound_fight=\"%s\"", real_packet->sound_fight);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->sound_fight_alt, sizeof(real_packet->sound_fight_alt));
#   ifdef REPLAY_2
    printf(" sound_fight_alt=\"%s\"", real_packet->sound_fight_alt);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->move_type = readin;
#   ifdef REPLAY_2
    printf(" move_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->build_cost = readin;
#   ifdef REPLAY_2
    printf(" build_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->pop_cost = readin;
#   ifdef REPLAY_2
    printf(" pop_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->attack_strength = readin;
#   ifdef REPLAY_2
    printf(" attack_strength=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defense_strength = readin;
#   ifdef REPLAY_2
    printf(" defense_strength=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->move_rate = readin;
#   ifdef REPLAY_2
    printf(" move_rate=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_requirement = readin;
#   ifdef REPLAY_2
    printf(" tech_requirement=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->impr_requirement = readin;
#   ifdef REPLAY_2
    printf(" impr_requirement=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->vision_range = readin;
#   ifdef REPLAY_2
    printf(" vision_range=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->transport_capacity = readin;
#   ifdef REPLAY_2
    printf(" transport_capacity=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hp = readin;
#   ifdef REPLAY_2
    printf(" hp=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->firepower = readin;
#   ifdef REPLAY_2
    printf(" firepower=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->obsoleted_by = readin;
#   ifdef REPLAY_2
    printf(" obsoleted_by=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fuel = readin;
#   ifdef REPLAY_2
    printf(" fuel=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->happy_cost = readin;
#   ifdef REPLAY_2
    printf(" happy_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_cost = readin;
#   ifdef REPLAY_2
    printf(" shield_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_cost = readin;
#   ifdef REPLAY_2
    printf(" food_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->gold_cost = readin;
#   ifdef REPLAY_2
    printf(" gold_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_range = readin;
#   ifdef REPLAY_2
    printf(" paratroopers_range=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_req = readin;
#   ifdef REPLAY_2
    printf(" paratroopers_mr_req=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->paratroopers_mr_sub = readin;
#   ifdef REPLAY_2
    printf(" paratroopers_mr_sub=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int i;

#   ifdef REPLAY_2
    printf("veteran_name[%u]=", MAX_VET_LEVELS);
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      dio_get_string(&din, real_packet->veteran_name[i], sizeof(real_packet->veteran_name[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->veteran_name[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 30)) {
    int i;

#   ifdef REPLAY_2
    printf("power_fact[%u]=", MAX_VET_LEVELS);
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int tmp;

      dio_get_uint32(&din, &tmp);
      real_packet->power_fact[i] = (float)(tmp) / 10000.0;
#     ifdef REPLAY_2
      printf(" %u", tmp);
#     endif
    }
  }
  if (BV_ISSET(fields, 31)) {
    int i;

#   ifdef REPLAY_2
    printf("move_bonus[%u]=", MAX_VET_LEVELS);
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->move_bonus[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->bombard_rate = readin;
#   ifdef REPLAY_2
    printf(" bombard_rate=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 33)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
#   ifdef REPLAY_2
    printf(" team=\"%s\"", real_packet->helptext);
#   endif
  }
  if (BV_ISSET(fields, 34)) {
#   ifdef REPLAY_2
    int i;
#   endif

    DIO_BV_GET(&din, real_packet->flags);
#   ifdef REPLAY_2
    printf(" flags=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->flags.vec[i]);
    }
#   endif
  }
  if (BV_ISSET(fields, 35)) {
#   ifdef REPLAY_2
    int i;
#   endif

    DIO_BV_GET(&din, real_packet->roles);
#   ifdef REPLAY_2
    printf(" roles=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->roles.vec[i]);
    }
  } else {
    printf("\n");
#  endif
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
# ifdef REPLAY_2
  printf("sc op=97 RULESET_GAME");
# endif
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

#   ifdef REPLAY_2
    printf("specialist_name[%u]=", SP_COUNT);
#   endif
    for (i = 0; i < SP_COUNT; i++) {
      dio_get_string(&din, real_packet->specialist_name[i],
                     sizeof(real_packet->specialist_name[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->specialist_name[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 1)) {
    int i;

#   ifdef REPLAY_2
    printf("specialist_min_size[%u]=", SP_COUNT);
#   endif
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialist_min_size[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 2)) {
    int i;

#   ifdef REPLAY_2
    printf("specialist_bonus[%u]=", SP_COUNT);
#   endif
    for (i = 0; i < SP_COUNT; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->specialist_bonus[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  real_packet->changable_tax = BV_ISSET(fields, 3);
# ifdef REPLAY_2
  printf(" changable_tax=%u", real_packet->changable_tax);
# endif

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_science = readin;
#   ifdef REPLAY_2
    printf(" forced_science=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_luxury = readin;
#   ifdef REPLAY_2
    printf(" forced_luxury=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->forced_gold = readin;
#   ifdef REPLAY_2
    printf(" forced_gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_food = readin;
#   ifdef REPLAY_2
    printf(" min_city_center_food=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_shield = readin;
#   ifdef REPLAY_2
    printf(" min_city_center_shield=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_city_center_trade = readin;
#   ifdef REPLAY_2
    printf(" min_city_center_trade=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->min_dist_bw_cities = readin;
#   ifdef REPLAY_2
    printf(" min_dist_bw_cities=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->init_vis_radius_sq = readin;
#   ifdef REPLAY_2
    printf(" init_vis_radius_sq=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->hut_overflight = readin;
#   ifdef REPLAY_2
    printf(" hut_overflight=%u", readin);
#   endif
  }
  real_packet->pillage_select = BV_ISSET(fields, 13);
# ifdef REPLAY_2
  printf(" pillage_select=%u", real_packet->pillage_select);
# endif

  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nuke_contamination = readin;
#   ifdef REPLAY_2
    printf(" nuke_contamination=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int i;

#   ifdef REPLAY_2
    printf("granary_food_ini[%u]=", MAX_GRANARY_INIS);
#   endif
    for (i = 0; i < MAX_GRANARY_INIS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->granary_food_ini[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->granary_num_inis = readin;
#   ifdef REPLAY_2
    printf(" granary_num_inis=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->granary_food_inc = readin;
#   ifdef REPLAY_2
    printf(" granary_food_inc=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_cost_style = readin;
#   ifdef REPLAY_2
    printf(" tech_cost_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->tech_cost_double_year = readin;
#   ifdef REPLAY_2
    printf(" tech_cost_double_year=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_leakage = readin;
#   ifdef REPLAY_2
    printf(" tech_leakage=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_tech_list(&din, real_packet->global_init_techs);
#   ifdef REPLAY_2
    printf("global_init_techs[]=");
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (real_packet->global_init_techs[i] == A_LAST) {
        break;
      } else {
        printf(" %u", real_packet->global_init_techs[i]);
      }
    }
#   endif
  }
  real_packet->killstack = BV_ISSET(fields, 22);
# ifdef REPLAY_2
  printf(" killstack=%u", real_packet->killstack);
# endif

  if (BV_ISSET(fields, 23)) {
    int i;

#   ifdef REPLAY_2
    printf("trireme_loss_chance[i]=");
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->trireme_loss_chance[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 24)) {
    int i;

#   ifdef REPLAY_2
    printf("work_veteran_chance[i]=");
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->work_veteran_chance[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 25)) {
    int i;

#   ifdef REPLAY_2
    printf("veteran_chance[i]=");
#   endif
    for (i = 0; i < MAX_VET_LEVELS; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->veteran_chance[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=98 RULESET_GOVERNMENT_RULER_TITLE");
# endif
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
#   ifdef REPLAY_2
    printf(" gov=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->id = readin;
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->nation = readin;
#   ifdef REPLAY_2
    printf(" nation=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->male_title, sizeof(real_packet->male_title));
#   ifdef REPLAY_2
    printf(" male_title=\"%s\"", real_packet->male_title);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->female_title, sizeof(real_packet->female_title));
#   ifdef REPLAY_2
    printf(" female_title=\"%s\"\n", real_packet->female_title);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=99 RULESET_TECH");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int i;

#   ifdef REPLAY_2
    printf("req[2]=");
#   endif
    for (i = 0; i < 2; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->req[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->root_req = readin;
#   ifdef REPLAY_2
    printf(" root_req=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
#   ifdef REPLAY_2
    printf(" flags=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->preset_cost = readin;
#   ifdef REPLAY_2
    printf(" preset_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->num_reqs = readin;
#   ifdef REPLAY_2
    printf(" num_reqs=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
#   ifdef REPLAY_2
    printf(" helptext=\"%s\"", real_packet->helptext);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"\n", real_packet->graphic_alt);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=100 RULESET_GOVERNMENT");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->required_tech = readin;
#   ifdef REPLAY_2
    printf(" required_tech=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->max_rate = readin;
#   ifdef REPLAY_2
    printf(" max_rate=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->civil_war = readin;
#   ifdef REPLAY_2
    printf(" civil_war=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->martial_law_max = readin;
#   ifdef REPLAY_2
    printf(" martial_law_max=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->martial_law_per = readin;
#   ifdef REPLAY_2
    printf(" martial_law_per=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->empire_size_mod = readin;
#   ifdef REPLAY_2
    printf(" empire_size_mod=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->empire_size_inc = readin;
#   ifdef REPLAY_2
    printf(" empire_size_inc=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rapture_size = readin;
#   ifdef REPLAY_2
    printf(" rapture_size=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_happy_cost_factor = readin;
#   ifdef REPLAY_2
    printf(" unit_happy_cost_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_shield_cost_factor = readin;
#   ifdef REPLAY_2
    printf(" unit_shield_cost_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_food_cost_factor = readin;
#   ifdef REPLAY_2
    printf(" unit_food_cost_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->unit_gold_cost_factor = readin;
#   ifdef REPLAY_2
    printf(" unit_gold_cost_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_happy = readin;
#   ifdef REPLAY_2
    printf(" free_happy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_shield = readin;
#   ifdef REPLAY_2
    printf(" free_shield=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_food = readin;
#   ifdef REPLAY_2
    printf(" free_food=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->free_gold = readin;
#   ifdef REPLAY_2
    printf(" free_gold=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" trade_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shields_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" shields_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" food_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_trade_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" celeb_trade_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_shields_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" celeb_shields_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_food_before_penalty = readin;
#   ifdef REPLAY_2
    printf(" celeb_food_before_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_bonus = readin;
#   ifdef REPLAY_2
    printf(" trade_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_bonus = readin;
#   ifdef REPLAY_2
    printf(" shield_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_bonus = readin;
#   ifdef REPLAY_2
    printf(" food_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_trade_bonus = readin;
#   ifdef REPLAY_2
    printf(" celeb_trade_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_shield_bonus = readin;
#   ifdef REPLAY_2
    printf(" celeb_shield_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->celeb_food_bonus = readin;
#   ifdef REPLAY_2
    printf(" celeb_food_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->corruption_level = readin;
#   ifdef REPLAY_2
    printf(" corruption_level=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 30)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fixed_corruption_distance = readin;
#   ifdef REPLAY_2
    printf(" fixed_corruption_distance=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->corruption_distance_factor = readin;
#   ifdef REPLAY_2
    printf(" corruption_distance_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->extra_corruption_distance = readin;
#   ifdef REPLAY_2
    printf(" extra_corruption_distance=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->corruption_max_distance_cap = readin;
#   ifdef REPLAY_2
    printf(" corruption_max_distance_cap=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->waste_level = readin;
#   ifdef REPLAY_2
    printf(" waste_level=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 35)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fixed_waste_distance = readin;
#   ifdef REPLAY_2
    printf(" fixed_waste_distance=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 36)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->waste_distance_factor = readin;
#   ifdef REPLAY_2
    printf(" waste_distance_factor=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 37)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->extra_waste_distance = readin;
#   ifdef REPLAY_2
    printf(" extra_waste_distance=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 38)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->waste_max_distance_cap = readin;
#   ifdef REPLAY_2
    printf(" waste_max_distance_cap=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 39)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->flags = readin;
#   ifdef REPLAY_2
    printf(" flags=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 40)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_ruler_titles = readin;
#   ifdef REPLAY_2
    printf(" num_ruler_titles=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 41)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 42)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 43)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 44)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
#   ifdef REPLAY_2
    printf(" helptext=\"%s\"\n", real_packet->helptext);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=101 RULESET_TERRAIN_CONTROL");
# endif
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
# ifdef REPLAY_2
  printf(" may_road=%u", real_packet->may_road);
# endif

  real_packet->may_irrigate = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" may_irrigate=%u", real_packet->may_irrigate);
# endif

  real_packet->may_mine = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" may_mine=%u", real_packet->may_mine);
# endif

  real_packet->may_transform = BV_ISSET(fields, 3);
# ifdef REPLAY_2
  printf(" may_transform=%u", real_packet->may_transform);
# endif

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->ocean_reclaim_requirement_pct = readin;
#   ifdef REPLAY_2
    printf(" ocean_reclaim_requirement_pct=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->land_channel_requirement_pct = readin;
#   ifdef REPLAY_2
    printf(" land_channel_requirement_pct=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->river_move_mode = readin;
#   ifdef REPLAY_2
    printf(" river_move_mode=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->river_defense_bonus = readin;
#   ifdef REPLAY_2
    printf(" river_defense_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->river_trade_incr = readin;
#   ifdef REPLAY_2
    printf(" river_trade_incr=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->river_help_text, sizeof(real_packet->river_help_text));
#   ifdef REPLAY_2
    printf(" river_help_text=\"%s\"", real_packet->river_help_text);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fortress_defense_bonus = readin;
#   ifdef REPLAY_2
    printf(" fortress_defense_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->road_superhighway_trade_bonus = readin;
#   ifdef REPLAY_2
    printf(" road_superhighway_trade_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_food_bonus = readin;
#   ifdef REPLAY_2
    printf(" rail_food_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_shield_bonus = readin;
#   ifdef REPLAY_2
    printf(" rail_shield_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->rail_trade_bonus = readin;
#   ifdef REPLAY_2
    printf(" rail_trade_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->farmland_supermarket_food_bonus = readin;
#   ifdef REPLAY_2
    printf(" farmland_supermarket_food_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_food_penalty = readin;
#   ifdef REPLAY_2
    printf(" pollution_food_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_shield_penalty = readin;
#   ifdef REPLAY_2
    printf(" pollution_shield_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->pollution_trade_penalty = readin;
#   ifdef REPLAY_2
    printf(" pollution_trade_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_food_penalty = readin;
#   ifdef REPLAY_2
    printf(" fallout_food_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_shield_penalty = readin;
#   ifdef REPLAY_2
    printf(" fallout_shield_penalty=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->fallout_trade_penalty = readin;
#   ifdef REPLAY_2
    printf(" fallout_trade_penalty=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=102 RULESET_NATION");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->name_plural, sizeof(real_packet->name_plural));
#   ifdef REPLAY_2
    printf(" name_plural=\"%s\"", real_packet->name_plural);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_str, sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->graphic_alt, sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->class_, sizeof(real_packet->class_));
#   ifdef REPLAY_2
    printf(" class_=\"%s\"", real_packet->class_);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->legend, sizeof(real_packet->legend));
#   ifdef REPLAY_2
    printf(" legend=\"%s\"", real_packet->legend);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->city_style = readin;
#   ifdef REPLAY_2
    printf(" city_style=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_tech_list(&din, real_packet->init_techs);
#   ifdef REPLAY_2
    printf(" init_techs=");
    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (real_packet->init_techs[i] == A_LAST) {
        break;
      } else {
        printf(" %u", real_packet->init_techs[i]);
      }
    }
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->leader_count = readin;
#   ifdef REPLAY_2
    printf(" leader_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int i;

    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
#   ifdef REPLAY_2
    printf(" leader_name[i]=");
#   endif
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_string(&din, real_packet->leader_name[i], sizeof(real_packet->leader_name[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->leader_name[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 11)) {
    int i;

    if(real_packet->leader_count > MAX_NUM_LEADERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->leader_count = MAX_NUM_LEADERS;
    }
#   ifdef REPLAY_2
    printf("leader_sex[i]=");
#   endif
    for (i = 0; i < real_packet->leader_count; i++) {
      dio_get_bool8(&din, &real_packet->leader_sex[i]);
#     ifdef REPLAY_2
      printf(" %u", real_packet->leader_sex[i]);
#     endif
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=103 RULESET_CITY");
# endif
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
#   ifdef REPLAY_2
    printf(" style_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techreq = readin;
#   ifdef REPLAY_2
    printf(" techreq=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->citizens_graphic,
                   sizeof(real_packet->citizens_graphic));
#   ifdef REPLAY_2
    printf(" citizens_graphic=\"%s\"", real_packet->citizens_graphic);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->citizens_graphic_alt,
                   sizeof(real_packet->citizens_graphic_alt));
#   ifdef REPLAY_2
    printf(" citizens_graphic_alt=\"%s\"", real_packet->citizens_graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    dio_get_string(&din, real_packet->graphic, sizeof(real_packet->graphic));
#   ifdef REPLAY_2
    printf(" graphic=\"%s\"", real_packet->graphic);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint8(&din, &readin);
    real_packet->replaced_by = readin;
#   ifdef REPLAY_2
    printf(" replaced_by=%u", readin);
#   endif
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
# ifdef REPLAY_2
  printf("sc op=104 RULESET_BUILDING");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name,
                   sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->graphic_str,
                   sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->tech_req = readin;
#   ifdef REPLAY_2
    printf(" tech_req=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->obsolete_by = readin;
#   ifdef REPLAY_2
    printf(" obsolete_by=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->bldg_req = readin;
#   ifdef REPLAY_2
    printf(" bldg_req=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->replaced_by = readin;
#   ifdef REPLAY_2
    printf(" replaced_by=%u", readin);
#   endif
  }
  real_packet->is_wonder = BV_ISSET(fields, 8);
# ifdef REPLAY_2
  printf(" is_wonder=%u", real_packet->is_wonder);
# endif

  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->equiv_range = readin;
#   ifdef REPLAY_2
    printf(" equiv_range=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->build_cost = readin;
#   ifdef REPLAY_2
    printf(" build_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->upkeep = readin;
#   ifdef REPLAY_2
    printf(" upkeep=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->sabotage = readin;
#   ifdef REPLAY_2
    printf(" sabotage=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    dio_get_string(&din, real_packet->soundtag,
                   sizeof(real_packet->soundtag));
#   ifdef REPLAY_2
    printf(" soundtag=\"%s\"", real_packet->soundtag);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    dio_get_string(&din, real_packet->soundtag_alt,
                   sizeof(real_packet->soundtag_alt));
#   ifdef REPLAY_2
    printf(" soundtag_alt=\"%s\"", real_packet->soundtag_alt);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    dio_get_string(&din, real_packet->helptext, sizeof(real_packet->helptext));
#   ifdef REPLAY_2
    printf(" helptext=\"%s\"", real_packet->helptext);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->terr_gate_count = readin;
#   ifdef REPLAY_2
    printf(" terr_gate_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int i;

    if(real_packet->terr_gate_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->terr_gate_count = 255;
    }
#   ifdef REPLAY_2
    printf(" terr_gate[i]=");
#   endif
    for (i = 0; i < real_packet->terr_gate_count; i++) {
      int readin;

      dio_get_sint16(&din, &readin);
      real_packet->terr_gate[i] = readin;
#     ifdef REPLAY_2
      printf(" %d", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->spec_gate_count = readin;
#   ifdef REPLAY_2
    printf(" spec_gate_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int i;

    if(real_packet->spec_gate_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->spec_gate_count = 255;
    }
#   ifdef REPLAY_2
    printf(" spec_gate[i]=");
#   endif
    for (i = 0; i < real_packet->spec_gate_count; i++) {
      int readin;

      dio_get_uint16(&din, &readin);
      real_packet->spec_gate[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
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
#   ifdef REPLAY_2
    printf(" equiv_dupl[i]=");
#   endif
    for (i = 0; i < real_packet->equiv_dupl_count; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->equiv_dupl[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->equiv_repl_count = readin;
#   ifdef REPLAY_2
    printf(" equiv_repl_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int i;

    if(real_packet->equiv_repl_count > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->equiv_repl_count = 255;
    }
#   ifdef REPLAY_2
    printf(" equiv_repl[i]=");
#   endif
    for (i = 0; i < real_packet->equiv_repl_count; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->equiv_repl[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=105 RULESET_TERRAIN");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
#   ifdef REPLAY_2
    int i;
#   endif

    DIO_BV_GET(&din, real_packet->tags);
#   ifdef REPLAY_2
    printf(" tags=");
    for (i = 0; i < 8; i++ ) {
      printf(" %0X", real_packet->tags.vec[i]);
    }
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->terrain_name, sizeof(real_packet->terrain_name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->terrain_name);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->graphic_str,
                   sizeof(real_packet->graphic_str));
#   ifdef REPLAY_2
    printf(" graphic_str=\"%s\"", real_packet->graphic_str);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    dio_get_string(&din, real_packet->graphic_alt,
                   sizeof(real_packet->graphic_alt));
#   ifdef REPLAY_2
    printf(" graphic_alt=\"%s\"", real_packet->graphic_alt);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->movement_cost = readin;
#   ifdef REPLAY_2
    printf(" movement_cost=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->defense_bonus = readin;
#   ifdef REPLAY_2
    printf(" defense_bonus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food = readin;
#   ifdef REPLAY_2
    printf(" food=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield = readin;
#   ifdef REPLAY_2
    printf(" shield=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade = readin;
#   ifdef REPLAY_2
    printf(" trade=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    dio_get_string(&din, real_packet->special_1_name,
                   sizeof(real_packet->special_1_name));
#   ifdef REPLAY_2
    printf(" special_1_name=\"%s\"", real_packet->special_1_name);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_special_1 = readin;
#   ifdef REPLAY_2
    printf(" food_special_1=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_special_1 = readin;
#   ifdef REPLAY_2
    printf(" shield_special_1=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_special_1 = readin;
#   ifdef REPLAY_2
    printf(" trade_special_1=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    dio_get_string(&din, real_packet->graphic_str_special_1,
                   sizeof(real_packet->graphic_str_special_1));
#   ifdef REPLAY_2
    printf(" graphic_str_special_1=\"%s\"", real_packet->graphic_str_special_1);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    dio_get_string(&din, real_packet->graphic_alt_special_1, sizeof(real_packet->graphic_alt_special_1));
#   ifdef REPLAY_2
    printf(" graphic_alt_special_1=\"%s\"", real_packet->graphic_alt_special_1);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    dio_get_string(&din, real_packet->special_2_name, sizeof(real_packet->special_2_name));
#   ifdef REPLAY_2
    printf(" special_2_name=\"%s\"", real_packet->special_2_name);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->food_special_2 = readin;
#   ifdef REPLAY_2
    printf(" food_special_2=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->shield_special_2 = readin;
#   ifdef REPLAY_2
    printf(" shield_special_2=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->trade_special_2 = readin;
#   ifdef REPLAY_2
    printf(" trade_special_2=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 20)) {
    dio_get_string(&din, real_packet->graphic_str_special_2,
                   sizeof(real_packet->graphic_str_special_2));
#   ifdef REPLAY_2
    printf(" graphic_str_special_2=\"%s\"", real_packet->graphic_str_special_2);
#   endif
  }
  if (BV_ISSET(fields, 21)) {
    dio_get_string(&din, real_packet->graphic_alt_special_2,
                   sizeof(real_packet->graphic_alt_special_2));
#   ifdef REPLAY_2
    printf(" graphic_alt_special_2=\"%s\"", real_packet->graphic_alt_special_2);
#   endif
  }
  if (BV_ISSET(fields, 22)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->road_trade_incr = readin;
#   ifdef REPLAY_2
    printf(" road_trade_incr=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->road_time = readin;
#   ifdef REPLAY_2
    printf(" road_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 24)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->irrigation_result = readin;
#   ifdef REPLAY_2
    printf(" irrigation_result=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 25)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->irrigation_food_incr = readin;
#   ifdef REPLAY_2
    printf(" irrigation_food_incr=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 26)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->irrigation_time = readin;
#   ifdef REPLAY_2
    printf(" irrigation_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 27)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->mining_result = readin;
#   ifdef REPLAY_2
    printf(" mining_result=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 28)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->mining_shield_incr = readin;
#   ifdef REPLAY_2
    printf(" mining_shield_incr=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 29)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->mining_time = readin;
#   ifdef REPLAY_2
    printf(" mining_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 30)) {
    int readin;

    dio_get_sint16(&din, &readin);
    real_packet->transform_result = readin;
#   ifdef REPLAY_2
    printf(" transform_result=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 31)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->transform_time = readin;
#   ifdef REPLAY_2
    printf(" transform_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 32)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rail_time = readin;
#   ifdef REPLAY_2
    printf(" rail_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 33)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->airbase_time = readin;
#   ifdef REPLAY_2
    printf(" airbase_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 34)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fortress_time = readin;
#   ifdef REPLAY_2
    printf(" fortress_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 35)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->clean_pollution_time = readin;
#   ifdef REPLAY_2
    printf(" clean_pollution_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 36)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->clean_fallout_time = readin;
#   ifdef REPLAY_2
    printf(" clean_fallout_time=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 37)) {
    dio_get_string(&din, real_packet->helptext,
                   sizeof(real_packet->helptext));
#   ifdef REPLAY_2
    printf(" helptext=\"%s\"\n", real_packet->helptext);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=106 RULESET_CONTROL");
# endif
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
#   ifdef REPLAY_2
    printf(" aqueduct_size=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->add_to_size_limit = readin;
#   ifdef REPLAY_2
    printf(" add_to_size_limit=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->notradesize = readin;
#   ifdef REPLAY_2
    printf(" notradesize=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->fulltradesize = readin;
#   ifdef REPLAY_2
    printf(" fulltradesize=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_unit_types = readin;
#   ifdef REPLAY_2
    printf(" num_unit_types=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_impr_types = readin;
#   ifdef REPLAY_2
    printf(" num_impr_types=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_tech_types = readin;
#   ifdef REPLAY_2
    printf(" num_tech_types=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_cathedral_plus = readin;
#   ifdef REPLAY_2
    printf(" rtech_cathedral_plus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_cathedral_minus = readin;
#   ifdef REPLAY_2
    printf(" rtech_cathedral_minus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_colosseum_plus = readin;
#   ifdef REPLAY_2
    printf(" rtech_colosseum_plus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->rtech_temple_plus = readin;
#   ifdef REPLAY_2
    printf(" rtech_temple_plus=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
#   ifdef REPLAY_2
    int i;
#   endif

    dio_get_tech_list(&din, real_packet->rtech_partisan_req);
#   ifdef REPLAY_2
    printf("rtech_partisan_req[%d]={", MAX_NUM_TECH_LIST);
    for ( i = 0; i < MAX_NUM_TECH_LIST; i++) {
      printf(" %u", real_packet->rtech_partisan_req[i]);
    }
    printf("}");
#   endif
  }
  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government_when_anarchy = readin;
#   ifdef REPLAY_2
    printf(" government_when_anarchy=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->default_government = readin;
#   ifdef REPLAY_2
    printf(" default_government=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 14)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->government_count = readin;
#   ifdef REPLAY_2
    printf(" government_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 15)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nation_count = readin;
#   ifdef REPLAY_2
    printf(" nation_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 16)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->playable_nation_count = readin;
#   ifdef REPLAY_2
    printf(" playable_nation_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 17)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->style_count = readin;
#   ifdef REPLAY_2
    printf(" style_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 18)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->terrain_count = readin;
#   ifdef REPLAY_2
    printf(" terrain_count=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 19)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->borders = readin;
#   ifdef REPLAY_2
    printf(" borders=%u", readin);
#   endif
  }
  real_packet->happyborders = BV_ISSET(fields, 20);
# ifdef REPLAY_2
  printf(" happyborders=%u", real_packet->happyborders);
# endif

  real_packet->slow_invasions = BV_ISSET(fields, 21);
# ifdef REPLAY_2
  printf(" slow_invasions=%u", real_packet->slow_invasions);
# endif

  if (BV_ISSET(fields, 22)) {
    int i;

    for (i = 0; i < MAX_NUM_TEAMS; i++) {
      dio_get_string(&din, real_packet->team_name[i], sizeof(real_packet->team_name[i]));
#     ifdef REPLAY_2
      printf(" team_name[i]=\"%s\"", (char*)&real_packet->team_name[0]);
#     endif
    }
  }
  if (BV_ISSET(fields, 23)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->default_building = readin;
#   ifdef REPLAY_2
    printf(" default_building=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
#define hash_packet_unit_load_100 hash_const

#define cmp_packet_unit_load_100 cmp_const

BV_DEFINE(packet_unit_load_100_fields, 2);

static int send_packet_unit_load_100(
               connection_t *pconn,
               const struct packet_unit_load *packet)
{
  const struct packet_unit_load *real_packet = packet;
  packet_unit_load_100_fields fields;
  struct packet_unit_load *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_LOAD];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_LOAD);
# ifdef REPLAY_2
  printf("cs opc=107 UNIT_LOAD");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_load_100,
                     cmp_packet_unit_load_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->cargo_id != real_packet->cargo_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->transporter_id != real_packet->transporter_id);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->cargo_id);
# ifdef REPLAY_2
    printf(" cargo_id=%u", real_packet->cargo_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->transporter_id);
# ifdef REPLAY_2
    printf(" transporter_id=%u\n", real_packet->transporter_id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_load(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_LOAD] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_LOAD] = variant;
}

int send_packet_unit_load(
        connection_t *pconn,
        const struct packet_unit_load *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_load from the server.");
  }
  send_ensure_valid_variant_packet_unit_load(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_LOAD]) {
    case 100: {
      return send_packet_unit_load_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_load(
         connection_t *pconn,
         int cargo_id,
         int transporter_id)
{
  struct packet_unit_load packet, *real_packet = &packet;

  real_packet->cargo_id = cargo_id;
  real_packet->transporter_id = transporter_id;

  return send_packet_unit_load(pconn, real_packet);
}

/* 108 cs */
#define hash_packet_single_want_hack_req_100 hash_const

#define cmp_packet_single_want_hack_req_100 cmp_const

BV_DEFINE(packet_single_want_hack_req_100_fields, 1);

static int send_packet_single_want_hack_req_100(
               connection_t *pconn,
               const struct packet_single_want_hack_req *packet)
{
  const struct packet_single_want_hack_req *real_packet = packet;
  packet_single_want_hack_req_100_fields fields;
  struct packet_single_want_hack_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_SINGLE_WANT_HACK_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_SINGLE_WANT_HACK_REQ);
# ifdef REPLAY_2
  printf("cs opc=108 SINGLE_WANT_HACK_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_single_want_hack_req_100,
                     cmp_packet_single_want_hack_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (strcmp(old->token, real_packet->token) != 0);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_string(&dout, real_packet->token);
# ifdef REPLAY_2
    printf(" token=\"%s\"\n", real_packet->token);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

#define hash_packet_single_want_hack_req_101 hash_const

#define cmp_packet_single_want_hack_req_101 cmp_const

BV_DEFINE(packet_single_want_hack_req_101_fields, 1);

static int send_packet_single_want_hack_req_101(
               connection_t *pconn,
               const struct packet_single_want_hack_req *packet)
{
  const struct packet_single_want_hack_req *real_packet = packet;
  packet_single_want_hack_req_101_fields fields;
  struct packet_single_want_hack_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_SINGLE_WANT_HACK_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_SINGLE_WANT_HACK_REQ);
# ifdef REPLAY_2
  printf("cs opc=108 SINGLE_WANT_HACK_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_single_want_hack_req_101,
                     cmp_packet_single_want_hack_req_101);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->old_token != real_packet->old_token);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint32(&dout, real_packet->old_token);
# ifdef REPLAY_2
    printf(" old_token=%0X\n", real_packet->old_token);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_single_want_hack_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REQ] != -1) {
    return;
  }

  if((has_capability("new_hack", pconn->capability)
      && has_capability("new_hack", our_capability)))
  {
    variant = 100;
  } else if(!(has_capability("new_hack", pconn->capability)
              && has_capability("new_hack", our_capability)))
  {
    variant = 101;
  } else {
    die("unknown variant");
  }
  pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REQ] = variant;
}

int send_packet_single_want_hack_req(
        connection_t *pconn,
        const struct packet_single_want_hack_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_single_want_hack_req from the server.");
  }
  send_ensure_valid_variant_packet_single_want_hack_req(pconn);

  switch(pconn->phs.variant[PACKET_SINGLE_WANT_HACK_REQ]) {
    case 100: {
      return send_packet_single_want_hack_req_100(pconn, packet);
    }
    case 101: {
      return send_packet_single_want_hack_req_101(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

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
# ifdef REPLAY_2
  printf("sc op=109 SINGLE_WANT_HACK_REPLY");
# endif
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
# ifdef REPLAY_2
  printf(" you_have_hack=%u\n", real_packet->you_have_hack);
# endif

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
# ifdef REPLAY_2
  printf("sc op=111 GAME_LOAD");
# endif
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
# ifdef REPLAY_2
  printf(" load_successful=%u", real_packet->load_successful);
# endif

  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->nplayers = readin;
#   ifdef REPLAY_2
    printf(" nplayers=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->load_filename, sizeof(real_packet->load_filename));
#   ifdef REPLAY_2
    printf(" load_filename=\"%s\"", real_packet->load_filename);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" name[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->name[i], sizeof(real_packet->name[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->name[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 4)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" username[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->username[i], sizeof(real_packet->username[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->username[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 5)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" nation_name[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->nation_name[i], sizeof(real_packet->nation_name[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->nation_name[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 6)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" nation_flag[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_string(&din, real_packet->nation_flag[i], sizeof(real_packet->nation_flag[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->nation_flag[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 7)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" is_alive[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_alive[i]);
#     ifdef REPLAY_2
      printf(" %u", real_packet->is_alive[i]);
#     endif
    }
  }
  if (BV_ISSET(fields, 8)) {
    int i;

    if(real_packet->nplayers > MAX_NUM_PLAYERS) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->nplayers = MAX_NUM_PLAYERS;
    }
#   ifdef REPLAY_2
    printf(" is_ai[i]=");
#   endif
    for (i = 0; i < real_packet->nplayers; i++) {
      dio_get_bool8(&din, &real_packet->is_ai[i]);
#     ifdef REPLAY_2
      printf(" %u", real_packet->is_ai[i]);
#     endif
     }
#   ifdef REPLAY_2
    printf("\n");
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=112 OPTIONS_SETTABLE_CONTROL");
# endif
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
#   ifdef REPLAY_2
    printf(" nids=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->ncategories = readin;
#   ifdef REPLAY_2
    printf(" ncategories=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->ncategories > 256) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->ncategories = 256;
    }
#   ifdef REPLAY_2
    printf(" category_names=");
#   endif
    for (i = 0; i < real_packet->ncategories; i++) {
      dio_get_string(&din, real_packet->category_names[i],
                     sizeof(real_packet->category_names[i]));
#     ifdef REPLAY_2
      printf(" \"%s\"", real_packet->category_names[i]);
#     endif
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=113 OPTIONS_SETTABLE");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->name, sizeof(real_packet->name));
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->short_help, sizeof(real_packet->short_help));
#   ifdef REPLAY_2
    printf(" short_help=\"%s\"", real_packet->short_help);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    dio_get_string(&din, real_packet->extra_help, sizeof(real_packet->extra_help));
#   ifdef REPLAY_2
    printf(" extra_help=\"%s\"", real_packet->extra_help);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->type = readin;
#   ifdef REPLAY_2
    printf(" type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->val = readin;
#   ifdef REPLAY_2
    printf(" val=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->default_val = readin;
#   ifdef REPLAY_2
    printf(" default_val=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->min = readin;
#   ifdef REPLAY_2
    printf(" min=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->max = readin;
#   ifdef REPLAY_2
    printf(" max=%d", readin);
#   endif
  }
  if (BV_ISSET(fields, 9)) {
    dio_get_string(&din, real_packet->strval, sizeof(real_packet->strval));
#   ifdef REPLAY_2
    printf(" strval=\"%s\"", real_packet->strval);
#   endif
  }
  if (BV_ISSET(fields, 10)) {
    dio_get_string(&din, real_packet->default_strval, sizeof(real_packet->default_strval));
#   ifdef REPLAY_2
    printf(" default_strval=\"%s\"", real_packet->default_strval);
#   endif
  }
  if (BV_ISSET(fields, 11)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->category = readin;
#   ifdef REPLAY_2
    printf(" category=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=114 SELECT_RACES\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=120 RULESET_CACHE_GROUP");
# endif
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
#   ifdef REPLAY_2
    printf(" name=\"%s\"", real_packet->name);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_elements = readin;
#   ifdef REPLAY_2
    printf(" num_elements=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
#   ifdef REPLAY_2
    printf(" source_buildings[i]=");
#   endif
    for (i = 0; i < real_packet->num_elements; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->source_buildings[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 3)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
#   ifdef REPLAY_2
    printf(" ranges[i]=");
#   endif
    for (i = 0; i < real_packet->num_elements; i++) {
      int readin;

      dio_get_uint8(&din, &readin);
      real_packet->ranges[i] = readin;
#     ifdef REPLAY_2
      printf(" %u", readin);
#     endif
    }
  }
  if (BV_ISSET(fields, 4)) {
    int i;

    if(real_packet->num_elements > 255) {
      freelog(LOG_ERROR, "packets_gen.c: WARNING: truncation array");
      real_packet->num_elements = 255;
    }
#   ifdef REPLAY_2
    printf(" survives[i]=");
#   endif
    for (i = 0; i < real_packet->num_elements; i++) {
      dio_get_bool8(&din, &real_packet->survives[i]);
#     ifdef REPLAY_2
      printf(" %u", real_packet->survives[i]);
#     endif
    }
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=121 RULESET_CACHE_EFFECT");
# endif
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
#   ifdef REPLAY_2
    printf(" id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->effect_type = readin;
#   ifdef REPLAY_2
    printf(" effect_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->range = readin;
#   ifdef REPLAY_2
    printf(" range=%u", readin);
#   endif
  }
  real_packet->survives = BV_ISSET(fields, 3);
# ifdef REPLAY_2
  printf(" survives=%u", real_packet->survives);
# endif

  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->eff_value = readin;
#   ifdef REPLAY_2
    printf(" eff_value=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 5)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->req_type = readin;
#   ifdef REPLAY_2
    printf(" req_type=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 6)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->req_value = readin;
#   ifdef REPLAY_2
    printf(" req_value=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 7)) {
    int readin;

    dio_get_sint32(&din, &readin);
    real_packet->group_id = readin;
#   ifdef REPLAY_2
    printf(" group_id=%u", readin);
#   endif
  }
# ifdef REPLAY_2
  printf("\n");
# endif

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
# ifdef REPLAY_2
  printf("sc op=122 TRADEROUTE_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" trademindist=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuepct = readin;
#   ifdef REPLAY_2
    printf(" traderevenuepct=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuestyle = readin;
#   ifdef REPLAY_2
    printf(" traderevenuestyle=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->caravanbonusstyle = readin;
#   ifdef REPLAY_2
    printf(" caravanbonusstyle=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->maxtraderoutes = readin;
#   ifdef REPLAY_2
    printf(" maxtraderoutes=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=122 TRADEROUTE_INFO");
# endif
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
#   ifdef REPLAY_2
    printf(" trademindist=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuepct = readin;
#   ifdef REPLAY_2
    printf(" traderevenuepct=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->traderevenuestyle = readin;
#   ifdef REPLAY_2
    printf(" traderevenuestyle=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->caravanbonusstyle = readin;
#   ifdef REPLAY_2
    printf(" caravanbonusstyle=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=123 EXTGAME_INFO");
# endif
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
# ifdef REPLAY_2
  printf(" futuretechsscore=%u", real_packet->futuretechsscore);
# endif

  real_packet->improvedautoattack = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" improvedautoattack=%u", real_packet->improvedautoattack);
# endif

  real_packet->stackbribing = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" stackbribing=%u", real_packet->stackbribing);
# endif

  real_packet->experimentalbribingcost = BV_ISSET(fields, 3);
# ifdef REPLAY_2
  printf(" experimentalbribingcost=%u", real_packet->experimentalbribingcost);
# endif

  real_packet->techtrading = BV_ISSET(fields, 4);
# ifdef REPLAY_2
  printf(" techtrading=%u", real_packet->techtrading);
# endif

  real_packet->ignoreruleset = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" ignoreruleset=%u", real_packet->ignoreruleset);
# endif

  real_packet->goldtrading = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" goldtrading=%u", real_packet->goldtrading);
# endif

  real_packet->citytrading = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" citytrading=%u", real_packet->citytrading);
# endif

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->airliftingstyle = readin;
#   ifdef REPLAY_2
    printf(" airliftingstyle=%u", readin);
#   endif
  }
  real_packet->teamplacement = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" teamplacement=%u", real_packet->teamplacement);
# endif

  real_packet->globalwarmingon = BV_ISSET(fields, 10);
# ifdef REPLAY_2
  printf(" globalwarmingon=%u", real_packet->globalwarmingon);
# endif

  real_packet->nuclearwinteron = BV_ISSET(fields, 11);
# ifdef REPLAY_2
  printf(" nuclearwinteron=%u", real_packet->nuclearwinteron);
# endif

  if (BV_ISSET(fields, 12)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->maxallies = readin;
#   ifdef REPLAY_2
    printf(" maxallies=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 13)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->techleakagerate = readin;
#   ifdef REPLAY_2
    printf(" techleakagerate=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=123 EXTGAME_INFO");
# endif
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
# ifdef REPLAY_2
  printf(" futuretechsscore=%u", real_packet->futuretechsscore);
# endif

  real_packet->improvedautoattack = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" improvedautoattack=%u", real_packet->improvedautoattack);
# endif

  real_packet->stackbribing = BV_ISSET(fields, 2);
# ifdef REPLAY_2
  printf(" stackbribing=%u", real_packet->stackbribing);
# endif

  real_packet->experimentalbribingcost = BV_ISSET(fields, 3);
# ifdef REPLAY_2
  printf(" experimentalbribingcost=%u", real_packet->experimentalbribingcost);
# endif

  real_packet->techtrading = BV_ISSET(fields, 4);
# ifdef REPLAY_2
  printf(" techtrading =%u", real_packet->techtrading);
# endif

  real_packet->ignoreruleset = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" ignoreruleset =%u", real_packet->ignoreruleset);
# endif

  real_packet->goldtrading = BV_ISSET(fields, 6);
# ifdef REPLAY_2
  printf(" goldtrading =%u", real_packet->goldtrading);
# endif

  real_packet->citytrading = BV_ISSET(fields, 7);
# ifdef REPLAY_2
  printf(" citytrading =%u", real_packet->citytrading);
# endif

  if (BV_ISSET(fields, 8)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->airliftingstyle = readin;
#   ifdef REPLAY_2
    printf(" airliftingstyle =%u", readin);
#   endif
  }
  real_packet->teamplacement = BV_ISSET(fields, 9);
# ifdef REPLAY_2
  printf(" teamplacement =%u", real_packet->teamplacement);
# endif

  real_packet->globalwarmingon = BV_ISSET(fields, 10);
# ifdef REPLAY_2
  printf(" globalwarmingon =%u", real_packet->globalwarmingon);
# endif

  real_packet->nuclearwinteron = BV_ISSET(fields, 11);
# ifdef REPLAY_2
  printf(" nuclearwinteron =%u\n", real_packet->nuclearwinteron);
# endif

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
# ifdef REPLAY_2
  printf("sc op=124 VOTE_NEW");
# endif
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
#   ifdef REPLAY_2
    printf(" vote_no=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_get_string(&din, real_packet->user, sizeof(real_packet->user));
#   ifdef REPLAY_2
    printf(" user=\"%s\"", real_packet->user);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_get_string(&din, real_packet->desc, sizeof(real_packet->desc));
#   ifdef REPLAY_2
    printf(" desc=\"%s\"", real_packet->desc);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->percent_required = readin;
#   ifdef REPLAY_2
    printf(" percent_required=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint32(&din, &readin);
    real_packet->flags = readin;
#   ifdef REPLAY_2
    printf(" flags=%u", readin);
#   endif
  }
  real_packet->is_poll = BV_ISSET(fields, 5);
# ifdef REPLAY_2
  printf(" is_poll=%u\n", real_packet->is_poll);
# endif

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
# ifdef REPLAY_2
  printf("sc op-125 VOTE_UPDATE");
# endif
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
#   ifdef REPLAY_2
    printf(" vote_no=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->yes = readin;
#   ifdef REPLAY_2
    printf(" yes=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->no = readin;
#   ifdef REPLAY_2
    printf(" no=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->abstain = readin;
#   ifdef REPLAY_2
    printf(" abstain=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 4)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->num_voters = readin;
#   ifdef REPLAY_2
    printf(" num_voters=%u\n", readin);
#   endif
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
# ifdef REPLAY_2
  printf("sc op=126 VOTE_REMOVE");
# endif
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
#   ifdef REPLAY_2
    printf(" vote_no=%u\n", readin);
  } else {
    printf("\n");
#   endif
  }

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
# ifdef REPLAY_2
  printf("sc op=127 VOTE_RESOLVE");
# endif
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
#   ifdef REPLAY_2
    printf(" vote_no=%u", readin);
#   endif
  }
  real_packet->passed = BV_ISSET(fields, 1);
# ifdef REPLAY_2
  printf(" passed=%u", real_packet->passed);
# endif

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
#define hash_packet_vote_submit_100 hash_const

#define cmp_packet_vote_submit_100 cmp_const

BV_DEFINE(packet_vote_submit_100_fields, 2);

static int send_packet_vote_submit_100(
               connection_t *pconn,
               const struct packet_vote_submit *packet)
{
  const struct packet_vote_submit *real_packet = packet;
  packet_vote_submit_100_fields fields;
  struct packet_vote_submit *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = FALSE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_VOTE_SUBMIT];
  int different = 0;

  SEND_PACKET_START(PACKET_VOTE_SUBMIT);
# ifdef REPLAY_2
  printf("cs opc=128 VOTE_SUBMIT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_vote_submit_100,
                     cmp_packet_vote_submit_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->vote_no != real_packet->vote_no);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->value != real_packet->value);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint32(&dout, real_packet->vote_no);
# ifdef REPLAY_2
    printf(" vote_no=%u", real_packet->vote_no);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_sint8(&dout, real_packet->value);
# ifdef REPLAY_2
    printf(" value=%d\n", real_packet->value);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_vote_submit(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_VOTE_SUBMIT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_VOTE_SUBMIT] = variant;
}

int send_packet_vote_submit(
        connection_t *pconn,
        const struct packet_vote_submit *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_vote_submit from the server.");
  }
  send_ensure_valid_variant_packet_vote_submit(pconn);

  switch(pconn->phs.variant[PACKET_VOTE_SUBMIT]) {
    case 100: {
      return send_packet_vote_submit_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

/* 129 */
/* 130 cs */
#define hash_packet_trade_route_plan_100 hash_const

#define cmp_packet_trade_route_plan_100 cmp_const

BV_DEFINE(packet_trade_route_plan_100_fields, 2);

static int send_packet_trade_route_plan_100(
               connection_t *pconn,
               const struct packet_trade_route_plan *packet)
{
  const struct packet_trade_route_plan *real_packet = packet;
  packet_trade_route_plan_100_fields fields;
  struct packet_trade_route_plan *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_TRADE_ROUTE_PLAN];
  int different = 0;

  SEND_PACKET_START(PACKET_TRADE_ROUTE_PLAN);
# ifdef REPLAY_2
  printf("cs opc=130 TRADE_ROUTE_PLAN");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_trade_route_plan_100,
                     cmp_packet_trade_route_plan_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city1 != real_packet->city1);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->city2 != real_packet->city2);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city1);
# ifdef REPLAY_2
    printf(" city1=%u", real_packet->city1);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->city2);
# ifdef REPLAY_2
    printf(" city2=%u\n", real_packet->city2);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_trade_route_plan(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_TRADE_ROUTE_PLAN] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_TRADE_ROUTE_PLAN] = variant;
}

int send_packet_trade_route_plan(
        connection_t *pconn,
        const struct packet_trade_route_plan *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_trade_route_plan from the server.");
  }
  send_ensure_valid_variant_packet_trade_route_plan(pconn);

  switch(pconn->phs.variant[PACKET_TRADE_ROUTE_PLAN]) {
    case 100: {
      return send_packet_trade_route_plan_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_trade_route_plan(
         connection_t *pconn,
         int city1,
         int city2)
{
  struct packet_trade_route_plan packet, *real_packet = &packet;

  real_packet->city1 = city1;
  real_packet->city2 = city2;

  return send_packet_trade_route_plan(pconn, real_packet);
}

/* 131 cs */
#define hash_packet_trade_route_remove_100 hash_const

#define cmp_packet_trade_route_remove_100 cmp_const

BV_DEFINE(packet_trade_route_remove_100_fields, 2);

static int send_packet_trade_route_remove_100(
               connection_t *pconn,
               const struct packet_trade_route_remove *packet)
{
  const struct packet_trade_route_remove *real_packet = packet;
  packet_trade_route_remove_100_fields fields;
  struct packet_trade_route_remove *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_TRADE_ROUTE_REMOVE];
  int different = 0;

  SEND_PACKET_START(PACKET_TRADE_ROUTE_REMOVE);
# ifdef REPLAY_2
  printf("cs opc=131 TRADE_ROUTE_REMOVE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_trade_route_remove_100,
                     cmp_packet_trade_route_remove_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->city1 != real_packet->city1);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->city2 != real_packet->city2);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->city1);
# ifdef REPLAY_2
    printf(" city1=%u", real_packet->city1);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->city2);
# ifdef REPLAY_2
    printf(" city2=%u\n", real_packet->city2);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_trade_route_remove(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_TRADE_ROUTE_REMOVE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_TRADE_ROUTE_REMOVE] = variant;
}

int send_packet_trade_route_remove(
        connection_t *pconn,
        const struct packet_trade_route_remove *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  send_ensure_valid_variant_packet_trade_route_remove(pconn);

  switch(pconn->phs.variant[PACKET_TRADE_ROUTE_REMOVE]) {
    case 100: {
      return send_packet_trade_route_remove_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_trade_route_remove(
         connection_t *pconn,
         int city1,
         int city2)
{
  struct packet_trade_route_remove packet, *real_packet = &packet;

  real_packet->city1 = city1;
  real_packet->city2 = city2;

  return send_packet_trade_route_remove(pconn, real_packet);
}

/* 132 cs */
#define hash_packet_unit_trade_route_100 hash_const

#define cmp_packet_unit_trade_route_100 cmp_const

BV_DEFINE(packet_unit_trade_route_100_fields, 3);

static int send_packet_unit_trade_route_100(
               connection_t *pconn,
               const struct packet_unit_trade_route *packet)
{
  const struct packet_unit_trade_route *real_packet = packet;
  packet_unit_trade_route_100_fields fields;
  struct packet_unit_trade_route *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_TRADE_ROUTE];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_TRADE_ROUTE);
# ifdef REPLAY_2
  printf("cs opc=132 UNIT_TRADE_ROUTE");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_trade_route_100,
                     cmp_packet_unit_trade_route_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->unit_id != real_packet->unit_id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->city1 != real_packet->city1);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->city2 != real_packet->city2);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->unit_id);
# ifdef REPLAY_2
    printf(" unit_id=%u", real_packet->unit_id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint16(&dout, real_packet->city1);
# ifdef REPLAY_2
    printf(" city1=%u", real_packet->city1);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint16(&dout, real_packet->city2);
# ifdef REPLAY_2
    printf(" city2=%u\n", real_packet->city2);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_trade_route(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_TRADE_ROUTE] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_TRADE_ROUTE] = variant;
}

int send_packet_unit_trade_route(
        connection_t *pconn,
        const struct packet_unit_trade_route *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_trade_route from the server.");
  }
  send_ensure_valid_variant_packet_unit_trade_route(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_TRADE_ROUTE]) {
    case 100: {
      return send_packet_unit_trade_route_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_trade_route(
         connection_t *pconn,
         int unit_id,
         int city1, int city2)
{
  struct packet_unit_trade_route packet, *real_packet = &packet;

  real_packet->unit_id = unit_id;
  real_packet->city1 = city1;
  real_packet->city2 = city2;

  return send_packet_unit_trade_route(pconn, real_packet);
}

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
# ifdef REPLAY_2
  printf("sc op=133 TRADE_ROUTE_INFO\n");
# endif
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
#   ifdef REPLAY_2
    printf(" city1=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 1)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->city2 = readin;
#   ifdef REPLAY_2
    printf(" city2=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 2)) {
    int readin;

    dio_get_uint16(&din, &readin);
    real_packet->unit_id = readin;
#   ifdef REPLAY_2
    printf(" unit_id=%u", readin);
#   endif
  }
  if (BV_ISSET(fields, 3)) {
    int readin;

    dio_get_uint8(&din, &readin);
    real_packet->status = readin;
#   ifdef REPLAY_2
    printf(" status=%u\n", readin);
  } else {
    printf("\n");
#   endif
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
# ifdef REPLAY_2
  printf("sc op=135 FREEZE_CLIENT\n");
# endif
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
# ifdef REPLAY_2
  printf("sc op=136 THAW_CLIENT\n");
# endif
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
#define hash_packet_city_set_rally_point_100 hash_const

#define cmp_packet_city_set_rally_point_100 cmp_const

BV_DEFINE(packet_city_set_rally_point_100_fields, 3);

static int send_packet_city_set_rally_point_100(
               connection_t *pconn,
               const struct packet_city_set_rally_point *packet)
{
  const struct packet_city_set_rally_point *real_packet = packet;
  packet_city_set_rally_point_100_fields fields;
  struct packet_city_set_rally_point *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_SET_RALLY_POINT];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_SET_RALLY_POINT);
# ifdef REPLAY_2
  printf("cs opc=138 CITY_SET_RALLY_POINT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_set_rally_point_100,
                     cmp_packet_city_set_rally_point_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->id != real_packet->id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->x != real_packet->x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->y != real_packet->y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->id);
# ifdef REPLAY_2
    printf(" id=%u", real_packet->id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->x);
# ifdef REPLAY_2
    printf(" x=%u", real_packet->x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->y);
# ifdef REPLAY_2
    printf(" y=%u\n", real_packet->y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_set_rally_point(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_SET_RALLY_POINT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_SET_RALLY_POINT] = variant;
}

int send_packet_city_set_rally_point(
        connection_t *pconn,
        const struct packet_city_set_rally_point *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_set_rally_point from the server.");
  }
  send_ensure_valid_variant_packet_city_set_rally_point(pconn);

  switch(pconn->phs.variant[PACKET_CITY_SET_RALLY_POINT]) {
    case 100: {
      return send_packet_city_set_rally_point_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_set_rally_point(
         connection_t *pconn,
         int id,
         int x, int y)
{
  struct packet_city_set_rally_point packet, *real_packet = &packet;

  real_packet->id = id;
  real_packet->x = x;
  real_packet->y = y;

  return send_packet_city_set_rally_point(pconn, real_packet);
}

/* 139 cs */
#define hash_packet_city_clear_rally_point_100 hash_const

#define cmp_packet_city_clear_rally_point_100 cmp_const

BV_DEFINE(packet_city_clear_rally_point_100_fields, 1);

static int send_packet_city_clear_rally_point_100(
               connection_t *pconn,
               const struct packet_city_clear_rally_point *packet)
{
  const struct packet_city_clear_rally_point *real_packet = packet;
  packet_city_clear_rally_point_100_fields fields;
  struct packet_city_clear_rally_point *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_CITY_CLEAR_RALLY_POINT];
  int different = 0;

  SEND_PACKET_START(PACKET_CITY_CLEAR_RALLY_POINT);
# ifdef REPLAY_2
  printf("cs opc=139 CITY_CLEAR_RALLY_POINT");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_city_clear_rally_point_100,
                     cmp_packet_city_clear_rally_point_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->id != real_packet->id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->id);
# ifdef REPLAY_2
    printf(" id=%u\n", real_packet->id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_city_clear_rally_point(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_CITY_CLEAR_RALLY_POINT] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_CITY_CLEAR_RALLY_POINT] = variant;
}

int send_packet_city_clear_rally_point(
        connection_t *pconn,
        const struct packet_city_clear_rally_point *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_city_clear_rally_point from the server.");
  }
  send_ensure_valid_variant_packet_city_clear_rally_point(pconn);

  switch(pconn->phs.variant[PACKET_CITY_CLEAR_RALLY_POINT]) {
    case 100: {
      return send_packet_city_clear_rally_point_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_city_clear_rally_point(connection_t *pconn, int id)
{
  struct packet_city_clear_rally_point packet, *real_packet = &packet;

  real_packet->id = id;

  return send_packet_city_clear_rally_point(pconn, real_packet);
}

/* 140 */
/* 141 cs */
#define hash_packet_unit_air_patrol_100 hash_const

#define cmp_packet_unit_air_patrol_100 cmp_const

BV_DEFINE(packet_unit_air_patrol_100_fields, 3);

static int send_packet_unit_air_patrol_100(
               connection_t *pconn,
               const struct packet_unit_air_patrol *packet)
{
  const struct packet_unit_air_patrol *real_packet = packet;
  packet_unit_air_patrol_100_fields fields;
  struct packet_unit_air_patrol *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_AIR_PATROL];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_AIR_PATROL);
# ifdef REPLAY_2
  printf("cs opc=141 UNIT_AIR_PATROL");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_air_patrol_100,
                     cmp_packet_unit_air_patrol_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->id != real_packet->id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  differ = (old->x != real_packet->x);
  if (differ) {
    different++;
    BV_SET(fields, 1);
  }

  differ = (old->y != real_packet->y);
  if (differ) {
    different++;
    BV_SET(fields, 2);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->id);
# ifdef REPLAY_2
    printf("id=%u ", real_packet->id);
# endif
  }
  if (BV_ISSET(fields, 1)) {
    dio_put_uint8(&dout, real_packet->x);
# ifdef REPLAY_2
    printf(" x=%u ", real_packet->x);
# endif
  }
  if (BV_ISSET(fields, 2)) {
    dio_put_uint8(&dout, real_packet->y);
# ifdef REPLAY_2
    printf(" y=%u\n", real_packet->y);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_air_patrol(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_AIR_PATROL] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_AIR_PATROL] = variant;
}

int send_packet_unit_air_patrol(
        connection_t *pconn,
        const struct packet_unit_air_patrol *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_air_patrol from the server.");
  }
  send_ensure_valid_variant_packet_unit_air_patrol(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_AIR_PATROL]) {
    case 100: {
      return send_packet_unit_air_patrol_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_air_patrol(
         connection_t *pconn,
         int id,
         int x, int y)
{
  struct packet_unit_air_patrol packet, *real_packet = &packet;

  real_packet->id = id;
  real_packet->x = x;
  real_packet->y = y;

  return send_packet_unit_air_patrol(pconn, real_packet);
}

/* 142 cs */
#define hash_packet_unit_air_patrol_stop_100 hash_const

#define cmp_packet_unit_air_patrol_stop_100 cmp_const

BV_DEFINE(packet_unit_air_patrol_stop_100_fields, 1);

static int send_packet_unit_air_patrol_stop_100(
               connection_t *pconn,
               const struct packet_unit_air_patrol_stop *packet)
{
  const struct packet_unit_air_patrol_stop *real_packet = packet;
  packet_unit_air_patrol_stop_100_fields fields;
  struct packet_unit_air_patrol_stop *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_UNIT_AIR_PATROL_STOP];
  int different = 0;

  SEND_PACKET_START(PACKET_UNIT_AIR_PATROL_STOP);
# ifdef REPLAY_2
  printf("cs opc=142 UNIT_AIR_PATROL_STOP");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_unit_air_patrol_stop_100,
                     cmp_packet_unit_air_patrol_stop_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->id != real_packet->id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint16(&dout, real_packet->id);
# ifdef REPLAY_2
    printf(" id=%u\n", real_packet->id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_unit_air_patrol_stop(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_UNIT_AIR_PATROL_STOP] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_UNIT_AIR_PATROL_STOP] = variant;
}

int send_packet_unit_air_patrol_stop(
        connection_t *pconn,
        const struct packet_unit_air_patrol_stop *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_unit_air_patrol_stop from the server.");
  }
  send_ensure_valid_variant_packet_unit_air_patrol_stop(pconn);

  switch(pconn->phs.variant[PACKET_UNIT_AIR_PATROL_STOP]) {
    case 100: {
      return send_packet_unit_air_patrol_stop_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_unit_air_patrol_stop(connection_t *pconn, int id)
{
  struct packet_unit_air_patrol_stop packet, *real_packet = &packet;

  real_packet->id = id;

  return send_packet_unit_air_patrol_stop(pconn, real_packet);
}

/* 143 */
/* 144 */
/* 145 cs sc */
/* 146 cs sc */
/* 147 */
/* 148 */
/* 149 */
/* 150 cs */
#define hash_packet_player_info_req_100 hash_const

#define cmp_packet_player_info_req_100 cmp_const

BV_DEFINE(packet_player_info_req_100_fields, 1);

static int send_packet_player_info_req_100(
               connection_t *pconn,
               const struct packet_player_info_req *packet)
{
  const struct packet_player_info_req *real_packet = packet;
  packet_player_info_req_100_fields fields;
  struct packet_player_info_req *old, *clone;
  bool differ, old_from_hash, force_send_of_unchanged = TRUE;
  struct hash_table **hash = &pconn->phs.sent[PACKET_PLAYER_INFO_REQ];
  int different = 0;

  SEND_PACKET_START(PACKET_PLAYER_INFO_REQ);
# ifdef REPLAY_2
  printf("cs opc=150 PLAYER_INFO_REQ");
# endif
  if (!*hash) {
    *hash = hash_new(hash_packet_player_info_req_100,
                     cmp_packet_player_info_req_100);
  }
  BV_CLR_ALL(fields);

  old = hash_lookup_data(*hash, real_packet);
  old_from_hash = (old != NULL);
  if (!old) {
    old = wc_malloc(sizeof(*old));
    memset(old, 0, sizeof(*old));
    force_send_of_unchanged = TRUE;
  }

  differ = (old->id != real_packet->id);
  if (differ) {
    different++;
    BV_SET(fields, 0);
  }

  if (different == 0 && !force_send_of_unchanged) {
# ifdef REPLAY_2
    printf("\n");
# endif
    return 0;
  }

  DIO_BV_PUT(&dout, fields);

  if (BV_ISSET(fields, 0)) {
    dio_put_uint8(&dout, real_packet->id);
# ifdef REPLAY_2
    printf(" id=%u\n", real_packet->id);
  } else {
    printf("\n");
# endif
  }

  if (old_from_hash) {
    hash_delete_entry(*hash, old);
  }

  clone = old;

  *clone = *real_packet;
  hash_insert(*hash, clone, clone);
  SEND_PACKET_END;
}

static void
send_ensure_valid_variant_packet_player_info_req(connection_t *pconn)
{
  int variant = -1;

  if(pconn->phs.variant[PACKET_PLAYER_INFO_REQ] != -1) {
    return;
  }

  variant = 100;
  pconn->phs.variant[PACKET_PLAYER_INFO_REQ] = variant;
}

int send_packet_player_info_req(
        connection_t *pconn,
        const struct packet_player_info_req *packet)
{
  if(!pconn->used) {
    freelog(LOG_ERROR,
            "WARNING: trying to send data to the closed connection %s",
            conn_description(pconn));
    return -1;
  }
  assert(pconn->phs.variant != NULL);
  if(is_server) {
    freelog(LOG_ERROR, "Sending packet_player_info_req from the server.");
  }
  send_ensure_valid_variant_packet_player_info_req(pconn);

  switch(pconn->phs.variant[PACKET_PLAYER_INFO_REQ]) {
    case 100: {
      return send_packet_player_info_req_100(pconn, packet);
    }
    default: {
      die("unknown variant");
      return -1;
    }
  }
}

int dsend_packet_player_info_req(connection_t *pconn, int id)
{
  struct packet_player_info_req packet, *real_packet = &packet;

  real_packet->id = id;

  return send_packet_player_info_req(pconn, real_packet);
}

