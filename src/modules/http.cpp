// require "http": non-blocking HTTP(S) requests.
#include "modules/http.hpp"

#include <curl/curl.h>
#include <fmt/format.h>
#include <lua.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "bindings/args.hpp"
#include "bindings/bindings.hpp"
#include "modules/modules.hpp"
#include "runtime/log.hpp"
#include "runtime/preload.hpp"
#include "runtime/runtime.hpp"
#include "runtime/version.hpp"

namespace vcmp_lua {

#ifndef _WIN32
namespace embedded {
std::span<const unsigned char> CaBundle() noexcept;  // cmake/deps/cacert.cmake
}
#endif

namespace {

CURLM* Multi(void* multi) noexcept {
    return static_cast<CURLM*>(multi);
}

#ifndef _WIN32
// Where Linux distributions keep their CA bundle; the first readable one is
// used. Without one, the embedded Mozilla bundle is.
constexpr std::array<const char*, 6> kCaBundles = {
    "/etc/ssl/certs/ca-certificates.crt",                 // Debian, Ubuntu, Arch
    "/etc/pki/tls/certs/ca-bundle.crt",                   // Fedora, RHEL, CentOS
    "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",  // RHEL 7+
    "/etc/ssl/ca-bundle.pem",                             // openSUSE
    "/etc/ssl/cert.pem",                                  // Alpine
    "/etc/pki/tls/cacert.pem",                            // OpenELEC
};

const char* DistributionCaBundle() noexcept {
    for (const char* path : kCaBundles) {
        if (access(path, R_OK) == 0) {
            return path;
        }
    }
    return nullptr;
}
#endif

void SetOption(CURL* easy, CURLoption option, auto value) {
    const CURLcode code = curl_easy_setopt(easy, option, value);
    if (code != CURLE_OK) {
        throw std::runtime_error(
            fmt::format("http.request: curl refused option {}: {}", static_cast<int>(option),
                        curl_easy_strerror(code)));
    }
}

}  // namespace

struct Http::Transfer {
    std::uint64_t id = 0;
    CURL* easy = nullptr;
    CURLM* multi = nullptr;  // set while the easy handle is in the multi handle
    curl_slist* headers = nullptr;
    sol::main_protected_function callback;
    CURLcode result = CURLE_OK;
    char error[CURL_ERROR_SIZE] = {};
    bool too_large = false;
    std::vector<std::pair<std::string, std::string>> response_headers;
    std::string body;

    Transfer() = default;
    Transfer(const Transfer&) = delete;
    Transfer& operator=(const Transfer&) = delete;
    ~Transfer() {
        if (multi != nullptr) {
            curl_multi_remove_handle(multi, easy);
        }
        if (easy != nullptr) {
            curl_easy_cleanup(easy);
        }
        curl_slist_free_all(headers);
    }

    // libcurl passes 1 as the item size, so the item count is the size.
    static std::size_t Write(char* data, std::size_t, std::size_t size, void* user) {
        auto* transfer = static_cast<Transfer*>(user);
        if (transfer->body.size() + size > kMaxBody) {
            transfer->too_large = true;
            return 0;  // aborts the transfer
        }
        transfer->body.append(data, size);
        return size;
    }

    // One header line per call, the status line included. A new status
    // line (a redirect, or 100 Continue) starts the headers over.
    static std::size_t Header(char* data, std::size_t, std::size_t size, void* user) {
        auto* transfer = static_cast<Transfer*>(user);
        std::string_view line(data, size);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.remove_suffix(1);
        }
        if (line.starts_with("HTTP/")) {
            transfer->response_headers.clear();
            return size;
        }
        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            return size;  // the blank line at the end, or a malformed line
        }
        std::string name(line.substr(0, colon));
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string_view value = line.substr(colon + 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.remove_suffix(1);
        }
        transfer->response_headers.emplace_back(std::move(name), std::string(value));
        return size;
    }
};

