#include "obs-noise-source.h"
#include "obs-noise.h"

struct obs_source_info obs_noise_source = {
	.id = "obs_noise_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_SRGB,
	.get_name = noise_source_name,
	.create = noise_source_create,
	.destroy = noise_source_destroy,
	.update = noise_source_update,
	.video_render = noise_source_video_render,
	.video_tick = noise_source_video_tick,
	.get_width = noise_source_width,
	.get_height = noise_source_height,
	.get_properties = noise_source_properties,
	.get_defaults = noise_source_defaults
};

struct obs_source_info obs_noise_displace_filter = {
	.id = "obs_noise_displace_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = noise_displace_filter_name,
	.create = noise_displace_filter_create,
	.destroy = noise_source_destroy,
	.update = noise_source_update,
	.video_render = noise_displace_filter_video_render,
	.video_tick = noise_source_video_tick,
	.get_width = noise_source_width,
	.get_height = noise_source_height,
	.get_properties = noise_source_properties,
	.get_defaults = noise_source_defaults};

static const char *noise_source_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Noise";
}

static const char *noise_displace_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Noise Displacement";
}

static void *noise_displace_filter_create(obs_data_t *settings,
					  obs_source_t *source)
{
	noise_data_t *filter = bzalloc(sizeof(noise_data_t));
	filter->context = source;
	filter->is_filter = true;

	noise_create(filter);

	load_noise_displace_effect(filter);
	load_output_effect(filter);

	obs_source_update(source, settings);
	return filter;
}

static void *noise_source_create(obs_data_t *settings, obs_source_t *source)
{
	// This function should initialize all pointers in the data
	// structure.
	noise_data_t *filter = bzalloc(sizeof(noise_data_t));
	filter->context = source;
	filter->is_filter = false;

	noise_create(filter);

	load_noise_effect(filter);

	obs_source_update(source, settings);
	return filter;
}

static void noise_create(noise_data_t *filter)
{
	filter->input_texrender =
		create_or_reset_texrender(filter->input_texrender);
	filter->input_texrender =
		create_or_reset_texrender(filter->output_texrender);

	
	filter->rendered = false;
	filter->rendering = false;
	filter->width = 1920u;
	filter->height = 1080u;
	filter->time_param = NULL;
	filter->uv_size_param = NULL;
	filter->pixel_size_param = NULL;
	filter->layers_param = NULL;
	filter->sub_influence_param = NULL;
	filter->noise_type_param = NULL;
	filter->noise_map_type_param = NULL;
}

static void noise_source_destroy(void *data)
{
	// This function should clear up all memory the plugin uses.
	noise_data_t *filter = data;

	obs_enter_graphics();

	// EXAMPLE OF DESTROYING EFFECTS AND TEXRENDER
	//if (filter->effect_stroke) {
	//	gs_effect_destroy(filter->effect_stroke);
	//}

	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}

	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}

	obs_leave_graphics();
	bfree(filter);
}

static uint32_t noise_source_width(void *data)
{
	noise_data_t *filter = data;
	return filter->width;
}

static uint32_t noise_source_height(void *data)
{
	noise_data_t *filter = data;
	return filter->height;
}

static void noise_source_update(void *data, obs_data_t *settings)
{
	// Called after UI is updated, should assign new UI values to
	// data structure pointers/values/etc..
	noise_data_t *filter = data;

	filter->time = (float)obs_data_get_double(settings,"time");
	if (!filter->is_filter) {
		filter->width =
			(uint32_t)obs_data_get_int(settings, "source_width");
		filter->height =
			(uint32_t)obs_data_get_int(settings, "source_height");
	}

	filter->uv_size.x = (float)filter->width;
	filter->uv_size.y = (float)filter->height;
	filter->pixel_size.x = (float)obs_data_get_double(settings, "pixel_width");
	filter->pixel_size.y = (float)obs_data_get_double(settings, "pixel_height");
	filter->layers = (uint32_t)obs_data_get_int(settings, "layers");
	filter->speed = (float)obs_data_get_double(settings, "speed") / 100.0f;
	filter->sub_influence =
		(float)obs_data_get_double(settings, "sub_influence");
	filter->noise_type = (uint32_t)obs_data_get_int(settings, "noise_type");
	filter->noise_map_type =
		(uint32_t)obs_data_get_int(settings, "noise_map_type");
	filter->invert = obs_data_get_bool(settings, "invert");
	filter->sub_scaling.x =
		(float)obs_data_get_double(settings, "sub_scale_x") / 100.0f;
	filter->sub_scaling.y =
		(float)obs_data_get_double(settings, "sub_scale_y") / 100.0f;
	filter->sub_displace.x =
		(float)obs_data_get_double(settings, "sub_displace_x");
	filter->sub_displace.y =
		(float)obs_data_get_double(settings, "sub_displace_y");
	filter->sub_rotation =
		(float)(obs_data_get_double(settings, "sub_rotation") * M_PI /
			180.0);
	filter->displace_scale.x = (float)obs_data_get_double(settings, "filter_displace_scale_x");
	filter->displace_scale.y = (float)obs_data_get_double(settings, "filter_displace_scale_y");
	filter->brightness = (float)obs_data_get_double(settings, "brightness");
	filter->contrast = (float)obs_data_get_double(settings, "contrast");
}

