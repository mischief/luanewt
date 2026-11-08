/* SPDX-License-Identifier: ISC */
/*
 * lua bindings for libnewt.
 */
#include <string.h>

#include <newt.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef LUANEWT_COMPILATION
#include "luanewt_vis.h"
#endif

#include "luanewt.h"

#include "banned.h"

#define NEWT_CO_REGISTRY "NEWTCOMPONENT"

/***
Lua bindings for libnewt, a text-mode UI toolkit.
@section newt
@module newt
*/
/***
Module-level functions.
@section functions
*/
/***
Key and file-descriptor constants.
@section constants
*/

static int g_initialized;
static newtComponent g_runform;
static int g_errref = LUA_NOREF;

int
luanewt_is_finished(void)
{
	return !g_initialized;
}

void
luanewt_set_runform(newtComponent f)
{
	g_runform = f;
}

newtComponent
luanewt_get_runform(void)
{
	return g_runform;
}

int
luanewt_take_error(lua_State *L)
{
	if (g_errref == LUA_NOREF)
		return 0;

	lua_rawgeti(L, LUA_REGISTRYINDEX, g_errref);
	luaL_unref(L, LUA_REGISTRYINDEX, g_errref);
	g_errref = LUA_NOREF;

	return 1;
}

void
lua_component_get(lua_State *L, newtComponent co)
{
	lua_getfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);
	lua_rawgetp(L, -1, co);
	lua_remove(L, -2);
}

struct luanewtco *
lua_component_set(lua_State *L, newtComponent co, const char *mt)
{
	struct luanewtco *box;
	int top;

	if (co == NULL)
		luaL_error(L, "newt: out of memory");

	box = lua_newuserdata(L, sizeof(*box));
	luaL_setmetatable(L, mt);
	memset(box, 0, sizeof(*box));
	box->co = co;
	box->cbref = LUA_NOREF;
	box->owned = 1;

	top = lua_gettop(L);

	lua_getfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);
	lua_pushlightuserdata(L, co);
	lua_pushvalue(L, top);
	lua_settable(L, -3);
	lua_pop(L, 1);

	return box;
}

void
lua_component_unset(lua_State *L, newtComponent co)
{
	lua_getfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);
	lua_pushlightuserdata(L, co);
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

struct luanewtco *
lua_to_anycomponent(lua_State *L, int index)
{
	int i;
	const char *typs[] = {
		NEWTCOMPONENT_MT,
		NEWTCHECKBOX_MT,
		NEWTLABEL_MT,
		NEWTLISTBOX_MT,
		NEWTENTRY_MT,
		NEWTTEXTBOX_MT,
		NEWTSCALE_MT,
		NEWTSCROLLBAR_MT,
		0,
	};

	for (i = 0; typs[i] != NULL; i++) {
		struct luanewtco *co = luaL_testudata(L, index, typs[i]);

		if (co != NULL)
			return co;
	}

	return NULL;
}

struct luanewtco *
lua_get_anycomponent(lua_State *L, int index)
{
	struct luanewtco *co = lua_to_anycomponent(L, index);

	if (co == NULL)
		luaL_argerror(L, index, "expected newt component");

	return co;
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

/*
 * Error stash: a callback error must not longjmp through newt.  We stash
 * the message, force the running form to return, and re-raise it after
 * newtFormRun returns.
 */
static void
newtcallback(newtComponent co, void *v)
{
	struct luanewtco *c = v, *co2;
	lua_State *L;
	int top;

	L = c->cbL;
	if (L == NULL)
		return;

	top = lua_gettop(L);

	lua_component_get(L, co);
	co2 = lua_get_anycomponent(L, -1);

	lua_rawgeti(L, LUA_REGISTRYINDEX, co2->cbref);
	lua_rotate(L, -2, 1);

	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		if (g_errref != LUA_NOREF)
			luaL_unref(L, LUA_REGISTRYINDEX, g_errref);
		g_errref = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_settop(L, top);

		if (g_runform != NULL)
			newtFormSetTimer(g_runform, 1);
	}
}

