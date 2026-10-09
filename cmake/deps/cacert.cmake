# Mozilla's CA certificates as extracted by the curl project: the http
# module's last resort on Linux, when no distribution bundle is found
# (src/modules/http.cpp). Windows uses the system store instead.
set(VCMP_LUA_CACERT_DATE 2026-09-25)
FetchContent_Declare(cacert
    URL "https://curl.se/ca/cacert-${VCMP_LUA_CACERT_DATE}.pem"
    URL_HASH SHA256=a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_NAME cacert.pem
    SOURCE_SUBDIR vcmp-lua-no-cmake-project)
FetchContent_MakeAvailable(cacert)
set(VCMP_LUA_CACERT_FILE "${cacert_SOURCE_DIR}/cacert.pem")
