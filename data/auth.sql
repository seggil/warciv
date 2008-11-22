--
-- Tables for user authentication and login recording.
--
-- N.B. if the tables are not of this format, then the select, insert,
-- and update syntax in the auth functions in server/database.c
-- must be changed.
--

CREATE TABLE auth (
  id int(11) NOT NULL auto_increment,
  name varchar(32) default NULL,
  password varchar(32) default NULL,
  salt int default 0,
  email varchar(128) default NULL,
  createtime int(11) default NULL,
  accesstime int(11) default NULL,
  address varchar(255) default NULL,
  createaddress varchar(15) default NULL,
  logincount int(11) default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY name (name)
) TYPE=MyISAM;

CREATE TABLE loginlog (
  id int(11) NOT NULL auto_increment,
  name varchar(32) default NULL,
  logintime int(11) default NULL,
  address varchar(255) default NULL,
  succeed enum('S','F') default 'S',
  PRIMARY KEY  (id)
) TYPE=MyISAM;
