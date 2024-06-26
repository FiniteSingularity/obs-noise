#pragma once

#include <obs-module.h>

#define PLUGIN_INFO                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-noise/\">Noise</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

// Default glow color: white in ABGR
#define DEFAULT_MAP_COLOR_1 0xFF000000
// Default shadow color: dark gray in ABGR
#define DEFAULT_MAP_COLOR_2 0xFFFFFFFF

#define NOISE_TYPE_BLOCK 0
#define NOISE_TYPE_BLOCK_LABEL "Noise.Type.Block"
#define NOISE_TYPE_LINEAR 1
#define NOISE_TYPE_LINEAR_LABEL "Noise.Type.Linear"
#define NOISE_TYPE_SMOOTHSTEP 2
#define NOISE_TYPE_SMOOTHSTEP_LABEL "Noise.Type.Smoothstep"
#define NOISE_TYPE_OPEN_SIMPLEX 3
#define NOISE_TYPE_OPEN_SIMPLEX_LABEL "Noise.Type.OpenSimplex"
#define NOISE_TYPE_WORLEY 4
#define NOISE_TYPE_WORLEY_LABEL "Noise.Type.Worley"

#define NOISE_LAYER_WEIGHTED_AVERAGE_LABEL "Noise.LayerComb.WeightedAverage"
#define NOISE_LAYER_WEIGHTED_AVERAGE 1
#define NOISE_LAYER_MAX_LABEL "Noise.LayerComb.Max"
#define NOISE_LAYER_MAX 2

#define NOISE_MAP_TYPE_BASIC 0
#define NOISE_MAP_TYPE_BASIC_LABEL "Noise.MapType.Basic"
#define NOISE_MAP_TYPE_BILLOW 1
#define NOISE_MAP_TYPE_BILLOW_LABEL "Noise.MapType.Billow"
#define NOISE_MAP_TYPE_RIDGED 2
#define NOISE_MAP_TYPE_RIDGED_LABEL "Noise.MapType.Ridged"
#define NOISE_MAP_TYPE_SMOOTH 3
#define NOISE_MAP_TYPE_SMOOTH_LABEL "Noise.MapType.Smooth"

#define NOISE_CHANNELS_3 3
#define NOISE_CHANNELS_3_LABEL "Noise.Channels.Three"
#define NOISE_CHANNELS_2 2
#define NOISE_CHANNELS_2_LABEL "Noise.Channels.Two"
#define NOISE_CHANNELS_1 1
#define NOISE_CHANNELS_1_LABEL "Noise.Channels.One"
#define NOISE_CHANNELS_COLOR_MAP 0
#define NOISE_CHANNELS_COLOR_MAP_LABEL "Noise.Channels.ColorMap"

#define NOISE_QUALITY_FAST 0
#define NOISE_QUALITY_FAST_LABEL "Noise.Quality.Fast"
#define NOISE_QUALITY_HIGH 1
#define NOISE_QUALITY_HIGH_LABEL "Noise.Quality.HighQuality"

#define NOISE_DISPLACEMENT_TWO_CHANNEL 0
#define NOISE_DISPLACEMENT_TWO_CHANNEL_LABEL "Noise.Displacement.Algorithm.TwoChannel"
#define NOISE_DISPLACEMENT_GRADIENT 1
#define NOISE_DISPLACEMENT_GRADIENT_LABEL "Noise.Displacement.Algorithm.Gradient"

struct noise_data;
typedef struct noise_data noise_data_t;

struct noise_data {
	obs_source_t *context;

	// Render pipeline
	bool input_texture_generated;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	gs_texrender_t *dispmap_texrender;
	gs_texrender_t *grad_texrender;
	gs_texrender_t *dk_render;
	gs_texrender_t *dk_render2;
	gs_texrender_t *blur_output;

	obs_data_t *global_preset_data;

	gs_effect_t *noise_effect;
	gs_effect_t *gradient_effect;
	gs_effect_t *displacement_effect;
	gs_effect_t *output_effect;
	gs_effect_t *effect_dual_kawase_downsample;
	gs_effect_t *effect_dual_kawase_upsample;
	gs_effect_t *effect_dual_kawase_mix;

	bool rendered;
	bool rendering;
	bool is_filter;

	uint32_t channels;

	uint32_t quality;

	struct vec4 map_color_1;
	struct vec4 map_color_2;

	struct vec2 displace_scale;
	float time;
	float speed;
	float sub_influence;
	struct vec2 sub_scaling;
	struct vec2 sub_displace;
	float sub_rotation;
	float brightness;
	float contrast;
	float power;
	float sum_influence;
	float std_scale;
	float global_rotation;
	struct vec2 global_offset;
	bool contour;
	int num_contours;

	uint8_t displacement_algorithm;

	bool billow;
	bool ridged;
	bool comb_max;

	uint32_t num_channels;
	uint32_t noise_type;
	uint32_t noise_map_type;
	int dw_iterations;
	struct vec2 dw_strength;
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
	gs_eparam_t *param_image;
	gs_eparam_t *param_contrast;
	gs_eparam_t *param_brightness;
	gs_eparam_t *param_billow;
	gs_eparam_t *param_power;
	gs_eparam_t *param_ridged;
	gs_eparam_t *param_sum_influence;
	gs_eparam_t *param_std_scale;
	gs_eparam_t *param_dw_iterations;
	gs_eparam_t *param_dw_strength;
	gs_eparam_t *param_global_rotation;
	gs_eparam_t *param_global_offset;

	gs_eparam_t *param_output_image;

	gs_eparam_t *param_color_1;
	gs_eparam_t *param_color_2;
	gs_eparam_t *param_contours;
	gs_eparam_t *param_num_contours;

	gs_eparam_t *param_displace_uv_size;
	gs_eparam_t *param_displace_image;
	gs_eparam_t *param_displace_displacement_map;
	gs_eparam_t *param_displace_scale;
	gs_eparam_t *param_displace_average_pixel;

	gs_eparam_t *param_grad_uv_size;
	gs_eparam_t *param_grad_image;

	gs_eparam_t *param_downsample_texel_step;
	gs_eparam_t *param_upsample_texel_step;
	gs_eparam_t *param_mix_image;
	gs_eparam_t *param_mix_image2;
	gs_eparam_t *param_mix_ratio;

	float clock_time;

	uint32_t width;
	uint32_t height;
	struct vec2 uv_size;

	bool loading_effect;
	bool reload_effect;
};
