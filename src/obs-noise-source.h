#pragma once

#include <obs-module.h>

#include "version.h"
#include "obs-noise.h"
#include "obs-utils.h"

static const char *noise_source_name(void *unused);
static const char *noise_displace_filter_name(void *unused);
static void *noise_displace_filter_create(obs_data_t *settings,
					  obs_source_t *source);
static void *noise_source_create(obs_data_t *settings, obs_source_t *source);
static void noise_create(noise_data_t *filter);
static void noise_source_destroy(void *data);
static uint32_t noise_source_width(void *data);
static uint32_t noise_source_height(void *data);
static void noise_source_update(void *data, obs_data_t *settings);
static void noise_displace_filter_video_render(void *data, gs_effect_t *effect);
static void noise_source_video_render(void *data, gs_effect_t *effect);
static void get_input_source(noise_data_t *filter);
static void draw_output_filter(noise_data_t *filter);
static obs_properties_t *noise_source_properties(void *data);
static void noise_source_video_tick(void *data, float seconds);
static void noise_source_defaults(obs_data_t *settings);
static void draw_output(noise_data_t *filter);
static void render_noise(noise_data_t *filter);
static void render_noise_displace(noise_data_t *filter);
static void load_noise_effect(noise_data_t *filter);
static void load_noise_displace_effect(noise_data_t *filter);
static void load_output_effect(noise_data_t *filter);

static bool setting_preset_selected(void *data, obs_properties_t *props,
				    obs_property_t *p, obs_data_t *settings);
static bool setting_channels_modified(obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_noise_type_modified(obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_contours_modified(obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
