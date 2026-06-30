#include <string.h>
#include <stdio.h>

#include "../include/quadratic.h"

/* ===========================
   Userdata: a quadratic object
   =========================== */

typedef struct {
    double a, b, c;
    char var;   /* variable symbol used when printing, default 'x' */
} QuadObj;

static QuadObj *check_quad(lua_State *L, int idx)
{
    return (QuadObj *) luaL_checkudata(L, idx, QUAD_META);
}

/* ===========================
   Core Solver Logic
   =========================== */

static QuadraticResult solve_quadratic(double a, double b, double c)
{
    QuadraticResult result;
    result.discriminant = 0.0;
    result.root1 = result.root2 = 0.0;
    result.real = result.imag = 0.0;

    if (a == 0.0) {
        if (b == 0.0) {
            result.type = QUAD_INVALID;
            return result;
        }

        result.type = QUAD_LINEAR;
        result.root1 = -c / b;
        return result;
    }

    result.discriminant = (b * b) - (4.0 * a * c);

    if (result.discriminant > 0.0) {
        result.type = QUAD_REAL;

        result.root1 =
            (-b + sqrt(result.discriminant)) / (2.0 * a);

        result.root2 =
            (-b - sqrt(result.discriminant)) / (2.0 * a);
    }
    else if (result.discriminant == 0.0) {
        result.type = QUAD_REPEATED;

        result.root1 = -b / (2.0 * a);
        result.root2 = result.root1;
    }
    else {
        result.type = QUAD_COMPLEX;

        result.real = -b / (2.0 * a);
        result.imag = sqrt(-(result.discriminant)) / (2.0 * a);
    }

    return result;
}

/* Push a QuadraticResult as a Lua table, shared by both the free
 * quadratic.solve() function and the obj:solve() method. */
static void push_result_table(lua_State *L, QuadraticResult result)
{
    lua_newtable(L);

    switch (result.type) {
        case QUAD_INVALID:
            lua_pushstring(L, "invalid");
            lua_setfield(L, -2, "type");
            break;

        case QUAD_LINEAR:
            lua_pushstring(L, "linear");
            lua_setfield(L, -2, "type");

            lua_pushnumber(L, result.root1);
            lua_setfield(L, -2, "root");
            break;

        case QUAD_REAL:
            lua_pushstring(L, "real");
            lua_setfield(L, -2, "type");

            lua_pushnumber(L, result.discriminant);
            lua_setfield(L, -2, "discriminant");

            lua_pushnumber(L, result.root1);
            lua_setfield(L, -2, "root1");

            lua_pushnumber(L, result.root2);
            lua_setfield(L, -2, "root2");
            break;

        case QUAD_REPEATED:
            lua_pushstring(L, "repeated");
            lua_setfield(L, -2, "type");

            lua_pushnumber(L, result.discriminant);
            lua_setfield(L, -2, "discriminant");

            lua_pushnumber(L, result.root1);
            lua_setfield(L, -2, "root");
            break;

        case QUAD_COMPLEX:
            lua_pushstring(L, "complex");
            lua_setfield(L, -2, "type");

            lua_pushnumber(L, result.discriminant);
            lua_setfield(L, -2, "discriminant");

            lua_pushnumber(L, result.real);
            lua_setfield(L, -2, "real");

            lua_pushnumber(L, result.imag);
            lua_setfield(L, -2, "imag");
            break;
    }
}

static const char *type_name(QuadraticType t)
{
    switch (t) {
        case QUAD_LINEAR:   return "linear";
        case QUAD_REAL:     return "real";
        case QUAD_REPEATED: return "repeated";
        case QUAD_COMPLEX:  return "complex";
        case QUAD_INVALID:  return "invalid";
    }
    return "invalid";
}

/* ===========================
   Equation pretty-printing
   =========================== */

/* Formats a double without a trailing ".0" when it's a whole number. */
static void format_number(double v, char *buf, size_t n)
{
    if (fabs(v - round(v)) < 1e-9) {
        snprintf(buf, n, "%g", round(v) + 0.0);
    } else {
        snprintf(buf, n, "%.4g", v);
    }
}

/* Appends one term (coeff * suffix) to out, handling signs, the
 * "+"/"-" joiner, and dropping a coefficient of 1. */
static void append_term(char *out, size_t outsz, double coeff,
                         const char *suffix, int *first)
{
    if (coeff == 0.0) return;

    char numbuf[64];
    char piece[80];
    double absval = fabs(coeff);
    int is_one = (fabs(absval - 1.0) < 1e-9) && suffix[0] != '\0';

    if (is_one) {
        snprintf(piece, sizeof(piece), "%s", suffix);
    } else {
        format_number(absval, numbuf, sizeof(numbuf));
        snprintf(piece, sizeof(piece), "%s%s", numbuf, suffix);
    }

    if (*first) {
        if (coeff < 0) strncat(out, "-", outsz - strlen(out) - 1);
        strncat(out, piece, outsz - strlen(out) - 1);
        *first = 0;
    } else {
        strncat(out, coeff < 0 ? " - " : " + ", outsz - strlen(out) - 1);
        strncat(out, piece, outsz - strlen(out) - 1);
    }
}

