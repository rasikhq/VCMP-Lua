# Modules

Every library below is built into the plugin and loads with `require`, with
no files on disk. Your own Lua modules load from `package_path` in
`luaconfig.lua` (see [configuration](../configuration.md)).

| `require` | Library | Version |
|---|---|---|
| `http` | Non-blocking HTTP(S) client (this plugin) | |
| `hash` | Same table as the global `Hash` (this plugin) | |
| `sql` | `sql.format` for LuaSQL (this plugin) | |
| `luasql.sqlite3`, `luasql.postgres`, `luasql.mysql` | [LuaSQL](https://lunarmodules.github.io/luasql/manual.html) over SQLite, libpq and MariaDB Connector/C | 2.8.1 |
| `cjson`, `cjson.safe` | [lua-cjson](https://github.com/openresty/lua-cjson) (OpenResty) | 2.1.0.19 |
| `socket`, `socket.http`, `socket.url`, `socket.ftp`, `socket.smtp`, `socket.headers`, `socket.tp`, `ltn12`, `mime` | [LuaSocket](https://lunarmodules.github.io/luasocket/) | 3.1.0 |
| `copas`, `copas.http`, `copas.timer`, `copas.lock`, `copas.queue`, `copas.semaphore`, `copas.future`, `copas.ftp`, `copas.smtp`, `binaryheap`, `timerwheel` | [Copas](https://lunarmodules.github.io/copas/) | 4.12.0 |
| `lfs` | [LuaFileSystem](https://lunarmodules.github.io/luafilesystem/) | 1.9.0 |
| `inspect` | [inspect.lua](https://github.com/kikito/inspect.lua) | 3.1.3 |

Scripts run on the server's thread, and a script that waits stops the
server for that long. `http` never waits. A LuaSQL query waits for the
database, so keep queries short and the database close. Database queries
that do not wait will come once LuaSQL releases its asynchronous API.

## http

```lua
local http = require "http"
http.request(options, callback)
http.request(url, callback)        -- a GET
```

Starts a request and returns at once. `callback(res, err)` runs on the
server thread in a later server frame: `res` is the response, or `nil` and
`err` says why the request failed. An HTTP error status such as 404 is a
response, not a failure.

`options` fields (any other field is an error):

| Field | Type | Default | |
|---|---|---|---|
| `url` | string | required | `http://` or `https://` only |
| `method` | string | `GET`, or `POST` with a body | Letters only, any case. `GET` and `HEAD` take no body |
| `headers` | table | none | `{ ["Name"] = "value" }`. Names must be valid header names; values must not contain line breaks |
| `body` | string | none | |
| `timeout` | number | 30 | Seconds for the whole request, more than 0 and at most 3600 |
| `redirects` | integer | 5 | Redirects to follow, 0 to 20 |

The response table:

| Field | |
|---|---|
| `status` | HTTP status code |
| `headers` | Header names in lower case; repeated headers joined with `", "` |
| `body` | The body, at most 16 MiB (a larger one fails the request) |
| `url` | The final URL, after redirects |

- Certificates and host names are always checked. Windows uses the system
  certificate store. Linux uses the distribution's CA bundle, or a built-in
  copy of Mozilla's when it has none. To trust another CA, set
  `http = { cafile = "ca.pem" }` in `luaconfig.lua`.
- At most 1000 requests may be pending; `http.request` raises an error
  beyond that.
- Host names are resolved without blocking the server.
- Requests still pending at `Server.reload()` or shutdown are cancelled,
  and their callbacks do not run.

```lua
local cjson = require "cjson"
local http = require "http"

http.request({
  url = "https://api.example.com/scores",
  method = "POST",
  headers = { ["Content-Type"] = "application/json" },
  body = cjson.encode({ player = "Tommy", score = 42 }),
  timeout = 10,
}, function(res, err)
  if not res then
    Logger.warn("request failed: " .. err)
  elseif res.status ~= 200 then
    Logger.warn("HTTP " .. res.status .. ": " .. res.body)
  end
end)
```

## Hash

The global `Hash`, also returned by `require "hash"`. Every function takes
strings and returns lower-case hex, except `randomBytes` and `equals`.

| Function | |
|---|---|
| `Hash.MD5(text)`, `Hash.SHA1(text)`, `Hash.SHA256(text)`, `Hash.SHA512(text)`, `Hash.Whirlpool(text)` | Digest of `text` |
| `Hash.KMAC256(key, text)`, `Hash.SKEIN256(key, text)`, `Hash.SKEIN512(key, text)` | Keyed hash, 64 bytes |
| `Hash.hmac(digest, key, data)` | HMAC |
| `Hash.pbkdf2(password, salt, iterations, length[, digest])` | PBKDF2-HMAC, `length` bytes (1 to 1024). `iterations` 1 to 10,000,000. `digest` defaults to `"sha256"` |
| `Hash.scrypt(password, salt, N, r, p, length)` | scrypt, `length` bytes (1 to 1024). `N` is a power of two; `N * r * 128` bytes must fit in 32 MiB |
| `Hash.randomBytes(n)` | `n` random bytes (0 to 1 MiB) from a cryptographic generator, as a binary string |
| `Hash.toHex(bytes)` | Hex of a binary string |
| `Hash.equals(a, b)` | `true` if the strings are equal; takes the same time wherever they differ |

Digests for `hmac` and `pbkdf2`: `md5`, `sha1`, `sha224`, `sha256`,
`sha384`, `sha512`, `sha3-256`, `sha3-512`.

Store passwords with `pbkdf2` or `scrypt` and a random salt, never a plain
or salted digest, which is fast to guess. Both run on the server thread:
choose a cost the server can afford at each login.

```lua
local salt = Hash.toHex(Hash.randomBytes(16))
local stored = Hash.pbkdf2(password, salt, 100000, 32)
-- at login:
local ok = Hash.equals(Hash.pbkdf2(attempt, salt, 100000, 32), stored)
```

## sql

LuaSQL has no prepared statements. `sql.format(conn, text, ...)` builds the
SQL text instead: each `?` takes the next value, escaped with
`conn:escape` for that connection.

```lua
local sql = require "sql"
local env = assert(require("luasql.sqlite3").sqlite3())
local conn = assert(env:connect("server.sqlite3"))

assert(conn:execute(sql.format(conn,
  "INSERT INTO scores (name, score) VALUES (?, ?)", player.name, 42)))

local cursor = assert(conn:execute(sql.format(conn,
  "SELECT name, score FROM scores WHERE score > ?", 10)))
local row = cursor:fetch({}, "a")
while row do
  print(row.name, row.score)
  row = cursor:fetch(row, "a")
end
cursor:close()
```

| Value | Becomes |
|---|---|
| `nil` | `NULL` |
| `true`, `false` | `TRUE`, `FALSE` |
| integer | its exact digits |
| float | 17 significant digits; NaN and infinities raise an error |
| string | a quoted literal |

- `??` is a literal `?`. A `?` inside a quoted string, a quoted name or a
  comment stays as it is.
- A wrong number of values, or a value of another type, raises an error.
- Backslash escapes and Postgres dollar-quoted strings are not recognised
  in the SQL text; pass such literals as values instead.

## LuaSQL connections

- **SQLite:** `require("luasql.sqlite3").sqlite3():connect("file.sqlite3")`.
- **PostgreSQL:** `require("luasql.postgres").postgres():connect(conninfo)`,
  with a libpq connection string such as
  `"host=127.0.0.1 dbname=game user=game password=secret connect_timeout=5"`.
- **MySQL and MariaDB:**
  `require("luasql.mysql").mysql():connect(db, user, password, host, port, nil, nil, options)`.
  The last argument is specific to this plugin:

  | Option | |
  |---|---|
  | `connect_timeout`, `read_timeout`, `write_timeout` | Seconds |
  | `ssl` | `"disable"`, `"require"` or `"verify"` |
  | `ssl_ca` | CA file for `"verify"` |

  The MySQL client reads no option files and loads no authentication
  plugins from disk. `caching_sha2_password` (MySQL 8) and
  `mysql_native_password` work; `ed25519` does not.

PostgreSQL and MySQL return every value as a string.

## Copas and LuaSocket

Once a script requires `copas`, the plugin runs one Copas step every server
frame. Start work with `copas.addthread`; never call `copas.loop()`, which
would block the server.

LuaSocket and Copas resolve host names on the server thread, which blocks
it; prefer `http` for HTTP. LuaSec is not included, so `copas.http` and
`socket.http` speak plain HTTP only.

## Loading code

- `load`, `loadfile`, `dofile` and `require` accept Lua source only, not
  precompiled bytecode.
- `loadfile()` and `dofile()` without a file name raise an error instead of
  reading the console.
- C modules cannot be loaded from disk: `package.cpath` is empty and
  `package.loadlib` raises an error. The plugin's copy of Lua is private to
  it, so an external C module could not link to it.
