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

#include <obs/obs-module.h>

#include <wlr_source.h>

OBS_DECLARE_MODULE()

bool obs_module_load() {
	obs_register_source(&wlr_source);
	return true;
}