static void noise_displace_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	noise_data_t *filter = data;

	if (filter->rendered) {
		draw_output(filter);
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	get_input_source(filter);
	if (!filter->input_texture_generated) {
		filter->rendering = false;
		obs_source_skip_video_filter(filter->context);
		return;
	}

	// 3. Create Stroke Mask
	// Call a rendering functioner, e.g.:
	render_noise_displace(filter);

	// 3. Draw result (filter->output_texrender) to source
	draw_output_filter(filter);
	filter->rendered = true;
	filter->rendering = false;
}

static void get_input_source(noise_data_t *filter)
{
	// Use the OBS default effect file as our effect.
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Set up our color space info.
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	// Set up our input_texrender to catch the output texture.
	filter->input_texrender =
		create_or_reset_texrender(filter->input_texrender);

	// Start the rendering process with our correct color space params,
	// And set up your texrender to recieve the created texture.
	if (obs_source_process_filter_begin_with_color_space(
		    filter->context, format, source_space,
		    OBS_ALLOW_DIRECT_RENDERING) &&
	    gs_texrender_begin(filter->input_texrender,
			       filter->width, filter->height)) {

		set_blending_parameters();
		gs_ortho(0.0f, (float)filter->width, 0.0f,
			 (float)filter->height, -100.0f, 100.0f);
		// The incoming source is pre-multiplied alpha, so use the
		// OBS default effect "DrawAlphaDivide" technique to convert
		// the colors back into non-pre-multiplied space.
		obs_source_process_filter_tech_end(
			filter->context, pass_through, filter->width,
			filter->height, "DrawAlphaDivide");
		gs_texrender_end(filter->input_texrender);
		gs_blend_state_pop();
		filter->input_texture_generated = true;
	}
}

static void draw_output_filter(noise_data_t *filter)
{
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	if (!obs_source_process_filter_begin_with_color_space(
		    filter->context, format, source_space,
		    OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	gs_texture_t *texture =
		gs_texrender_get_texture(filter->output_texrender);
	gs_effect_t *pass_through = filter->output_effect;

	if (filter->param_output_image) {
		gs_effect_set_texture(filter->param_output_image,
				      texture);
	}

	gs_blend_state_push();
	gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA,
				   GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, pass_through,
				      filter->width,
				      filter->height);
	gs_blend_state_pop();
}

static void noise_source_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	noise_data_t *filter = data;

	if (filter->rendered) {
		draw_output(filter);
		return;
	}

	filter->rendering = true;

	//// 1. Get the input source as a texture renderer
	////    accessed as filter->input_texrender after call
	//get_input_source(filter);
	//if (!filter->input_texture_generated) {
	//	filter->rendering = false;
	//	obs_source_skip_video_filter(filter->context);
	//	return;
	//}
	render_noise(filter);


	// 3. Create Stroke Mask
	// Call a rendering functioner, e.g.:
	//template_render_filter(filter);

	// 3. Draw result (filter->output_texrender) to source
	draw_output(filter);
	filter->rendered = true;
	filter->rendering = false;
}

