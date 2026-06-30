--[[
    tests/test_quadratic.lua

    Self-contained unit tests for the `quadratic` library. No external
    test framework is required (the rockspec only depends on lua >= 5.4),
    so this file implements a tiny assertion-based runner.

    Run with:
        lua5.4 tests/test_quadratic.lua

    Exits with status 0 if all tests pass, 1 otherwise (CI-friendly).
]]

package.cpath = "./?.so;" .. package.cpath
local quad = require("quadratic")

-- ===========================================================
-- Tiny test runner
-- ===========================================================

local tests = {}
local order = {}

local function test(name, fn)
    tests[name] = fn
    order[#order + 1] = name
end

local EPS = 1e-9

local function approx(a, b, eps)
    eps = eps or EPS
    return math.abs(a - b) <= eps
end

local function assert_approx(actual, expected, msg)
    if not approx(actual, expected) then
        error(string.format("%s: expected %s, got %s",
            msg or "assert_approx", tostring(expected), tostring(actual)), 2)
    end
end

local function assert_eq(actual, expected, msg)
    if actual ~= expected then
        error(string.format("%s: expected %s, got %s",
            msg or "assert_eq", tostring(expected), tostring(actual)), 2)
    end
end

local function assert_true(cond, msg)
    if not cond then
        error(msg or "assertion failed", 2)
    end
end

local function assert_contains(haystack, needle, msg)
    if not haystack:find(needle, 1, true) then
        error(string.format("%s: expected to find %q in:\n%s",
            msg or "assert_contains", needle, haystack), 2)
    end
end

local function assert_errors(fn, msg)
    local ok = pcall(fn)
    if ok then
        error(msg or "expected an error, but none was raised", 2)
    end
end

-- ===========================================================
-- quadratic.solve() / quadratic.discriminant()
-- ===========================================================

test("solve: two distinct real roots", function()
    local r = quad.solve(1, -3, 2)
    assert_eq(r.type, "real")
    assert_approx(r.discriminant, 1)
    assert_approx(r.root1, 2)
    assert_approx(r.root2, 1)
end)

test("solve: repeated root", function()
    local r = quad.solve(1, -2, 1)
    assert_eq(r.type, "repeated")
    assert_approx(r.discriminant, 0)
    assert_approx(r.root, 1)
end)

test("solve: complex roots", function()
    local r = quad.solve(1, 2, 5)
    assert_eq(r.type, "complex")
    assert_approx(r.discriminant, -16)
    assert_approx(r.real, -1)
    assert_approx(r.imag, 2)
end)

test("solve: linear (a = 0, b != 0)", function()
    local r = quad.solve(0, 2, -8)
    assert_eq(r.type, "linear")
    assert_approx(r.root, 4)
end)

test("solve: invalid (a = 0, b = 0)", function()
    local r = quad.solve(0, 0, 5)
    assert_eq(r.type, "invalid")
end)

test("discriminant: matches b^2 - 4ac", function()
    assert_approx(quad.discriminant(1, -3, 2), 1)
    assert_approx(quad.discriminant(1, 2, 5), -16)
    assert_approx(quad.discriminant(3, 0, -12), 144)
end)

-- ===========================================================
-- quadratic.tostring() (free function)
-- ===========================================================

test("tostring: standard equation", function()
    assert_eq(quad.tostring(2, -4, -6), "2x^2 - 4x - 6 = 0")
end)

test("tostring: coefficient of 1 is omitted", function()
    assert_eq(quad.tostring(1, -3, 2), "x^2 - 3x + 2 = 0")
end)

test("tostring: zero terms are skipped", function()
    assert_eq(quad.tostring(1, 0, -4), "x^2 - 4 = 0")
    assert_eq(quad.tostring(1, 5, 0), "x^2 + 5x = 0")
end)

test("tostring: all-zero equation", function()
    assert_eq(quad.tostring(0, 0, 0), "0 = 0")
end)

test("tostring: custom variable symbol", function()
    assert_eq(quad.tostring(1, -5, 6, "t"), "t^2 - 5t + 6 = 0")
end)

-- ===========================================================
-- quadratic.process() (free function)
-- ===========================================================

test("process: returns the equation and final roots (real case)", function()
    local out = quad.process(1, -3, 2)
    assert_contains(out, "x^2 - 3x + 2 = 0")
    assert_contains(out, "two distinct real roots")
end)

test("process: complex case mentions conjugates", function()
    local out = quad.process(1, 2, 5)
    assert_contains(out, "complex conjugates")
end)

test("process: linear case (a = 0)", function()
    local out = quad.process(0, 2, -8)
    assert_contains(out, "LINEAR equation")
end)

test("process: invalid case (a = 0, b = 0)", function()
    local out = quad.process(0, 0, 5)
    assert_contains(out, "not a valid equation")
end)

-- ===========================================================
-- Object API: quadratic.quadratic() / quadratic.new()
-- ===========================================================

test("object: quadratic() and new() both construct objects", function()
    local q1 = quad.quadratic(1, -3, 2)
    local q2 = quad.new(1, -3, 2)
    assert_eq(tostring(q1), tostring(q2))
end)

test("object: coefficients() round-trips constructor args", function()
    local q = quad.quadratic(2, -4, -6)
    local a, b, c = q:coefficients()
    assert_approx(a, 2)
    assert_approx(b, -4)
    assert_approx(c, -6)
end)

test("object: discriminant() matches free function", function()
    local q = quad.quadratic(2, -4, -6)
    assert_approx(q:discriminant(), quad.discriminant(2, -4, -6))
end)

test("object: solve() matches free function", function()
    local q = quad.quadratic(2, -4, -6)
    local r = q:solve()
    assert_eq(r.type, "real")
    assert_approx(r.root1, 3)
    assert_approx(r.root2, -1)
end)

test("object: type() reflects the equation kind", function()
    assert_eq(quad.quadratic(1, -3, 2):type(), "real")
    assert_eq(quad.quadratic(1, -2, 1):type(), "repeated")
    assert_eq(quad.quadratic(1, 2, 5):type(), "complex")
    assert_eq(quad.quadratic(0, 2, -8):type(), "linear")
    assert_eq(quad.quadratic(0, 0, 5):type(), "invalid")
end)

test("object: evaluate() computes a*x^2 + b*x + c", function()
    local q = quad.quadratic(2, -4, -6)
    assert_approx(q:evaluate(3), 0)    -- known root
    assert_approx(q:evaluate(0), -6)   -- y-intercept
    assert_approx(q:evaluate(1), -8)
end)

test("object: vertex() returns the turning point", function()
    local q = quad.quadratic(1, -2, 3) -- vertex at (1, 2)
    local h, k = q:vertex()
    assert_approx(h, 1)
    assert_approx(k, 2)
end)

test("object: vertex() fails gracefully when a == 0", function()
    local q = quad.quadratic(0, 2, -8)
    local h, err = q:vertex()
    assert_eq(h, nil)
    assert_true(type(err) == "string" and #err > 0, "expected an error message")
end)

test("object: derivative() returns 2a, b", function()
    local q = quad.quadratic(3, -5, 7)
    local da, db = q:derivative()
    assert_approx(da, 6)
    assert_approx(db, -5)
end)

test("object: setvar() changes the printed variable", function()
    local q = quad.quadratic(1, -5, 6, "t")
    assert_eq(tostring(q), "t^2 - 5t + 6 = 0")
    q:setvar("y")
    assert_eq(tostring(q), "y^2 - 5y + 6 = 0")
end)

test("object: tostring() and __tostring agree", function()
    local q = quad.quadratic(2, -4, -6)
    assert_eq(q:tostring(), tostring(q))
end)

test("object: process() returns the equation and respects custom var", function()
    local q = quad.quadratic(1, -3, 2, "z")
    local out = q:process()
    assert_contains(out, "z^2 - 3z + 2 = 0")
end)

-- ===========================================================
-- Error handling
-- ===========================================================

test("errors: solve() requires numeric arguments", function()
    assert_errors(function() quad.solve("a", "b", "c") end)
end)

test("errors: calling a method with the wrong self type fails", function()
    local q = quad.quadratic(1, 2, 3)
    local coefficients_method = getmetatable(q).__index.coefficients
    assert_errors(function() coefficients_method({}) end)
end)

-- ===========================================================
-- Runner
-- ===========================================================

local passed, failed = 0, 0

for _, name in ipairs(order) do
    local ok, err = pcall(tests[name])
    if ok then
        passed = passed + 1
        print(string.format("[PASS] %s", name))
    else
        failed = failed + 1
        print(string.format("[FAIL] %s\n       %s", name, tostring(err)))
    end
end

print(string.format("\n%d passed, %d failed, %d total", passed, failed, passed + failed))

os.exit(failed == 0 and 0 or 1)
