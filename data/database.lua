require 'freeciv'
require 'luasql'

local AUTH_TABLE = 'auth'
local LOGIN_TABLE = 'loginlog'

local LOG_NORMAL = freeciv.LOG_NORMAL
local LOG_ERROR = freeciv.LOG_ERROR
local LOG_VERBOSE = freeciv.LOG_VERBOSE

local env = luasql.mysql()
local is_salted


local function freelog(level, fmt, ...)
  freeciv.freelog(level, fmt:format(...))
end

local get_params = freeciv.database.get_params
local create_checksum = freeciv.database.create_checksum

local function connect()
  -- This gets the database parameters from the values
  -- set by the /authdb command in the server.
  local params = get_params()

  local dbh, err = env:connect(params.dbname, params.user,
                               params.password, params.host)
  if not dbh then
    error(err, 2)
  end
  return dbh
end

local function execute(dbh, query, ...)
  local cur, err = dbh:execute(query:format(...))
  if not cur then
    error(err, 2)
  end
  return cur
end

local function check_salt()
  local dbh = connect()
  local res = execute(dbh, [[
    SHOW COLUMNS FROM %s LIKE 'salt'
  ]], AUTH_TABLE);
  local numrows = res:numrows()
  res:close()
  dbh:close()
  return numrows == 1
end

function load_user(conn)
  local dbh = connect()
  local username = dbh:escape(conn:get_username())

  local fields
  if is_salted then
    fields = 'password, salt'
  else
    fields = 'password'
  end

  local res = execute(dbh, [[
    SELECT %s FROM %s WHERE name = '%s'
  ]], fields, AUTH_TABLE, username);

  local numrows = res:numrows()
  if numrows < 1 then
    return freeciv.AUTH_DB_NOT_FOUND
  end

  if numrows > 1 then
    freelog(LOG_ERROR, 'load_user: multiple entries (%d) for user: %s',
            numrows, username)
  end

  local row = res:fetch()
  pconn:set_password(row[0])

  if is_salted then
    pconn:set_salt(row[1])
  end

  res:close()
  dbh:close()

  return freeciv.AUTH_DB_SUCCESS
end

function save_user(conn)
  local dbh = connect()

  local username = dbh:escape(conn:get_username())
  local ipaddr = conn:get_ipaddr()

  local salt
  if is_salted then
    salt = conn:get_salt()
  end
  local checksum = create_checksum(conn:get_password(), salt)

  execute(dbh, [[
    INSERT INTO %s VALUES (NULL, '%s', '%s', %d,
      NULL, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), '%s', '%s', 0)
  ]], AUTH_TABLE, username, checksum, ipaddr, ipaddr)

  if is_salted then
    execute(dbh, [[
      UPDATE %s SET salt = %d WHERE name = '%s'
    ]], AUTH_TABLE, salt, username)
  end

  execute(dbh, [[
    INSERT INTO %s (name, logintime, address, succeed)
      VALUES ('%s', UNIX_TIMESTAMP(),'%s', 'S')
  ]], LOGIN_TABLE, username, ipaddr)

  dbh:close()

  return true
end


freelog(LOG_VERBOSE, 'MySQL database version is %s.', luasql._MYSQLVERSION)
freelog(LOG_VERBOSE, 'Testing database connection...')
local test = connect()
test:close()
is_salted = check_salt()
freelog(LOG_VERBOSE, 'Database connection successful.')
freelog(LOG_VERBOSE, 'Password salt is '
        .. (is_salted and 'enabled' or 'disabled') .. '.')

