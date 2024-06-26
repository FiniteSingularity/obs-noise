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
static void render_noise(noise_data_t *filter, gs_texrender_t *output_texrender);
static void render_noise_displace(noise_data_t *filter);
static void render_noise_gradient(noise_data_t *filter);
static void load_noise_effect(noise_data_t *filter);
static void load_noise_displace_effect(noise_data_t *filter);
static void load_noise_gradient_effect(noise_data_t *filter);
static void load_output_effect(noise_data_t *filter);

static bool save_as_button_clicked(obs_properties_t *props,
				   obs_property_t *property, void *data);
static bool setting_preset_selected(void *data, obs_properties_t *props,
				    obs_property_t *p, obs_data_t *settings);
static bool setting_channels_modified(obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_noise_type_modified(void *data, obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_contours_modified(void *data, obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_billow_modified(void *data, obs_properties_t *props,
				      obs_property_t *p, obs_data_t *settings);
static bool setting_quality_modified(void *data, obs_properties_t *props,
				     obs_property_t *p, obs_data_t *settings);
static bool preset_saved(void *data, obs_properties_t *props, obs_property_t *p,
			 obs_data_t *settings);
static bool preset_loaded(void *data, obs_properties_t *props,
			  obs_property_t *p, obs_data_t *settings);
static bool cancel_save_button_clicked(obs_properties_t *props,
				       obs_property_t *property, void *data);

static gs_effect_t *load_noise_shader_effect(noise_data_t *filter,
					     const char *effect_file_path);
char *load_noise_shader_from_file(noise_data_t *filter, const char *file_name);

static const char *get_noise_type_string(uint32_t noise_type);
static const char *get_hash_effect_string(uint32_t quality);
static const char *get_map_type_string(bool billow);
static const char *get_contour_1_string(bool contour);
static const char *get_contour_2_string(bool contour);
static const char *get_contour_3_string(bool contour);