/* Builds "ax^2 + bx + c = 0" (skipping zero terms, handling a==0, etc). */
static void build_equation(double a, double b, double c, char var,
                            char *out, size_t outsz)
{
    char sq[8], lin[8];
    int first = 1;

    snprintf(sq, sizeof(sq), "%c^2", var);
    snprintf(lin, sizeof(lin), "%c", var);

    out[0] = '\0';
    append_term(out, outsz, a, sq, &first);
    append_term(out, outsz, b, lin, &first);
    append_term(out, outsz, c, "", &first);

    if (first) strncat(out, "0", outsz - strlen(out) - 1);
    strncat(out, " = 0", outsz - strlen(out) - 1);
}

/* ===========================
   Step-by-step solver (quadratic.process / obj:process())
   =========================== */

#define EMIT(...)                                              \
    do {                                                       \
        snprintf(line, sizeof(line), __VA_ARGS__);             \
        printf("%s\n", line);                                  \
        luaL_addstring(&lbuf, line);                           \
        luaL_addchar(&lbuf, '\n');                              \
    } while (0)

static int do_process(lua_State *L, double a, double b, double c, char var)
{
    luaL_Buffer lbuf;
    char line[512];
    char eq[256];

    luaL_buffinit(L, &lbuf);

    build_equation(a, b, c, var, eq, sizeof(eq));

    EMIT("Step 1: Write down the equation");
    EMIT("  %s", eq);
    EMIT(" ");

    if (a == 0.0) {
        if (b == 0.0) {
            EMIT("Step 2: a = 0 and b = 0, so this is not a valid equation.");
            if (c == 0.0)
                EMIT("  Every value of %c satisfies the equation (0 = 0).", var);
            else
                EMIT("  Since c = %g and 0 != %g, there is no solution.", c, c);
        } else {
            EMIT("Step 2: a = 0, so this is actually a LINEAR equation, not a quadratic one.");
            EMIT("  %gx + (%g) = 0", b, c);
            EMIT("Step 3: Solve for %c", var);
            EMIT("  %c = -c / b = -(%g) / %g = %g", var, c, b, -c / b);
        }
        luaL_pushresult(&lbuf);
        return 1;
    }

    EMIT("Step 2: Identify the coefficients");
    EMIT("  a = %g, b = %g, c = %g", a, b, c);
    EMIT(" ");

    EMIT("Step 3: Apply the quadratic formula");
    EMIT("  %c = (-b +/- sqrt(b^2 - 4ac)) / (2a)", var);
    EMIT(" ");

    {
        double D = (b * b) - (4.0 * a * c);

        EMIT("Step 4: Compute the discriminant D = b^2 - 4ac");
        EMIT("  D = (%g)^2 - 4(%g)(%g) = %g - %g = %g",
             b, a, c, b * b, 4.0 * a * c, D);
        EMIT(" ");

        if (D > 0.0) {
            double sq = sqrt(D);
            double r1 = (-b + sq) / (2.0 * a);
            double r2 = (-b - sq) / (2.0 * a);

            EMIT("Step 5: D > 0, so there are two distinct real roots.");
            EMIT("  sqrt(D) = sqrt(%g) = %g", D, sq);
            EMIT("Step 6: Compute the roots");
            EMIT("  %c1 = (-(%g) + %g) / (2 * %g) = %g", var, b, sq, a, r1);
            EMIT("  %c2 = (-(%g) - %g) / (2 * %g) = %g", var, b, sq, a, r2);
        } else if (D == 0.0) {
            double r = -b / (2.0 * a);

            EMIT("Step 5: D = 0, so there is one repeated real root.");
            EMIT("Step 6: Compute the root");
            EMIT("  %c = -(%g) / (2 * %g) = %g", var, b, a, r);
        } else {
            double re = -b / (2.0 * a);
            double im = sqrt(-D) / (2.0 * a);

            EMIT("Step 5: D < 0, so the roots are complex conjugates.");
            EMIT("  sqrt(D) = sqrt(%g) = i * sqrt(%g) = %gi", D, -D, sqrt(-D));
            EMIT("Step 6: Compute the real and imaginary parts");
            EMIT("  real = -(%g) / (2 * %g) = %g", b, a, re);
            EMIT("  imag = sqrt(%g) / (2 * %g) = %g", -D, a, im);
            EMIT("  Roots: %g + %gi   and   %g - %gi", re, im, re, im);
        }
    }

    luaL_pushresult(&lbuf);
    return 1;
}
#undef EMIT

