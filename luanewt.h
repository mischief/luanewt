#ifndef luanewt
#define luanewt

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
	int cbref;
	int tref;
};

void lua_component_get(lua_State*, newtComponent);
struct luanewtco* lua_component_set(lua_State*, newtComponent, const char*);

struct luanewtco* lua_get_anycomponent(lua_State*, int);

void luanewt_component_register(lua_State *L);
void luanewt_form_register(lua_State *L);

void luanewt_metatable(lua_State*, const char*, const luaL_Reg*);

#endif

