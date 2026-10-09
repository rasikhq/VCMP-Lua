#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vcmp_lua::bindings {

// Stream: client script data (plan A0). Scripts write one to send it with
// SendClientScriptData, and receive one in onClientData. The bytes live in
// a vector: reads and writes are bounds-checked, nothing is copied into a
// fixed array.
//
// Layout, as in v1 and the client's Squirrel streams: bytes as is, numbers
// and floats as 4 bytes little-endian, strings as a big-endian 16-bit length
// and the bytes.
struct Stream {
    // Largest stream a script may build (v1's limit).
    static constexpr std::size_t kMaxSize = 4096;

    std::vector<std::uint8_t> bytes;
    std::size_t read = 0;
};

}  // namespace vcmp_lua::bindings
