# luanewt

incomplete, undocumented lua bindings for libnewt (lua 5.4 and luajit).

## Installation

Clone the repo and run

    meson setup build && ninja -C build install

Use `-Dlua=luajit` for luajit.

or

    luarocks install --local https://github.com/mischief/luanewt/raw/refs/heads/main/newt-scm-0.rockspec

## Example

    local newt = require('newt')
    newt.init()
    newt.openwindow(2, 2, 30, 8, "hello")
    local b = newt.button(10, 3, "Ok")
    local f = newt.form()
    f:addcomponents(b)
    f:run()
    newt.popwindow()
    newt.finished()

See `examples/` for more. `examples/launcher.lua` needs `inifile` and `posix`.

# License

Copyright Nick Owens <mischief@offblast.org>

SPDX-License-Identifier: ISC
