// Hash: v1's digests, unchanged, so stored hashes still verify; and OpenSSL's
// HMAC, PBKDF2, scrypt and random bytes for new password storage.
#include <digestpp.hpp>
#include <fmt/format.h>
#include <lua.hpp>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <sol/sol.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "bindings/args.hpp"
#include "modules/modules.hpp"
#include "runtime/preload.hpp"

namespace vcmp_lua::modules {
namespace {

using bindings::ArgError;
using bindings::CheckInteger;
using bindings::CheckString;
using bindings::String;

// Digest algorithms of hmac and pbkdf2, by the names scripts use.
constexpr std::array<std::string_view, 8> kDigests = {
    "md5", "sha1", "sha224", "sha256", "sha384", "sha512", "sha3-256", "sha3-512",
};

// The largest output pbkdf2 and scrypt produce, and the most random bytes in
// one call.
constexpr std::int64_t kMaxOutput = 1024;
constexpr std::int64_t kMaxRandom = 1 << 20;
// PBKDF2 runs on the server thread: more rounds would stop it for minutes.
constexpr std::int64_t kMaxIterations = 10'000'000;

struct MdDeleter {
    void operator()(EVP_MD* md) const noexcept { EVP_MD_free(md); }
};
using Md = std::unique_ptr<EVP_MD, MdDeleter>;

std::string Hex(const unsigned char* data, std::size_t size) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string hex(size * 2, '\0');
    for (std::size_t i = 0; i < size; ++i) {
        hex[2 * i] = kDigits[data[i] >> 4];
        hex[2 * i + 1] = kDigits[data[i] & 0x0f];
    }
    return hex;
}

std::string Hex(const std::string& bytes) {
    return Hex(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
}

// The digest named by argument index (default: sha256 if absent).
Md CheckDigest(lua_State* L, int index) {
    std::string name = "sha256";
    if (!lua_isnoneornil(L, index)) {
        name = CheckString(L, index);
    }
    bool known = false;
    for (const std::string_view digest : kDigests) {
        known = known || name == digest;
    }
    Md md(known ? EVP_MD_fetch(nullptr, name.c_str(), nullptr) : nullptr);
    if (md == nullptr) {
        ArgError(L, index,
                 fmt::format("unknown digest '{}' (use md5, sha1, sha224, sha256, sha384, "
                             "sha512, sha3-256 or sha3-512)",
                             name));
    }
    return md;
}

template <typename Hasher>
std::string Digest(String input) {
    return Hasher().absorb(input.value).hexdigest();
}

// v1's keyed XOFs: 64 bytes of output, as hex.
template <typename Hasher>
std::string KeyedXof(String key, String input) {
    return Hasher().set_key(key.value).absorb(input.value).hexsqueeze(64);
}

// Hash.hmac(digest, key, data): hex.
std::string Hmac(sol::this_state state) {
    lua_State* L = state;
    const Md md = CheckDigest(L, 1);
    const std::string key = CheckString(L, 2);
    const std::string data = CheckString(L, 3);
    std::array<unsigned char, EVP_MAX_MD_SIZE> out{};
    unsigned int size = 0;
    if (HMAC(md.get(), key.data(), static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char*>(data.data()), data.size(), out.data(),
             &size) == nullptr) {
        throw std::runtime_error("Hash.hmac failed");
    }
    return Hex(out.data(), size);
}

// Hash.pbkdf2(password, salt, iterations, length[, digest]): hex of length
// bytes, PBKDF2-HMAC with digest (default sha256).
std::string Pbkdf2(sol::this_state state) {
    lua_State* L = state;
    const std::string password = CheckString(L, 1);
    const std::string salt = CheckString(L, 2);
    const auto iterations = static_cast<int>(CheckInteger(L, 3, 1, kMaxIterations));
    const auto length = static_cast<std::size_t>(CheckInteger(L, 4, 1, kMaxOutput));
    const Md md = CheckDigest(L, 5);
    std::string out(length, '\0');
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char*>(salt.data()),
                          static_cast<int>(salt.size()), iterations, md.get(),
                          static_cast<int>(length),
                          reinterpret_cast<unsigned char*>(out.data())) != 1) {
        throw std::runtime_error("Hash.pbkdf2 failed");
    }
    return Hex(out);
}

// Hash.scrypt(password, salt, N, r, p, length): hex of length bytes. N is a
// power of two; OpenSSL refuses parameters that need more than 32 MiB.
std::string Scrypt(sol::this_state state) {
    lua_State* L = state;
    const std::string password = CheckString(L, 1);
    const std::string salt = CheckString(L, 2);
    const auto n = static_cast<std::uint64_t>(CheckInteger(L, 3, 2, std::int64_t{1} << 30));
    const auto r = static_cast<std::uint64_t>(CheckInteger(L, 4, 1, 1 << 16));
    const auto p = static_cast<std::uint64_t>(CheckInteger(L, 5, 1, 1 << 16));
    const auto length = static_cast<std::size_t>(CheckInteger(L, 6, 1, kMaxOutput));
    if ((n & (n - 1)) != 0) {
        ArgError(L, 3, "N must be a power of two");
    }
    std::string out(length, '\0');
    if (EVP_PBE_scrypt(password.data(), password.size(),
                       reinterpret_cast<const unsigned char*>(salt.data()), salt.size(), n, r, p,
                       0, reinterpret_cast<unsigned char*>(out.data()), length) != 1) {
        throw std::runtime_error(
            "Hash.scrypt: the parameters need too much memory (N * r * 128 bytes, at most 32 MiB)");
    }
    return Hex(out);
}

// Hash.randomBytes(n): n bytes from OpenSSL's CSPRNG, as a binary string.
std::string RandomBytes(sol::this_state state) {
    lua_State* L = state;
    const auto count = static_cast<std::size_t>(CheckInteger(L, 1, 0, kMaxRandom));
    std::string out(count, '\0');
    if (count > 0 &&
        RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), static_cast<int>(count)) != 1) {
        throw std::runtime_error("Hash.randomBytes: the random generator failed");
    }
    return out;
}

// Hash.toHex(bytes): lower-case hex.
std::string ToHex(String bytes) {
    return Hex(bytes.value);
}

// Hash.equals(a, b): compares in constant time (for a stored hash).
bool Equals(String a, String b) {
    return a.value.size() == b.value.size() &&
           CRYPTO_memcmp(a.value.data(), b.value.data(), a.value.size()) == 0;
}

}  // namespace

void RegisterHash(sol::state& lua) {
    sol::table hash = lua.create_named_table("Hash");
    hash["MD5"] = &Digest<digestpp::md5>;
    hash["SHA1"] = &Digest<digestpp::sha1>;
    hash["SHA256"] = &Digest<digestpp::sha256>;
    hash["SHA512"] = &Digest<digestpp::sha512>;
    hash["Whirlpool"] = &Digest<digestpp::whirlpool>;
    hash["KMAC256"] = &KeyedXof<digestpp::kmac256_xof>;
    hash["SKEIN256"] = &KeyedXof<digestpp::skein256_xof>;
    hash["SKEIN512"] = &KeyedXof<digestpp::skein512_xof>;
    hash["hmac"] = &Hmac;
    hash["pbkdf2"] = &Pbkdf2;
    hash["scrypt"] = &Scrypt;
    hash["randomBytes"] = &RandomBytes;
    hash["toHex"] = &ToHex;
    hash["equals"] = &Equals;
    PreloadValue(lua, "hash", hash);
}

}  // namespace vcmp_lua::modules
