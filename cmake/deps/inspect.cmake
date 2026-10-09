# inspect.lua 3.1.3, unmodified: require "inspect".
vcmp_lua_fetch(inspect
    URL https://github.com/kikito/inspect.lua/archive/refs/tags/v3.1.3.tar.gz
    SHA256 43386a613f9916186af4919f36392d8b8dad8c4428a5f8c85ebac80acba71eeb)

vcmp_lua_embed_lua(inspect "${inspect_SOURCE_DIR}/inspect.lua")
