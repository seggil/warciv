------------------------------------------------------------
-- Game logging and player rating tables for Freeciv.
--
-- Version 3 created 10-12-2007.
--
------------------------------------------------------------

-- A database server has a collection of games.
CREATE TABLE games (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	PRIMARY KEY(id),

	-- Attributes.
	created TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	last_turn_id INT UNSIGNED DEFAULT NULL,
	last_update TIMESTAMP DEFAULT 0,
	host VARCHAR(64) DEFAULT NULL,
	port SMALLINT UNSIGNED DEFAULT 0,
	version VARCHAR(16) DEFAULT NULL,
	patches VARCHAR(64) DEFAULT NULL,
	capabilities VARCHAR(256) DEFAULT NULL,
	ruleset VARCHAR(32) DEFAULT NULL,

	-- Game type. E.g. "ffa", "team", "duel", etc.
	-- See game_type_strings in common/game.c.
	type VARCHAR(32) NOT NULL DEFAULT 'ffa',

	-- Extra information about how the game ended.
	-- See game_outcome_strings in common/game.c.
	outcome VARCHAR(64) DEFAULT NULL
) TYPE=MyISAM;

-- Each game has one and only one original terrain map.
-- The only reason why this is not in the games table is
-- so that it does not overwhelm and poor unsuspecting
-- terminals that do a SELECT *.
CREATE TABLE terrain_maps (
	game_id INT UNSIGNED NOT NULL UNIQUE,
	-- A zlib compressed binary blob representing the
	-- terrain of the initial game map. See function
	-- etm_encode_terrain_map in server/database.c for
	-- a description of the format.
	map BLOB NOT NULL
) TYPE=MyISAM;

-- A game may have one or more non-default settings.
-- As long as the hardcoded default server settings
-- don't change too much, this would seem preferable
-- to recording every setting of every game.
CREATE TABLE non_default_settings (
	game_id INT UNSIGNED NOT NULL,
	name VARCHAR(32) NOT NULL,
	value VARCHAR(32) NOT NULL
) TYPE=MyISAM;

-- A game has a collection of turns.
CREATE TABLE turns (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	game_id INT UNSIGNED NOT NULL,
	PRIMARY KEY(id),

	-- Attributes.
	turn_no INT UNSIGNED NOT NULL,
	year INT NOT NULL
) TYPE=MyISAM;

-- Each turn has a 'turn_map', i.e. a unit/city overlay.
-- In its own table for the same reason as terrain maps.
CREATE TABLE turn_maps (
	turn_id INT UNSIGNED NOT NULL UNIQUE,
	-- A map 'overlay' representing the positions of
	-- units and cities for all the players. See the
	-- function etm_encode_turn_map in server/database.c
	-- for a description of the format.
	-- Zlib compressed.
	map BLOB NOT NULL
) TYPE=MyISAM;

-- A table of terrains that have changed from their original
-- values on a given turn.
CREATE TABLE changed_terrain (
	turn_id INT UNSIGNED NOT NULL,
	terrain_map_index INT UNSIGNED NOT NULL,
	encoded_value TINYINT NOT NULL
) TYPE=MyISAM;

-- A game may have some teams.
CREATE TABLE teams (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	game_id INT UNSIGNED NOT NULL,
	PRIMARY KEY(id),

	-- Attributes.
	name VARCHAR(32) NOT NULL,
	num_players TINYINT UNSIGNED NOT NULL,
	score FLOAT DEFAULT NULL,
	rank FLOAT UNSIGNED DEFAULT NULL,
	result VARCHAR(32) DEFAULT NULL
) TYPE=MyISAM;

-- A game also has a collection of players.
CREATE TABLE players (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	game_id INT UNSIGNED NOT NULL,
	PRIMARY KEY(id),

	-- Attributes.
	name VARCHAR(32) NOT NULL,  -- NB must be escaped.
	nation VARCHAR(32) NOT NULL,

	-- Id in teams table NOT from common/nation.h.
	team_id INT UNSIGNED DEFAULT NULL,
	
	-- Could be NULL for AIs.
	creating_user_name VARCHAR(32) DEFAULT NULL,

	-- Typically one of 'win' 'lose' 'draw' for completed games.
	-- See player_result_strings in common/player.c
	result VARCHAR(32) DEFAULT NULL,

	-- For completed games this field gives the standing
	-- of this player compared to others. 0 is first place.
	-- It is floating point to allow for fractional ranking.
	rank FLOAT UNSIGNED DEFAULT NULL
) TYPE=MyISAM;

-- A turn has a collection of player statuses.
CREATE TABLE player_status (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	turn_id INT UNSIGNED NOT NULL,
	player_id INT UNSIGNED NOT NULL,
	PRIMARY KEY(id),

	-- Attributes.

	-- NULL if no user connected that turn.
	user_name VARCHAR(32) DEFAULT NULL,
	alive BOOLEAN NOT NULL,
	score INT NOT NULL,

	gold INT NOT NULL,
	techs TINYINT UNSIGNED NOT NULL,
	gold_income INT NOT NULL,
	science_income INT NOT NULL,
	luxury_income INT NOT NULL,
	trade_production INT NOT NULL,
	shield_production INT NOT NULL,

	population INT UNSIGNED NOT NULL,
	happy_citizens SMALLINT UNSIGNED NOT NULL,
	content_citizens SMALLINT UNSIGNED NOT NULL,
	unhappy_citizens SMALLINT UNSIGNED NOT NULL,
	angry_citizens SMALLINT UNSIGNED NOT NULL,
	taxmen SMALLINT UNSIGNED NOT NULL,
	scientists SMALLINT UNSIGNED NOT NULL,
	entertainers SMALLINT UNSIGNED NOT NULL,

	cities SMALLINT UNSIGNED NOT NULL,
	settledarea INT UNSIGNED NOT NULL,
	landarea INT UNSIGNED NOT NULL,
	pollution INT UNSIGNED NOT NULL,
	military_units SMALLINT UNSIGNED NOT NULL,
	civilian_units SMALLINT UNSIGNED NOT NULL,

	literacy TINYINT NOT NULL,  -- get_literacy(pplayer)
	wonders TINYINT UNSIGNED NOT NULL,
	government VARCHAR(32) NOT NULL,
	tax_rate TINYINT UNSIGNED NOT NULL,
	science_rate TINYINT UNSIGNED NOT NULL,
	luxury_rate TINYINT UNSIGNED NOT NULL,

	-- See name_of_skill_level in server/stdinhand.c.
	ai_mode VARCHAR(16) DEFAULT NULL
) TYPE=MyISAM;

-- During each turn each player may have a collection of allies.
-- NB this is not used for team 'allies', only in other types
-- of games.
CREATE TABLE allies (
	player_status_id INT UNSIGNED NOT NULL,
	ally_player_id INT UNSIGNED NOT NULL
) TYPE=MyISAM;

-- Each time a user is rated in a game, a row is added
-- into this table.
CREATE TABLE ratings (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE,
	user_id INT UNSIGNED NOT NULL,
	game_id INT UNSIGNED NOT NULL, 
	player_id INT UNSIGNED NOT NULL,
	PRIMARY KEY(id),

	-- Attributes.
	rating DOUBLE NOT NULL,
	rating_deviation DOUBLE NOT NULL,
	old_rating DOUBLE NOT NULL,
	old_rating_deviation DOUBLE NOT NULL,
	timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	game_type VARCHAR(32) NOT NULL DEFAULT 'ffa'
) TYPE=MyISAM;

