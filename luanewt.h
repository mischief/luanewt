/* SPDX-License-Identifier: ISC */
#ifndef LUANEWT_H
#define LUANEWT_H

#include <lua.h>
#include <lauxlib.h>

#include <newt.h>

#ifndef LUANEWT_API
#define LUANEWT_API
#endif

#define NEWTCOMPONENT_MT "NEWTCOMPONENT_MT"
#define NEWTCHECKBOX_MT "NEWTCHECKBOX_MT"
#define NEWTLABEL_MT "NEWTLABEL_MT"
#define NEWTLISTBOX_MT "NEWTLISTBOX_MT"
#define NEWTFORM_MT "NEWTFORM_MT"

struct luanewtco {
	newtComponent co;
	lua_State *cbL;
	int cbref;
	int tref;
	int owned;
};

void lua_component_get(lua_State *, newtComponent);
struct luanewtco *lua_component_set(lua_State *, newtComponent, const char *);
void lua_component_unset(lua_State *, newtComponent);
struct luanewtco *lua_get_anycomponent(lua_State *, int);
struct luanewtco *lua_to_anycomponent(lua_State *, int);
void luanewt_component_register(lua_State *);
void luanewt_form_register(lua_State *);
void luanewt_metatable(lua_State *, const char *, const luaL_Reg *);

int luanewt_component_callback(lua_State *);
void luanewt_set_runform(newtComponent);
newtComponent luanewt_get_runform(void);
int luanewt_take_error(lua_State *);
int luanewt_is_finished(void);

#endif
