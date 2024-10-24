project(
	'udjathttp', 
	['cpp'],
	version: '1.2.1',
	default_options : ['c_std=c11', 'cpp_std=c++17'],
	license: 'GPL-3.0-or-later',
)

project_description = 'HTTP module and library for udjat'

#
# Versioning
#
pkg_version = meson.project_version()
version_array = pkg_version.split('.')
pkg_major_version = version_array[0].to_int()
pkg_minor_version = version_array[1].to_int()
pkg_micro_version = version_array[2].to_int()

libversion = '@0@.@1@'.format(pkg_major_version,pkg_minor_version)

#
# LibUdjat
#
libudjat = dependency('libudjat') 

lib_deps = [
  libudjat,
]

#
# Compiler flags
#
cxx = meson.get_compiler('cpp')
enable_debug = get_option('debug') or get_option('buildtype').contains('debug')

compiler_flags_common = [
  '-ggdb3',
  '-ffat-lto-objects',
  '-fvisibility=hidden',
  '-fPIC',
  '-DBUILD_DATE='+run_command('date','+%Y%m%d').stdout().strip(),
]

if enable_debug
  compiler_flags_common += [
	  '-DDEBUG=1'
  ]
else
  compiler_flags_common += [
	'-DNDEBUG=1'
  ]
endif

compiler_flags_common += [
  '-DHAVE_CONFIG_H=1'
]

global_cxxflags = cxx.get_supported_arguments(compiler_flags_common)

add_project_arguments(global_cxxflags, language: 'cpp')

#
# Configuration
#
app_conf = configuration_data()
app_conf.set_quoted('PACKAGE_NAME', meson.project_name())
app_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
app_conf.set_quoted('PACKAGE_DESCRIPTION', project_description)
app_conf.set('PACKAGE_VERSION_MAJOR', pkg_major_version)
app_conf.set('PACKAGE_VERSION_MINOR', pkg_minor_version)
app_conf.set('PACKAGE_VERSION_MICRO', pkg_micro_version)

if cxx.compiles('#include <unistd.h>', name : 'unistd.h')
  app_conf.set('HAVE_UNISTD_H', 1)
endif

config_src = [
  configure_file(
      output : 'config.h', 
      configuration : app_conf
  )
]

#
# SDK
#
sdk_conf = configuration_data()
sdk_conf.set('PACKAGE_NAME', 'udjathttp')
sdk_conf.set('PACKAGE_VERSION', meson.project_version())
sdk_conf.set('PACKAGE_DESCRIPTION','HTTP module and library for udjat')
sdk_conf.set('prefix',get_option('prefix'))
sdk_conf.set('exec_prefix',get_option('prefix'))
sdk_conf.set('includedir',get_option('includedir'))
sdk_conf.set('libdir',get_option('libdir'))

includes_dir = include_directories('../src/include')

#
# Sources
#
lib_src = [
  'src/library/engine.cc',
  'src/library/worker.cc',
]

module_src = [
  'src/module/init.cc',
]

test_src = [
  'src/testprogram/testprogram.cc'
]

#
# OS dependencies
#
if host_machine.system() == 'windows'
  sdk_conf.set('REQUIRES','')
  sdk_conf.set('DYNLIBS','-ludjathttp.dll -lws2_32 -lwinhttp')
  sdk_conf.set('STATICLIBS','-ludjathttp -lws2_32 -lwinhttp')
  lib_src += [
    'src/library/engines/winhttp/get.cc',
    'src/library/engines/winhttp/test.cc',
    'src/library/engines/winhttp/winhttp.cc',
    'src/library/engines/winhttp/worker.cc',
    'src/library/os/windows/base64.cc'
  ]
else
  sdk_conf.set('REQUIRES','cups')
  sdk_conf.set('DYNLIBS','-ludjathttp')
  sdk_conf.set('STATICLIBS','-l:libudjathttp.a')
  lib_deps += [
    dependency('libcurl'),
  ]
  lib_src += [
    'src/library/engines/curl/test.cc',
    'src/library/engines/curl/curl.cc',
  ]
endif

#
# Targets
#
dynamic = shared_library(
  'udjathttp', 
  config_src + lib_src,
  install: true,
  version : libversion,
  soversion : libversion,
  gnu_symbol_visibility: 'hidden',
  dependencies: lib_deps,
  include_directories: includes_dir
)

static_library(
  'udjathttp', 
  config_src + lib_src,
  install: true,
  dependencies: lib_deps,
  include_directories: includes_dir
)

# https://mesonbuild.com/Reference-manual_functions.html#shared_library
module = shared_library(
  'http', 
  config_src + module_src,
  name_prefix: 'udjat-module-',
  install: true,
  link_with: dynamic,
  install_dir: libudjat.get_variable('module_path'),
  gnu_symbol_visibility: 'hidden',
  dependencies: lib_deps,
  include_directories: includes_dir
)

executable(
  'httptest',
  config_src + test_src,
  install: false,
  link_with : [ dynamic, module ],
  include_directories: includes_dir
)

configure_file(
  input : 'sdk/dynamic.pc.in',
  output : 'udjathttp.pc',
  install: true,
  install_dir: get_option('libdir') + '/pkgconfig',
  configuration : sdk_conf
)

configure_file(
  input : 'sdk/static.pc.in',
  output : 'udjathttp-static.pc',
  install: true,
  install_dir: get_option('libdir') + '/pkgconfig',
  configuration : sdk_conf
)


#
# Common targets
#
install_subdir('src/include/udjat', install_dir : get_option('includedir'))
