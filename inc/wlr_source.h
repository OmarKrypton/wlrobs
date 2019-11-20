/*
 *  Copyright (C) 2019 Scoopta
 *  This file is part of wlrobs
 *  wlrobs is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    wlrobs is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with wlrobs.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WLR_SOURCE_H
#define WLR_SOURCE_H

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <obs/obs-module.h>

#include <wayland-client.h>

#include <xdg-output-unstable-v1-client-protocol.h>
#include <wlr-screencopy-unstable-v1-client-protocol.h>

struct obs_source_info wlr_source;

#endif