/***
Register a callback function for a component.  The callback receives the
component as its only argument.  When a callback raises an error, the
current `form:run()` propagates it after the form exits.
@param comp component
@param func callback function
@raise an error if the component is destroyed or func is not a function
*/
int
luanewt_component_callback(lua_State *L)
{
	struct luanewtco *c = lua_get_anycomponent(L, 1);
	lua_State *M;

	luaL_argcheck(L, lua_isfunction(L, 2), 2, "expected function");

	if (c->co == NULL)
		return luaL_error(L, "component destroyed");

	if (c->cbref != LUA_NOREF)
		luaL_unref(L, LUA_REGISTRYINDEX, c->cbref);

	c->cbref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushthread(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
	M = lua_tothread(L, -1);
	lua_pop(L, 2);
	c->cbL = M;

	newtComponentAddCallback(c->co, newtcallback, c);

	return 0;
}

#define CHECK_INIT()							\
	do {								\
		if (!g_initialized)					\
			return luaL_error(L, "newt: not initialized");	\
	} while (0)

/*
 * newt's API is non-const but never modifies these strings.
 */
static char *
noconst(const char *s)
{
	union {
		const char *c;
		char *m;
	} u;

	u.c = s;
	return u.m;
}

/***
Initialize the newt library.  Must be called before any other function.
@function init
@raise an error if newtInit fails or if init is called twice
*/
static int
lua_newt_init(lua_State *L)
{
	if (g_initialized)
		return luaL_error(L, "newt: already initialized");

	if (newtInit() != 0)
		return luaL_error(L, "newt: newtInit failed");

	g_initialized = 1;

	return 0;
}

/***
Shut down the newt library.  Must pair with init.  Idempotent guard: an
error is raised if finished is called before init or twice.
@function finished
@raise an error if not initialized
*/
static int
lua_newt_finished(lua_State *L)
{
	CHECK_INIT();

	(void)newtFinished();
	g_initialized = 0;

	return 0;
}

/***
Clear the screen.
@function cls
*/
static int
lua_newt_cls(lua_State *L)
{
	CHECK_INIT();
	newtCls();
	return 0;
}

/***
Redraw the screen to fit the current terminal size.
@function resizescreen
@param redraw boolean, true to redraw
*/
static int
lua_newt_resizescreen(lua_State *L)
{
	int redraw;

	CHECK_INIT();
	redraw = lua_toboolean(L, 1);
	newtResizeScreen(redraw);
	return 0;
}

/***
Wait for a keypress.
@function waitforkey
*/
static int
lua_newt_waitforkey(lua_State *L)
{
	CHECK_INIT();
	newtWaitForKey();
	return 0;
}

/***
Discard any pending queued key events.
@function clearkeybuffer
*/
static int
lua_newt_clearkeybuffer(lua_State *L)
{
	CHECK_INIT();
	newtClearKeyBuffer();
	return 0;
}

/***
Sleep for a number of microseconds.  Zero is allowed.
@function delay
@param usecs number of microseconds to sleep
@raise an error if usecs is negative
*/
static int
lua_newt_delay(lua_State *L)
{
	int delay;

	CHECK_INIT();
	delay = luaL_checkint(L, 1);
	luaL_argcheck(L, delay >= 0, 1, "delay must be non-negative");
	newtDelay((unsigned int)delay);
	return 0;
}

/***
Open a window at a position with a given size and title.
@function openwindow
@param left column of the window (not counting border)
@param top row of the window
@param width window width, must be positive
@param height window height, must be positive
@param title window title string
@raise an error if width/height are not positive or the window stack is full
*/
static int
lua_newt_openwindow(lua_State *L)
{
	int left = luaL_checkint(L, 1);
	int top = luaL_checkint(L, 2);
	int width = luaL_checkint(L, 3);
	int height = luaL_checkint(L, 4);
	const char *title = luaL_checkstring(L, 5);

	CHECK_INIT();

	luaL_argcheck(L, width > 0, 3, "width must be positive");
	luaL_argcheck(L, height > 0, 4, "height must be positive");

	if (newtOpenWindow(left, top, (unsigned)width, (unsigned)height, title) != 0)
		return luaL_error(L, "newt: newtOpenWindow failed");

	return 0;
}

/***
Open a centered window of a given size and title.
@function centeredwindow
@param width window width, must be positive
@param height window height, must be positive
@param title window title string
@raise an error if width/height are not positive or the window stack is full
*/
static int
lua_newt_centeredwindow(lua_State *L)
{
	int width = luaL_checkint(L, 1);
	int height = luaL_checkint(L, 2);
	const char *title = luaL_checkstring(L, 3);

	CHECK_INIT();

	luaL_argcheck(L, width > 0, 1, "width must be positive");
	luaL_argcheck(L, height > 0, 2, "height must be positive");

	if (newtCenteredWindow((unsigned)width, (unsigned)height, title) != 0)
		return luaL_error(L, "newt: newtCenteredWindow failed");

	return 0;
}

/***
Close the most recently opened window and redraw.
@function popwindow
*/
static int
lua_newt_popwindow(lua_State *L)
{
	CHECK_INIT();
	newtPopWindow();
	return 0;
}

/***
Close the most recently opened window without redrawing.
@function popwindownorefresh
*/
static int
lua_newt_popwindownorefresh(lua_State *L)
{
	CHECK_INIT();
	newtPopWindowNoRefresh();
	return 0;
}

/***
Set the whole color palette.  Unimplemented.
@function setcolors
@raise always raises "unimplemented"
*/
static int
lua_newt_setcolors(lua_State *L)
{
	(void)L;
	return luaL_error(L, "newt: setcolors unimplemented");
}

/***
Set the colors of a single colorset.
@function setcolor
@param colorset a NEWT_COLORSET_* constant
@param fg foreground color name
@param bg background color name
*/
static int
lua_newt_setcolor(lua_State *L)
{
	int colorset = luaL_checkint(L, 1);
	const char *fg = luaL_checkstring(L, 2);
	const char *bg = luaL_checkstring(L, 3);

	CHECK_INIT();
	newtSetColor(colorset, noconst(fg), noconst(bg));

	return 0;
}

/***
Refresh the screen, flushing any pending changes.
@function refresh
*/
static int
lua_newt_refresh(lua_State *L)
{
	CHECK_INIT();
	newtRefresh();
	return 0;
}

/***
Suspend the newt display so the caller can draw with another library.
@function suspend
*/
static int
lua_newt_suspend(lua_State *L)
{
	CHECK_INIT();
	newtSuspend();
	return 0;
}

/***
Set a callback invoked on suspend.  Unimplemented.
@function setsuspendcallback
@raise always raises "unimplemented"
*/
static int
lua_newt_setsuspendcallback(lua_State *L)
{
	(void)L;
	return luaL_error(L, "newt: setsuspendcallback unimplemented");
}

/***
Set a callback invoked on the help key.  Unimplemented.
@function sethelpcallback
@raise always raises "unimplemented"
*/
static int
lua_newt_sethelpcallback(lua_State *L)
{
	(void)L;
	return luaL_error(L, "newt: sethelpcallback unimplemented");
}

/***
Resume the newt display after a suspend.
@function resume
*/
static int
lua_newt_resume(lua_State *L)
{
	CHECK_INIT();
	(void)newtResume();
	return 0;
}

/***
Push a line onto the help-line stack.
@function pushhelpline
@param text the help line text
*/
static int
lua_newt_pushhelpline(lua_State *L)
{
	const char *text;

	CHECK_INIT();
	text = luaL_checkstring(L, 1);
	newtPushHelpLine(text);
	return 0;
}

/***
Redraw the current help line.
@function redrawhelpline
*/
static int
lua_newt_redrawhelpline(lua_State *L)
{
	CHECK_INIT();
	newtRedrawHelpLine();
	return 0;
}

/***
Pop a line off the help-line stack.
@function pophelpline
*/
static int
lua_newt_pophelpline(lua_State *L)
{
	CHECK_INIT();
	newtPopHelpLine();
	return 0;
}

/***
Draw text at absolute screen coordinates outside any window.
@function drawroottext
@param col screen column
@param row screen row
@param text the text to draw
*/
static int
lua_newt_drawroottext(lua_State *L)
{
	int col = luaL_checkint(L, 1);
	int row = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);

	CHECK_INIT();
	newtDrawRootText(col, row, text);
	return 0;
}

