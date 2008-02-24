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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "capability.h"
#include "connection.h"
#include "fcintl.h"
#include "log.h"
#include "packets.h"
#include "player.h"
#include "support.h"

#include "commands.h"
#include "console.h"
#include "plrhand.h"
#include "settings.h"
#include "stdinhand.h"
#include "vote.h"

struct vote_list *vote_list = NULL;
int vote_number_sequence = 0;

/**************************************************************************
  ...
**************************************************************************/
static int count_voters(void)
{
  int num_voters = 0;

  conn_list_iterate(game.est_connections, pconn) {
    if (connection_can_vote(pconn)) {
      num_voters++;
    }
  } conn_list_iterate_end;

  return num_voters;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_new(struct conn_list *dest, struct vote *pvote)
{
  struct packet_vote_new packet;
  struct connection *pconn;

  if (pvote == NULL) {
    return;
  }

  pconn = find_conn_by_id(pvote->caller_id);
  if (pconn == NULL) {
    return;
  }

  packet.vote_no = pvote->vote_no;
  sz_strlcpy(packet.user, pconn->username);
  describe_vote(pvote, packet.desc, sizeof(packet.desc));

  /* For possible future use. */
  packet.percent_required = 100 * ((int) pvote->need_pc);
  packet.flags = pvote->flags;
  packet.is_poll = pvote->command_id == CMD_POLL;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, conn) {
    if (!has_capability("voteinfo", conn->capability)) {
      continue;
    }
    send_packet_vote_new(conn, &packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_update(struct conn_list *dest, struct vote *pvote,
                              int num_voters)
{
  struct packet_vote_update packet;

  packet.vote_no = pvote->vote_no;
  packet.yes = pvote->yes;
  packet.no = pvote->no;
  packet.abstain = pvote->abstain;
  packet.num_voters = num_voters;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    if (!has_capability("voteinfo", pconn->capability)) {
      continue;
    }
    send_packet_vote_update(pconn, &packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_remove(struct conn_list *dest, struct vote *pvote)
{
  struct packet_vote_remove packet;

  packet.vote_no = pvote->vote_no;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    if (!has_capability("voteinfo", pconn->capability)) {
      continue;
    }
    send_packet_vote_remove(pconn, &packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_resolve(struct conn_list *dest,
                               struct vote *pvote, bool passed)
{
  struct packet_vote_resolve packet;

  packet.vote_no = pvote->vote_no;
  packet.passed = passed;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  conn_list_iterate(dest, pconn) {
    if (!has_capability("voteinfo", pconn->capability)) {
      continue;
    }
    send_packet_vote_resolve(pconn, &packet);
  } conn_list_iterate_end;
}

/**************************************************************************
  Remove a vote of the vote list.
**************************************************************************/
static void free_vote(struct vote *pvote)
{
  if (!pvote) {
    return;
  }

  assert(vote_list != NULL);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    free(pvc);
  } vote_cast_list_iterate_end;
  vote_cast_list_free(pvote->votes_cast);
  free(pvote);
}

/**************************************************************************
  ...
**************************************************************************/
void remove_vote(struct vote *pvote)
{
  assert(vote_list != NULL);

  if (pvote == NULL) {
    return;
  }

  vote_list_unlink(vote_list, pvote);
  lsend_vote_remove(NULL, pvote);
  free_vote(pvote);
}

/**************************************************************************
  Remove all votes.
**************************************************************************/
void clear_all_votes(void)
{
  if (!vote_list) {
    return;
  }

  vote_list_iterate(vote_list, pvote) {
    lsend_vote_remove(NULL, pvote);
    free_vote(pvote);
  } vote_list_iterate_end;
  vote_list_unlink_all(vote_list);
}

/**************************************************************************
  ...
**************************************************************************/
static bool connection_is_player(struct connection *pconn)
{
  return pconn && pconn->player && !pconn->observer
      && pconn->player->is_alive;
}

/**************************************************************************
  Cannot vote if:
    * is muted
    * is not connected
    * access level < basic
    * isn't a player
**************************************************************************/
bool connection_can_vote(struct connection *pconn)
{
  if (conn_is_muted(pconn)) {
    return FALSE;
  }
  if (connection_is_player(pconn) && pconn->access_level >= ALLOW_BASIC) {
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
struct vote *get_vote_by_no(int vote_no)
{
  assert(vote_list != NULL);

  vote_list_iterate(vote_list, pvote) {
    if (pvote->vote_no == vote_no) {
      return pvote;
    }
  } vote_list_iterate_end;

  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
struct vote *get_vote_by_caller(const struct connection *caller)
{
  assert(vote_list != NULL);

  if (caller == NULL) {
    return NULL;
  }

  vote_list_iterate(vote_list, pvote) {
    if (pvote->caller_id == caller->id) {
      return pvote;
    }
  } vote_list_iterate_end;

  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
struct vote *vote_new(struct connection *caller,
                      const char *allargs,
                      int command_id,
                      struct setting_value *sv)
{
  struct vote *pvote;

  assert(vote_list != NULL);

  if (!connection_can_vote(caller)) {
    return NULL;
  }

  /* Cancel previous vote */
  remove_vote(get_vote_by_caller(caller));

  /* Make a new vote */
  pvote = fc_malloc(sizeof(struct vote));
  pvote->caller_id = caller->id;
  pvote->command_id = command_id;

  my_snprintf(pvote->cmdline, sizeof(pvote->cmdline), "%s%s%s",
              commands[command_id].name,
              allargs[0] == '\0' ? "" : " ", allargs);

  pvote->turn_count = 0;
  pvote->votes_cast = vote_cast_list_new();
  pvote->vote_no = ++vote_number_sequence;

  vote_list_append(vote_list, pvote);

  pvote->flags = commands[command_id].vote_flags;
  pvote->need_pc = (double) commands[command_id].vote_percent / 100.0;

  if (pvote->command_id == CMD_SET) {
    struct settings_s *op = NULL;

    assert(sv != NULL);
    assert(sv->setting_idx >= 0);

    op = &settings[sv->setting_idx];
    if (op->vote_flags > VCF_NONE) {
      pvote->flags = op->vote_flags;
    }
    if (op->vote_percent > 0) {
      pvote->need_pc = (double) op->vote_percent / 100.0;
    }

    /* Extra special kludge for the timeout setting.
     * NB If this is changed, do not forget to update
     * help texts. */
    if (op->int_value == &game.info.timeout
        && sv->int_value > *op->int_value) {
      pvote->need_pc = 0.25;
    }
  }

  lsend_vote_new(NULL, pvote);

  return pvote;
}

/**************************************************************************
  Check if we satisfy the criteria for resolving a vote, and resolve it
  if these critera are indeed met. Updates yes and no variables in voting 
  struct as well.
**************************************************************************/
static void check_vote(struct vote *pvote)
{
  int num_cast = 0, num_voters = 0;
  bool resolve = FALSE, passed = FALSE;
  struct connection *pconn = NULL;
  double yes_pc = 0.0, no_pc = 0.0, rem_pc = 0.0, base = 0.0;
  int flags;
  double need_pc;
  char cmdline[MAX_LEN_CONSOLE_LINE];

  assert(vote_list != NULL);

  pvote->yes = 0;
  pvote->no = 0;
  pvote->abstain = 0;

  num_voters = count_voters();

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = find_conn_by_id(pvc->conn_id))
        || !connection_can_vote(pconn)) {
      continue;
    }
    num_cast++;

    switch (pvc->vote_cast) {
    case VOTE_YES:
      pvote->yes++;
      break;
    case VOTE_NO:
      pvote->no++;
      break;
    case VOTE_ABSTAIN:
      pvote->abstain++;
      break;
    default:
      assert(0);
      break;
    }
  } vote_cast_list_iterate_end;

  flags = pvote->flags;
  need_pc = pvote->need_pc;

  /* Check if we should resolve the vote. */
  if (num_voters > 0) {

    /* Players that abstain essentially remove themselves from
     * the voting pool. */
    base = num_voters - pvote->abstain;

    if (base > 0.0) {
      yes_pc = (double) pvote->yes / base;
      no_pc = (double) pvote->no / base;

      /* The fraction of people who have not voted at all. */
      rem_pc = (double) (num_voters - num_cast) / base;
    }

    if (flags & VCF_NO_DISSENT && no_pc > 0.0) {
      resolve = TRUE;
    }
    if (flags & VCF_UNANIMOUS && yes_pc < 1.0) {
      resolve = TRUE;
    }

    if (!resolve && !(flags & VCF_WAITFORALL)) {
      if (flags & VCF_FASTPASS) {
        /* We have enough votes and yes is in majority. */
        resolve = (yes_pc > no_pc && 1.0 - rem_pc > need_pc)
            /* Can't get enough yes votes from the remainder. */
            || yes_pc + rem_pc <= no_pc
            /* Everyone voted. */
            || rem_pc == 0.0;
      } else {
        /* We have enough yes votes. */
        resolve = yes_pc > need_pc
            /* We have too many no votes. */
            || no_pc >= 1.0 - need_pc
            /* We can't get enough no votes. */
            || no_pc + rem_pc < 1.0 - need_pc
            /* We can't get enough yes votes. */
            || yes_pc + rem_pc <= need_pc;
      }
    }

    /* Resolve if everyone voted already. */
    if (!resolve && rem_pc == 0.0) {
      resolve = TRUE;
    }

    /* Resolve this vote if it has been around long enough. */
    if (!resolve && pvote->turn_count > 1) {
      resolve = TRUE;
    }

    /* Resolve this vote if everyone tries to abstain. */
    if (!resolve && base == 0.0) {
      resolve = TRUE;
    }
  }

  freelog(LOG_DEBUG, "check_vote flags=%d need_pc=%0.2f yes_pc=%0.2f "
          "no_pc=%0.2f rem_pc=%0.2f base=%0.2f resolve=%d",
          flags, need_pc, yes_pc, no_pc, rem_pc, base, resolve);

  lsend_vote_update(NULL, pvote, num_voters);

  if (!resolve) {
    return;
  }

  if (flags & VCF_FASTPASS) {
    passed = yes_pc > no_pc && 1.0 - rem_pc > need_pc;
  } else {
    passed = yes_pc > need_pc;
  }

  if (passed && flags & VCF_UNANIMOUS) {
    passed = yes_pc == 1.0;
  }
  if (passed && flags & VCF_NO_DISSENT) {
    passed = no_pc == 0.0;
  }

  if (passed) {
    notify_conn(NULL, _("Vote %d \"%s\" is passed %d to %d with "
                        "%d abstentions and %d who did not vote."),
                pvote->vote_no, pvote->cmdline,
                pvote->yes, pvote->no,
                pvote->abstain, num_voters - num_cast);
  } else {
    notify_conn(NULL, _("Vote %d \"%s\" failed with %d against, %d for, "
                        "%d abstentions and %d who did not vote."),
                pvote->vote_no, pvote->cmdline,
                pvote->no, pvote->yes,
                pvote->abstain, num_voters - num_cast);
  }

  lsend_vote_resolve(NULL, pvote, passed);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = find_conn_by_id(pvc->conn_id))) {
      freelog(LOG_ERROR, "Got a vote from a lost connection");
      continue;
    } else if (!connection_can_vote(pconn)) {
      freelog(LOG_ERROR, "Got a vote from a non-voting connection");
      continue;
    }

    switch (pvc->vote_cast) {
    case VOTE_YES:
      notify_conn(NULL, _("Vote %d: %s voted yes."),
                  pvote->vote_no, pconn->username);
      break;
    case VOTE_NO:
      notify_conn(NULL, _("Vote %d: %s voted no."),
                  pvote->vote_no, pconn->username);
      break;
    case VOTE_ABSTAIN:
      notify_conn(NULL, _("Vote %d: %s chose to abstain."),
                  pvote->vote_no, pconn->username);
      break;
    default:
      break;
    }
  } vote_cast_list_iterate_end;

  /* Remove the vote before to exexute the command because it's the
   * cause of many crashes due to the /cut command:
   *   - If the caller is the target.
   *   - If the target is voted to this vote. */
  sz_strlcpy(cmdline, pvote->cmdline);
  remove_vote(pvote);

  if (passed) {
    handle_stdin_input(NULL, cmdline, FALSE);
  }
}

/**************************************************************************
  Find the vote cast for the user id conn_id in a vote.
**************************************************************************/
static struct vote_cast *find_vote_cast(struct vote *pvote, int conn_id)
{
  assert(vote_list != NULL);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (pvc->conn_id == conn_id) {
      return pvc;
    }
  } vote_cast_list_iterate_end;

  return NULL;
}

/**************************************************************************
  Return a new vote cast.
**************************************************************************/
static struct vote_cast *vote_cast_new(struct vote *pvote)
{
  assert(vote_list != NULL);

  struct vote_cast *pvc = fc_malloc(sizeof(struct vote_cast));

  pvc->conn_id = -1;
  pvc->vote_cast = VOTE_ABSTAIN;

  vote_cast_list_append(pvote->votes_cast, pvc);

  return pvc;
}

/**************************************************************************
  Remove a vote cast.
**************************************************************************/
static void remove_vote_cast(struct vote *pvote, struct vote_cast *pvc)
{
  assert(vote_list != NULL);

  if (!pvc) {
    return;
  }

  vote_cast_list_unlink(pvote->votes_cast, pvc);
  free(pvc);
  check_vote(pvote);            /* Maybe can pass */
}

/**************************************************************************
  ...
**************************************************************************/
void connection_vote(struct connection *pconn,
                     struct vote *pvote,
                     enum vote_type type)
{
  assert(vote_list != NULL);

  struct vote_cast *pvc;

  if (!connection_can_vote(pconn)) {
    return;
  }

  /* Try to find a previous vote */
  if ((pvc = find_vote_cast(pvote, pconn->id))) {
    pvc->vote_cast = type;
  } else if ((pvc = vote_cast_new(pvote))) {
    pvc->vote_cast = type;
    pvc->conn_id = pconn->id;
  } else {
    /* Must never happen */
    assert(0);
  }
  check_vote(pvote);
}

/**************************************************************************
  Cancel the votes of a lost or a detached connection...
**************************************************************************/
void cancel_connection_votes(struct connection *pconn)
{
  if (!pconn || !vote_list) {
    return;
  }

  remove_vote(get_vote_by_caller(pconn));

  vote_list_iterate(vote_list, pvote) {
    remove_vote_cast(pvote, find_vote_cast(pvote, pconn->id));
  } vote_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void voting_init(void)
{
  if (!vote_list) {
    vote_list = vote_list_new();
    vote_number_sequence = 0;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void voting_turn(void)
{
  vote_list_iterate(vote_list, pvote) {
    pvote->turn_count++;
    check_vote(pvote);
  } vote_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
void voting_free(void)
{
  clear_all_votes();
  vote_list_free(vote_list);
  vote_list = NULL;
}

/**************************************************************************
  ...
**************************************************************************/
int describe_vote(struct vote *pvote, char *buf, int buflen)
{
  int ret = 0;

  if (pvote->command_id == CMD_POLL) {
    /* Special case for the /poll command. */
    ret = my_snprintf(buf, buflen, "%s", pvote->cmdline);

    /* FIXME hackish... */
    if (0 == mystrncasecmp("poll ", buf, 5)) {
      memmove(buf, buf + 5, ret - 5 + 1);
      ret -= 5;
    }
    return ret;
  }

  /* NB We don't handle votes with multiple
   * flags here. */
  
  if (pvote->flags & VCF_FASTPASS) {
    ret = my_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that is resolved (i.e.
         * checked that it can pass) when the given percentage of
         * players have voted. */
        _("%s (needs %0.0f%% participation)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  } else if (pvote->flags & VCF_UNANIMOUS) {
    ret = my_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that requires unanimous
         * agreement of the given percentage of players to pass. */
        _("%s (needs unanimous %0.0f%%)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  } else if (pvote->flags & VCF_NO_DISSENT) {
    ret = my_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that can only pass
         * if there are no dissenting votes. */
        _("%s (needs %0.0f%% and no dissent)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  } else {
    ret = my_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that can pass only if the
         * given percentage of players votes 'yes'. */
        _("%s (needs %0.0f%% in favor)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  }

  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
void handle_vote_submit(struct connection *pconn, int vote_no, int value)
{
  struct vote *pvote;
  enum vote_type type;

  freelog(LOG_DEBUG, "Got vote submit (%d %d) from %s.",
          vote_no, value, conn_description(pconn));

  pvote = get_vote_by_no(vote_no);
  if (pvote == NULL) {
    freelog(LOG_ERROR, "Submit request for unknown vote_no %d "
            "from %s ignored.",
            vote_no, conn_description(pconn));
    return;
  }

  if (value == 1) {
    type = VOTE_YES;
  } else if (value == -1) {
    type = VOTE_NO;
  } else if (value == 0) {
    type = VOTE_ABSTAIN;
  } else {
    freelog(LOG_ERROR, "Invalid packet data for submit of vote %d "
            "from %s ignored.", vote_no, conn_description(pconn));
    return;
  }

  connection_vote(pconn, pvote, type);
}

/**************************************************************************
  ...
**************************************************************************/
void send_running_votes(struct connection *pconn)
{
  int num_voters;

  if (!connection_can_vote(pconn)) {
    return;
  }

  if (!has_capability("voteinfo", pconn->capability)) {
    return;
  }

  if (vote_list == NULL || vote_list_size(vote_list) <= 0) {
    return;
  }

  freelog(LOG_DEBUG, "Sending running votes to %s.",
          conn_description(pconn));

  num_voters = count_voters();

  connection_do_buffer(pconn);
  vote_list_iterate(vote_list, pvote) {
    freelog(LOG_DEBUG, "Sending running vote %p %d.",
            pvote, pvote->vote_no);
    lsend_vote_new(pconn->self, pvote);
    lsend_vote_update(pconn->self, pvote, num_voters);
  } vote_list_iterate_end;
  connection_do_unbuffer(pconn);
}
