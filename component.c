/* SPDX-License-Identifier: ISC */
#include <string.h>

#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#include "luanewt.h"

#include "banned.h"

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

/***
Create a compact button.
@function compactbutton
@param left column
@param top row
@param text button label
@treturn newtcomponent the button
*/
static int
compactbutton(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtCompactButton(left, top, text),
	    NEWTCOMPONENT_MT);

	return 1;
}

/***
Create a button.
@function button
@param left column
@param top row
@param text button label
@treturn newtcomponent the button
*/
static int
button(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtButton(left, top, text),
	    NEWTCOMPONENT_MT);

	return 1;
}

/***
Create a checkbox.
@function checkbox
@param left column
@param top row
@param text label
@param on boolean initial state
@treturn newtcheckbox the checkbox
*/
static int
checkbox(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);
	int on = lua_toboolean(L, 4);

	(void)lua_component_set(L,
	    newtCheckbox(left, top, text, on ? '*' : ' ', NULL, NULL),
	    NEWTCHECKBOX_MT);

	return 1;
}

/***
Get the checkbox state.  For the default two-state checkbox this returns a
boolean; custom sequences return their single-character value.
@function get
@treturn boolean|string the state
*/
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

/***
Set the checkbox state with a single character.
@function set
@param c single character state
*/
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

/***
Create a label.
@function label
@param left column
@param top row
@param text label text
@treturn newtlabel the label
*/
static int
label(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);

	(void)lua_component_set(L, newtLabel(left, top, text), NEWTLABEL_MT);

	return 1;
}

/***
Set the label text.
@function settext
@param text new text
*/
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

/***
Create a listbox.
@function listbox
@param left column
@param top row
@param height visible rows
@param flags table `{returnexit=, border=, multiple=, showcursor=}`
@treturn newtlistbox the listbox
*/
static int
listbox(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	int height = luaL_checkint(L, 3);
	int flags = 0;

	listboxflags(L, 4, &flags);

	(void)lua_component_set(L,
	    newtListbox(left, top, height, flags), NEWTLISTBOX_MT);

	return 1;
}

/***
Append an entry to the listbox.
@function append
@param text entry text
@param value arbitrary value associated with the entry
*/
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

/***
Return the value of the current listbox entry, or nil.
@function current
@treturn any the current value or nil
*/
static int
listboxcurrent(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	const void *vp;
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

/***
Set the listbox width.
@function setwidth
@param width width in columns
*/
static int
listboxsetwidth(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	int width = luaL_checkint(L, 2);

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

/***
Delete the first entry whose associated value equals the argument.
@function delete
@param value the value to match
*/
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

/***
Select an entry by its text key.  The optional third argument is
`"set"`, `"reset"` or `"toggle"` (default `"set"`).
@function select
@param key entry text
@param sense optional selection sense
*/
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

/***
Make an entry current, by its 1-based position.
@function setcurrent
@int n position of the entry
*/
static int
listboxsetcurrent(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);
	int n = luaL_checkint(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");
	luaL_argcheck(L, n >= 1 && n <= newtListboxItemCount(co->co), 2,
	    "position out of range");

	newtListboxSetCurrent(co->co, n - 1);

	return 0;
}

/***
Return the number of entries in the listbox.
@function itemcount
@treturn int number of entries
*/
static int
listboxitemcount(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTLISTBOX_MT);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	lua_pushinteger(L, newtListboxItemCount(co->co));

	return 1;
}

/***
Return a table of the currently selected entry values, or nil if there is
no selection.
@function selected
@treturn table|nil selected values
*/
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
	if (sel == NULL) {
		lua_pushnil(L);
		return 1;
	}

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
	{"setcurrent", listboxsetcurrent},
	{"selected", listboxselected},
	{"__gc", gc},
	{0, 0},
};

/*
 * Generic flag-table reader for components that take a flags table.
 */
static void
compflags(lua_State *L, int idx, int *flags)
{
	int f = 0;

	if (lua_istable(L, idx)) {
		checkflag(L, idx, f, "returnexit", NEWT_FLAG_RETURNEXIT);
		checkflag(L, idx, f, "border", NEWT_FLAG_BORDER);
		checkflag(L, idx, f, "scroll", NEWT_FLAG_SCROLL);
		checkflag(L, idx, f, "hidden", NEWT_FLAG_HIDDEN);
		checkflag(L, idx, f, "disabled", NEWT_FLAG_DISABLED);
		checkflag(L, idx, f, "password", NEWT_FLAG_PASSWORD);
		checkflag(L, idx, f, "wrap", NEWT_FLAG_WRAP);
	}

	*flags = f;
}

/***
Create an entry (single-line input) component.
@function entry
@param left column
@param top row
@param initial initial text (optional)
@param width width in columns
@param flags table `{scroll=, hidden=, disabled=, password=, returnexit=}`
@treturn newtentry the entry
*/
static int
entry(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	const char *initial = luaL_optstring(L, 3, NULL);
	int width = luaL_checkint(L, 4);
	int flags = 0;

	compflags(L, 5, &flags);

	(void)lua_component_set(L,
	    newtEntry(left, top, initial, width, NULL, flags), NEWTENTRY_MT);

	return 1;
}

