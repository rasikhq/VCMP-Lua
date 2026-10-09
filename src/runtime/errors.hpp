#pragma once

#include <optional>
#include <stdexcept>
#include <string>

struct lua_State;

namespace vcmp_lua {

// Thrown by the panic handler: Lua raised an error outside any protected
// call. The Lua state must not be used again.
class LuaPanic : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// lua_atpanic handler. Never returns: it marks the owning runtime dead (if the
// state has one) and throws LuaPanic, which the callback boundary catches.
int Panic(lua_State* L);

// Message handler for lua_pcall that appends a stack traceback.
int Traceback(lua_State* L);

// Text of the error value at idx, whatever its type.
std::string ErrorText(lua_State* L, int idx);

// Calls the function below the nargs arguments on top of the stack in
// protected mode, with Traceback as message handler. On success the results
// are left on the stack and nothing is returned; on error the stack is
// restored to below the function and the error text is returned.
std::optional<std::string> ProtectedCall(lua_State* L, int nargs, int nresults);

}  // namespace vcmp_lua