static obs_properties_t *noise_source_properties(void *data)
{
	noise_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	if (!filter->is_filter) {
		obs_properties_add_int(props, "source_width", "Width", 0, 8000,
				       1);
		obs_properties_add_int(props, "source_height", "Height", 0,
				       8000, 1);
	} else {
		obs_properties_add_float_slider(props, "filter_displace_scale_x",
					 "Displace Scale X", 0.0, 400.0, 0.1);
		obs_properties_add_float_slider(props, "filter_displace_scale_y",
					 "Displace Scale Y", 0.0, 400.0, 0.1);
	}

	obs_property_t *noise_type_list = obs_properties_add_list(
		props, "noise_type", obs_module_text("Noise.Type"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(noise_type_list,
				  obs_module_text(NOISE_TYPE_BLOCK_LABEL),
				  NOISE_TYPE_BLOCK);
	obs_property_list_add_int(noise_type_list,
				  obs_module_text(NOISE_TYPE_LINEAR_LABEL),
				  NOISE_TYPE_LINEAR);
	obs_property_list_add_int(noise_type_list,
				  obs_module_text(NOISE_TYPE_SMOOTHSTEP_LABEL),
				  NOISE_TYPE_SMOOTHSTEP);


	obs_property_t *noise_map_type_list = obs_properties_add_list(
		props, "noise_map_type", obs_module_text("Noise.MapType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(noise_map_type_list,
				  obs_module_text(NOISE_MAP_TYPE_BASIC_LABEL),
				  NOISE_MAP_TYPE_BASIC);
	obs_property_list_add_int(noise_map_type_list,
				  obs_module_text(NOISE_MAP_TYPE_BILLOW_LABEL),
				  NOISE_MAP_TYPE_BILLOW);
	obs_property_list_add_int(noise_map_type_list,
				  obs_module_text(NOISE_MAP_TYPE_RIDGED_LABEL),
				  NOISE_MAP_TYPE_RIDGED);
	obs_property_list_add_int(noise_map_type_list,
				  obs_module_text(NOISE_MAP_TYPE_SMOOTH_LABEL),
				  NOISE_MAP_TYPE_SMOOTH);

	obs_properties_add_bool(props, "invert", obs_module_text("Noise.Invert"));

	obs_properties_add_float_slider(props, "brightness",
					obs_module_text("Noise.Brightness"),
					-1.0, 1.0, 0.01);

	obs_properties_add_float_slider(props, "contrast",
					obs_module_text("Noise.Contrast"),
					-1.0, 1.0, 0.01);

	obs_properties_add_float_slider(
		props, "speed", "Speed", 0.0f, 500.0f, 0.1f);

	obs_properties_add_int_slider(props, "layers", "Layers", 1, 9, 1);

	obs_properties_add_float_slider(props, "pixel_width", "Pixel Width",
					1.0, 1920.0, 1.0);
	obs_properties_add_float_slider(props, "pixel_height", "Pixel Height",
					1.0, 1080.0, 1.0);
	obs_properties_add_float_slider(props, "sub_scale_x",
					obs_module_text("Noise.Sub.Scale.X"),
					1.0, 200.0, 0.1);
	obs_properties_add_float_slider(props, "sub_scale_y",
					obs_module_text("Noise.Sub.Scale.Y"),
					1.0, 200.0, 0.1);
	obs_properties_add_float_slider(props, "sub_influence", "Sub Influence",
					0.0, 2.0, 0.01);
	obs_properties_add_float_slider(props, "sub_rotation",
					obs_module_text("Noise.Sub.Rotation"),
					-360.0, 360.0, 0.1);
	obs_properties_add_float_slider(props, "sub_displace_x",
					obs_module_text("Noise.Sub.Displace.X"),
					0.0, 4000.0, 1.0);
	obs_properties_add_float_slider(props, "sub_displace_y",
					obs_module_text("Noise.Sub.Displace.Y"),
					0.0, 4000.0, 1.0);
	return props;
}

static void noise_source_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	noise_data_t *filter = data;
	if (filter->is_filter) {
		obs_source_t *target = obs_filter_get_target(filter->context);
		if (!target) {
			return;
		}
		filter->width = (uint32_t)obs_source_get_base_width(target);
		filter->height = (uint32_t)obs_source_get_base_height(target);
	}

	filter->clock_time += seconds * filter->speed;
	filter->rendered = false;
}

static void noise_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "layers", 1);
	obs_data_set_default_int(settings, "source_width", 1920);
	obs_data_set_default_int(settings, "source_height", 1080);
	obs_data_set_default_double(settings, "sub_influence", 0.7);
	obs_data_set_default_double(settings, "speed", 100.0);
	obs_data_set_default_bool(settings, "invert", false);
	obs_data_set_default_double(settings, "sub_scale_x", 50.0);
	obs_data_set_default_double(settings, "sub_scale_y", 50.0);
	obs_data_set_default_double(settings, "brightness", 0.0);
	obs_data_set_default_double(settings, "contrast", 0.0);
}

