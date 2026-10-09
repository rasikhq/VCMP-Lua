// Stream: v1's members, ported with fixes (plan A0):
// - Received data is copied into a vector of its size. v1 copied it into a
//   fixed 4096-byte array on the stack without checking the size the client
//   sent: a remote stack overflow.
// - Reads check that the whole value is there (v1 checked only that one
//   byte was left) and raise an error instead of returning 0.
// - Writes check the space for the whole value (v1's writeString could
//   write one byte past the buffer) and raise an error when it is full.
#include "bindings/stream.hpp"

#include <fmt/format.h>
#include <sol/sol.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include "bindings/args.hpp"
#include "bindings/bindings.hpp"

namespace vcmp_lua::bindings {
namespace {

void Need(const Stream& stream, std::size_t size, const char* what) {
    if (stream.bytes.size() - stream.read < size) {
        throw std::out_of_range(fmt::format("Stream: not enough data to read {} ({} of {} bytes left)",
                                            what, stream.bytes.size() - stream.read, size));
    }
}

void Room(const Stream& stream, std::size_t size, const char* what) {
    if (Stream::kMaxSize - stream.bytes.size() < size) {
        throw std::length_error(fmt::format("Stream: no room to write {} ({} of {} bytes used)",
                                            what, stream.bytes.size(), Stream::kMaxSize));
    }
}

std::uint32_t ReadU32(Stream& stream, const char* what) {
    Need(stream, 4, what);
    const std::uint8_t* p = stream.bytes.data() + stream.read;
    stream.read += 4;
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 |
           static_cast<std::uint32_t>(p[2]) << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}

void WriteU32(Stream& stream, std::uint32_t value, const char* what) {
    Room(stream, 4, what);
    for (int shift = 0; shift < 32; shift += 8) {
        stream.bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

}  // namespace

void RegisterStream(sol::state& lua) {
    lua.new_usertype<Stream>(
        "Stream", sol::constructors<Stream()>(),
        // Stream() as well as Stream.new().
        sol::call_constructor, sol::constructors<Stream()>(),

        "readByte",
        [](Stream& stream) {
            Need(stream, 1, "a byte");
            return static_cast<int>(stream.bytes[stream.read++]);
        },
        "readNumber",
        [](Stream& stream) { return static_cast<std::int32_t>(ReadU32(stream, "a number")); },
        "readFloat",
        [](Stream& stream) {
            const std::uint32_t bits = ReadU32(stream, "a float");
            float value = 0;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        },
        "readString",
        [](Stream& stream) {
            Need(stream, 2, "a string length");
            const std::size_t length = static_cast<std::size_t>(stream.bytes[stream.read]) << 8 |
                                       stream.bytes[stream.read + 1];
            if (stream.bytes.size() - stream.read - 2 < length) {
                throw std::out_of_range(
                    fmt::format("Stream: not enough data to read a string of {} bytes", length));
            }
            stream.read += 2;
            std::string text(reinterpret_cast<const char*>(stream.bytes.data() + stream.read),
                             length);
            stream.read += length;
            return text;
        },

        // writeByte(b): -128 to 255; the low 8 bits are written.
        "writeByte",
        [](Stream& stream, sol::this_state L) {
            const auto value = CheckInteger(L, 2, -128, 255);
            Room(stream, 1, "a byte");
            stream.bytes.push_back(static_cast<std::uint8_t>(value));
        },
        // writeNumber(n): a 32-bit integer, signed or unsigned.
        "writeNumber",
        [](Stream& stream, sol::this_state L) {
            const auto value = CheckInteger(L, 2, std::numeric_limits<std::int32_t>::min(),
                                            std::numeric_limits<std::uint32_t>::max());
            WriteU32(stream, static_cast<std::uint32_t>(value), "a number");
        },
        "writeFloat",
        [](Stream& stream, Float value) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value.value, sizeof(bits));
            WriteU32(stream, bits, "a float");
        },
        "writeString",
        [](Stream& stream, String text) {
            const std::string& value = text.value;
            if (value.size() > 0xFFFF) {
                throw std::length_error("Stream: a string is at most 65535 bytes");
            }
            Room(stream, 2 + value.size(), "a string");
            stream.bytes.push_back(static_cast<std::uint8_t>(value.size() >> 8));
            stream.bytes.push_back(static_cast<std::uint8_t>(value.size() & 0xFF));
            stream.bytes.insert(stream.bytes.end(), value.begin(), value.end());
        },

        // stream:send(player): to one player; stream:send() or send(nil): to
        // every player.
        "send",
        [](Stream& stream, Ctx ctx, Opt<Live<EntityKind::Player>> player) {
            const auto send = VCMP_FN(ctx, SendClientScriptData);
            if (player.has_value()) {
                return Check(ctx.L, send(player->id, stream.bytes.data(), stream.bytes.size()));
            }
            EntityPool& players = ctx.runtime->Entities().Players();
            for (const std::int32_t id : players.AliveIds()) {
                send(id, stream.bytes.data(), stream.bytes.size());
            }
            return true;
        },

        // stream:clear(): empty, ready to write again.
        "clear",
        [](Stream& stream) {
            stream.bytes.clear();
            stream.read = 0;
        },

        // stream.size: bytes in the stream; stream.remaining: bytes not read yet.
        "size", sol::property([](const Stream& stream) { return stream.bytes.size(); }),
        "remaining",
        sol::property([](const Stream& stream) { return stream.bytes.size() - stream.read; }),

        sol::meta_function::to_string,
        [](const Stream& stream) { return fmt::format("Stream({} bytes)", stream.bytes.size()); });
}

}  // namespace vcmp_lua::bindings
