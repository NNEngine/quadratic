/* quadratic.h */
#ifndef QUADRATIC_H
#define QUADRATIC_H

#include <math.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Root type classification */
typedef enum {
    QUAD_LINEAR,
    QUAD_REAL,
    QUAD_REPEATED,
    QUAD_COMPLEX,
    QUAD_INVALID
} QuadraticType;

/* Solver result */
typedef struct {
    QuadraticType type;
    double discriminant;

    double root1;
    double root2;

    double real;
    double imag;
} QuadraticResult;

/* Name used for the userdata metatable that backs quadratic objects
 * created with quadratic.quadratic(a, b, c [, var]) */
#define QUAD_META "Quadratic.meta"

/* ---- Free (module-level) functions ----
 * quadratic.solve(a, b, c)
 * quadratic.discriminant(a, b, c)
 * quadratic.process(a, b, c [, var])
 * quadratic.tostring(a, b, c [, var])
 * quadratic.quadratic(a, b, c [, var])  -- constructor, alias: quadratic.new
 */
int l_solve(lua_State *L);
int l_discriminant(lua_State *L);
int l_process_free(lua_State *L);
int l_tostring_free(lua_State *L);
int l_new(lua_State *L);

/* ---- Object methods (called as obj:method(...)) ----
 * obj:coefficients()  -> a, b, c
 * obj:discriminant()  -> number
 * obj:solve()         -> result table (same shape as quadratic.solve)
 * obj:type()          -> "linear" | "real" | "repeated" | "complex" | "invalid"
 * obj:evaluate(x)      -> a*x^2 + b*x + c
 * obj:vertex()          -> h, k   (nil, err if a == 0)
 * obj:derivative()      -> 2a, b  (coefficients of the linear derivative)
 * obj:setvar(name)      -> change the variable symbol used in tostring/process
 * obj:tostring()         -> "ax^2 + bx + c = 0" style string (also __tostring)
 * obj:process()          -> prints + returns the step-by-step solution
 */
int m_coefficients(lua_State *L);
int m_discriminant(lua_State *L);
int m_solve(lua_State *L);
int m_type(lua_State *L);
int m_evaluate(lua_State *L);
int m_vertex(lua_State *L);
int m_derivative(lua_State *L);
int m_setvar(lua_State *L);
int m_tostring(lua_State *L);
int m_process(lua_State *L);

/* Module entry point */
int luaopen_quadratic(lua_State *L);

#endif
