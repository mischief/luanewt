#include <string.h>

#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#include "luanewt.h"

static int
gc(lua_State *L)
{
	struct luanewtco *c = lua_get_anycomponent(L, 1);

	if(c->cbref > 0)
		luaL_unref(L, LUA_REGISTRYINDEX, c->cbref);

	if(c->tref > 0)
		luaL_unref(L, LUA_REGISTRYINDEX, c->tref);

	return 0;
}

static void
newtcallback(newtComponent co, void *v)
{
	lua_State *L = v;
	struct luanewtco *c;
	const char *err;

	lua_component_get(L, co);

	c = lua_get_anycomponent(L, lua_gettop(L));

	lua_rawgeti(L, LUA_REGISTRYINDEX, c->cbref);
	lua_rotate(L, -2, 1);

	if(lua_pcall(L, 1, 0, 0) != LUA_OK){
		err = lua_tostring(L, -1);
		lua_pop(L, 1);

		newtPushHelpLine(err);
		newtRedrawHelpLine();
		newtBell();
		newtRefresh();
		newtDelay(2000000);
		newtPopHelpLine();
		newtRefresh();
	}
}

static int
callback(lua_State *L)
{
	struct luanewtco *c = lua_get_anycomponent(L, 1);

	luaL_argcheck(L, lua_isfunction(L, 2), 2, "expected function");

	c->cbref = luaL_ref(L, LUA_REGISTRYINDEX);

	newtComponentAddCallback(c->co, newtcallback, L);

	return 0;
}

static int
compactbutton(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	newtComponent c = newtCompactButton(left, top, text);

	(void) lua_component_set(L, c, NEWTCOMPONENT_MT);

	return 1;
}

static int
button(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	newtComponent c = newtButton(left, top, text);

	(void) lua_component_set(L, c, NEWTCOMPONENT_MT);

	return 1;
}

static int
checkbox(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);
	int on = lua_toboolean(L, 4);

	newtComponent c = newtCheckbox(left, top, text, on ? '*' : ' ', NULL, NULL);

	(void) lua_component_set(L, c, NEWTCHECKBOX_MT);

	return 1;
}

static int
checkbox_get(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTCHECKBOX_MT);
	char s[2];

	s[0] = newtCheckboxGetValue(co->co);
	s[1] = 0;

	lua_pushstring(L, s);

	return 1;
}

static int
checkbox_set(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTCHECKBOX_MT);
	const char *set = luaL_checkstring(L, 2);

	luaL_argcheck(L, strlen(set) == 1, 2, "expected single character");

	newtCheckboxSetValue(co->co, set[0]);

	return 0;
}

static const luaL_Reg newtcheckbox_meta[] = {
	{"get", checkbox_get},
	{"set", checkbox_set},
	{"callback", callback},
	{"__gc", gc},
	{0, 0},
};

static int
label(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	newtComponent c = newtLabel(left, top, text);

	(void) lua_component_set(L, c, NEWTLABEL_MT);

	return 1;
}

static int
label_settext(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLABEL_MT);
	const char *set = luaL_checkstring(L, 2);

	newtLabelSetText(co->co, set);

	return 0;
}

static const luaL_Reg newtlabel_meta[] = {
	{"settext", label_settext},
	{"__gc", gc},
	{0, 0},
};

#define checkflag(L, idx, bits, name, flag) { \
	if(lua_getfield(L, idx, name) != LUA_TNIL && \
		lua_toboolean(L, -1)) \
		bits |= flag; \
	lua_pop(L, 1); \
	}

static void
listboxflags(lua_State *L, int idx, int *flags)
{
	int f = 0;

	if(lua_istable(L, idx)){
		checkflag(L, idx, f, "returnexit", NEWT_FLAG_RETURNEXIT);
		checkflag(L, idx, f, "border", NEWT_FLAG_BORDER);
		checkflag(L, idx, f, "multiple", NEWT_FLAG_MULTIPLE);
		checkflag(L, idx, f, "showcursor", NEWT_FLAG_SHOWCURSOR);
	}

	*flags = f;
}

static int
listbox(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	int height = luaL_checkinteger(L, 3);
	int flags = 0;

	listboxflags(L, 4, &flags);

	newtComponent c = newtListbox(left, top, height, flags);

	(void) lua_component_set(L, c, NEWTLISTBOX_MT);

	return 1;
}

static int
listboxappend(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	const char *key = luaL_checkstring(L, 2);
	int top, ref;

	if(co->tref == 0){
		lua_newtable(L);
		lua_pushstring(L, "v");
		lua_setfield(L, -2, "__mode");

		co->tref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);
	top = lua_gettop(L);

	lua_pushvalue(L, 3);
	ref = luaL_ref(L, top);

	newtListboxAppendEntry(co->co, key, (void*)(uintptr_t) ref);

	return 0;
}

static int
listboxcurrent(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	void *vp;
	int ref;

	if(!co->tref)
		return 0;

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);

	vp = newtListboxGetCurrent(co->co);
	if(!vp)
		return 0;

	ref = (int)(uintptr_t)vp;

	lua_rawgeti(L, -1, ref);

	return 1;
}

static int
listboxsetwidth(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	int width = luaL_checkinteger(L, 2);

	newtListboxSetWidth(co->co, width);
	return 0;
}

static const luaL_Reg newtlistbox_meta[] = {
	{"append", listboxappend},
	{"current", listboxcurrent},
	{"setwidth", listboxsetwidth},
	{"__gc", gc},
	{0, 0},
};

static luaL_Reg const componentlib[] = {
	{"compactbutton", compactbutton},
	{"button", button},
	{"checkbox", checkbox},
	{"label", label},
	{"listbox", listbox},
	{0, 0}
};

static const luaL_Reg newtcomponent_meta[] = {
	{"callback", callback},
	{"__gc", gc},
	{0, 0},
};

void luanewt_component_register(lua_State *L)
{
	luaL_setfuncs(L, componentlib, 0);

	luanewt_metatable(L, NEWTCOMPONENT_MT, newtcomponent_meta);
	luanewt_metatable(L, NEWTCHECKBOX_MT, newtcheckbox_meta);
	luanewt_metatable(L, NEWTLABEL_MT, newtlabel_meta);
	luanewt_metatable(L, NEWTLISTBOX_MT, newtlistbox_meta);
}

