project('luanewt', 'c',
  version: '0.1.0',
  license: 'ISC',
  meson_version: '>=1.10.0',
  default_options: [
    'c_std=c17',
    'warning_level=3',
    'werror=true',
  ],
)

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')

newt_dep = dependency('libnewt')
lua_dep = dependency('lua5.4')

# hardening flags, modelled on luapf
hardening_c_args = cc.get_supported_arguments([
  '-fstack-protector-strong',
  '-Wundef',
  '-Wshadow',
  '-Wpointer-arith',
  '-Wcast-qual',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Wcast-align',
  '-Wwrite-strings',
  '-Wconversion',
  '-Wsign-conversion',
  '-Wno-unused-parameter',
])

if get_option('trap')
  hardening_c_args += '-DTRAP'
endif

if get_option('optimization') != '0'
  hardening_c_args += '-D_FORTIFY_SOURCE=2'
endif

hardening_link_args = cc.get_supported_link_arguments([
  '-Wl,-z,relro',
  '-Wl,-z,now',
  '-Wl,-z,noexecstack',
])

add_project_arguments(hardening_c_args, language: 'c')
add_project_link_arguments(hardening_link_args, language: 'c')

install_cmod = get_option('prefix') / 'lib/lua/5.4'
cmod_override = get_option('lua-module-directory')
if cmod_override != ''
  install_cmod = cmod_override
endif

lua_libnewt_srcs = [
  'newt.c',
  'component.c',
  'form.c',
]

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

format_srcs = files('newt.c', 'component.c', 'form.c', 'luanewt.h')

clang_format = find_program('clang-format', required: false)
if clang_format.found()
  run_target('format',
    command: [clang_format, '-i', '--style=file', format_srcs],
  )
endif

cppcheck = find_program('cppcheck', required: false)
if cppcheck.found()
  run_target('cppcheck',
    command: [
      cppcheck, '--quiet', '--error-exitcode=1',
      '--enable=warning,style,performance,portability',
      '--std=c11', '--suppress=missingIncludeSystem',
      files('newt.c', 'component.c', 'form.c'),
    ],
  )
endif

ldoc_bin = find_program('ldoc', required: false)
if ldoc_bin.found()
  documentation = custom_target('ldoc',
    input: [files('newt.c', 'component.c', 'form.c')],
    output: ['index.html', 'ldoc.css'],
    command: [ldoc_bin, '-c', 'config.ld', '-d', '@OUTDIR@', '@INPUT@'],
  )
endif

lua = find_program('lua5.4', required: false)
script = find_program('script', required: false)

lua_cpath_env = 'LUA_CPATH=' + meson.current_build_dir() + '/?.so;;'

if lua.found() and script.found()
  test('newt',
    script,
    args: [
      '-qec',
      ' '.join([lua.full_path(), meson.current_source_dir() / 'tests/test.lua']),
    ],
    env: [lua_cpath_env],
    timeout: 60,
  )
endif

valgrind = find_program('valgrind', required: false)
if lua.found() and script.found() and valgrind.found()
  vg_cmd = ' '.join([
    valgrind.full_path(), '-q', '--error-exitcode=1', '--leak-check=no',
    lua.full_path(), meson.current_source_dir() / 'tests/test.lua',
  ])
  run_target('check-valgrind',
    command: [script, '-qec', vg_cmd],
    env: [lua_cpath_env],
  )
endif
