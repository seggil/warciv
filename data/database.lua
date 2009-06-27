require 'freeciv'
require 'luasql'

local AUTH_TABLE = 'auth'

local LOG_NORMAL = freeciv.LOG_NORMAL
local LOG_ERROR = freeciv.LOG_ERROR
local LOG_VERBOSE = freeciv.LOG_VERBOSE

local env = luasql.mysql()
local is_salted


local function freelog(level, fmt, ...)
  freeciv.freelog(level, fmt:format(...))
end

local get_params = freeciv.get_db_params

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

local function execute(dbh, query)
  local cur, err = dbh:execute(query)
  if not cur then
    error(err, 2)
  end
  return cur
end

local function have_salt_column()
  local dbh = connect()
  local stmt = "SHOW COLUMNS FROM " .. AUTH_TABLE .. " LIKE 'salt'"
  local res = execute(dbh, stmt)
  local numrows = res:numrows()
  res:close()
  dbh:close()
  return numrows == 1
end

function load_user(pconn)
  local dbh = connect()
  local username = dbh:escape(pconn:get_username())

  local fields
  if is_salted then
    fields = 'password, salt'
  else
    fields = 'password'
  end

  local stmt = "SELECT %s FROM %s WHERE name = '%s'"
  stmt:format(fields, AUTH_TABLE, username);

  local res = execute(dbh, stmt)
  local numrows = res:numrows()
  if numrows < 1 then
    return freeciv.AUTH_DB_NOT_FOUND
  end

  if numrows > 1 then
    freelog(LOG_ERROR, 'load_user: multiple entries (%d) '
            .. 'for user: %s', numrows, username)
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

function save_user(pconn)
  local dbh = connect()
  local username = dbh:escape(pconn:get_username())

  -- FIXME: Not yet implemented.

  dbh:close()

  return freeciv.AUTH_DB_SUCCESS
end


freelog(LOG_VERBOSE, 'MySQL database version is %s.', luasql._MYSQLVERSION)
freelog(LOG_VERBOSE, 'Testing database connection...')
local test = connect()
test:close()
is_salted = have_salt_column()
freelog(LOG_VERBOSE, 'Database connection successful.')
freelog(LOG_VERBOSE, 'Password salt is '
        .. (is_salted and 'enabled' or 'disabled') .. '.')

