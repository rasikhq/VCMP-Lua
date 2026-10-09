#pragma once

#include <sol/sol.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/invoker.hpp"
#include "runtime/config.hpp"

namespace vcmp_lua {

// HTTP(S) requests over libcurl's multi interface (plan B5). Nothing blocks:
// curl resolves names on its own thread, Pump() moves every transfer forward
// once per server frame, and callbacks run on the main thread, from Pump().
class Http {
public:
    // What a script asked for, already checked (modules/http.cpp).
    struct Request {
        std::string url;
        std::string method;  // upper case
        std::vector<std::string> headers;  // "Name: value"
        std::string body;
        bool has_body = false;
        std::int64_t timeout_ms = 30'000;
        long max_redirects = 5;  // 0: redirects are not followed
    };

    // More pending requests than this are refused.
    static constexpr std::size_t kMaxPending = 1000;
    // A larger response body fails the request.
    static constexpr std::size_t kMaxBody = 16 * 1024 * 1024;

    Http(Invoker& invoker, const HttpConfig& config);
    ~Http();

    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;

    // Starts a transfer; callback(response, nil) or callback(nil, error)
    // runs from a later Pump(). Throws when curl refuses the request or too
    // many are pending.
    void Start(const Request& request, sol::main_protected_function callback);

    // Moves the transfers forward. Collects the finished ones first, then
    // calls their callbacks (plan B3.7).
    void Pump(lua_State* L);

    // Cancels every transfer without calling back, and releases the
    // callbacks (Runtime::Shutdown, step 2).
    void Clear() noexcept;

    [[nodiscard]] std::size_t Pending() const noexcept { return transfers_.size(); }

    struct Transfer;

private:
    void Complete(lua_State* L, Transfer& transfer);

    Invoker& invoker_;
    std::string cafile_;  // empty: the platform default (http.cpp)
    void* multi_ = nullptr;  // CURLM*
    std::uint64_t next_id_ = 1;
    std::unordered_map<std::uint64_t, std::unique_ptr<Transfer>> transfers_;
};

}  // namespace vcmp_lua
