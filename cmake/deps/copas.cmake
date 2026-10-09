# Copas 4.12.0 with its dependencies binaryheap 0.4 and timerwheel 1.0.2.
vcmp_lua_fetch(copas
    URL https://github.com/lunarmodules/copas/archive/refs/tags/v4_12_0.tar.gz
    SHA256 ce9c4c3b3b9b6ed76ba25ebe29934a1e5c5e2e0e2addba3693cfd59fffae9368)
vcmp_lua_fetch(binaryheap
    URL https://github.com/Tieske/binaryheap.lua/archive/refs/tags/version_0v4.tar.gz
    SHA256 10b1b6c6f2d22560f512f9896a6672ec5ae0eea1390ff8e662be1d5d9625b438)
vcmp_lua_fetch(timerwheel
    URL https://github.com/Tieske/timerwheel.lua/archive/refs/tags/1.0.2.tar.gz
    SHA256 a3d0159bcf996f3c73ac20d6168d2aaedcd6877df8f7ae6a1994010ad8492784)

set(_copas_src "${copas_SOURCE_DIR}/src")
vcmp_lua_embed_lua(
    copas           "${_copas_src}/copas.lua"
    copas.ftp       "${_copas_src}/copas/ftp.lua"
    copas.future    "${_copas_src}/copas/future.lua"
    copas.http      "${_copas_src}/copas/http.lua"
    copas.lock      "${_copas_src}/copas/lock.lua"
    copas.queue     "${_copas_src}/copas/queue.lua"
    copas.semaphore "${_copas_src}/copas/semaphore.lua"
    copas.smtp      "${_copas_src}/copas/smtp.lua"
    copas.timer     "${_copas_src}/copas/timer.lua"
    binaryheap      "${binaryheap_SOURCE_DIR}/src/binaryheap.lua"
    timerwheel      "${timerwheel_SOURCE_DIR}/src/timerwheel/init.lua")
