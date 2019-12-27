project('wlrobs', 'c', default_options : ['buildtype=release', 'warning_level=2'])
cc = meson.get_compiler('c')

inc = include_directories('inc')
wayland = dependency('wayland-client')
obs = dependency('libobs')

shared_library(meson.project_name(),
'src/main.c',
'src/wlr_source.c',
'proto/wlr-screencopy-unstable-v1-protocol.c',
'proto/xdg-output-unstable-v1-protocol.c',
include_directories : inc,
dependencies : [wayland, obs])
