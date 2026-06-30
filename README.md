# quadratic

A small C extension for Lua 5.4 that solves quadratic (and degenerate linear)
equations, with both a quick functional API and an object-oriented API for
working with reusable quadratic equations — including step-by-step solutions.

## Features

- Solve `ax^2 + bx + c = 0` for real, repeated, complex, or linear roots
- Compute the discriminant
- Create reusable quadratic objects with `quadratic.quadratic(a, b, c)`
- Pretty-print equations (`2x^2 - 4x - 6 = 0`) with a custom variable symbol
- Evaluate the quadratic at a point, find its vertex, and get its derivative
- Print (and capture) a full step-by-step quadratic-formula derivation via `process()`

## Building

Requires Lua 5.4 development headers (`liblua5.4-dev` on Debian/Ubuntu).

```bash
make
```

This produces `quadratic.so`, loadable from Lua via `require("quadratic")`.

```bash
lua5.4 test.lua
```

## Quick start

```lua
local quad = require("quadratic")

-- Functional API
print_result = quad.solve(1, -3, 2)   -- { type="real", root1=2, root2=1, discriminant=1 }
print(quad.discriminant(1, -3, 2))    -- 1

-- Object API
local q = quad.quadratic(2, -4, -6)
print(tostring(q))         -- 2x^2 - 4x - 6 = 0
print(q:discriminant())    -- 64
print(q:type())            -- real
print(q:evaluate(3))       -- 0

q:process()                -- prints the full step-by-step solution
```

## API Reference

### Module functions

| Function | Description |
|---|---|
| `quadratic.quadratic(a, b, c [, var])` | Creates a reusable quadratic object. `var` defaults to `"x"`. Alias: `quadratic.new` |
| `quadratic.solve(a, b, c)` | Returns a result table — see [Result table](#result-table) |
| `quadratic.discriminant(a, b, c)` | Returns `b^2 - 4ac` |
| `quadratic.tostring(a, b, c [, var])` | Returns the equation as a formatted string |
| `quadratic.process(a, b, c [, var])` | Prints and returns the step-by-step solution |

### Object methods

Created via `local q = quadratic.quadratic(a, b, c)`.

| Method | Description |
|---|---|
| `q:coefficients()` | Returns `a, b, c` |
| `q:discriminant()` | Returns `b^2 - 4ac` |
| `q:solve()` | Returns a result table — see [Result table](#result-table) |
| `q:type()` | Returns `"linear"`, `"real"`, `"repeated"`, `"complex"`, or `"invalid"` |
| `q:evaluate(x)` | Returns `a*x^2 + b*x + c` |
| `q:vertex()` | Returns `h, k` (the vertex). Returns `nil, errmsg` if `a == 0` |
| `q:derivative()` | Returns the linear derivative's coefficients `2a, b` |
| `q:setvar(name)` | Changes the variable symbol used by `tostring`/`process` |
| `q:tostring()` / `tostring(q)` | Returns the equation as a formatted string, e.g. `2x^2 - 4x - 6 = 0` |
| `q:process()` | Prints and returns the step-by-step quadratic-formula solution |

### Result table

`solve()` returns a table whose shape depends on `type`:

| `type` | Fields |
|---|---|
| `"real"` | `discriminant`, `root1`, `root2` |
| `"repeated"` | `discriminant`, `root` |
| `"complex"` | `discriminant`, `real`, `imag` |
| `"linear"` | `root` (used when `a == 0`, `b != 0`) |
| `"invalid"` | *(no extra fields; `a == 0` and `b == 0`)* |

## Project layout

```
include/quadratic.h   Header: types, struct, function declarations
src/quadratic.c        Implementation: solver, formatting, Lua bindings
test.lua                Example usage / smoke test
Makefile                Build script
```

## License

MIT

## Author

NNEngine