static void draw_output(noise_data_t *filter)
{
	gs_texture_t *texture =
		gs_texrender_get_texture(filter->output_texrender);
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *param = gs_effect_get_param_by_name(pass_through, "image");
	gs_effect_set_texture(param, texture);
	uint32_t width = gs_texture_get_width(texture);
	uint32_t height = gs_texture_get_height(texture);
	while (gs_effect_loop(pass_through, "Draw")) {
		gs_draw_sprite(texture, 0, width, height);
	}
}

static void render_noise(noise_data_t *filter)
{
	gs_effect_t *effect = filter->noise_effect;

	filter->output_texrender =
		create_or_reset_texrender(filter->output_texrender);

	gs_texture_t *texture =
		gs_texrender_get_texture(filter->output_texrender);

	if (!effect) {
		return;
	}

	if (filter->time_param) {
		gs_effect_set_float(filter->time_param,
				    filter->clock_time);
	}

	if (filter->pixel_size_param) {
		gs_effect_set_vec2(filter->pixel_size_param,
				   &filter->pixel_size);
	}

	if (filter->uv_size_param) {
		gs_effect_set_vec2(filter->uv_size_param,
				   &filter->uv_size);
	}

	if (filter->layers_param) {
		gs_effect_set_int(filter->layers_param, (int)filter->layers);
	}

	if (filter->sub_influence_param) {
		gs_effect_set_float(filter->sub_influence_param, filter->sub_influence);
	}

	if (filter->noise_type_param) {
		gs_effect_set_int(filter->noise_type_param, filter->noise_type);
	}

	if (filter->noise_map_type_param) {
		gs_effect_set_int(filter->noise_map_type_param, filter->noise_map_type);
	}

	if (filter->invert_param) {
		gs_effect_set_bool(filter->invert_param, filter->invert);
	}

	if (filter->sub_scaling_param) {
		gs_effect_set_vec2(filter->sub_scaling_param,
				   &filter->sub_scaling);
	}

	if (filter->sub_displace_param) {
		gs_effect_set_vec2(filter->sub_displace_param,
				   &filter->sub_displace);
	}

	if (filter->sub_rotation_param) {
		gs_effect_set_float(filter->sub_rotation_param,
				   filter->sub_rotation);
	}

	if (filter->param_brightness) {
		gs_effect_set_float(filter->param_brightness,
				    filter->brightness);
	}

	if (filter->param_contrast) {
		gs_effect_set_float(filter->param_contrast,
				    filter->contrast);
	}

	set_blending_parameters();
	uint32_t width = filter->width;
	uint32_t height = filter->height;
	if (gs_texrender_begin(filter->output_texrender, width,
			       height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(filter->output_texrender);
	}

	gs_blend_state_pop();
}

static void render_noise_displace(noise_data_t *filter)
{
	gs_effect_t *effect = filter->noise_effect;

	gs_texture_t *texture = gs_texrender_get_texture(filter->input_texrender);
	if (!effect || !texture) {
		return;
	}

	filter->output_texrender =
		create_or_reset_texrender(filter->output_texrender);

	if (!effect) {
		return;
	}

	if (filter->param_image) {
		gs_effect_set_texture(filter->param_image, texture);
	}

	if (filter->param_displace_scale) {
		gs_effect_set_vec2(filter->param_displace_scale,
				   &filter->displace_scale);
	}

	if (filter->time_param) {
		gs_effect_set_float(filter->time_param, filter->clock_time);
	}

	if (filter->pixel_size_param) {
		gs_effect_set_vec2(filter->pixel_size_param,
				   &filter->pixel_size);
	}

	if (filter->uv_size_param) {
		gs_effect_set_vec2(filter->uv_size_param, &filter->uv_size);
	}

	if (filter->layers_param) {
		gs_effect_set_int(filter->layers_param, (int)filter->layers);
	}

	if (filter->sub_influence_param) {
		gs_effect_set_float(filter->sub_influence_param,
				    filter->sub_influence);
	}

	if (filter->noise_type_param) {
		gs_effect_set_int(filter->noise_type_param, filter->noise_type);
	}

	if (filter->noise_map_type_param) {
		gs_effect_set_int(filter->noise_map_type_param,
				  filter->noise_map_type);
	}

	if (filter->invert_param) {
		gs_effect_set_bool(filter->invert_param, filter->invert);
	}

	if (filter->sub_scaling_param) {
		gs_effect_set_vec2(filter->sub_scaling_param,
				   &filter->sub_scaling);
	}

	if (filter->sub_displace_param) {
		gs_effect_set_vec2(filter->sub_displace_param,
				   &filter->sub_displace);
	}

	if (filter->sub_rotation_param) {
		gs_effect_set_float(filter->sub_rotation_param,
				    filter->sub_rotation);
	}

	if (filter->param_brightness) {
		gs_effect_set_float(filter->param_brightness,
				    filter->brightness);
	}

	if (filter->param_contrast) {
		gs_effect_set_float(filter->param_contrast, filter->contrast);
	}

	set_blending_parameters();
	uint32_t width = filter->width;
	uint32_t height = filter->height;
	if (gs_texrender_begin(filter->output_texrender, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(filter->output_texrender);
	}

	gs_blend_state_pop();
}

static void load_noise_effect(noise_data_t *filter)
{
	const char *effect_file_path = "/shaders/noise.effect";
	filter->noise_effect =
		load_shader_effect(filter->noise_effect, effect_file_path);
	if (filter->noise_effect) {
		size_t effect_count =
			gs_effect_get_num_params(filter->noise_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->noise_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
				if (strcmp(info.name, "time") == 0) {
					filter->time_param = param;
				} else if (strcmp(info.name, "pixel_size") == 0) {
					filter->pixel_size_param = param;
				} else if (strcmp(info.name, "uv_size") == 0) {
					filter->uv_size_param = param;
				} else if (strcmp(info.name, "layers") == 0) {
					filter->layers_param = param;
				} else if (strcmp(info.name, "sub_influence") == 0) {
					filter->sub_influence_param = param;
				} else if (strcmp(info.name, "noise_type") == 0) {
					filter->noise_type_param = param;
				} else if (strcmp(info.name, "noise_map_type") == 0) {
					filter->noise_map_type_param = param;
				} else if (strcmp(info.name, "invert") == 0) {
					filter->invert_param = param;
				} else if (strcmp(info.name, "sub_scaling") == 0) {
					filter->sub_scaling_param = param;
				} else if (strcmp(info.name, "sub_displace") == 0) {
					filter->sub_displace_param = param;
				} else if (strcmp(info.name, "sub_rotation") == 0) {
					filter->sub_rotation_param = param;
				} else if (strcmp(info.name, "contrast") == 0) {
					filter->param_contrast = param;
				} else if (strcmp(info.name, "brightness") == 0) {
					filter->param_brightness= param;
				}
		}
	}
}

static void load_noise_displace_effect(noise_data_t *filter)
{
	const char *effect_file_path = "/shaders/noise_displace.effect";
	filter->noise_effect =
		load_shader_effect(filter->noise_effect, effect_file_path);
	if (filter->noise_effect) {
		size_t effect_count =
			gs_effect_get_num_params(filter->noise_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
					filter->noise_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "time") == 0) {
				filter->time_param = param;
			} else if (strcmp(info.name, "pixel_size") == 0) {
				filter->pixel_size_param = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->uv_size_param = param;
			} else if (strcmp(info.name, "layers") == 0) {
				filter->layers_param = param;
			} else if (strcmp(info.name, "sub_influence") == 0) {
				filter->sub_influence_param = param;
			} else if (strcmp(info.name, "noise_type") == 0) {
				filter->noise_type_param = param;
			} else if (strcmp(info.name, "noise_map_type") == 0) {
				filter->noise_map_type_param = param;
			} else if (strcmp(info.name, "invert") == 0) {
				filter->invert_param = param;
			} else if (strcmp(info.name, "sub_scaling") == 0) {
				filter->sub_scaling_param = param;
			} else if (strcmp(info.name, "sub_displace") == 0) {
				filter->sub_displace_param = param;
			} else if (strcmp(info.name, "sub_rotation") == 0) {
				filter->sub_rotation_param = param;
			} else if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "displace_scale") == 0) {
				filter->param_displace_scale = param;
			} else if (strcmp(info.name, "contrast") == 0) {
				filter->param_contrast = param;
			} else if (strcmp(info.name, "brightness") == 0) {
				filter->param_brightness = param;
			}
		}
	}
}

static void load_output_effect(noise_data_t *filter)
{
	if (filter->output_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->output_effect);
		filter->output_effect = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/render_output.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->output_effect =
		gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->output_effect == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load output.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->output_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
				gs_eparam_t *param = gs_effect_get_param_by_idx(
					filter->output_effect,
					effect_index);
				struct gs_effect_param_info info;
				gs_effect_get_param_info(param, &info);
				if (strcmp(info.name, "output_image") == 0) {
					filter->param_output_image =
						param;
				}
		}
	}
}
