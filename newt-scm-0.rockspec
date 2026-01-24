package = "newt"
version = "scm-0"
source = {
   url = "git+https://github.com/mischief/luanewt.git"
}
description = {
   homepage = "https://github.com/mischief/luanewt",
   license = "GPL-2.0-or-later",
}
dependencies = {
   "lua >= 5.4, < 5.5",
}

build = {
  type = "builtin",
  modules = {
  	newt = {
		sources = {"newt.c", "component.c", "form.c"},
		libraries = "newt",
	},
  },
}

