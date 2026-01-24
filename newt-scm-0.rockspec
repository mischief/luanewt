package = "newt"
version = "scm-0"
source = {
   url = "git+https://github.com/mischief/luanewt.git"
}
description = {
   homepage = "https://github.com/mischief/luanewt",
   license = "ISC",
}
dependencies = {
   "lua >= 5.4, < 5.5",
}

build = {
--[[
   type = "command",

   build_command = "CFLAGS='$(CFLAGS)' meson setup --reconfigure --wipe -Dlua=lua$(LUA_VERSION) -Dlua-module-directory=$(LIBDIR) build",
   install_command = "ninja -C build -v install",
--]]
---[[
--]]
  type = "builtin",
  modules = {
  	newt = {
		sources = {"newt.c", "component.c", "form.c"},
		libraries = "newt",
	},
  },
}