/***
Get the entry's current text.
@function get
@treturn string the text, or nil
*/
static int
entry_get(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTENTRY_MT);
	char *v;

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	v = newtEntryGetValue(co->co);
	if (v != NULL)
		lua_pushstring(L, v);
	else
		lua_pushnil(L);

	return 1;
}

/***
Set the entry's text.
@function set
@param text new text
@param cursoratend boolean, place cursor at end
*/
static int
entry_set(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTENTRY_MT);
	const char *v = luaL_checkstring(L, 2);
	int cursor = lua_toboolean(L, 3);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtEntrySet(co->co, v, cursor);

	return 0;
}

static const luaL_Reg newtentry_meta[] = {
	{"get", entry_get},
	{"set", entry_set},
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

/***
Create a textbox (multi-line read-only display) component.
@function textbox
@param left column
@param top row
@param width width in columns
@param height height in rows
@param flags table `{wrap=, scroll=}`
@treturn newttextbox the textbox
*/
static int
textbox(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	int width = luaL_checkint(L, 3);
	int height = luaL_checkint(L, 4);
	int flags = 0;

	compflags(L, 5, &flags);

	(void)lua_component_set(L,
	    newtTextbox(left, top, width, height, flags), NEWTTEXTBOX_MT);

	return 1;
}

/***
Set the textbox content.
@function settext
@param text the text
*/
static int
textbox_settext(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTTEXTBOX_MT);
	const char *text = luaL_checkstring(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtTextboxSetText(co->co, text);

	return 0;
}

/***
Set the textbox height.
@function setheight
@param height height in rows
*/
static int
textbox_setheight(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTTEXTBOX_MT);
	int height = luaL_checkint(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtTextboxSetHeight(co->co, height);

	return 0;
}

/***
Return the number of lines in the textbox.
@function getnumlines
@treturn int number of lines
*/
static int
textbox_getnumlines(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTTEXTBOX_MT);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	lua_pushinteger(L, newtTextboxGetNumLines(co->co));

	return 1;
}

static const luaL_Reg newttextbox_meta[] = {
	{"settext", textbox_settext},
	{"setheight", textbox_setheight},
	{"getnumlines", textbox_getnumlines},
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

/***
Create a scale (progress bar) component.
@function scale
@param left column
@param top row
@param width width in columns
@param fullvalue maximum value
@treturn newtscale the scale
*/
static int
scale(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	int width = luaL_checkint(L, 3);
	long long full = luaL_checkinteger(L, 4);

	(void)lua_component_set(L,
	    newtScale(left, top, width, full), NEWTSCALE_MT);

	return 1;
}

/***
Set the scale's current value.
@function set
@param amount current value
*/
static int
scale_set(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTSCALE_MT);
	unsigned long long amount =
	    (unsigned long long)luaL_checkinteger(L, 2);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtScaleSet(co->co, amount);

	return 0;
}

static const luaL_Reg newtscale_meta[] = {
	{"set", scale_set},
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

/***
Create a vertical scrollbar component.
@function verticalscrollbar
@param left column
@param top row
@param height height in rows
@param normal normal colorset
@param thumb thumb colorset
@treturn newtscrollbar the scrollbar
*/
static int
scrollbar(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	int height = luaL_checkint(L, 3);
	int normal = luaL_checkint(L, 4);
	int thumb = luaL_checkint(L, 5);

	(void)lua_component_set(L,
	    newtVerticalScrollbar(left, top, height, normal, thumb),
	    NEWTSCROLLBAR_MT);

	return 1;
}

/***
Set the scrollbar position.
@function set
@param where current position
@param total total range
*/
static int
scrollbar_set(lua_State *L)
{
	struct luanewtco *co = luaL_checkudata(L, 1, NEWTSCROLLBAR_MT);
	int where = luaL_checkint(L, 2);
	int total = luaL_checkint(L, 3);

	if (co->co == NULL)
		return luaL_error(L, "component destroyed");

	newtScrollbarSet(co->co, where, total);

	return 0;
}

static const luaL_Reg newtscrollbar_meta[] = {
	{"set", scrollbar_set},
	{"callback", luanewt_component_callback},
	{"__gc", gc},
	{0, 0},
};

static const luaL_Reg componentlib[] = {
	{"compactbutton", compactbutton},
	{"button", button},
	{"checkbox", checkbox},
	{"label", label},
	{"listbox", listbox},
	{"entry", entry},
	{"textbox", textbox},
	{"scale", scale},
	{"verticalscrollbar", scrollbar},
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
	luanewt_metatable(L, NEWTENTRY_MT, newtentry_meta);
	luanewt_metatable(L, NEWTTEXTBOX_MT, newttextbox_meta);
	luanewt_metatable(L, NEWTSCALE_MT, newtscale_meta);
	luanewt_metatable(L, NEWTSCROLLBAR_MT, newtscrollbar_meta);
}
