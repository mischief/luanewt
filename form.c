#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#include "luanewt.h"

static int
lua_newt_form(lua_State *L)
{
	newtComponent form, bar;
	struct luanewtco *vbar;

	if(lua_isnoneornil(L, 1))
		bar = NULL;
	else {
		vbar = luaL_checkudata(L, 1, NEWTCOMPONENT_MT);
		bar = vbar->co;
	}

	form = newtForm(bar, NULL, 0);

	(void) lua_component_set(L, form, NEWTFORM_MT);

	return 1;
}

static luaL_Reg const formlib[] = {
	{"form", lua_newt_form},
	{0, 0}
};

static int
addcomponents(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	struct luanewtco *co;
	int i;

	for(i = 2; i <= lua_gettop(L); i++){
		co = lua_get_anycomponent(L, i);
		newtFormAddComponent(form->co, co->co);
	}

	lua_settop(L, 1);

	return 1;
}

static int
run(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	struct newtExitStruct es;

	newtFormRun(form->co, &es);

	switch(es.reason){
	case NEWT_EXIT_HOTKEY:
		lua_pushstring(L, "hotkey");
		lua_pushinteger(L, es.u.key);
		return 2;

	case NEWT_EXIT_COMPONENT:
		lua_pushstring(L, "component");
		lua_component_get(L, es.u.co);
		return 2;

	case NEWT_EXIT_FDREADY:
		lua_pushstring(L, "fd");
		lua_pushinteger(L, es.u.watch);
		return 2;

	case NEWT_EXIT_TIMER:
		lua_pushstring(L, "timer");
		return 1;

	case NEWT_EXIT_ERROR:
		lua_pushstring(L, "error");
		return 1;
	}

	luaL_error(L, "unhandled exit reason %d", es.reason);
	return  0;
}

static int
draw(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);

	newtDrawForm(form->co);

	return 1;
}

static int
addhotkey(lua_State *L)
{
	struct luanewtco *form = luaL_checkudata(L, 1, NEWTFORM_MT);
	int key = luaL_checkinteger(L, 2);

	newtFormAddHotKey(form->co, key);

	return 0;
}

static int
gc(lua_State *L)
{
	struct luanewtco *c = luaL_checkudata(L, 1, NEWTCOMPONENT_MT);

	newtFormDestroy(c->co);

	return 0;
}

static const luaL_Reg newtform_meta[] = {
	{"addcomponents",addcomponents},
	{"run",	run},
	{"draw", draw},
	{"addhotkey", addhotkey},
	{"__gc", gc},
	{0, 0},
};

void luanewt_form_register(lua_State *L)
{
	luaL_setfuncs(L, formlib, 0);

	luaL_newmetatable(L, NEWTFORM_MT);
	luaL_setfuncs(L, newtform_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}

