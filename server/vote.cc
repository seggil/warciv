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
#  include "../config.h"
#endif

#include <math.h>

#include "wc_intl.h"
#include "log.h"
#include "support.h"

#include "capability.h"
#include "connection.h"
#include "packets.h"
#include "player.h"

#include "commands.h"
#include "console.h"
#include "gamehand.h"
#include "plrhand.h"
#include "settings.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "vote.h"

struct vote_list *vote_list = NULL;
int vote_number_sequence = 0;
int last_server_vote = -1;

/**************************************************************************
  ...
**************************************************************************/
static int count_voters(const struct vote *pvote)
{
  int num_voters = 0;

  connection_list_iterate(game.est_connections, pconn) {
    if (conn_can_vote(pconn, pvote)) {
      num_voters++;
    }
  } connection_list_iterate_end;

  return num_voters;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_new(struct connection_list *dest, struct vote *pvote)
{
  struct packet_vote_new packet;
  connection_t *pconn = NULL;

  if (pvote == NULL) {
    return;
  }

  if (pvote->caller_id != 0) {
    pconn = find_conn_by_id(pvote->caller_id);
    if (pconn == NULL) {
      return;
    }
  }

  freelog(LOG_DEBUG, "lsend_vote_new %p (%d) --> %p",
          pvote, pvote->vote_no, dest);

  packet.vote_no = pvote->vote_no;
  sz_strlcpy(packet.user, pconn ? pconn->username : _("(server prompt)"));
  describe_vote(pvote, packet.desc, sizeof(packet.desc));

  /* For possible future use. */
  packet.percent_required = 100 * pvote->need_pc;
  packet.flags = pvote->flags;
  packet.is_poll = pvote->command_id == CMD_POLL;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  connection_list_iterate(dest, conn) {
    if (!has_capability("voteinfo", conn->capability)
        || !conn_can_see_vote(conn, pvote)) {
      continue;
    }
    send_packet_vote_new(conn, &packet);
  } connection_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_update(struct connection_list *dest, struct vote *pvote,
                              int num_voters)
{
  struct packet_vote_update packet;

  if (pvote == NULL) {
    return;
  }

  if (pvote->caller_id != 0 && !find_conn_by_id(pvote->caller_id)) {
    return;
  }

  freelog(LOG_DEBUG, "lsend_vote_update %p (%d) --> %p",
          pvote, pvote->vote_no, dest);

  packet.vote_no = pvote->vote_no;
  packet.yes = pvote->yes;
  packet.no = pvote->no;
  packet.abstain = pvote->abstain;
  packet.num_voters = num_voters;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  connection_list_iterate(dest, aconn) {
    if (!has_capability("voteinfo", aconn->capability)
        || !conn_can_see_vote(aconn, pvote)) {
      continue;
    }
    send_packet_vote_update(aconn, &packet);
  } connection_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_remove(struct connection_list *dest, struct vote *pvote)
{
  struct packet_vote_remove packet;

  if (!pvote) {
    return;
  }

  packet.vote_no = pvote->vote_no;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  connection_list_iterate(dest, pconn) {
    if (!has_capability("voteinfo", pconn->capability)) {
      continue;
    }
    send_packet_vote_remove(pconn, &packet);
  } connection_list_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void lsend_vote_resolve(struct connection_list *dest,
                               struct vote *pvote, bool passed)
{
  struct packet_vote_resolve packet;

  if (!pvote) {
    return;
  }

  packet.vote_no = pvote->vote_no;
  packet.passed = passed;

  if (dest == NULL) {
    dest = game.est_connections;
  }

  connection_list_iterate(dest, pconn) {
    if (!has_capability("voteinfo", pconn->capability)
        || !conn_can_see_vote(pconn, pvote)) {
      continue;
    }
    send_packet_vote_resolve(pconn, &packet);
  } connection_list_iterate_end;
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

/***************************************************************************
  ...
***************************************************************************/
bool vote_is_team_only(const struct vote *pvote)
{
  return pvote && (pvote->flags & VCF_TEAMONLY);
}

/***************************************************************************
  A user cannot vote if:
    * is muted
    * is not connected
    * access level < basic
    * isn't a player
    * the vote is a team vote and not on the caller's team
  NB: If 'pvote' is NULL, then the team condition is not checked.
***************************************************************************/
bool conn_can_vote(const connection_t *pconn, const struct vote *pvote)
{
  if (!pconn || conn_is_muted(pconn) || !conn_controls_player(pconn)
      || conn_get_access(pconn) < ALLOW_BASIC) {
    return FALSE;
  }

  if (vote_is_team_only(pvote)) {
    const player_t *pplayer;
    const player_t *caller_plr;

    pplayer = conn_get_player(pconn);
    caller_plr = conn_get_player(vote_get_caller(pvote));
    if (!pplayer || !caller_plr ||
        !players_on_same_team(pplayer, caller_plr)) {
      return FALSE;
    }
  }

  return TRUE;
}

/***************************************************************************
  Usually, all users can see, except in the team vote case.
***************************************************************************/
bool conn_can_see_vote(const connection_t *pconn, const struct vote *pvote)
{
  if (!pconn) {
    return FALSE;
  }

  if (conn_is_global_observer(pconn)) {
    /* All is visible for global observer. */
    return TRUE;
  }

  if (vote_is_team_only(pvote)) {
    const player_t *pplayer;
    const player_t *caller_plr;

    pplayer = conn_get_player(pconn);
    caller_plr = conn_get_player(vote_get_caller(pvote));
    if (!pplayer || !caller_plr ||
        !players_on_same_team(pplayer, caller_plr)) {
      return FALSE;
    }
  }

  return TRUE;
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
struct vote *get_vote_by_caller(const connection_t *caller)
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
struct vote *vote_new(connection_t *caller,
                      const char *allargs,
                      int command_id,
                      struct setting_value *sv)
{
  struct vote *pvote;
  char votedesc[MAX_LEN_CONSOLE_LINE];
  const char *what;

  assert(vote_list != NULL);

  if (caller) {
    if (!conn_can_vote(caller, NULL)) {
      return NULL;
    }

    /* Cancel previous vote */
    remove_vote(get_vote_by_caller(caller));
  }

  /* Make a new vote */
  pvote = wc_malloc(sizeof(struct vote));
  pvote->caller_id = caller ? caller->id : 0;
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

    /* Vote reduction hack for 30 second timeout increases.
     * NB: If this is changed, be sure to update help text. */
    if (op->int_value == &game.info.timeout) {
      int diff = sv->int_value - *op->int_value;
      if (0 < diff && diff <= 30) {
        pvote->need_pc = 0.33;
        pvote->flags |= VCF_NOPASSALONE;
      }
    }
  }

  if (pvote->flags & VCF_NOPASSALONE) {
    int num_voters = count_voters(pvote);
    double min_pc = 1.0 / (double) num_voters;

    if (num_voters > 1 && min_pc > pvote->need_pc) {
      pvote->need_pc = MIN(0.5, 2.0 * min_pc);
    }
  }

  lsend_vote_new(NULL, pvote);

  describe_vote(pvote, votedesc, sizeof(votedesc));

  if (pvote->command_id == CMD_POLL) {
    what = _("New poll");
  } else if (vote_is_team_only(pvote)) {
    what = _("New teamvote");
  } else {
    what = _("New vote");
  }
  notify_team(vote_get_team(pvote), _("%s (number %d) by %s: %s"),
              what, pvote->vote_no,
              caller ? caller->username : _("(server prompt)"), votedesc);

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
  connection_t *pconn = NULL;
  double yes_pc = 0.0, no_pc = 0.0, rem_pc = 0.0, base = 0.0;
  int flags;
  double need_pc;
  char cmdline[MAX_LEN_CONSOLE_LINE];
  const double MY_EPSILON = 0.000001;
  const char *title;
  const struct team *pteam = NULL;

  assert(vote_list != NULL);

  pvote->yes = 0;
  pvote->no = 0;
  pvote->abstain = 0;

  num_voters = count_voters(pvote);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = find_conn_by_id(pvc->conn_id))
        || !conn_can_vote(pconn, pvote)) {
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

    if (base > MY_EPSILON) {
      yes_pc = (double) pvote->yes / base;
      no_pc = (double) pvote->no / base;

      /* The fraction of people who have not voted at all. */
      rem_pc = (double) (num_voters - num_cast) / base;
    }

    if (flags & VCF_NODISSENT && no_pc > MY_EPSILON) {
      resolve = TRUE;
    }
    if (flags & VCF_UNANIMOUS && yes_pc - 1.0 < -MY_EPSILON) {
      resolve = TRUE;
    }

    if (!resolve && !(flags & VCF_WAITFORALL)) {
      resolve =
          /* We have enough yes votes. */
          (yes_pc - need_pc > MY_EPSILON)
          /* We have too many no votes. */
          || (no_pc - 1.0 + need_pc > MY_EPSILON
              || fabs(no_pc - 1.0 + need_pc) < MY_EPSILON)
          /* We can't get enough no votes. */
          || (no_pc + rem_pc - 1.0 + need_pc < -MY_EPSILON)
          /* We can't get enough yes votes. */
          || (yes_pc + rem_pc - need_pc < -MY_EPSILON
              || fabs(yes_pc + rem_pc - need_pc) < MY_EPSILON);
    }

    /* Resolve if everyone voted already. */
    if (!resolve && fabs(rem_pc) < MY_EPSILON) {
      resolve = TRUE;
    }

    /* Resolve this vote if it has been around long enough. */
    if (!resolve && pvote->turn_count > 1) {
      resolve = TRUE;
    }

    /* Resolve this vote if everyone tries to abstain. */
    if (!resolve && fabs(base) < MY_EPSILON) {
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

  passed = yes_pc - need_pc > MY_EPSILON;

  if (passed && flags & VCF_UNANIMOUS) {
    passed = fabs(yes_pc - 1.0) < MY_EPSILON;
  }
  if (passed && flags & VCF_NODISSENT) {
    passed = fabs(no_pc) < MY_EPSILON;
  }

  title = vote_is_team_only(pvote) ? _("Teamvote") : _("Vote");
  pteam = vote_get_team(pvote);
  if (passed) {
    notify_team(pteam, _("%s %d \"%s\" is passed %d to %d with "
                         "%d abstentions and %d who did not vote."),
                title, pvote->vote_no, pvote->cmdline, pvote->yes,
                pvote->no, pvote->abstain, num_voters - num_cast);
  } else {
    notify_team(pteam, _("%s %d \"%s\" failed with %d against, %d for, "
                         "%d abstentions and %d who did not vote."),
                title, pvote->vote_no, pvote->cmdline, pvote->no,
                pvote->yes, pvote->abstain, num_voters - num_cast);
  }

  lsend_vote_resolve(NULL, pvote, passed);

  vote_cast_list_iterate(pvote->votes_cast, pvc) {
    if (!(pconn = find_conn_by_id(pvc->conn_id))) {
      freelog(LOG_ERROR, "Got a vote from a lost connection");
      continue;
    } else if (!conn_can_vote(pconn, pvote)) {
      freelog(LOG_ERROR, "Got a vote from a non-voting connection");
      continue;
    }

    switch (pvc->vote_cast) {
    case VOTE_YES:
      notify_team(pteam, _("%s %d: %s voted yes."),
                  title, pvote->vote_no, pconn->username);
      break;
    case VOTE_NO:
      notify_team(pteam, _("%s %d: %s voted no."),
                  title, pvote->vote_no, pconn->username);
      break;
    case VOTE_ABSTAIN:
      notify_team(pteam, _("%s %d: %s chose to abstain."),
                  title, pvote->vote_no, pconn->username);
      break;
    default:
      break;
    }
  } vote_cast_list_iterate_end;

  /* Remove the vote before executing the command because it's the
   * cause of many crashes due to the /cut command:
   *   - If the caller is the target.
   *   - If the target votes on this vote. */
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

  struct vote_cast *pvc = wc_malloc(sizeof(struct vote_cast));

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
void connection_vote(connection_t *pconn,
                     struct vote *pvote,
                     enum vote_type type)
{
  assert(vote_list != NULL);

  struct vote_cast *pvc;

  if (!conn_can_vote(pconn, pvote)) {
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
void cancel_connection_votes(connection_t *pconn)
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
    last_server_vote = -1;
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

  if (pvote->flags & VCF_UNANIMOUS) {
    ret = my_snprintf(buf, buflen,
        /* TRANS: Describing a new vote that requires unanimous
         * agreement of the given percentage of players to pass. */
        _("%s (needs unanimous %0.0f%%)."),
        pvote->cmdline, MIN(100.0, pvote->need_pc * 100.0 + 1));
  } else if (pvote->flags & VCF_NODISSENT) {
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
void handle_vote_submit(connection_t *pconn, int vote_no, int value)
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
  Sends a packet_vote_new to pconn (if voteinfo capable) for every
  currently running vote.
**************************************************************************/
void send_running_votes(connection_t *pconn)
{
  if (!pconn || !has_capability("voteinfo", pconn->capability)) {
    return;
  }

  if (!vote_list || vote_list_size(vote_list) < 1) {
    return;
  }

  freelog(LOG_DEBUG, "Sending running votes to %s.",
          conn_description(pconn));

  connection_do_buffer(pconn);
  vote_list_iterate(vote_list, pvote) {
    if (conn_can_see_vote(pconn, pvote)) {
      lsend_vote_new(pconn->self, pvote);
      lsend_vote_update(pconn->self, pvote, count_voters(pvote));
    }
  } vote_list_iterate_end;
  connection_do_unbuffer(pconn);
}

/**************************************************************************
  Sends a packet_vote_update to every voteinfo-capabale conn in dest. If
  dest is NULL, then sends to all established connections.
**************************************************************************/
void send_updated_vote_totals(struct connection_list *dest)
{
  int num_voters;

  if (vote_list == NULL || vote_list_size(vote_list) <= 0) {
    return;
  }

  freelog(LOG_DEBUG, "Sending updated vote totals to connection_list %p", dest);

  if (dest == NULL) {
    dest = game.est_connections;
  }

  connection_list_do_buffer(dest);
  vote_list_iterate(vote_list, pvote) {
    num_voters = count_voters(pvote);
    lsend_vote_update(dest, pvote, num_voters);
  } vote_list_iterate_end;
  connection_list_do_unbuffer(dest);
}

/**************************************************************************
  Returns the connection that called this vote.
**************************************************************************/
const connection_t *vote_get_caller(const struct vote *pvote)
{
  return find_conn_by_id(pvote->caller_id);
}

/**************************************************************************
  Return the team of the caller of this teamvote, or NULL if the vote
  is not a teamvote.
**************************************************************************/
const struct team *vote_get_team(const struct vote *pvote)
{
  const player_t *pplayer;
  if (!pvote || !vote_is_team_only(pvote)) {
    return NULL;
  }
  pplayer = conn_get_player(vote_get_caller(pvote));
  return pplayer ? team_get_by_id(pplayer->team) : NULL;
}

/**************************************************************************
  The server makes a vote
**************************************************************************/
void server_request_pause_vote(connection_t *pconn)
{
  struct vote *pvote;
  char buf[256];

  if (game_is_paused()
      || count_voters(NULL) == 0
      || server_state != RUN_GAME_STATE) {
    return;
  }

  if (last_server_vote != -1
      && (pvote = get_vote_by_no(last_server_vote))
      && pvote->command_id == CMD_PAUSE) {
    /* There is already a vote for pause. */
    return;
  }

  if (pconn) {
    my_snprintf(buf, sizeof(buf), "--- %s is lost", pconn->username);
  } else {
    buf[0] = '\0';
  }

  if ((pvote = vote_new(NULL, buf, CMD_PAUSE, NULL))) {
    lsend_vote_update(NULL, pvote, count_voters(pvote));
    /* HACK: initialize some fields which will be sent to the clients. */
    pvote->yes = 0;
    pvote->no = 0;
    pvote->abstain = 0;
    last_server_vote = pvote->vote_no;
  }
}
