#pragma once

#include <obs-module.h>

#define PLUGIN_INFO                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-noise/\">Noise</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

#define NOISE_TYPE_BLOCK 0
#define NOISE_TYPE_BLOCK_LABEL "Noise.Type.Block"
#define NOISE_TYPE_LINEAR 1
#define NOISE_TYPE_LINEAR_LABEL "Noise.Type.Linear"
#define NOISE_TYPE_SMOOTHSTEP 2
#define NOISE_TYPE_SMOOTHSTEP_LABEL "Noise.Type.Smoothstep"

#define NOISE_MAP_TYPE_BASIC 0
#define NOISE_MAP_TYPE_BASIC_LABEL "Noise.MapType.Basic"
#define NOISE_MAP_TYPE_BILLOW 1
#define NOISE_MAP_TYPE_BILLOW_LABEL "Noise.MapType.Billow"
#define NOISE_MAP_TYPE_RIDGED 2
#define NOISE_MAP_TYPE_RIDGED_LABEL "Noise.MapType.Ridged"
#define NOISE_MAP_TYPE_SMOOTH 3
#define NOISE_MAP_TYPE_SMOOTH_LABEL "Noise.MapType.Smooth"

struct noise_data;
typedef struct noise_data noise_data_t;

struct noise_data {
	obs_source_t *context;

	// Render pipeline
	bool input_texture_generated;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;

	gs_effect_t *noise_effect;
	gs_effect_t *output_effect;

	bool rendered;
	bool rendering;
	bool is_filter;

	struct vec2 displace_scale;
	float time;
	float speed;
	float sub_influence;
	struct vec2 sub_scaling;
	struct vec2 sub_displace;
	float sub_rotation;
	float brightness;
	float contrast;

	uint32_t num_channels;
	uint32_t noise_type;
	uint32_t noise_map_type;
	bool invert;

	uint32_t layers;
	gs_eparam_t *time_param;
	gs_eparam_t *layers_param;
	struct vec2 pixel_size;
	gs_eparam_t *pixel_size_param;
	gs_eparam_t *uv_size_param;
	gs_eparam_t *sub_influence_param;
	gs_eparam_t *noise_type_param;
	gs_eparam_t *noise_map_type_param;
	gs_eparam_t *sub_scaling_param;
	gs_eparam_t *invert_param;
	gs_eparam_t *sub_displace_param;
	gs_eparam_t *sub_rotation_param;
	gs_eparam_t *param_displace_scale;
	gs_eparam_t *param_image;
	gs_eparam_t *param_contrast;
	gs_eparam_t *param_brightness;

	gs_eparam_t *param_output_image;

	float clock_time;

	uint32_t width;
	uint32_t height;
	struct vec2 uv_size;
};
