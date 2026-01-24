/// lua libnewt wrapper
// @module newt
// @author Nick Owens <mischief@offblast.org>
// @license GPL-2.0-or-later

// SPDX-License-Identifier: GPL-2.0-or-later

#include <string.h>

#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef LUANEWT_COMPILATION
#include "luanewt_vis.h"
#endif

#include "luanewt.h"

#define NEWT_CO_REGISTRY "NEWTCOMPONENT"

void
lua_component_get(lua_State *L, newtComponent co)
{
	lua_getfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);
	lua_rawgetp(L, -1, co);
	lua_remove(L, -2);
}

struct luanewtco*
lua_component_set(lua_State *L, newtComponent co, const char *mt)
{
	int top;
	struct luanewtco *box;

	box = lua_newuserdata(L, sizeof(*box));
	luaL_setmetatable(L, mt);
	memset(box, 0, sizeof(*box));
	box->co = co;

	top = lua_gettop(L);

	lua_getfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);
	lua_pushlightuserdata(L, co);
	lua_pushvalue(L, top);
	lua_settable(L, -3);
	lua_pop(L, 1);

	return box;
}

struct luanewtco*
lua_get_anycomponent(lua_State *L, int index)
{
	struct luanewtco *co;
	int i;

	const char *t, *typs[] = {
		NEWTCOMPONENT_MT,
		NEWTCHECKBOX_MT,
		NEWTLABEL_MT,
		NEWTLISTBOX_MT,
		0,
	};

	for(i = 0; ; i++){
		t = typs[i];
		if(!t) break;
		co = luaL_testudata(L, index, t);
		if(co) return co;
	}

	luaL_argerror(L, index, "expected newt component");
	return NULL;
}

void
luanewt_metatable(lua_State *L, const char *mt, const luaL_Reg *reg)
{
	luaL_newmetatable(L, mt);
	luaL_setfuncs(L, reg, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}

/***
newt
@section newt
*/

/***
Functions
@section functions
*/

/***
Create a new imsgbuf
@function new
@int fd file descriptor
@treturn imsgbuf a new imsgbuf
*/
static int
lua_newt_init(lua_State *L)
{
	if(newtInit() != 0)
		luaL_error(L, "newtInit failed");

	return 0;
}

static int
lua_newt_finished(lua_State *L)
{
	(void) newtFinished();

	return 0;
}

static int
lua_newt_cls(lua_State *L)
{
	newtCls();
	return 0;
}

static int
lua_newt_resizescreen(lua_State *L)
{
	int redraw = luaL_checkinteger(L, 1);

	newtResizeScreen(redraw);

	return 0;
}

static int
lua_newt_waitforkey(lua_State *L)
{
	newtWaitForKey();
	return 0;
}

static int
lua_newt_clearkeybuffer(lua_State *L)
{
	newtClearKeyBuffer();
	return 0;
}

static int
lua_newt_delay(lua_State *L)
{
	int delay = luaL_checkinteger(L, 1);

	luaL_argcheck(L, delay > 0, 1, "delay must be positive");

	newtDelay((unsigned int)delay);

	return 0;
}

static int
lua_newt_openwindow(lua_State *L)
{
	int left = luaL_checkinteger(L, 1);
	int top = luaL_checkinteger(L, 2);
	int width = luaL_checkinteger(L, 3);
	int height = luaL_checkinteger(L, 4);
	const char *title = luaL_checkstring(L, 5);

//	luaL_argcheck(L, width > 0, 3, "width must be positive");
//	luaL_argcheck(L, height > 0, 4, "height must be positive");

	newtOpenWindow(left, top, width, height, title);

	return 0;
}

static int
lua_newt_centeredwindow(lua_State *L)
{
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	const char *title = luaL_checkstring(L, 3);

	luaL_argcheck(L, width > 0, 1, "width must be positive");
	luaL_argcheck(L, height > 0, 2, "height must be positive");

	newtCenteredWindow(width, height, title);

	return 0;
}

static int
lua_newt_popwindow(lua_State *L)
{
	newtPopWindow();
	return 0;
}

static int
lua_newt_popwindownorefresh(lua_State *L)
{
	newtPopWindowNoRefresh();
	return 0;
}

static int
lua_newt_setcolors(lua_State *L)
{
	luaL_error(L, "unimplemented");
	return 0;
}

static int
lua_newt_setcolor(lua_State *L)
{
	int colorset = luaL_checkinteger(L, 1);
	const char *fg = luaL_checkstring(L, 2);
	const char *bg = luaL_checkstring(L, 3);

	newtSetColor(colorset, (char*)fg, (char*)bg);

	return 0;
}

static int
lua_newt_refresh(lua_State *L)
{
	newtRefresh();
	return 0;
}

static int
lua_newt_suspend(lua_State *L)
{
	newtSuspend();
	return 0;
}

static int
lua_newt_setsuspendcallback(lua_State *L)
{
	luaL_error(L, "unimplemented");
	return 0;
}

static int
lua_newt_sethelpcallback(lua_State *L)
{
	luaL_error(L, "unimplemented");
	return 0;
}

static int
lua_newt_resume(lua_State *L)
{
	newtResume();
	return 0;
}

static int
lua_newt_pushhelpline(lua_State *L)
{
	const char *text = luaL_checkstring(L, 1);

	newtPushHelpLine(text);

	return 0;
}

static int
lua_newt_redrawhelpline(lua_State *L)
{
	newtRedrawHelpLine();
	return 0;
}

static int
lua_newt_pophelpline(lua_State *L)
{
	newtPopHelpLine();
	return 0;
}

static int
lua_newt_drawroottext(lua_State *L)
{
	int col = luaL_checkinteger(L, 1);
	int row = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);

	newtDrawRootText(col, row, text);

	return 0;
}

