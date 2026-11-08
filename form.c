/* SPDX-License-Identifier: ISC */
#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#include "luanewt.h"

#include "banned.h"

static void
form_destroy(lua_State *L, struct luanewtco *c)
{
	if (c->co == NULL)
		return;

	/* mark every child dead and drop its registry entry */
	if (lua_getuservalue(L, 1) == LUA_TTABLE) {
		for (int i = 1;; i++) {
			int ty = lua_geti(L, -1, i);

			if (ty != LUA_TUSERDATA)
				break;

			struct luanewtco *ch = lua_to_anycomponent(L, -1);

			if (ch != NULL && ch->co != NULL) {
				newtComponent chco = ch->co;

				lua_component_unset(L, chco);
				ch->co = NULL;
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}

	if (!luanewt_is_finished())
		newtFormDestroy(c->co);

	lua_component_unset(L, c->co);
	c->co = NULL;
}

/***
Create a new form.
@function form
@param vertbar optional vertical scrollbar component
@treturn newtform the form userdata
*/
static int
lua_newt_form(lua_State *L)
{
	newtComponent form, bar;
	const struct luanewtco *vbar;

	if (lua_isnoneornil(L, 1))
		bar = NULL;
	else {
		vbar = luaL_checkudata(L, 1, NEWTCOMPONENT_MT);
		if (vbar->co == NULL)
			return luaL_error(L, "component destroyed");
		bar = vbar->co;
	}

	form = newtForm(bar, NULL, 0);

	(void)lua_component_set(L, form, NEWTFORM_MT);

	lua_newtable(L);
	lua_setuservalue(L, -2);

	return 1;
}

static const luaL_Reg formlib[] = {
	{"form", lua_newt_form},
	{0, 0}
};

/***
Add one or more components to the form.  A component may belong to only one
form.  The form keeps its children alive and destroys them with itself.
@function addcomponents
@param ... one or more components
@raise an error if a component belongs to another form or is destroyed
*/
static int
addcomponents(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	int i, n = lua_gettop(L), idx = 1;

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	if (n < 2)
		return luaL_argerror(L, 2, "expected at least one component");

	for (i = 2; i <= n; i++) {
		struct luanewtco *co = lua_get_anycomponent(L, i);

		if (co->co == NULL)
			return luaL_error(L, "component destroyed");
		if (!co->owned)
			return luaL_error(L, "component already belongs to a form");

		newtFormAddComponent(form->co, co->co);

		co->owned = 0;

		lua_getuservalue(L, 1);
		lua_pushvalue(L, i);
		lua_rawseti(L, -2, idx);
		lua_pop(L, 1);
		idx++;
	}

	return 0;
}

/***
Run the form's event loop until it exits.  Returns a reason string and
optional value:
  - `"hotkey"`, key
  - `"component"`, component userdata
  - `"fd"`, fd
  - `"timer"`
  - `"error"`
A callback error is propagated as a Lua error.
@function run
@treturn string reason
@treturn [any] associated value
*/
static int
run(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	struct newtExitStruct es;
	newtComponent prev;

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	prev = luanewt_get_runform();
	luanewt_set_runform(form->co);
	newtFormRun(form->co, &es);
	luanewt_set_runform(prev);

	if (luanewt_take_error(L)) {
		luaL_tolstring(L, -1, NULL);
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}

	switch (es.reason) {
	case NEWT_EXIT_HOTKEY:
		lua_pushliteral(L, "hotkey");
		lua_pushinteger(L, es.u.key);
		return 2;
	case NEWT_EXIT_COMPONENT:
		lua_pushliteral(L, "component");
		lua_component_get(L, es.u.co);
		return 2;
	case NEWT_EXIT_FDREADY:
		lua_pushliteral(L, "fd");
		lua_pushinteger(L, es.u.watch);
		return 2;
	case NEWT_EXIT_TIMER:
		lua_pushliteral(L, "timer");
		return 1;
	case NEWT_EXIT_ERROR:
		lua_pushliteral(L, "error");
		return 1;
	}

	return luaL_error(L, "unhandled exit reason %d", (int)es.reason);
}

/***
Draw the form without processing input.
@function draw
*/
static int
draw(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	newtDrawForm(form->co);

	return 0;
}

/***
Add a hotkey that exits the form with reason `"hotkey"`.
@function addhotkey
@param key a key constant
*/
static int
addhotkey(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	int key = luaL_checkint(L, 2);

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	newtFormAddHotKey(form->co, key);

	return 0;
}

/***
Watch a file descriptor for readability/writability.  Calling again for the
same fd replaces its flags.  `form:run()` returns `"fd"`, fd when ready.
Note: newt's select has no EINTR handling and a closed watched fd can loop
forever.
@function watchfd
@param fd file descriptor, non-negative
@param flags table `{read=, write=, except=}` booleans
*/
static int
watchfd(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	int fd = luaL_checkint(L, 2);
	int flags = 0;

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	luaL_argcheck(L, fd >= 0, 2, "fd must be non-negative");

	if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "read");
		if (lua_toboolean(L, -1))
			flags |= NEWT_FD_READ;
		lua_pop(L, 1);

		lua_getfield(L, 3, "write");
		if (lua_toboolean(L, -1))
			flags |= NEWT_FD_WRITE;
		lua_pop(L, 1);

		lua_getfield(L, 3, "except");
		if (lua_toboolean(L, -1))
			flags |= NEWT_FD_EXCEPT;
		lua_pop(L, 1);
	}

	newtFormWatchFd(form->co, fd, flags);

	return 0;
}

/***
Set a timer in milliseconds; `0` disables it.  `form:run()` returns
`"timer"` when it fires.
@function settimer
@param ms milliseconds, non-negative
*/
static int
settimer(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	int ms = luaL_checkint(L, 2);

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	luaL_argcheck(L, ms >= 0, 2, "ms must be non-negative");

	newtFormSetTimer(form->co, ms);

	return 0;
}

/***
Return the currently-focused component, or nil.
@function current
@treturn newtcomponent the current component or nil
*/
static int
current(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	newtComponent co;

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");

	co = newtFormGetCurrent(form->co);
	if (co == NULL) {
		lua_pushnil(L);
		return 1;
	}

	lua_component_get(L, co);

	return 1;
}

/***
Set the focused component.
@function setcurrent
@param comp component to focus
*/
static int
setcurrent(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	struct luanewtco *co = lua_get_anycomponent(L, 2);

	if (form->co == NULL)
		return luaL_error(L, "form destroyed");
	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtFormSetCurrent(form->co, co->co);

	return 0;
}

/***
Destroy the form and all of its components now.  Any later use of the form
or its children raises an error.  Normally destruction happens in `__gc`.
@function destroy
*/
static int
destroy(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);

	form_destroy(L, form);

	return 0;
}

static int
gc(lua_State *L)
{
	struct luanewtco *c = luaL_checkudata(L, 1, NEWTFORM_MT);

	form_destroy(L, c);

	return 0;
}

static const luaL_Reg newtform_meta[] = {
	{"addcomponents", addcomponents},
	{"run", run},
	{"draw", draw},
	{"addhotkey", addhotkey},
	{"watchfd", watchfd},
	{"settimer", settimer},
	{"current", current},
	{"setcurrent", setcurrent},
	{"destroy", destroy},
	{"__gc", gc},
	{0, 0},
};

void
luanewt_form_register(lua_State *L)
{
	luaL_setfuncs(L, formlib, 0);

	luaL_newmetatable(L, NEWTFORM_MT);
	luaL_setfuncs(L, newtform_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
