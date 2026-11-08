/* SPDX-License-Identifier: ISC */
#include <string.h>

#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#include "luanewt.h"

static int
gc(lua_State *L)
{
	struct luanewtco *c = lua_get_anycomponent(L, 1);

	if (c->cbref != LUA_NOREF)
		luaL_unref(L, LUA_REGISTRYINDEX, c->cbref);

	if (c->tref != 0)
		luaL_unref(L, LUA_REGISTRYINDEX, c->tref);

	if (c->co != NULL) {
		if (c->owned)
			newtComponentDestroy(c->co);
		lua_component_unset(L, c->co);
		c->co = NULL;
	}

	return 0;
}

static int
compactbutton(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtCompactButton(left, top, text),
	    NEWTCOMPONENT_MT);

	return 1;
}

static int
button(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtButton(left, top, text),
	    NEWTCOMPONENT_MT);

	return 1;
}

static int
checkbox(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);
	int on = lua_toboolean(L, 4);

	(void)lua_component_set(L,
	    newtCheckbox(left, top, text, on ? '*' : ' ', NULL, NULL),
	    NEWTCHECKBOX_MT);

	return 1;
}

static int
checkbox_get(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTCHECKBOX_MT);
	char v;

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	v = newtCheckboxGetValue(co->co);

	if (v == ' ')
		lua_pushboolean(L, 0);
	else if (v == '*')
		lua_pushboolean(L, 1);
	else {
		char s[2];

		s[0] = v;
		s[1] = 0;
		lua_pushstring(L, s);
	}

	return 1;
}

static int
checkbox_set(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTCHECKBOX_MT);
	const char *set = luaL_checkstring(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	luaL_argcheck(L, strlen(set) == 1, 2, "expected single character");

	newtCheckboxSetValue(co->co, set[0]);

	return 0;
}

static const luaL_Reg newtcheckbox_meta[] = {
	{"get", checkbox_get},
	{"set", checkbox_set},
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

static int
label(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtLabel(left, top, text), NEWTLABEL_MT);

	return 1;
}

static int
label_settext(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLABEL_MT);
	const char *set = luaL_checkstring(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtLabelSetText(co->co, set);

	return 0;
}

static const luaL_Reg newtlabel_meta[] = {
	{"settext", label_settext},
	{"__gc", gc},
	{0, 0},
};

#define checkflag(L, idx, bits, name, flag)				\
	do {								\
		if (lua_getfield(L, idx, name) != LUA_TNIL &&		\
		    lua_toboolean(L, -1))				\
			bits |= flag;					\
		lua_pop(L, 1);						\
	} while (0)

static void
listboxflags(lua_State *L, int idx, int *flags)
{
	int f = 0;

	if (lua_istable(L, idx)) {
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

	(void)lua_component_set(L,
	    newtListbox(left, top, height, flags), NEWTLISTBOX_MT);

	return 1;
}

static int
listboxappend(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	const char *key = luaL_checkstring(L, 2);
	int top, ref;

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	if (co->tref == 0) {
		lua_newtable(L);
		co->tref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);
	top = lua_gettop(L);

	lua_pushvalue(L, 3);
	ref = luaL_ref(L, top);

	newtListboxAppendEntry(co->co, key, (void *)(uintptr_t)ref);

	lua_pop(L, 1);

	return 0;
}

static int
listboxcurrent(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	void *vp;
	int ref;

	if (co->co == NULL || co->tref == 0)
		return 0;

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);

	vp = newtListboxGetCurrent(co->co);
	if (vp == NULL) {
		lua_pop(L, 1);
		return 0;
	}

	ref = (int)(uintptr_t)vp;

	lua_rawgeti(L, -1, ref);
	lua_remove(L, -2);

	return 1;
}

static int
listboxsetwidth(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	int width = luaL_checkinteger(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtListboxSetWidth(co->co, width);

	return 0;
}

/*
 * Find the registry index (packed into the newt data pointer) whose value
 * is rawequal to the value at stack index v.  Returns the index, or 0 if
 * no match.  The item table is at stack index t.
 */
static int
findbyvalue(lua_State *L, int t, int v)
{
	int i;

	for (i = 1;; i++) {
		int ty = lua_rawgeti(L, t, i);

		if (ty == LUA_TNIL) {
			lua_pop(L, 1);
			break;
		}
		if (lua_rawequal(L, -1, v)) {
			lua_pop(L, 1);
			return i;
		}
		lua_pop(L, 1);
	}

	return 0;
}

static int
listboxdelete(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	int ref;

	if (co->co == NULL || co->tref == 0)
		return 0;

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);

	ref = findbyvalue(L, -1, 2);
	if (ref != 0)
		newtListboxDeleteEntry(co->co, (void *)(uintptr_t)ref);

	lua_pop(L, 1);

	return 0;
}

static int
listboxclear(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtListboxClear(co->co);

	if (co->tref != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, co->tref);
		co->tref = 0;
	}

	return 0;
}

static int
listboxselect(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	const char *key = luaL_checkstring(L, 2);
	enum newtFlagsSense sense = NEWT_FLAGS_SET;
	const char *s;

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	s = lua_tostring(L, 3);
	if (s != NULL) {
		if (strcmp(s, "set") == 0)
			sense = NEWT_FLAGS_SET;
		else if (strcmp(s, "reset") == 0)
			sense = NEWT_FLAGS_RESET;
		else if (strcmp(s, "toggle") == 0)
			sense = NEWT_FLAGS_TOGGLE;
		else
			return luaL_error(L, "expected \"set\", \"reset\" or \"toggle\"");
	}

	newtListboxSelectItem(co->co, key, sense);

	return 0;
}

static int
listboxitemcount(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	lua_pushinteger(L, newtListboxItemCount(co->co));

	return 1;
}

static int
listboxselected(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	void **sel;
	int n, i, count = 0;

	if (co->co == NULL || co->tref == 0) {
		lua_pushnil(L);
		return 1;
	}

	sel = newtListboxGetSelection(co->co, &n);

	lua_rawgeti(L, LUA_REGISTRYINDEX, co->tref);

	lua_newtable(L);

	for (i = 0; i < n; i++) {
		int ref = (int)(uintptr_t)sel[i];

		count++;
		lua_rawgeti(L, -2, ref);
		lua_rawseti(L, -2, count);
	}

	lua_remove(L, -2);

	return 1;
}

static const luaL_Reg newtlistbox_meta[] = {
	{"append", listboxappend},
	{"current", listboxcurrent},
	{"setwidth", listboxsetwidth},
	{"delete", listboxdelete},
	{"clear", listboxclear},
	{"select", listboxselect},
	{"itemcount", listboxitemcount},
	{"selected", listboxselected},
	{"__gc", gc},
	{0, 0},
};

static const luaL_Reg componentlib[] = {
	{"compactbutton", compactbutton},
	{"button", button},
	{"checkbox", checkbox},
	{"label", label},
	{"listbox", listbox},
	{0, 0}
};

static const luaL_Reg newtcomponent_meta[] = {
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

void
luanewt_component_register(lua_State *L)
{
	luaL_setfuncs(L, componentlib, 0);

	luanewt_metatable(L, NEWTCOMPONENT_MT, newtcomponent_meta);
	luanewt_metatable(L, NEWTCHECKBOX_MT, newtcheckbox_meta);
	luanewt_metatable(L, NEWTLABEL_MT, newtlabel_meta);
	luanewt_metatable(L, NEWTLISTBOX_MT, newtlistbox_meta);
}
