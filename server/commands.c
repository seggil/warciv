/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option),
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fcintl.h"

#include "connection.h"

#include "srv_main.h"
#include "stdinhand.h"
#include "vote.h"

#include "commands.h"

/* Commands must match the values in enum command_id. */
const struct command commands[] = {
  {"start",	ALLOW_BASIC, ALLOW_BASIC,
   "start",
   N_("Start the game, or restart after loading a savegame."),
   N_("This command starts the game.  When starting a new game, "
      "it should be used after all human players have connected, and "
      "AI players have been created (if required), and any desired "
      "changes to initial server options have been made.  "
      "After 'start', each human player will be able to "
      "choose their nation, and then the game will begin.  "
      "This command is also required after loading a savegame "
      "for the game to recommence.  Once the game is running this command "
      "is no longer available, since it would have no effect."),
   ECHO_NONE, VCF_NONE, 0
  },

  {"help",	ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("help\n"
      "help commands\n"
      "help options\n"
      "help <command-name>\n"
      "help <option-name>"),
   N_("Show help about server commands and server options."),
   N_("With no arguments gives some introductory help.  "
      "With argument \"commands\" or \"options\" gives respectively "
      "a list of all commands or all options.  "
      "Otherwise the argument is taken as a command name or option name, "
      "and help is given for that command or option.  For options, the help "
      "information includes the current and default values for that option.  "
      "The argument may be abbreviated where unambiguous."),
   ECHO_NONE, VCF_NONE, 0
  },

  {"list",	ALLOW_OBSERVER, ALLOW_OBSERVER,
   "list\n"
   "list actionlist\n"
   "list connections\n"
   "list ignore\n"
   "list maps\n"
   "list mutes\n"
   "list players\n"
   "list rulesets\n"
   "list scenarios\n"
   "list teams\n"
   "list votes",
   N_("Show a list of various things."),
   N_("Show the action list, a list of connections to the server, your "
      "ignore list, all maps, a list of muted users on the server, a list "
      "of players, all rulesets, all scenarios, the teams and the players "
      "in them or all running votes. The argument may be abbreviated, "
      "and defaults to 'players' if absent."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"quit",	ALLOW_HACK, ALLOW_HACK,
   "quit",
   N_("Quit the game and shutdown the server."), NULL,
   ECHO_ALL, VCF_NONE, 0
  },
  {"cut",	ALLOW_CTRL, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cut <connection-name>"),
   N_("Cut a client's connection to server."),
   N_("Cut specified client's connection to the server, removing that client "
      "from the game.  If the game has not yet started that client's player "
      "is removed from the game, otherwise there is no effect on the player.  "
      "Note that this command now takes connection names, not player names."),
   ECHO_ALL, VCF_NONE, 50
  },
  {"unstart", ALLOW_NONE, ALLOW_BASIC,
   "unstart",
   N_("Set your ready state to 'not ready'."),
   N_("This command does the opposite of the start command: it "
      "notifies the server and other players that you are not "
      "ready to start the game."),
   ECHO_NONE, VCF_NONE, 0
  },

  {"explain",	ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("explain\n"
      "explain <option-name>"),
   N_("Explain server options."),
   N_("The 'explain' command gives a subset of the functionality of 'help', "
      "and is included for backward compatibility.  With no arguments it "
      "gives a list of options (like 'help options'), and with an argument "
      "it gives help for a particular option (like 'help <option-name>')."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"show",	ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("show\n"
      "show all\n"
      "show vital\n"
      "show situational\n"
      "show rare\n"
      "show changed\n"
      "show ruleset\n"
      "show <option-name>\n"
      "show <option-prefix>"),
   N_("Show server options."),
   N_("With no arguments, shows all server options (or available options, when "
      "used by clients).  With an argument, show only the named option, "
      "or options with that prefix. The argument \"changed\" will show only "
      "options that have been changed from the default value. With the "
      "the argument \"ruleset\", it will show only the current ruleset "
      "directory name."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"score",	ALLOW_CTRL, ALLOW_NEVER,
   "score",
   N_("Show current scores."),
   N_("For each connected client, pops up a window showing the current "
      "player scores."),
   ECHO_ALL, VCF_NOPASSALONE, 25
  },
  {"wall",	ALLOW_ADMIN, ALLOW_ADMIN,
   N_("wall <message>"),
   N_("Send message to all connections."),
   N_("For each connected client, pops up a window showing the message "
      "entered."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"vote",	ALLOW_BASIC,	ALLOW_BASIC,
   /* TRANS: translate text between [] only */
   N_("vote yes|no|abstain [vote number]"),
   N_("Cast a vote."),
      /* xgettext:no-c-format */
   N_("A player with basic level access issuing a control level command "
      "starts a new vote for the command.  The /vote command followed by "
      "\"yes\", \"no\" or \"abstain\", and optionally a vote number, "
      "gives your vote. If you do not add a vote number, your "
      "vote applies to the last vote made. You can only suggest one vote "
      "at a time. The vote will pass immediately if more than half of the "
      "players vote for it, or fail immediately if at least half of the "
      "players vote against it. If one full turn elapses the vote fails "
      "if not enough votes were cast one way or the other."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"poll",	ALLOW_CTRL,	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("poll <your yes/no poll question>\n"),
   N_("Start a poll.\n"),
   N_("If no other votes are running, a poll is started regarding the "
      "question you type in after the command."),
   ECHO_ADMINS, VCF_ALWAYSVOTE, 50
  },
  {"emote",	ALLOW_OBSERVER,	ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("emote <an action>\n"),
   N_("Display a message describing your action.\n"),
   N_("This completely gratuitous command prints a message "
      "with your name followed by the string you type after "
      "the command name."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"cancelvote",	ALLOW_BASIC,	ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("cancelvote\n"
      "cancelvote <vote number>\n"
      "cancelvote all\n"),
   N_("Cancel a running vote.\n"),
   N_("With no arguments this command removes your own vote. If you have "
      "hack access level, you can cancel any vote by vote number, or all "
      "votes with the \'all\' argument."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"debug",	ALLOW_HACK, ALLOW_HACK,
   N_("debug [ player <player> | city <x> <y> | units <x> <y> | unit <id> ]"),
   N_("Turn on or off AI debugging of given entity."),
   N_("Print AI debug information about given entity and turn continous "
      "debugging output for this entity on or off."),
   ECHO_ALL, VCF_NONE, 0
  },
  {"set",	ALLOW_CTRL, ALLOW_BASIC,
   N_("set <option-name> <value>"),
   N_("Set server option."), NULL,
   ECHO_PLAYERS,
   /* NB: Options may have their own values that
    * override these vote parameters. */
   VCF_NONE, 50
  },
  {"team",	ALLOW_NEVER, ALLOW_BASIC,
   N_("team <player> [team]"),
   N_("Change, add or remove a player's team affiliation."),
   N_("Sets a player as member of a team. If no team specified, the "
      "player is set teamless. Use \"\" if names contain whitespace. "
      "A team is a group of players that start out allied, with shared "
      "vision and embassies, and fight together to achieve team victory "
      "with averaged individual scores."),
   ECHO_PLAYERS, VCF_NONE, 0
  },
  {"rulesetdir", ALLOW_NEVER, ALLOW_CTRL,
   N_("rulesetdir <directory>\n"
      "rulesetdir <ruleset_id>"),
   N_("Choose new ruleset directory or modpack."),
   N_("Choose new ruleset directory or modpack. Use /list rulesets to get "
      "the complete list of available rulesets."),
   ECHO_ALL, VCF_NONE, 50
  },
  {"metamessage", ALLOW_BASIC, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("metainfo <meta-line>"),
   N_("Set metaserver info line."),
   N_("Set user defined metaserver info line. If parameter is omitted,\n"
      "previously set metamessage will be removed. For most of the time\n"
      "user defined metamessage will be used instead of automatically\n"
      "generated messages, if it is available."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"metatopic", ALLOW_BASIC, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("metatopic <meta-line>"),
   N_("Set metaserver topic line."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"metapatches", ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("metapatch <meta-line>"),
   N_("Set metaserver patches line."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaconnection",	ALLOW_ADMIN, ALLOW_ADMIN,
   "metaconnection u|up\n"
   "metaconnection d|down\n"
   "metaconnection ?",
   N_("Control metaserver connection."),
   N_("'metaconnection ?' reports on the status of the connection to metaserver.\n"
      "'metaconnection down' or 'metac d' brings the metaserver connection down.\n"
      "'metaconnection up' or 'metac u' brings the metaserver connection up."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaserver",ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("metaserver <address>"),
   N_("Set address (URL) for metaserver to report to."), NULL,
    ECHO_ADMINS, VCF_NONE, 0
  },
  {"aitoggle",	ALLOW_CTRL, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("aitoggle <player-name>"),
   N_("Toggle AI status of player."), NULL,
   ECHO_PLAYERS, VCF_NONE, 50
  },
  {"take",    ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between [] and <> only */
   N_("take [connection-name] <player-name>"),
   N_("Take over a player's place in the game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to take over a player. If you're not one of these, "
      "only the <player-name> argument is allowed"),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"observe",    ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between [] only */
   N_("observe [connection-name] [player-name]"),
   N_("Observe a player or the whole game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to observe a player. If you're not one of these, "
      "only the [player-name] argument is allowed. If the console gives no "
      "player-name or the connection uses no arguments, then the connection "
      "is attached to a global observer."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"detach",    ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between [] only */
   N_("detach [connection-name]"),
   N_("detach from a player."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to detach from a player."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"attach",    ALLOW_NEVER, ALLOW_OBSERVER,
   /* TRANS: translate text between [] only */
   N_("attach [connection-name]"),
   N_("attach to a new player."),
   N_("In the pregame, using this command will attach you to a new player "
      "if you are not already player and if you are allowed to take a player."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"create",	ALLOW_NEVER, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("create <player-name>"),
   N_("Create an AI player with a given name."),
   N_("The 'create' command is only available before the game has "
      "been started."),
   ECHO_ALL, VCF_NONE, 50
  },
  {"away",	ALLOW_BASIC, ALLOW_BASIC,
   N_("away\n"
      "away"),
   N_("Set yourself in away mode. The AI will watch your back."),
   N_("The AI will govern your nation but do minimal changes."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"novice",	ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("novice\n"
      "novice <player-name>"),
   N_("Set one or all AI players to 'novice'."),
   N_("With no arguments, sets all AI players to skill level 'novice', and "
      "sets the default level for any new AI players to 'novice'.  With an "
      "argument, sets the skill level for that player only."),
   ECHO_NONE, VCF_NONE, 33
  },
  {"easy",	ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("easy\n"
      "easy <player-name>"),
   N_("Set one or all AI players to 'easy'."),
   N_("With no arguments, sets all AI players to skill level 'easy', and "
      "sets the default level for any new AI players to 'easy'.  With an "
      "argument, sets the skill level for that player only."),
   ECHO_NONE, VCF_NONE, 33
  },
  {"normal",	ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("normal\n"
      "normal <player-name>"),
   N_("Set one or all AI players to 'normal'."),
   N_("With no arguments, sets all AI players to skill level 'normal', and "
      "sets the default level for any new AI players to 'normal'.  With an "
      "argument, sets the skill level for that player only."),
   ECHO_NONE, VCF_NONE, 33
  },
  {"hard",	ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("hard\n"
      "hard <player-name>"),
   N_("Set one or all AI players to 'hard'."),
   N_("With no arguments, sets all AI players to skill level 'hard', and "
      "sets the default level for any new AI players to 'hard'.  With an "
      "argument, sets the skill level for that player only."),
   ECHO_NONE, VCF_NONE, 33
  },
  {"experimental",	ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("experimental\n"
      "experimental <player-name>"),
   N_("Set one or all AI players to 'experimental'."),
   N_("With no arguments, sets all AI players to skill 'experimental', and "
      "sets the default level for any new AI players to this.  With an "
      "argument, sets the skill level for that player only. THIS IS ONLY "
      "FOR TESTING OF NEW AI FEATURES! For ordinary servers, this option "
      "has no effect."),
   ECHO_NONE, VCF_NONE, 33
  },
  {"cmdlevel",	ALLOW_ADMIN, ALLOW_ADMIN, /* confusing at ALLOW_CTRL */
   /* TRANS: translate text between <> only */
   N_("cmdlevel\n"
      "cmdlevel <level>\n"
      "cmdlevel <level> <connection-name>"),
   N_("Query or set command access level access."),
   N_("The command access level controls which server commands are available\n"
      "to users via the client chatline.  The available levels are:\n"
      "    none  -  no commands\n"
      "    observer -  observer commands only\n"
      "    basic -  baisc commands only\n"
      "    ctrl  -  commands that affect the game and users\n"
      "    admin -  includes banning, unbanning and removing players\n"
      "    hack  -  *all* commands - dangerous!\n"
      "With no arguments, the current command access levels are reported.\n"
      "With a single argument, the level is set for all existing connections.\n"
      "If a connection name is specified, the level is set for that "
      "connection only.\n"
      "Command access levels do not persist if a client disconnects, "
      "because some untrusted person could reconnect with the same name.  "
      "Note that this command now takes connection names, not player names."
      ),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"incrementaltimeout", ALLOW_CTRL, ALLOW_BASIC,
   /* TRANS: translate text between <> only */
   N_("incrementaltimeout <turn> <turninc> <value> <valuemult>"), 
   N_("See \"help incrementaltimeout\"."),
   N_("Every <turn> turns, add <value> to timeout timer, then add <turninc> "
      "to <turn> and multiply <value> by <valuemult>.  Use this command in "
      "concert with the option \"timeout\". Defaults are 0 0 0 1"),
   ECHO_ALL, VCF_NONE, 50
  },
  {"ignore", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> and [] only */
   N_("ignore [type=]<pattern>"), 
   N_("Block all messages from users matching the pattern."),
   N_("The given pattern will be added to your ignore "
      "list; you will not receive any messages from this users matching "
      "this pattern. The default type (if ommited) is to match against "
      "the username. Read /help addaction for other pattern types."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"unignore", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("unignore <range>"), 
   N_("Remove ignore list entries."),
   N_("The ignore list entries in the given range will be removed; "
      "you will be able to receive messages from the respective users. "
      "The range argument may be a single number or a pair of numbers "
      "separated by a dash '-'. If the first number is ommitted, it is "
      "assumed to be 1, if the last is ommitted, it is assumed to be "
      "the last valid ignore list index."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"autoteam", ALLOW_NEVER, ALLOW_CTRL, /* require vote in pregame */
   /* TRANS: translate text between <> only (NOT in []) */
#ifdef HAVE_MYSQL
   N_("autoteam <# of teams> [rating|ranking|rated|rank]\n"
      "autoteam <# of teams> list <best player; 2nd best; 3rd; etc.>"),
   N_("Assign teams automatically."),
   N_("Generate the given number of teams assuming that players' "
      "relative strength is given by their rating (if the rating "
      "database is enabled), or as ordered in the supplied list. "
      "Players not listed will be assumed to have an equal, lowest"
      "skill level. Names may be abbreviated so long as they are "
      "not ambiguous."),
#else
   N_("autoteam <# of teams> list <best player; 2nd best; 3rd; etc.>"),
   N_("Assign teams automatically."),
   N_("Generate the given number of teams assuming that players' "
      "relative strength is as ordered in the supplied list. Players "
      "not listed will be assumed to have an equal, lowest skill level. "
      "Names may be abbreviated so long as they are not ambiguous."),
#endif /* HAVE_MYSQL */
   ECHO_ALL, VCF_NONE, 50
  },
  {"mute", ALLOW_CTRL, ALLOW_CTRL,
   /* TRANS: translate text between <> and [] only */
   N_("mute <username> [# of turns = 3]"),
   N_("Prevent a user from making votes or chat messages."),
   N_("The user whose name matches the argument will not be allowed "
      "to make public, private or ally message, nor start votes. The "
      "optional '# of turns' argument specifies how long this should "
      "go on for; zero implies forever. The default value, if left "
      "unspecified, is 3."),
   ECHO_ADMINS, VCF_NOPASSALONE, 40
  },
  {"unmute", ALLOW_CTRL, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("unmute <username>"),
   N_("Allow a user to speak and make votes again."),
   N_("This command undoes what the /mute command did. The given "
      "user is again allowed to make public, private and ally "
      "messages and can create votes."),
   ECHO_ADMINS, VCF_NOPASSALONE, 25
  },
#ifdef HAVE_MYSQL
  {"stats", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("stats\n"
      "stats <username>"),
   N_("Show statitics about a user."),
   N_("If the server can access the game database, print some "
      "statistics about yourself or the given user."),
   ECHO_ADMINS
  },
  {"ratings", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("ratings\n"
      "ratings <game type>"),
   N_("List player ratings."),
   N_("If the server can access the ratings database, list the "
      "ratings for all connected players for the current game "
      "type, or for the given game type."),
   ECHO_ADMINS
  },
  {"examine", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("examine <game number>"),
   N_("Examine a game in the database."),
   N_("If the server can access the games database, request "
      "information about the game with the given number id."),
   ECHO_ADMINS
  },
  {"topten", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("topten\n"
      "topten <game type>"),
   N_("List the top rated players."),
   N_("If the server can access the ratings database, request "
      "the list of the top ten players for the given game type "
      "or the current game type if ommitted."),
   ECHO_ADMINS
  },
  {"gamelist", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("gamelist\n"
      "gamelist <range>\n"
      "gamelist type=<game type>\n"
      "gamelist user=<user name>"),
   N_("List games in the database."),
   N_("If the server can access the games database, request "
      "a list of games matching the given criteria. The range "
      "argument is a numeric interval of game numbers (e.g. "
      "20-34)."),
   ECHO_ADMINS
  },
  {"aka", ALLOW_OBSERVER, ALLOW_OBSERVER,
   /* TRANS: translate text between <> only */
   N_("aka <user name>"),
   N_("List user aliases."),
   N_("If the server can access the games database, request "
      "a list of aliases for the given user."),
   ECHO_ADMINS
  },

  {"authdb",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("authdb\n"
      "authdb on\n"
      "authdb off\n"
      "authdb host <hostname>\n"
      "authdb user <username>\n"
      "authdb password <password>\n"
      "authdb database <database name>\n"
      "authdb guests [yes|no]\n"
      "authdb newusers [yes|no]\n"),
   N_("Control authorization database parameters.\n"),
   N_("With no arguments simply prints the parameters, otherwise sets the "
      "specified parameter to the given value. Arguments \"on\" and \"off\" "
      "enable and disable authentication respectively. The parameters "
      "\"guests\" and \"newusers\" control whether guests are allow or "
      "new users are allowed when authentication is enabled."),
   ECHO_NONE, VCF_NONE, 0
  },
  {"fcdb", ALLOW_HACK, ALLOW_HACK,
    /* TRANS: translate text between <> only */
    N_("fcdb\n"
       "fcdb on\n"
       "fcdb off\n"
       "fcdb min_rated_turns <number>\n"
       "fcdb save_maps <yes|no>\n"
       "fcdb more_game_info <yes|no>\n"),
    N_("Game and rating database control.\n"),
    N_("If no arguments are given, show some information about the "
       "game database. Arguments 'on' and 'off' enable and "
       "disable database access respectively. The 'min_rated_turns' "
       "option controls the number of turns before a rated game "
       "should be considered for updating user ratings. The 'save_maps' "
       "option controls whether the encoded terrain and turn maps "
       "should be generated and saved to the database. The "
       "'more_game_info' option controls whether player scores and "
       "rating changes should be queried for the examine command."),
    ECHO_NONE
  },
#endif /* HAVE_MYSQL */
  {"endgame",	ALLOW_CTRL, ALLOW_NEVER,
   /* TRANS: translate text between <> only */
   N_("endgame <player1 player2 player3 ...>"),
   N_("End the game.  If players are listed, these win the game."),
   N_("This command ends the game immediately and credits the given players, "
      "if any, with winning it."),
   ECHO_ALL, VCF_NODISSENT, 50
  },
  {"remove",	ALLOW_ADMIN, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("remove <player-name>"),
   N_("Fully remove player from game."),
   N_("This *completely* removes a player from the game, including "
      "all cities and units etc. Works even in the middle of game. Use with care!"),
   ECHO_ALL, VCF_NONE, 50
  },
  {"save",	ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("save\n"
      "save <file-name>"),
   N_("Save game to file."),
   N_("Save the current game to file <file-name>.  If no file-name "
      "argument is given saves to \"<auto-save name prefix><year>m.sav[.gz]\".\n"
      "To reload a savegame created by 'save', start the server with "
      "the command-line argument:\n"
      "    --file <filename>\n"
      "and use the 'start' command once players have reconnected."),
   ECHO_ALL, VCF_NONE, 0
  },
  {"load",      ALLOW_NEVER, ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("load\n"
      "load <file-name>"),
   N_("Load game from file."),
   N_("Load a game from <file-name>. Any current data including players, "
      "rulesets and server options are lost.\n"),
   ECHO_ALL, VCF_NONE, 0
  },
    {"loadmap",      ALLOW_NEVER, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("loadmap\n"
      "loadmap <mapfile-name>"),
   N_("Load map from file."),
   N_("Load a map from <mapfile-name>. Only mapdata+startpos are loaded, use only " 
      "in pregame, rulesets and options have to be set seperatly.\n"
      "You can get a list of the available maps with /list maps.\n" 
      "<mapfile-name> can be the name of the mapfile or the number in the \n"
      "list of maps."),
   ECHO_ALL, VCF_NONE, 50
  },
  {"unloadmap",      ALLOW_NEVER, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("unloadmap\n"
      "unloadmap"),
   N_("Unload a filemap."),
   N_("Unload a map, that was loaded from a file (with /loadmap or /loadscenario)."
      "Use it if you want to play without a map after you have loaded a mapfile.\n"),
   ECHO_ALL, VCF_NONE, 50
  },
    {"loadscenario",      ALLOW_NEVER, ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("loadscenario\n"
      "loadscenario <scenariofile-name>"),
   N_("Load scenario from file."),
   N_("Load a scenario from <scenariofile-name>. <scenariofile-name> "
      "can be the name of the mapfile or the number in the list of scenarios."),
   ECHO_ALL, VCF_NONE, 50
  },
  {"read",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("read <file-name>"),
   N_("Process server commands from file."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"write",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("write <file-name>"),
   N_("Write current settings as server commands to file."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"reset",	ALLOW_NEVER, ALLOW_CTRL,
   N_("reset"),
   N_("Reset all server settings."),
   N_("Reset all settings and re-read the server start script, "
      "if there was one given with the --read server argument. "),
   ECHO_ALL, VCF_NONE, 50
  },
  {"wmessage",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("wmessage\n"
      "wmessage default\n"
      "wmessage <string>"),
   N_("Change the server welcome message."),
   N_("The welcome message is the string the server displays to all newly "
      "connected players. With no arguments this command displays the current "
      "welcome message. With the special argument \"default\" it sets the "
      "welcome message to the default. Otherwise sets the welcome message to "
      "the given string. The <string> argument may contain the following "
      "escape sequences:\n"
      "  %v - Freeciv version running on the server\n"
      "  %p - Port the server is running on\n"
      "  %h - Hostname of the server (blank if it does not exist)\n"
      "  %d - Current date\n"
      "  %t - Current time\n"
      "  %% - A single '%'\n"
      "For example, the default welcome message is equivalent to:\n"
      "  \"Welcome to the %v Server running at %h port %p.\""),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"wfile",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("wfile <file-name>"),
   N_("Set the welcome message to the contents of a file."),
   N_("Reads in the contents of the given file and uses that as the welcome "
      "message. File may contain escape sequences as described in help for "
      "/wmessage."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"dnslookup", ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between [] only */
   "dnslookup <on|off>",
   N_("Turn DNS hostname lookup on or off."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"loadactionlist",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("loadactionlist <filename>"),
   N_("Load actions from a file."),
   N_("Reads user actions from the given file and adds them to the "
      "action list applied to connecting users."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"saveactionlist",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("saveactionlist <filename>"),
   N_("Save actions to a file."),
   N_("Writes all of the user actions in the action list "
      "to the given file."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"clearactionlist",	ALLOW_HACK, ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("clearactionlist"),
   N_("Clear actions."),
   N_("Removes all actions from the list of actions applied to "
      "connecting users."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"addaction", ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> and [] only */
    N_("addaction <action> [type=]<pattern>"),
    N_("Add a user access action."),
    N_("Adds the given action to the list of user actions applied "
       "to newly connecting users. <action> is either ban, or an "
       "access level name to give that connection (see /help cmdlev). "
       "The optional type determines what the pattern string "
       "will be applied to, it may be address, hostname, or username. "
       "The pattern supports unix glob style wildcards, i.e. * matches "
       "zero or more character, ? exactly one character, [abc] exactly "
       "one of 'a' 'b' or 'c', etc."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"delaction", ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> and [] only */
    N_("delaction <n>"),
    N_("Delete an action."),
    N_("Remove the <n>th action from the action list."),
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"require", ALLOW_BASIC, ALLOW_BASIC,
   /* TRANS: translate text between <> and [] only */
    N_("require <capabilities> (require hack level access)\n"
       "require ?"),
    N_("Requires one or many capabilties to play."),
    N_("The users, who don't have this <capabilities> in their capability string, "
       "cannot play on this server. Use the '?' argument to see the current "
       "required capabilities."),
   ECHO_ALL, VCF_NONE, 0
  },
  {"kick", ALLOW_CTRL, ALLOW_CTRL,
   /* TRANS: translate text between <> */
    N_("kick <user>"),
    N_("Cut a connection and disallow reconnect."),
    N_("The connection given by the 'user' argument will be cut from the "
       "server and not allowed to reconnect for 30 minutes."),
    ECHO_ADMINS, VCF_NOPASSALONE, 50
  },
  {"ban",	ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> and [] only */
   N_("ban [type=]<pattern>"),
   N_("Ban a client connection."),
   N_("The given pattern with optional type will be added to the action "
      "list with action 'ban'. See /help addaction for a description of "
      "valid type and pattern values."),
   ECHO_ALL, VCF_NONE, 0
  },
  {"unban",	ALLOW_ADMIN, ALLOW_ADMIN,
   /* TRANS: translate text between <> and [] only */
   N_("unban [type=]<pattern>"),
   N_("Unban a client connection."),
   N_("Removes the action list entry with pattern <pattern> and "
      "action 'ban' (if <type> is ommitted, it is assumed to be "
      "'hostname'; it is also used to determine which entry to remove)."),
   ECHO_ALL, VCF_NONE, 0
  },

  {"rfcstyle",	ALLOW_HACK, ALLOW_HACK,
   "rfcstyle",
   N_("Switch server output between 'RFC-style' and normal style."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  },
  {"serverid",	ALLOW_BASIC, ALLOW_BASIC,
   "serverid",
   N_("Simply returns the id of the server."), NULL,
   ECHO_ADMINS, VCF_NONE, 0
  }
};


/**************************************************************************
  Return the access level of a command.
**************************************************************************/
enum cmdlevel_id command_access_level(enum command_id cmd)
{
  if (server_state == PRE_GAME_STATE) {
    return commands[cmd].pregame_level;
  } else {
    return commands[cmd].game_level;
  }
}

