# wlrobs
wlrobs is an obs-studio plugin that allows you to screen capture on wlroots based wayland compositors
## Dependencies
	libwayland-dev
	libobs-dev
## Building
	hg clone https://bitbucket.org/Scoopta/wlrobs
	cd wlrobs/Release
	make
## Installing
	mkdir -p ~/.config/obs-studio/plugins/wlrobs/bin/64bit
	cp libwlrobs.so ~/.config/obs-studio/plugins/wlrobs/bin/64bit
## Uninstalling
	rm -rf ~/.config/obs-studio/plugins/wlrobs
