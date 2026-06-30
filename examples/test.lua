local quad = require("quadratic")

local function print_result(title, result)
    print("========== " .. title .. " ==========")
    for k, v in pairs(result) do
        print(k, v)
    end
    print()
end

-- ===================================================
-- Old-style free functions (still work, unchanged)
-- ===================================================

print_result("Real Roots", quad.solve(1, -3, 2))
print_result("Repeated Root", quad.solve(1, -2, 1))
print_result("Complex Roots", quad.solve(1, 2, 5))
print_result("Linear Equation", quad.solve(0, 2, -8))
print_result("Invalid Equation", quad.solve(0, 0, 5))

print("Discriminant:", quad.discriminant(1, -3, 2))
print()

-- ===================================================
-- New object-oriented API
-- ===================================================

print("===== Object API: quad.quadratic(a, b, c) =====")
local q = quad.quadratic(2, -4, -6)

-- print the quadratic with variables (uses __tostring)
print("Equation:", tostring(q))

-- coefficients
local a, b, c = q:coefficients()
print(string.format("a=%g  b=%g  c=%g", a, b, c))

-- discriminant
print("Discriminant:", q:discriminant())

-- root type
print("Type:", q:type())

-- solve -> table
print_result("Solve()", q:solve())

-- evaluate at a point
print("q(3) =", q:evaluate(3))

-- vertex
local h, k = q:vertex()
print(string.format("Vertex: (%g, %g)", h, k))

-- derivative coefficients (2a, b)
local da, db = q:derivative()
print(string.format("Derivative: %gx %s %g", da, db < 0 and "-" or "+", math.abs(db)))

print()

-- custom variable symbol
print("===== Custom variable symbol =====")
local q2 = quad.quadratic(1, -5, 6, "t")
print("Equation:", tostring(q2))
q2:setvar("y")
print("After setvar('y'):", tostring(q2))
print()

-- ===================================================
-- Step-by-step solving
-- ===================================================

print("===== quad.process(a, b, c) - free function =====")
quad.process(1, -3, 2)
print()

print("===== obj:process() - method, complex case =====")
local q3 = quad.quadratic(1, 2, 5)
q3:process()
print()

print("===== obj:process() - repeated root =====")
local q4 = quad.quadratic(1, -2, 1)
q4:process()
print()

print("===== obj:process() - linear (a = 0) =====")
local q5 = quad.quadratic(0, 2, -8)
q5:process()
print()

print("===== obj:process() - invalid (a = 0, b = 0) =====")
local q6 = quad.quadratic(0, 0, 5)
q6:process()
