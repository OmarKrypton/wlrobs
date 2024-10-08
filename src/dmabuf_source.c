/*
 *  Copyright (C) 2019-2023 Scoopta
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

#include <dmabuf_source.h>

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

#include <wayland-client.h>

#include <xdg-output-unstable-v1-client-protocol.h>
#include <wlr-export-dmabuf-unstable-v1-client-protocol.h>

#define PROTO_VERSION(v1, v2) (v1 < v2 ? v1 : v2)

struct wlr_frame {
	uint32_t format;
	uint32_t width, height;
	uint32_t obj_count;
	uint32_t strides[4];
	uint32_t sizes[4];
	int32_t fds[4];
	uint32_t offsets[4];
	uint32_t plane_indices[4];
	uint64_t modifiers[4];
	gs_texture_t* texture;
	struct zwlr_export_dmabuf_frame_v1* frame;
};

struct wlr_source {
	struct wl_display* wl;
	struct wl_list outputs;
	struct output_node* current_output;
	struct wl_registry* wl_registry;
	struct wl_registry_listener* wl_listener;
	struct zxdg_output_manager_v1* output_manager;
	struct zwlr_export_dmabuf_manager_v1* dmabuf_manager;
	struct wlr_frame* current_frame, *next_frame;
	bool waiting;
	bool show_cursor;
	bool render;
	pthread_mutex_t mutex;
	pthread_cond_t _waiting;
	obs_property_t* obs_outputs;
};

struct output_node {
	struct wl_output* wl_output;
	struct zxdg_output_v1* xdg_output;
	uint32_t wl_name;
	size_t obs_idx;
	char* name;
	struct zxdg_output_v1_listener* listener;
	struct wl_list link;
};

static const char* get_name(void* data) {
	(void) data;
	return "Wayland output(dmabuf)";
}

static void nop() {}

static void get_xdg_name(void* data, struct zxdg_output_v1* output, const char* name) {
	(void) output;
	struct output_node* node = data;
	node->name = strdup(name);
}

static void add_interface(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
	struct wlr_source* this = data;
	if(strcmp(interface, wl_output_interface.name) == 0) {
		struct output_node* node = malloc(sizeof(struct output_node));
		node->wl_output = wl_registry_bind(registry, name, &wl_output_interface, PROTO_VERSION(version, 4));
		node->wl_name = name;
		wl_list_insert(&this->outputs, &node->link);

		node->xdg_output = zxdg_output_manager_v1_get_xdg_output(this->output_manager, node->wl_output);
		node->listener = malloc(sizeof(struct zxdg_output_v1_listener));
		node->listener->description = nop;
		node->listener->done = nop;
		node->listener->logical_position = nop;
		node->listener->logical_size = nop;
		node->listener->name = get_xdg_name;
		zxdg_output_v1_add_listener(node->xdg_output, node->listener, node);
		wl_display_roundtrip(this->wl);
	} else if(strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		this->output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, PROTO_VERSION(version, 3));
	} else if(strcmp(interface, zwlr_export_dmabuf_manager_v1_interface.name) == 0) {
		this->dmabuf_manager = wl_registry_bind(registry, name, &zwlr_export_dmabuf_manager_v1_interface, PROTO_VERSION(version, 1));
	}
}

static void rm_interface(void* data, struct wl_registry* registry, uint32_t name) {
	(void) registry;

	struct wlr_source* this = data;
	struct output_node* node, *safe_node;

	wl_list_for_each_safe(node, safe_node, &this->outputs, link) {
		if(node->wl_name == name) {
			wl_list_remove(&node->link);

			zxdg_output_v1_destroy(node->xdg_output);
			obs_property_list_item_remove(this->obs_outputs, node->obs_idx);

			free(node->name);
			free(node->listener);
			node->name = NULL;

			if(this->current_output == node) {
				this->current_output = NULL;
			}

			free(node);
		}
	}
}

static void destroy(void* data) {
	struct wlr_source* this = data;
	struct output_node* node, *safe_node;
	wl_list_for_each_safe(node, safe_node, &this->outputs, link) {
		wl_list_remove(&node->link);

		zxdg_output_v1_destroy(node->xdg_output);

		free(node->name);
		free(node->listener);
		node->name = NULL;
		free(node);
	}
	this->current_output = NULL;

	if(this->current_frame != NULL) {
		gs_texture_destroy(this->current_frame->texture);
		free(this->current_frame);
		this->current_frame = NULL;
	}

	if(this->next_frame != NULL) {
		gs_texture_destroy(this->next_frame->texture);
		free(this->next_frame);
		this->next_frame = NULL;
	}

	if(this->wl_registry != NULL) {
		wl_registry_destroy(this->wl_registry);
		free(this->wl_listener);
	}

	if(this->wl != NULL) {
		wl_display_disconnect(this->wl);
	}
}

static void destroy_complete(void* data) {
	destroy(data);
	free(data);
}

static void populate_outputs(struct wlr_source* this) {
	struct output_node* node;
	wl_list_for_each(node, &this->outputs, link) {
		node->obs_idx = obs_property_list_add_string(this->obs_outputs, node->name, node->name);
	}
}

static void setup_display(struct wlr_source* this, const char* display) {
	pthread_mutex_lock(&this->mutex);
	while(this->waiting) {
		pthread_cond_wait(&this->_waiting, &this->mutex);
	}
	pthread_mutex_unlock(&this->mutex);
	this->render = false;
	if(this->wl != NULL) {
		destroy(this);
	}
	wl_list_init(&this->outputs);
	if(strcmp(display, "") == 0) {
		display = NULL;
	}
	this->wl = wl_display_connect(display);
	if(this->wl == NULL) {
		return;
	}
	this->wl_registry = wl_display_get_registry(this->wl);
	this->wl_listener = malloc(sizeof(struct wl_registry_listener));
	this->wl_listener->global = add_interface;
	this->wl_listener->global_remove = rm_interface;
	wl_registry_add_listener(this->wl_registry, this->wl_listener, this);
	wl_display_roundtrip(this->wl);
	this->render = true;
}

static void update(void* data, obs_data_t* settings) {
	struct wlr_source* this = data;
	if(this->render) {
		struct output_node* node;
		wl_list_for_each(node, &this->outputs, link) {
			if(strcmp(node->name, obs_data_get_string(settings, "output")) == 0) {
				this->current_output = node;
			}
		}
		this->show_cursor = obs_data_get_bool(settings, "show_cursor");
	}
}

static void* create(obs_data_t* settings, obs_source_t* source) {
	(void) source;
	struct wlr_source* this = calloc(1, sizeof(struct wlr_source));
	pthread_mutex_init(&this->mutex, NULL);
	pthread_cond_init(&this->_waiting, NULL);
	setup_display(this, obs_data_get_string(settings, "display"));
	update(this, settings);
	return this;
}

static void _frame(void* data, struct zwlr_export_dmabuf_frame_v1* frame, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t buffer_flags, uint32_t flags, uint32_t format, uint32_t mod_high, uint32_t mod_low, uint32_t obj_count) {
	(void) x;
	(void) y;
	(void) buffer_flags;
	(void) flags;
	struct wlr_source* this = data;
	this->next_frame = calloc(1, sizeof(struct wlr_frame));
	this->next_frame->format = format;
	this->next_frame->width = width;
	this->next_frame->height = height;
	this->next_frame->obj_count = obj_count;
	this->next_frame->frame = frame;
	for (int i = 0; i < 4; ++i) {
		this->next_frame->modifiers[i] = (((uint64_t) mod_high) << 32) | mod_low;
	}
}

static void object(void* data, struct zwlr_export_dmabuf_frame_v1* frame, uint32_t index, int32_t fd, uint32_t size, uint32_t offset, uint32_t stride, uint32_t plane_index) {
	(void) frame;
	struct wlr_source* this = data;
	this->next_frame->fds[index] = fd;
	this->next_frame->sizes[index] = size;
	this->next_frame->strides[index] = stride;
	this->next_frame->offsets[index] = offset;
	this->next_frame->plane_indices[index] = plane_index;
}

static void ready(void* data, struct zwlr_export_dmabuf_frame_v1* frame, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec) {
	(void) frame;
	(void) tv_sec_hi;
	(void) tv_sec_lo;
	(void) tv_nsec;
	struct wlr_source* this = data;

	this->next_frame->texture = gs_texture_create_from_dmabuf(this->next_frame->width, this->next_frame->height,
								this->next_frame->format, GS_BGRA,
								this->next_frame->obj_count, this->next_frame->fds,
								this->next_frame->strides, this->next_frame->offsets,
								this->next_frame->modifiers);

	if(this->current_frame != NULL) {
		if(this->current_frame->texture != NULL) {
			gs_texture_destroy(this->current_frame->texture);
		}

		if(this->current_frame->frame != NULL) {
			zwlr_export_dmabuf_frame_v1_destroy(this->current_frame->frame);
		}

		for(uint32_t count = 0; count < this->current_frame->obj_count; ++count) {
			close(this->current_frame->fds[count]);
		}
		free(this->current_frame);
	}

	this->current_frame = this->next_frame;
	this->next_frame = NULL;
	this->waiting = false;
}

static void cancel(void* data, struct zwlr_export_dmabuf_frame_v1* frame, enum zwlr_export_dmabuf_frame_v1_cancel_reason reason) {
	(void) reason;
	struct wlr_source* this = data;
	zwlr_export_dmabuf_frame_v1_destroy(frame);

	struct wlr_frame* wlr_frame;

	if(this->current_frame->frame == frame) {
		wlr_frame = this->current_frame;
	} else if(this->next_frame != NULL && this->next_frame->frame == frame) {
		wlr_frame = this->next_frame;
	} else {
		this->waiting = false;
		return;
	}

	for(uint32_t count = 0; count < wlr_frame->obj_count; ++count) {
		close(wlr_frame->fds[count]);
	}
	this->waiting = false;
}

static struct zwlr_export_dmabuf_frame_v1_listener dmabuf_listener = {
	.frame = _frame,
	.object = object,
	.ready = ready,
	.cancel = cancel
};

static void render(void* data, gs_effect_t* effect) {
	(void) effect;
	struct wlr_source* this = data;

	if(!this->render || this->current_output == NULL) {
		this->waiting = false;
		return;
	}
	if(!this->waiting) {
		this->waiting = true;
		struct zwlr_export_dmabuf_frame_v1* frame = zwlr_export_dmabuf_manager_v1_capture_output(this->dmabuf_manager, this->show_cursor, this->current_output->wl_output);
		zwlr_export_dmabuf_frame_v1_add_listener(frame, &dmabuf_listener, this);
	}
	while(this->waiting && this->current_output != NULL) {
		wl_display_roundtrip(this->wl);
	}

	if(this->current_frame != NULL) {
		gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, this->current_frame->texture);
		while(gs_effect_loop(effect, "Draw")) {
			gs_draw_sprite(this->current_frame->texture, 0, 0, 0);
		}
	}
	pthread_mutex_lock(&this->mutex);
	pthread_cond_broadcast(&this->_waiting);
	pthread_mutex_unlock(&this->mutex);
}

static bool update_outputs(void* data, obs_properties_t* props, obs_property_t* property, obs_data_t* settings) {
	(void) props;
	(void) property;
	struct wlr_source* this = data;
	if(this->obs_outputs == NULL) {
		return false;
	}
	setup_display(this, obs_data_get_string(settings, "display"));
	obs_property_list_clear(this->obs_outputs);
	populate_outputs(this);
	update(data, settings);
	return true;
}

static obs_properties_t* get_properties(void* data) {
	struct wlr_source* this = data;
	obs_properties_t* props = obs_properties_create();
	obs_property_t* display = obs_properties_add_text(props, "display", "Wayland Display", OBS_TEXT_DEFAULT);
	obs_property_set_modified_callback2(display, update_outputs, this);
	this->obs_outputs = obs_properties_add_list(props, "output", "Output", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	populate_outputs(this);
	obs_properties_add_bool(props, "show_cursor", "Show mouse cursor");
	return props;
}

static uint32_t get_width(void* data) {
	struct wlr_source* this = data;
	if(this->current_frame == NULL) {
		return 0;
	} else {
		return this->current_frame->width;
	}
}

static uint32_t get_height(void* data) {
	struct wlr_source* this = data;
	if(this->current_frame == NULL) {
		return 0;
	} else {
		return this->current_frame->height;
	}
}

struct obs_source_info dmabuf_source = {
	.id = "wlrobs-dmabuf",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name = get_name,
	.create = create,
	.destroy = destroy_complete,
	.update = update,
	.video_render = render,
	.get_width = get_width,
	.get_height = get_height,
	.get_properties = get_properties
};
