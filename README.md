# luanewt

Lua 5.4 bindings for [libnewt](https://pagure.io/newt), the text-mode UI
toolkit used by the Anaconda installer.

## Features

- Window and form management: `openwindow`, `centeredwindow`, `popwindow`,
  `form`, `form:run`, `form:draw`, `form:addhotkey`.
- Components: `button`, `compactbutton`, `checkbox`, `label`, `listbox`.
- Component methods: `callback`, `checkbox:get()/set()`,
  `label:settext()`, listbox `append/current/setwidth/delete/clear/select/
  itemcount/selected`.
- Form extras: `form:watchfd()`, `form:settimer()`, `form:current()`,
  `form:setcurrent()`, `form:destroy()`.
- All `NEWT_KEY_*` and `NEWT_FD_*` constants are exported.
- Callback errors are propagated out of `form:run()`.

## Installation

    meson setup build && ninja -C build install

The module installs to `$prefix/lib/lua/5.4`.  Override with
`-Dlua-module-directory=/custom/path`.

Or with luarocks:

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

See `examples/` for more.  `examples/launcher.lua` needs `inifile` and
`posix`.

## Notes

- The module is **not thread safe**.
- `init` and `finished` must be paired.  Calling `finished` twice, or any
  call before `init`, raises a Lua error.  After `finished`, newt's screen
  is gone; do not use components or forms afterwards.
- A form owns its child components: destroying a form destroys its children
  and marks them dead.  Using a destroyed component or form raises an error.

## Development

    meson setup build
    ninja -C build          # build
    meson test -C build     # test suite (needs a tty)
    ninja -C build cppcheck # static analysis
    ninja -C build check-valgrind # memory-error check
    ninja -C build format    # clang-format
    ninja -C build index.html # ldoc docs

The build enables `-Werror`, a strict warning set (including `-Wconversion`
and `-Wsign-conversion`), and link hardening (`relro`, `now`,
`noexecstack`).  Use `-Doptimization=2` to also enable `_FORTIFY_SOURCE`.

## License

ISC.  See `LICENSE`.
