# digestpp (header-only, no releases; pinned commit): the v1-compatible
# digests of the Hash module.
vcmp_lua_fetch(digestpp
    URL https://github.com/kerukuro/digestpp/archive/4beae7541f5c280389898ae6e6111028852f466a.tar.gz
    SHA256 47b68a98477a88ffb039d549b2497dfb1cb71d18f61017ed567de7bf9d75ffbe)

add_library(vcmp_lua_digestpp INTERFACE)
target_include_directories(vcmp_lua_digestpp SYSTEM INTERFACE "${digestpp_SOURCE_DIR}")
