-- Loaded before every Lua test file (tests/unit/test_lua.cpp). Each file runs
-- in a new FakeServer: see tests/unit/fake_server.hpp for the fake.* table.

failures = {}

-- test(name, fn): runs fn with a cleared call log; a failure is collected
-- and the next test runs.
function test(name, fn)
    fake.clear()
    local ok, err = xpcall(fn, debug.traceback)
    if not ok then
        failures[#failures + 1] = name .. ": " .. tostring(err)
    end
end

function expect_eq(actual, expected, what)
    if actual ~= expected then
        error(("%s: expected %s, got %s"):format(what or "value", tostring(expected), tostring(actual)), 2)
    end
end

function expect_near(actual, expected, what)
    if type(actual) ~= "number" or math.abs(actual - expected) > 1e-4 then
        error(("%s: expected %s, got %s"):format(what or "value", tostring(expected), tostring(actual)), 2)
    end
end

function expect_vec(actual, x, y, z, what)
    if type(actual) ~= "table" then
        error(("%s: expected a table, got %s"):format(what or "value", type(actual)), 2)
    end
    for i, v in ipairs({ x, y, z }) do
        if math.abs(actual[i] - v) > 1e-4 then
            error(("%s[%d]: expected %s, got %s"):format(what or "value", i, v, tostring(actual[i])), 2)
        end
    end
end

-- expect_error(text, fn, ...): fn(...) raises an error containing text.
function expect_error(text, fn, ...)
    local ok, err = pcall(fn, ...)
    if ok then
        error(("expected an error containing '%s'"):format(text), 2)
    end
    if not tostring(err):find(text, 1, true) then
        error(("expected an error containing '%s', got '%s'"):format(text, tostring(err)), 2)
    end
end

-- expect_call(call): the last server call.
function expect_call(call)
    local last = fake.last()
    if last ~= call then
        error(("expected the server call %s, got %s"):format(call, tostring(last)), 2)
    end
end

-- expect_calls{...}: every server call since the test started, in order.
function expect_calls(expected)
    local calls = fake.calls()
    local same = #calls == #expected
    for i = 1, #expected do
        same = same and calls[i] == expected[i]
    end
    if not same then
        error(("expected the server calls\n  %s\ngot\n  %s"):format(
            table.concat(expected, "\n  "), table.concat(calls, "\n  ")), 2)
    end
end