/* ===========================
   Lua Bindings: free functions
   =========================== */

int l_discriminant(lua_State *L)
{
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    double c = luaL_checknumber(L, 3);

    double d = (b * b) - (4.0 * a * c);

    lua_pushnumber(L, d);
    return 1;
}

int l_solve(lua_State *L)
{
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    double c = luaL_checknumber(L, 3);

    QuadraticResult result = solve_quadratic(a, b, c);
    push_result_table(L, result);
    return 1;
}

int l_process_free(lua_State *L)
{
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    double c = luaL_checknumber(L, 3);
    const char *vname = luaL_optstring(L, 4, "x");

    return do_process(L, a, b, c, vname[0]);
}

int l_tostring_free(lua_State *L)
{
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    double c = luaL_checknumber(L, 3);
    const char *vname = luaL_optstring(L, 4, "x");

    char buf[256];
    build_equation(a, b, c, vname[0], buf, sizeof(buf));
    lua_pushstring(L, buf);
    return 1;
}

/* quadratic.quadratic(a, b, c [, var]) -- constructor */
int l_new(lua_State *L)
{
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    double c = luaL_checknumber(L, 3);
    const char *vname = luaL_optstring(L, 4, "x");

    QuadObj *q = (QuadObj *) lua_newuserdata(L, sizeof(QuadObj));
    q->a = a;
    q->b = b;
    q->c = c;
    q->var = vname[0];

    luaL_getmetatable(L, QUAD_META);
    lua_setmetatable(L, -2);
    return 1;
}

/* ===========================
   Lua Bindings: object methods
   =========================== */

int m_coefficients(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    lua_pushnumber(L, q->a);
    lua_pushnumber(L, q->b);
    lua_pushnumber(L, q->c);
    return 3;
}

int m_discriminant(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    lua_pushnumber(L, (q->b * q->b) - (4.0 * q->a * q->c));
    return 1;
}

int m_solve(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    QuadraticResult result = solve_quadratic(q->a, q->b, q->c);
    push_result_table(L, result);
    return 1;
}

int m_type(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    QuadraticResult result = solve_quadratic(q->a, q->b, q->c);
    lua_pushstring(L, type_name(result.type));
    return 1;
}

int m_evaluate(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    double x = luaL_checknumber(L, 2);
    lua_pushnumber(L, (q->a * x * x) + (q->b * x) + q->c);
    return 1;
}

int m_vertex(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);

    if (q->a == 0.0) {
        lua_pushnil(L);
        lua_pushstring(L, "vertex undefined: a = 0 (not a quadratic)");
        return 2;
    }

    double h = -q->b / (2.0 * q->a);
    double k = q->c - (q->b * q->b) / (4.0 * q->a);

    lua_pushnumber(L, h);
    lua_pushnumber(L, k);
    return 2;
}

int m_derivative(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    /* d/dx (ax^2 + bx + c) = 2ax + b */
    lua_pushnumber(L, 2.0 * q->a);
    lua_pushnumber(L, q->b);
    return 2;
}

int m_setvar(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    const char *v = luaL_checkstring(L, 2);
    q->var = v[0];
    return 0;
}

int m_tostring(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    char buf[256];
    build_equation(q->a, q->b, q->c, q->var, buf, sizeof(buf));
    lua_pushstring(L, buf);
    return 1;
}

int m_process(lua_State *L)
{
    QuadObj *q = check_quad(L, 1);
    return do_process(L, q->a, q->b, q->c, q->var);
}

/* ===========================
   Module Registration
   =========================== */

static const luaL_Reg quadratic_methods[] = {
    {"coefficients", m_coefficients},
    {"discriminant", m_discriminant},
    {"solve",        m_solve},
    {"type",         m_type},
    {"evaluate",      m_evaluate},
    {"vertex",        m_vertex},
    {"derivative",    m_derivative},
    {"setvar",        m_setvar},
    {"tostring",      m_tostring},
    {"process",       m_process},
    {NULL, NULL}
};

static const luaL_Reg quadratic_lib[] = {
    {"quadratic",    l_new},          /* quadratic.quadratic(a, b, c [, var]) */
    {"new",          l_new},          /* alias */
    {"solve",        l_solve},
    {"discriminant", l_discriminant},
    {"process",      l_process_free},
    {"tostring",     l_tostring_free},
    {NULL, NULL}
};

int luaopen_quadratic(lua_State *L)
{
    /* Build the object metatable: __index points back at itself so
     * obj:method(...) finds the functions in quadratic_methods. */
    luaL_newmetatable(L, QUAD_META);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, m_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_setfuncs(L, quadratic_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, quadratic_lib);
    return 1;
}