/***
Ring the terminal bell.
@function bell
*/
static int
lua_newt_bell(lua_State *L)
{
	CHECK_INIT();
	newtBell();
	return 0;
}

/***
Hide the terminal cursor.
@function cursoroff
*/
static int
lua_newt_cursoroff(lua_State *L)
{
	CHECK_INIT();
	newtCursorOff();
	return 0;
}

/***
Show the terminal cursor.
@function cursoron
*/
static int
lua_newt_cursoron(lua_State *L)
{
	CHECK_INIT();
	newtCursorOn();
	return 0;
}

/***
Return the current screen size.
@function getscreensize
@treturn int number of columns
@treturn int number of rows
*/
static int
lua_newt_getscreensize(lua_State *L)
{
	int cols, rows;

	CHECK_INIT();
	newtGetScreenSize(&cols, &rows);
	lua_pushinteger(L, cols);
	lua_pushinteger(L, rows);
	return 2;
}

/***
Show a modal message window and wait for the button.
@function winmessage
@param title window title
@param buttontext button label
@param text message text
*/
static int
lua_newt_winmessage(lua_State *L)
{
	const char *title = luaL_checkstring(L, 1);
	const char *buttontext = luaL_checkstring(L, 2);
	const char *text = luaL_checkstring(L, 3);

	CHECK_INIT();
	/* The text is a printf format in newt: pass it as an argument. */
	newtWinMessage(noconst(title), noconst(buttontext), noconst("%s"), text);

	return 0;
}

