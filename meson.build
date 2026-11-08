project('luanewt', 'c', meson_version: '>=1.10.0')

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')

newt_dep = dependency('libnewt')

which_lua = get_option('lua')

if which_lua == 'luajit'
  install_cmod = get_option('prefix') / 'lib/lua/5.1'
elif which_lua == 'lua5.4'
  install_cmod = get_option('prefix') / 'lib/lua/5.4'
else
  error('unhandled lua version')
endif

cmod_override = get_option('lua-module-directory')
if cmod_override != ''
  install_cmod = cmod_override
endif

dep_name = which_lua
if build_machine.system() == 'openbsd'
  dep_name = dep_name.replace('.', '')
endif

lua_dep = dependency(dep_name)

lua_libnewt_srcs = '''
newt.c
component.c
form.c
'''.split()

snippets = import('snippets')
vis_header = snippets.symbol_visibility_header('luanewt_vis.h')

lua_libnewt = shared_library('newt',
  lua_libnewt_srcs,
  name_prefix: '',
  dependencies: [lua_dep, newt_dep],
  install: true,
  install_dir: install_cmod,
  gnu_symbol_visibility: 'hidden',
  c_args: ['-DLUANEWT_COMPILATION'],
)


ldoc_bin = find_program('ldoc', required: false)
if ldoc_bin.found()
  documentation = custom_target('ldoc',
    input: [files('newt.c')],
    output: ['index.html', 'ldoc.css'],
    command: [ldoc_bin, '-d', '@OUTDIR@', '@INPUT@'],
  )
endif

