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
#ifndef WC_COMMON_PACKETS_H
#define WC_COMMON_PACKETS_H

struct connection_s;
struct data_in;

#include "connection.hh"         /* struct connection_s, MAX_LEN_* */
#include "diptreaty.hh"
#include "events.hh"
#include "improvement.hh"
#include "player.hh"
#include "shared.hh"             /* MAX_LEN_NAME, MAX_LEN_ADDR */
#include "spaceship.hh"
#include "unittype.hh"
#include "worklist.hh"
#include "effects.hh"

#define MAX_LEN_USERNAME        10        /* see below */
#define MAX_LEN_MSG             1536
#define MAX_LEN_ROUTE           2000      /* MAX_LEN_PACKET/2 - header */

/* The size of opaque (void *) data sent in the network packet.  To avoid
 * fragmentation issues, this SHOULD NOT be larger than the standard
 * ethernet or PPP 1500 byte frame size (with room for headers).
 *
 * Do not spend much time optimizing, you have no idea of the actual dynamic
 * path characteristics between systems, such as VPNs and tunnels.
 */
#define ATTRIBUTE_CHUNK_SIZE    (1400)

enum report_type {
  REPORT_WONDERS_OF_THE_WORLD,
  REPORT_TOP_5_CITIES,
  REPORT_DEMOGRAPHIC,
  REPORT_SERVER_OPTIONS,   /* obsolete */
  REPORT_SERVER_OPTIONS1,
  REPORT_SERVER_OPTIONS2
};

enum spaceship_place_type {
  SSHIP_PLACE_STRUCTURAL,
  SSHIP_PLACE_FUEL,
  SSHIP_PLACE_PROPULSION,
  SSHIP_PLACE_HABITATION,
  SSHIP_PLACE_LIFE_SUPPORT,
  SSHIP_PLACE_SOLAR_PANELS
};

enum unit_info_use {
  UNIT_INFO_IDENTITY,
  UNIT_INFO_CITY_SUPPORTED,
  UNIT_INFO_CITY_PRESENT
};

enum authentication_type {
  AUTH_LOGIN_FIRST,   /* request a password for a returning user */
  AUTH_NEWUSER_FIRST, /* request a password for a new user */
  AUTH_LOGIN_RETRY,   /* inform the client to try a different password */
  AUTH_NEWUSER_RETRY  /* inform the client to try a different [new] password */
};

#include "packets_gen.hh"

void *get_packet_from_connection(connection_t *pconn, enum packet_type *ptype, bool *presult);
void remove_packet_from_buffer(struct socket_packet_buffer *buffer);

void send_attribute_block(const player_t *pplayer,
                          connection_t *pconn);
void generic_handle_player_attribute_chunk(player_t *pplayer,
                          const struct packet_player_attribute_chunk *chunk,
                          connection_t *pconn);
const char *get_packet_name(enum packet_type type);

void pre_send_packet_chat_msg(connection_t *pconn,
                              struct packet_chat_msg *packet);
void post_receive_packet_chat_msg(connection_t *pconn,
                                  struct packet_chat_msg *packet);
void pre_send_packet_player_attribute_chunk(connection_t *pconn,
                                  struct packet_player_attribute_chunk *packet);
void post_receive_packet_game_state(connection_t *pconn,
                                    struct packet_game_state *packet);
void post_send_packet_game_state(connection_t *pconn,
                                 const struct packet_game_state *packet);

#define SEND_PACKET_START(type) \
  unsigned char buffer[MAX_LEN_PACKET]; \
  struct data_out dout; \
  \
  dio_output_init(&dout, buffer, sizeof(buffer)); \
  dio_put_uint16(&dout, 0); \
  dio_put_uint8(&dout, type);

#define SEND_PACKET_END \
    size_t size = dio_output_used(&dout); \
    \
    dio_output_rewind(&dout); \
    dio_put_uint16(&dout, size); \
    return send_packet_data(pconn, buffer, size)

#define RECEIVE_PACKET_START(type, result) \
  struct data_in din; \
  struct type *result = (struct type*)wc_malloc(sizeof(*result)); \
  \
  dio_input_init(&din, pconn->buffer->data, 2); \
  { \
    int size; \
  \
    dio_get_uint16(&din, &size); \
    dio_input_init(&din, pconn->buffer->data, MIN(size, pconn->buffer->ndata)); \
  } \
  dio_get_uint16(&din, NULL); \
  dio_get_uint8(&din, NULL);

#define RECEIVE_PACKET_END(result) \
  check_packet(&din, pconn); \
  remove_packet_from_buffer(pconn->buffer); \
  return result;

int send_packet_data(connection_t *pconn, unsigned char *data, int len);
void check_packet(struct data_in *din, connection_t *pconn);

#endif  /* WC_COMMON_PACKETS_H */