/***
Show a modal two-button choice window and return the chosen button.
@function winchoice
@param title window title
@param button1 first button label
@param button2 second button label
@param text message text
@treturn int 0 for F12, 1 for button1, 2 for button2
*/
static int
lua_newt_winchoice(lua_State *L)
{
	const char *title = luaL_checkstring(L, 1);
	const char *button1 = luaL_checkstring(L, 2);
	const char *button2 = luaL_checkstring(L, 3);
	const char *text = luaL_checkstring(L, 4);

	CHECK_INIT();
	lua_pushinteger(L,
	    newtWinChoice(noconst(title), noconst(button1), noconst(button2),
	        noconst("%s"), text));

	return 1;
}

static const luaL_Reg newtlib[] = {
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
	{"setsuspendcallback", lua_newt_setsuspendcallback},
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
	{"winmessage", lua_newt_winmessage},
	{"winchoice", lua_newt_winchoice},
	{0, 0}
};

static void
setintfield(lua_State *L, const char *name, int value)
{
	lua_pushinteger(L, value);
	lua_setfield(L, -2, name);
}

static void
luanewt_keys(lua_State *L)
{
	setintfield(L, "KEY_TAB", NEWT_KEY_TAB);
	setintfield(L, "KEY_ENTER", NEWT_KEY_ENTER);
	setintfield(L, "KEY_RETURN", NEWT_KEY_RETURN);
	setintfield(L, "KEY_SUSPEND", NEWT_KEY_SUSPEND);
	setintfield(L, "KEY_ESCAPE", NEWT_KEY_ESCAPE);
	setintfield(L, "KEY_UP", NEWT_KEY_UP);
	setintfield(L, "KEY_DOWN", NEWT_KEY_DOWN);
	setintfield(L, "KEY_LEFT", NEWT_KEY_LEFT);
	setintfield(L, "KEY_RIGHT", NEWT_KEY_RIGHT);
	setintfield(L, "KEY_BKSPC", NEWT_KEY_BKSPC);
	setintfield(L, "KEY_DELETE", NEWT_KEY_DELETE);
	setintfield(L, "KEY_HOME", NEWT_KEY_HOME);
	setintfield(L, "KEY_END", NEWT_KEY_END);
	setintfield(L, "KEY_UNTAB", NEWT_KEY_UNTAB);
	setintfield(L, "KEY_PGUP", NEWT_KEY_PGUP);
	setintfield(L, "KEY_PGDN", NEWT_KEY_PGDN);
	setintfield(L, "KEY_INSERT", NEWT_KEY_INSERT);
	setintfield(L, "KEY_F1", NEWT_KEY_F1);
	setintfield(L, "KEY_F2", NEWT_KEY_F2);
	setintfield(L, "KEY_F3", NEWT_KEY_F3);
	setintfield(L, "KEY_F4", NEWT_KEY_F4);
	setintfield(L, "KEY_F5", NEWT_KEY_F5);
	setintfield(L, "KEY_F6", NEWT_KEY_F6);
	setintfield(L, "KEY_F7", NEWT_KEY_F7);
	setintfield(L, "KEY_F8", NEWT_KEY_F8);
	setintfield(L, "KEY_F9", NEWT_KEY_F9);
	setintfield(L, "KEY_F10", NEWT_KEY_F10);
	setintfield(L, "KEY_F11", NEWT_KEY_F11);
	setintfield(L, "KEY_F12", NEWT_KEY_F12);
	setintfield(L, "KEY_RESIZE", NEWT_KEY_RESIZE);
	setintfield(L, "KEY_ERROR", NEWT_KEY_ERROR);
	setintfield(L, "FD_READ", NEWT_FD_READ);
	setintfield(L, "FD_WRITE", NEWT_FD_WRITE);
	setintfield(L, "FD_EXCEPT", NEWT_FD_EXCEPT);
}

LUANEWT_API int
luaopen_newt(lua_State *L)
{
	/* weak-valued registry: newtComponent * -> userdata */
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, NEWT_CO_REGISTRY);

	luaL_newlib(L, newtlib);

	luanewt_component_register(L);
	luanewt_form_register(L);

	luanewt_keys(L);

	return 1;
}
