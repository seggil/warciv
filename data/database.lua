require 'freeciv'
require 'luasql'

local freelog = freeciv.freelog
local LOG_NORMAL = freeciv.LOG_NORMAL
local LOG_ERROR = freeciv.LOG_ERROR


freelog(LOG_NORMAL, 'MySQL version: ' .. luasql._MYSQLVERSION)

local env = luasql.mysql()

local dbname = 'freeciv'
local dbuser = 'freeciv'
local dbpass = 'password'

local conn, err = env:connect(dbname, dbuser, dbpass)
if not conn then
  freelog(LOG_ERROR, err)
else
  freelog(LOG_NORMAL, 'Database connection OK.')
end
conn:close()
env:close()