Http::Http(Invoker& invoker, const HttpConfig& config) : invoker_(invoker), cafile_(config.cafile) {
    // Reference-counted: the plugin initialised libcurl already (after
    // OpenSSL, see runtime/libraries.cpp); the unit tests did not.
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        throw std::runtime_error("libcurl initialisation failed");
    }
    multi_ = curl_multi_init();
    if (multi_ == nullptr) {
        curl_global_cleanup();
        throw std::runtime_error("cannot create a libcurl multi handle");
    }
#ifdef _WIN32
    log::Debug("http: CA certificates from {}",
               cafile_.empty() ? "the Windows certificate store" : cafile_);
#else
    const char* bundle = DistributionCaBundle();
    log::Debug("http: CA certificates from {}",
               !cafile_.empty() ? cafile_
               : bundle != nullptr ? std::string(bundle)
                                   : std::string("the embedded Mozilla bundle"));
#endif
}

Http::~Http() {
    Clear();
    curl_multi_cleanup(Multi(multi_));
    curl_global_cleanup();
}

void Http::Start(const Request& request, sol::main_protected_function callback) {
    if (transfers_.size() >= kMaxPending) {
        throw std::runtime_error(
            fmt::format("http.request: {} requests are pending already", kMaxPending));
    }
    auto transfer = std::make_unique<Transfer>();
    Transfer& t = *transfer;
    t.id = next_id_++;
    t.callback = std::move(callback);
    t.easy = curl_easy_init();
    if (t.easy == nullptr) {
        throw std::runtime_error("http.request: cannot create a libcurl handle");
    }
    CURL* easy = t.easy;
    SetOption(easy, CURLOPT_PRIVATE, static_cast<void*>(&t));
    SetOption(easy, CURLOPT_ERRORBUFFER, t.error);
    SetOption(easy, CURLOPT_NOSIGNAL, 1L);
    SetOption(easy, CURLOPT_URL, request.url.c_str());
    SetOption(easy, CURLOPT_PROTOCOLS_STR, "http,https");
    SetOption(easy, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
    SetOption(easy, CURLOPT_FOLLOWLOCATION, request.max_redirects > 0 ? 1L : 0L);
    SetOption(easy, CURLOPT_MAXREDIRS, request.max_redirects);
    SetOption(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(request.timeout_ms));
    SetOption(easy, CURLOPT_CONNECTTIMEOUT_MS,
              std::min(10'000L, static_cast<long>(request.timeout_ms)));
    SetOption(easy, CURLOPT_USERAGENT, fmt::format("VCMP-Lua/{}", kVersion).c_str());
    SetOption(easy, CURLOPT_ACCEPT_ENCODING, "");  // whatever this libcurl decodes
    SetOption(easy, CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(kMaxBody));
    SetOption(easy, CURLOPT_WRITEFUNCTION, &Transfer::Write);
    SetOption(easy, CURLOPT_WRITEDATA, static_cast<void*>(&t));
    SetOption(easy, CURLOPT_HEADERFUNCTION, &Transfer::Header);
    SetOption(easy, CURLOPT_HEADERDATA, static_cast<void*>(&t));

    // TLS: peer and host name are always verified.
    SetOption(easy, CURLOPT_SSL_VERIFYPEER, 1L);
    SetOption(easy, CURLOPT_SSL_VERIFYHOST, 2L);
    if (!cafile_.empty()) {
        SetOption(easy, CURLOPT_CAINFO, cafile_.c_str());
        SetOption(easy, CURLOPT_CAPATH, static_cast<const char*>(nullptr));
    } else {
#ifdef _WIN32
        // Schannel uses the Windows certificate store.
#else
        // Never the paths of the machine libcurl and OpenSSL were built on.
        SetOption(easy, CURLOPT_CAPATH, static_cast<const char*>(nullptr));
        if (const char* bundle = DistributionCaBundle()) {
            SetOption(easy, CURLOPT_CAINFO, bundle);
        } else {
            const auto data = embedded::CaBundle();
            curl_blob blob{const_cast<unsigned char*>(data.data()), data.size(),
                           CURL_BLOB_NOCOPY};
            SetOption(easy, CURLOPT_CAINFO, static_cast<const char*>(nullptr));
            SetOption(easy, CURLOPT_CAINFO_BLOB, &blob);
        }
#endif
    }
#ifdef _WIN32
    // A revocation server that cannot be reached, or a private CA without
    // one, does not fail the request.
    SetOption(easy, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_REVOKE_BEST_EFFORT));
#endif

    for (const std::string& header : request.headers) {
        curl_slist* list = curl_slist_append(t.headers, header.c_str());
        if (list == nullptr) {
            throw std::bad_alloc();
        }
        t.headers = list;
    }
    if (t.headers != nullptr) {
        SetOption(easy, CURLOPT_HTTPHEADER, t.headers);
    }

    if (request.method == "HEAD") {
        SetOption(easy, CURLOPT_NOBODY, 1L);
    } else if (request.method != "GET" && request.method != "POST") {
        SetOption(easy, CURLOPT_CUSTOMREQUEST, request.method.c_str());
    }
    if (request.has_body) {
        SetOption(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        SetOption(easy, CURLOPT_COPYPOSTFIELDS, request.body.c_str());
    } else if (request.method == "POST") {
        SetOption(easy, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t{0});
        SetOption(easy, CURLOPT_COPYPOSTFIELDS, "");
    }

    const CURLMcode added = curl_multi_add_handle(Multi(multi_), easy);
    if (added != CURLM_OK) {
        throw std::runtime_error(
            fmt::format("http.request: cannot start the request: {}", curl_multi_strerror(added)));
    }
    t.multi = Multi(multi_);
    transfers_.emplace(t.id, std::move(transfer));
}

void Http::Pump(lua_State* L) {
    if (transfers_.empty()) {
        return;
    }
    int running = 0;
    const CURLMcode code = curl_multi_perform(Multi(multi_), &running);
    if (code != CURLM_OK) {
        log::Error("http: curl_multi_perform failed: {}", curl_multi_strerror(code));
    }
    std::vector<std::unique_ptr<Transfer>> done;
    int queued = 0;
    while (CURLMsg* message = curl_multi_info_read(Multi(multi_), &queued)) {
        if (message->msg != CURLMSG_DONE) {
            continue;
        }
        void* user = nullptr;
        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &user);
        auto* transfer = static_cast<Transfer*>(user);
        transfer->result = message->data.result;
        curl_multi_remove_handle(transfer->multi, transfer->easy);
        transfer->multi = nullptr;
        const auto it = transfers_.find(transfer->id);
        done.push_back(std::move(it->second));
        transfers_.erase(it);
    }
    if (done.empty()) {
        return;
    }
    // One call into Lua for all of them: `done` holds Lua references, so no
    // shutdown may run in between (it would be deferred).
    Invoker::Scope scope(invoker_);
    for (const std::unique_ptr<Transfer>& transfer : done) {
        Complete(L, *transfer);
    }
}

void Http::Complete(lua_State* L, Transfer& transfer) {
    invoker_.Call(L, transfer.callback, "http.request callback", [&transfer](lua_State* thread) {
        if (!lua_checkstack(thread, 6)) {
            throw std::runtime_error("Lua stack overflow");
        }
        if (transfer.result != CURLE_OK) {
            lua_pushnil(thread);
            std::string error;
            if (transfer.too_large) {
                error = fmt::format("the response body is larger than {} bytes", kMaxBody);
            } else if (transfer.error[0] != '\0') {
                error = transfer.error;
            } else {
                error = curl_easy_strerror(transfer.result);
            }
            lua_pushlstring(thread, error.data(), error.size());
            return 2;
        }
        long status = 0;
        curl_easy_getinfo(transfer.easy, CURLINFO_RESPONSE_CODE, &status);
        char* url = nullptr;
        curl_easy_getinfo(transfer.easy, CURLINFO_EFFECTIVE_URL, &url);

        lua_createtable(thread, 0, 4);
        lua_pushinteger(thread, status);
        lua_setfield(thread, -2, "status");
        lua_pushstring(thread, url != nullptr ? url : "");
        lua_setfield(thread, -2, "url");
        lua_pushlstring(thread, transfer.body.data(), transfer.body.size());
        lua_setfield(thread, -2, "body");
        // Header names in lower case; repeated headers joined with ", ".
        lua_createtable(thread, 0, static_cast<int>(transfer.response_headers.size()));
        for (const auto& [name, value] : transfer.response_headers) {
            lua_pushlstring(thread, name.data(), name.size());
            if (lua_rawget(thread, -2) == LUA_TSTRING) {
                lua_pushliteral(thread, ", ");
                lua_pushlstring(thread, value.data(), value.size());
                lua_concat(thread, 3);
            } else {
                lua_pop(thread, 1);
                lua_pushlstring(thread, value.data(), value.size());
            }
            lua_pushlstring(thread, name.data(), name.size());
            lua_insert(thread, -2);
            lua_rawset(thread, -3);
        }
        lua_setfield(thread, -2, "headers");
        lua_pushnil(thread);
        return 2;
    });
}

void Http::Clear() noexcept {
    transfers_.clear();
}

namespace modules {
namespace {

using bindings::ArgError;
using bindings::TypeName;

constexpr std::array<std::string_view, 6> kFields = {
    "url", "method", "headers", "body", "timeout", "redirects",
};

// The longest timeout a request may have, in seconds.
constexpr double kMaxTimeout = 3600;

bool TokenChar(unsigned char c) noexcept {
    return std::isalnum(c) != 0 || std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(c)) !=
                                       std::string_view::npos;
}

std::string_view View(lua_State* L, int idx) {
    std::size_t length = 0;
    const char* text = lua_tolstring(L, idx, &length);
    return {text, length};
}

// Pushes options[field] with a raw read, so no metamethod runs, and returns
// its type.
int Field(lua_State* L, const char* field) {
    lua_pushstring(L, field);
    return lua_rawget(L, 1);
}

[[noreturn]] void FieldError(lua_State* L, const char* field, std::string_view expected) {
    ArgError(L, 1, fmt::format("field '{}' must be {} (got {})", field, expected, TypeName(L, -1)));
}

void CheckUrl(lua_State* L, const std::string& url) {
    std::string scheme = url.substr(0, url.find("://"));
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (url.find("://") == std::string::npos || (scheme != "http" && scheme != "https")) {
        ArgError(L, 1, "the URL must start with http:// or https://");
    }
}

void ReadHeaders(lua_State* L, Http::Request& request) {
    const int headers = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, headers) != 0) {
        if (lua_type(L, -2) != LUA_TSTRING) {
            ArgError(L, 1, "header names must be strings");
        }
        const std::string_view name = View(L, -2);
        if (name.empty() || !std::all_of(name.begin(), name.end(), [](char c) {
                return TokenChar(static_cast<unsigned char>(c));
            })) {
            ArgError(L, 1, fmt::format("invalid header name '{}'", name));
        }
        const int type = lua_type(L, -1);
        if (type != LUA_TSTRING && type != LUA_TNUMBER) {
            ArgError(L, 1, fmt::format("header '{}' must be a string (got {})", name,
                                       TypeName(L, -1)));
        }
        lua_pushvalue(L, -1);  // lua_tolstring converts a number in place
        const std::string_view value = View(L, -1);
        if (value.find_first_of(std::string_view("\r\n\0", 3)) != std::string_view::npos) {
            ArgError(L, 1, fmt::format("header '{}' contains a line break or NUL", name));
        }
        // "Name;" sends an empty header; "Name:" would remove it.
        request.headers.push_back(value.empty() ? fmt::format("{};", name)
                                                : fmt::format("{}: {}", name, value));
        lua_pop(L, 2);
    }
}

Http::Request ReadRequest(lua_State* L) {
    Http::Request request;
    if (lua_type(L, 1) == LUA_TSTRING) {
        request.url = std::string(View(L, 1));
        request.method = "GET";
        CheckUrl(L, request.url);
        return request;
    }
    if (lua_type(L, 1) != LUA_TTABLE) {
        bindings::TypeError(L, 1, "table or URL");
    }
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        lua_pop(L, 1);
        if (lua_type(L, -1) != LUA_TSTRING ||
            std::find(kFields.begin(), kFields.end(), View(L, -1)) == kFields.end()) {
            ArgError(L, 1, fmt::format("unknown field '{}'", luaL_tolstring(L, -1, nullptr)));
        }
    }

    if (Field(L, "url") != LUA_TSTRING) {
        FieldError(L, "url", "a string");
    }
    request.url = std::string(View(L, -1));
    CheckUrl(L, request.url);
    lua_pop(L, 1);

    switch (Field(L, "body")) {
        case LUA_TNIL:
            break;
        case LUA_TSTRING:
            request.body = std::string(View(L, -1));
            request.has_body = true;
            break;
        default:
            FieldError(L, "body", "a string");
    }
    lua_pop(L, 1);

    switch (Field(L, "method")) {
        case LUA_TNIL:
            request.method = request.has_body ? "POST" : "GET";
            break;
        case LUA_TSTRING:
            request.method = std::string(View(L, -1));
            break;
        default:
            FieldError(L, "method", "a string");
    }
    lua_pop(L, 1);
    std::transform(request.method.begin(), request.method.end(), request.method.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (request.method.empty() || request.method.size() > 16 ||
        !std::all_of(request.method.begin(), request.method.end(),
                     [](char c) { return c >= 'A' && c <= 'Z'; })) {
        ArgError(L, 1, fmt::format("invalid method '{}'", request.method));
    }
    if (request.has_body && (request.method == "GET" || request.method == "HEAD")) {
        ArgError(L, 1, fmt::format("a {} request has no body", request.method));
    }

    switch (Field(L, "headers")) {
        case LUA_TNIL:
            break;
        case LUA_TTABLE:
            ReadHeaders(L, request);
            break;
        default:
            FieldError(L, "headers", "a table");
    }
    lua_pop(L, 1);

    switch (Field(L, "timeout")) {
        case LUA_TNIL:
            break;
        case LUA_TNUMBER: {
            const double seconds = lua_tonumber(L, -1);
            if (!(seconds > 0 && seconds <= kMaxTimeout)) {
                ArgError(L, 1, fmt::format("field 'timeout' must be between 0 and {} seconds",
                                           kMaxTimeout));
            }
            request.timeout_ms =
                std::max<std::int64_t>(1, static_cast<std::int64_t>(seconds * 1000));
            break;
        }
        default:
            FieldError(L, "timeout", "a number of seconds");
    }
    lua_pop(L, 1);

    switch (Field(L, "redirects")) {
        case LUA_TNIL:
            break;
        case LUA_TNUMBER: {
            const double redirects = lua_tonumber(L, -1);
            if (!(redirects >= 0 && redirects <= 20) || redirects != std::floor(redirects)) {
                ArgError(L, 1, "field 'redirects' must be an integer between 0 and 20");
            }
            request.max_redirects = static_cast<long>(redirects);
            break;
        }
        default:
            FieldError(L, "redirects", "an integer");
    }
    lua_pop(L, 1);
    return request;
}

// http.request(options, callback) or http.request(url, callback).
void Request(sol::this_state state) {
    lua_State* L = state;
    Runtime& runtime = Runtime::Require(L);
    const Http::Request request = ReadRequest(L);
    sol::main_protected_function callback =
        bindings::RequireFunction(sol::main_object(L, 2), "http.request", 2);
    runtime.Requests().Start(request, std::move(callback));
}

}  // namespace

void RegisterHttp(sol::state& lua) {
    sol::table http = lua.create_table();
    http["request"] = &Request;
    PreloadValue(lua, "http", http);
}

}  // namespace modules

}  // namespace vcmp_lua
