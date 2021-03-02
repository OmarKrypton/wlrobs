# wlrobs
wlrobs is an obs-studio plugin that allows you to screen capture on wlroots based wayland compositors

[![builds.sr.ht status](https://builds.sr.ht/~scoopta/wlrobs.svg)](https://builds.sr.ht/~scoopta/wlrobs?)

This plugin only records wayland desktops, it does not make OBS run wayland native. If you're not using a version of OBS with an EGL backend you need to set `QT_QPA_PLATFORM=xcb` or else OBS does not work.

## dmabuf backend
Please note that in order to use the dmabuf backend you have to update to OBS master commit 7867d16e6b1d1c66913e3f1acb079cef44a1204f or later

As of wlrobs 5f1c794e4614 the dmabuf backend will not work on older OBS EGL forks which do not support `gs_texture_create_from_dmabuf()`

## Dependencies
	libwayland-dev
	libobs-dev
	pkg-config
	meson
## Building
	hg clone https://hg.sr.ht/~scoopta/wlrobs
	cd wlrobs
	meson build
	ninja -C build
The screencopy backend can be disabled with the `-Duse_scpy=false` meson option, likewise dmabuf can be disabled with `-Duse_dmabuf=false`

## Installing
	mkdir -p ~/.config/obs-studio/plugins/wlrobs/bin/64bit
	cp build/libwlrobs.so ~/.config/obs-studio/plugins/wlrobs/bin/64bit
## Uninstalling
	rm -rf ~/.config/obs-studio/plugins/wlrobs
## Bug Reports
Please file bug reports at https://todo.sr.ht/~scoopta/wlrobs
## Contributing
Please submit patches to https://lists.sr.ht/~scoopta/wlrobs

You can find documentation here https://man.sr.ht/hg.sr.ht/email.md