static int
lua_newt_bell(lua_State *L)
{
	newtBell();
	return 0;
}

static int
lua_newt_cursoroff(lua_State *L)
{
	newtCursorOff();
	return 0;
}

static int
lua_newt_cursoron(lua_State *L)
{
	newtCursorOn();
	return 0;
}

static int
lua_newt_getscreensize(lua_State *L)
{
	int cols, rows;

	newtGetScreenSize(&cols, &rows);

	lua_pushinteger(L, cols);
	lua_pushinteger(L, rows);
	return 2;
}

static luaL_Reg const newtlib[] = {
	{"init", lua_newt_init},
	{"finished", lua_newt_finished},
	{"cls", lua_newt_cls},
	{"resizescreen", lua_newt_resizescreen},
	{"waitforkey", lua_newt_waitforkey},
	{"clearkeybuffer", lua_newt_clearkeybuffer},
	{"delay", lua_newt_delay},
	{"openwindow", lua_newt_openwindow},
	{"centeredwindow", lua_newt_centeredwindow},
	{"popwindow", lua_newt_popwindow},
	{"popwindownorefresh", lua_newt_popwindownorefresh},
	{"setcolors", lua_newt_setcolors},
	{"setcolor", lua_newt_setcolor},
	{"refresh", lua_newt_refresh},
	{"suspend", lua_newt_suspend},
	{"setsupendcallback", lua_newt_setsuspendcallback},
	{"sethelpcallback", lua_newt_sethelpcallback},
	{"resume", lua_newt_resume},
	{"pushhelpline", lua_newt_pushhelpline},
	{"redrawhelpline", lua_newt_redrawhelpline},
	{"pophelpline", lua_newt_pophelpline},
	{"drawroottext", lua_newt_drawroottext},
	{"bell", lua_newt_bell},
	{"cursoroff", lua_newt_cursoroff},
	{"cursoron", lua_newt_cursoron},
	{"getscreensize", lua_newt_getscreensize},
	{0, 0}
};

/***
Constants
@section constants
*/

/***
@table constants
@int IMSG_HEADER_SIZE Size of the imsg message header.
@int MAX_IMSGSIZE Default maximum imsg size.
*/

LUANEWT_API int
luaopen_newt(lua_State* L)
{

	lua_pushstring(L, NEWT_CO_REGISTRY);
	lua_newtable(L);

	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");

	lua_settable(L, LUA_REGISTRYINDEX);

	luaL_newlib(L, newtlib);

	luanewt_component_register(L);
	luanewt_form_register(L);

	lua_pushinteger(L, NEWT_KEY_ESCAPE);
	lua_setfield(L, -2, "KEY_ESCAPE");

	lua_pushinteger(L, NEWT_KEY_DELETE);
	lua_setfield(L, -2, "KEY_DELETE");
/*
	lua_pushinteger(L, IMSG_HEADER_SIZE);
	lua_setfield(L, -2, "IMSG_HEADER_SIZE");

	lua_pushinteger(L, MAX_IMSGSIZE);
	lua_setfield(L, -2, "MAX_IMSGSIZE");

	luaL_newmetatable(L, IMSG_MT);
	luaL_setfuncs(L, imsg_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
*/

	return 1;
}
