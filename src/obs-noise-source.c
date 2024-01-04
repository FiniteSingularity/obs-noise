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
	filter->output_texrender =
		create_or_reset_texrender(filter->output_texrender);
	
	struct dstr filepath = {0};
	dstr_cat(&filepath, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filepath, "/presets/global_presets.json");
	filter->global_preset_data = obs_data_create_from_json_file(filepath.array);
	dstr_free(&filepath);

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

	if (filter->noise_effect) {
		gs_effect_destroy(filter->noise_effect);
	}

	if (filter->output_effect) {
		gs_effect_destroy(filter->output_effect);
	}

	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}

	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}

	if (filter->global_preset_data) {
		obs_data_release(filter->global_preset_data);
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

	filter->time = (float)obs_data_get_double(settings, "time");
	if (!filter->is_filter) {
		filter->width =
			(uint32_t)obs_data_get_int(settings, "source_width");
		filter->height =
			(uint32_t)obs_data_get_int(settings, "source_height");
	}

	filter->uv_size.x = (float)filter->width;
	filter->uv_size.y = (float)filter->height;
	filter->pixel_size.x =
		(float)obs_data_get_double(settings, "pixel_width");
	filter->pixel_size.y =
		(float)obs_data_get_double(settings, "pixel_height");
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
	filter->displace_scale.x =
		(float)obs_data_get_double(settings, "filter_displace_scale_x");
	filter->displace_scale.y =
		(float)obs_data_get_double(settings, "filter_displace_scale_y");
	filter->brightness = (float)obs_data_get_double(settings, "brightness");
	filter->contrast = (float)obs_data_get_double(settings, "contrast");
	filter->channels =
		(uint32_t)obs_data_get_int(settings, "noise_channels");
	filter->billow = obs_data_get_bool(settings, "billow");
	filter->ridged = obs_data_get_bool(settings, "ridged");
	filter->power = (float)obs_data_get_double(settings, "power");
	filter->global_rotation = (float)obs_data_get_double(settings, "base_rotation") * M_PI / 180.0f;

	double sum_influence = 0.0;
	//double std_scale = 0.0;
	double var = 0.0;
	for (int i = 0; i < (int)filter->layers; i++) {
		double influence = pow((double)filter->sub_influence, (double)i);
		double var_factor = influence * influence;
		var += var_factor;
		sum_influence += influence;
	}
	double std = sqrt(var);
	filter->sum_influence = (float)sum_influence;
	filter->std_scale = (1.0f-0.05f*((float)filter->layers-1.0f))*(float)(sum_influence/std);
	filter->comb_max =
		(uint32_t)obs_data_get_int(settings, "layer_combo_type") ==
		NOISE_LAYER_MAX;
	filter->dw_iterations = (int)obs_data_get_int(settings, "dw_iterations");
	filter->dw_strength.x = (float)obs_data_get_double(settings, "dw_strength_x");
	filter->dw_strength.y = (float)obs_data_get_double(settings, "dw_strength_y");
	if (!filter->is_filter) {
		vec4_from_rgba(&filter->map_color_1,
			       (uint32_t)obs_data_get_int(settings,
							  "map_color_1"));
		vec4_from_rgba(&filter->map_color_2,
			       (uint32_t)obs_data_get_int(settings,
							  "map_color_2"));
	}

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

//static bool tmp_export_clicked(obs_properties_t* props,
//	obs_property_t* property, void* data)
//{
//	noise_data_t *filter = data;
//	obs_data_t *settings = obs_source_get_settings(filter->context);
//
//	obs_data_t *output = obs_data_get_defaults(settings);
//	obs_data_apply(output, settings);
//	obs_data_unset_user_value(output, "presets");
//	obs_data_unset_user_value(output, "source_width");
//	obs_data_unset_user_value(output, "source_height");
//	//noise_source_defaults(settings);
//	const char *json = obs_data_get_json(output);
//	blog(LOG_INFO, "Clicked!\n%s", json);
//	obs_data_release(output);
//	obs_data_release(settings);
//	return false;
//}

static obs_properties_t *noise_source_properties(void *data)
{
	noise_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);
	obs_property_t *p = NULL;
	if (!filter->is_filter) {
		obs_properties_t *source_dimensions = obs_properties_create();

		p = obs_properties_add_int(source_dimensions, "source_width",
					   obs_module_text("Noise.Width"), 0,
					   8000, 1);
		obs_property_int_set_suffix(p, "px");
		p = obs_properties_add_int(source_dimensions, "source_height",
					   obs_module_text("Noise.Height"), 0,
					   8000, 1);
		obs_property_int_set_suffix(p, "px");
		obs_properties_add_group(
			props, "source_dimensions",
			obs_module_text("Noise.SourceProperties"),
			OBS_GROUP_NORMAL, source_dimensions);
	} else {
		obs_properties_t *displacement_group = obs_properties_create();

		p = obs_properties_add_float_slider(
			displacement_group, "filter_displace_scale_x",
			obs_module_text("Noise.Displacement.ScaleX"), 0.0,
			400.0, 0.1);
		obs_property_float_set_suffix(p, "px");
		p = obs_properties_add_float_slider(
			displacement_group, "filter_displace_scale_y",
			obs_module_text("Noise.Displacement.ScaleY"), 0.0,
			400.0, 0.1);
		obs_property_float_set_suffix(p, "px");

		obs_properties_add_group(props, "displacement_group",
					 obs_module_text("Noise.Displacement"),
					 OBS_GROUP_NORMAL, displacement_group);
	}

	obs_property_t *presets_list = obs_properties_add_list(
		props, "presets", obs_module_text("Noise.Presets"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_data_array_t *data_array =
		obs_data_get_array(filter->global_preset_data, "presets");

	obs_property_list_add_int(presets_list, obs_module_text("Noise.Custom"), 0);

	for (size_t i = 0; i < obs_data_array_count(data_array); i++) {
		obs_data_t *preset = obs_data_array_item(data_array, i);
		const char *name = obs_data_get_string(preset, "name");
		obs_property_list_add_int(presets_list, name, i+1);
		obs_data_release(preset);
	}

	obs_data_array_release(data_array);

	obs_property_set_modified_callback2(presets_list, setting_preset_selected, data);

	obs_properties_t *general_noise_group = obs_properties_create();
	obs_property_t *noise_type_list = obs_properties_add_list(
		general_noise_group, "noise_type",
		obs_module_text("Noise.Type"),
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
	obs_property_t *noise_channels_list = NULL;
	if (!filter->is_filter) {
		noise_channels_list = obs_properties_add_list(
			general_noise_group, "noise_channels",
			obs_module_text("Noise.Channels"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);

		obs_property_list_add_int(
			noise_channels_list,
			obs_module_text(NOISE_CHANNELS_COLOR_MAP_LABEL),
			NOISE_CHANNELS_COLOR_MAP);
		obs_property_list_add_int(
			noise_channels_list,
			obs_module_text(NOISE_CHANNELS_1_LABEL),
			NOISE_CHANNELS_1);
		obs_property_list_add_int(
			noise_channels_list,
			obs_module_text(NOISE_CHANNELS_2_LABEL),
			NOISE_CHANNELS_2);
		obs_property_list_add_int(
			noise_channels_list,
			obs_module_text(NOISE_CHANNELS_3_LABEL),
			NOISE_CHANNELS_3);
	}

	obs_properties_add_bool(general_noise_group, "billow",
				obs_module_text("Noise.Billow"));
	obs_properties_add_bool(general_noise_group, "ridged",
				obs_module_text("Noise.Ridged"));
	obs_properties_add_float_slider(general_noise_group, "brightness",
					obs_module_text("Noise.Brightness"),
					-1.0, 1.0, 0.01);

	obs_properties_add_float_slider(general_noise_group, "contrast",
					obs_module_text("Noise.Contrast"), -1.0,
					1.0, 0.01);
	obs_properties_add_group(props, "general_noise_group",
				 obs_module_text("Noise.GeneralProperties"),
				 OBS_GROUP_NORMAL, general_noise_group);

	if (!filter->is_filter) {
		obs_properties_t *color_map_group = obs_properties_create();
		
		obs_properties_add_color_alpha(
			color_map_group, "map_color_1",
			obs_module_text("Noise.ColorMap.Color1"));

		obs_properties_add_color_alpha(
			color_map_group, "map_color_2",
			obs_module_text("Noise.ColorMap.Color2"));

		obs_properties_add_group(
			props, "color_map_group",
			obs_module_text("Noise.ColorMap"),
			OBS_GROUP_NORMAL, color_map_group);
		obs_property_set_modified_callback(noise_channels_list,
						   setting_channels_modified);
	}


	obs_properties_t *transform_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		transform_group, "base_rotation",
		obs_module_text("Noise.Transform.BaseRotation"), -360.0, 360.0, 0.1);
	obs_property_float_set_suffix(p, "deg");
	p = obs_properties_add_float_slider(transform_group, "pixel_width",
					obs_module_text("Noise.Transform.BasePixelWidth"),
					1.0, 1920.0, 1.0);
	obs_property_float_set_suffix(p, "px");
	p = obs_properties_add_float_slider(transform_group, "pixel_height",
					obs_module_text("Noise.Transform.BasePixelHeight"),
					1.0, 1080.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	obs_properties_add_group(props, "transform_group",
				 obs_module_text("Noise.Transform"),
				 OBS_GROUP_NORMAL, transform_group);

	obs_properties_t *complexity_group = obs_properties_create();
	obs_properties_add_int_slider(complexity_group, "layers", obs_module_text("Noise.Complexity.Layers"), 1,
				      9, 1);

	obs_property_t *layer_combination_type = obs_properties_add_list(
		complexity_group, "layer_combo_type",
		obs_module_text("Noise.LayerComb"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(layer_combination_type,
				  obs_module_text(NOISE_LAYER_WEIGHTED_AVERAGE_LABEL),
				  NOISE_LAYER_WEIGHTED_AVERAGE);
	obs_property_list_add_int(layer_combination_type,
				  obs_module_text(NOISE_LAYER_MAX_LABEL),
				  NOISE_LAYER_MAX);

	obs_properties_add_float_slider(
		complexity_group, "power", obs_module_text("Noise.Exponent"), 0.0, 3.0, 0.1);


	obs_properties_add_group(props, "complexity_group",
				 obs_module_text("Noise.Complexity"),
				 OBS_GROUP_NORMAL, complexity_group);

	obs_properties_t *subscale_group = obs_properties_create();

	obs_properties_add_float_slider(subscale_group, "sub_scale_x",
					obs_module_text("Noise.Sub.Scale.X"),
					1.0, 200.0, 0.1);
	obs_properties_add_float_slider(subscale_group, "sub_scale_y",
					obs_module_text("Noise.Sub.Scale.Y"),
					1.0, 200.0, 0.1);
	obs_properties_add_float_slider(subscale_group, "sub_influence",
					obs_module_text("Nose.Sub.Influence"), 0.0, 2.0, 0.01);
	obs_properties_add_float_slider(subscale_group, "sub_rotation",
					obs_module_text("Noise.Sub.Rotation"),
					-360.0, 360.0, 0.1);
	obs_properties_add_float_slider(subscale_group, "sub_displace_x",
					obs_module_text("Noise.Sub.Displace.X"),
					0.0, 4000.0, 1.0);
	obs_properties_add_float_slider(subscale_group, "sub_displace_y",
					obs_module_text("Noise.Sub.Displace.Y"),
					0.0, 4000.0, 1.0);

	obs_properties_add_group(props, "subscale_group",
				 obs_module_text("Noise.Sub"),
				 OBS_GROUP_NORMAL, subscale_group);

	obs_properties_t *domain_warping_group = obs_properties_create();

	obs_properties_add_int_slider(
		domain_warping_group, "dw_iterations",
		obs_module_text("Noise.DomainWarping.Iterations"), 0, 6, 1);

	obs_properties_add_float_slider(domain_warping_group, "dw_strength_x",
					obs_module_text("Noise.DomainWarping.StrengthX"),
					0.0, 250.0, 1.0);

	obs_properties_add_float_slider(domain_warping_group, "dw_strength_y",
					obs_module_text("Noise.DomainWarping.StrengthY"), 0.0, 250.0,
					1.0);

	obs_properties_add_group(props, "domain_warping_group",
				 obs_module_text("Noise.DomainWarping"),
				 OBS_GROUP_NORMAL, domain_warping_group);


	obs_properties_t *evolution_group = obs_properties_create();

	obs_properties_add_float_slider(evolution_group, "speed", "Speed", 0.0f,
					9001.0f,
					0.1f);
	obs_properties_add_group(props, "evolution_group",
				 obs_module_text("Noise.Evolution"), OBS_GROUP_NORMAL,
				 evolution_group);

	obs_properties_add_text(props, "plugin_info", PLUGIN_INFO,
				OBS_TEXT_INFO);

	//obs_properties_add_button2(props, "export_btn", "Temp Export",
	//			  tmp_export_clicked, data);

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

static bool setting_channels_modified(obs_properties_t *props,
				    obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	int output = (int)obs_data_get_int(settings, "noise_channels");
	if (output == NOISE_CHANNELS_COLOR_MAP) {
		setting_visibility("color_map_group", true, props);
	} else {
		setting_visibility("color_map_group", false, props);
	}
	 
	return true;
}

static bool setting_preset_selected(void *data, obs_properties_t *props,
					 obs_property_t *p,
					 obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(props);
	noise_data_t *filter = data;

	size_t index = (size_t)obs_data_get_int(settings, "presets");

	if (index == 0) {
		return false;
	}

	obs_data_array_t *data_array =
		obs_data_get_array(filter->global_preset_data, "presets");

	obs_data_t *preset = obs_data_array_item(data_array, index-1);
	obs_data_t *preset_settings = obs_data_get_obj(preset, "settings");

	obs_data_apply(settings, preset_settings);

	obs_data_release(preset_settings);
	obs_data_release(preset);
	obs_data_array_release(data_array);

	return true;
}

static void noise_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "layers", 4);
	obs_data_set_default_int(settings, "source_width", 1920);
	obs_data_set_default_int(settings, "source_height", 1080);
	obs_data_set_default_bool(settings, "billow", false);
	obs_data_set_default_bool(settings, "ridged", false);
	obs_data_set_default_double(settings, "power", 1.0);
	obs_data_set_default_double(settings, "sub_influence", 0.7);
	obs_data_set_default_double(settings, "speed", 100.0);
	obs_data_set_default_bool(settings, "invert", false);
	obs_data_set_default_double(settings, "sub_scale_x", 50.0);
	obs_data_set_default_double(settings, "sub_scale_y", 50.0);
	obs_data_set_default_double(settings, "sub_rotation", 0.0);
	obs_data_set_default_double(settings, "sub_displace_x", 0.0);
	obs_data_set_default_double(settings, "sub_displace_y", 0.0);
	obs_data_set_default_double(settings, "brightness", 0.0);
	obs_data_set_default_double(settings, "contrast", 0.0);
	obs_data_set_default_double(settings, "base_rotation", 0.0);
	obs_data_set_default_double(settings, "pixel_width", 64.0);
	obs_data_set_default_double(settings, "pixel_height", 64.0);
	obs_data_set_default_int(settings, "noise_channels", NOISE_CHANNELS_COLOR_MAP);
	obs_data_set_default_int(settings, "noise_type", NOISE_TYPE_SMOOTHSTEP);
	obs_data_set_default_int(settings, "layer_combo_type", NOISE_LAYER_WEIGHTED_AVERAGE);
	obs_data_set_default_double(settings, "dw_strength_x", 25.0);
	obs_data_set_default_double(settings, "dw_strength_y", 25.0);
	obs_data_set_default_int(settings, "dw_iterations", 0);
	obs_data_set_default_int(settings, "map_color_1", DEFAULT_MAP_COLOR_1);
	obs_data_set_default_int(settings, "map_color_2", DEFAULT_MAP_COLOR_2);
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

	if (filter->param_billow) {
		gs_effect_set_bool(filter->param_billow, filter->billow);
	}

	if (filter->param_ridged) {
		gs_effect_set_bool(filter->param_ridged, filter->ridged);
	}

	if (filter->param_power) {
		gs_effect_set_float(filter->param_power, filter->power);
	}

	if (filter->param_sum_influence) {
		gs_effect_set_float(filter->param_sum_influence, filter->sum_influence);
	}

	if (filter->param_std_scale) {
		gs_effect_set_float(filter->param_std_scale, filter->std_scale);
	}

	if (filter->param_dw_iterations) {
		gs_effect_set_int(filter->param_dw_iterations, filter->dw_iterations);
	}

	if (filter->param_dw_strength) {
		gs_effect_set_vec2(filter->param_dw_strength, &filter->dw_strength);
	}

	if (filter->param_global_rotation) {
		gs_effect_set_float(filter->param_global_rotation,
				    filter->global_rotation);
	}

	if (filter->channels == 0) {
		if (filter->param_color_1) {
			gs_effect_set_vec4(filter->param_color_1,
					   &filter->map_color_1);
		}
		if (filter->param_color_2) {
			gs_effect_set_vec4(filter->param_color_2,
					   &filter->map_color_2);
		}
	}

	set_blending_parameters();
	uint32_t width = filter->width;
	uint32_t height = filter->height;
	//const char *technique = filter->channels == 1 ? "Draw1"
	//			: filter->channels == 2 ? "Draw2"
	//						: "Draw3";
	struct dstr technique;
	dstr_init_copy(&technique, "Draw");
	dstr_catf(&technique, "%i", filter->channels);
	if (filter->comb_max) {
		dstr_cat(&technique, "Max");
	}
	if (gs_texrender_begin(filter->output_texrender, width,
			       height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique.array))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(filter->output_texrender);
	}
	dstr_free(&technique);
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

	if (filter->param_billow) {
		gs_effect_set_bool(filter->param_billow, filter->billow);
	}

	if (filter->param_ridged) {
		gs_effect_set_bool(filter->param_ridged, filter->ridged);
	}

	if (filter->param_power) {
		gs_effect_set_float(filter->param_power, filter->power);
	}

	if (filter->param_sum_influence) {
		gs_effect_set_bool(filter->param_sum_influence, filter->sum_influence);
	}

	if (filter->param_std_scale) {
		gs_effect_set_float(filter->param_std_scale, filter->std_scale);
	}

	if (filter->param_dw_iterations) {
		gs_effect_set_int(filter->param_dw_iterations, filter->dw_iterations);
	}

	if (filter->param_dw_strength) {
		gs_effect_set_vec2(filter->param_dw_strength, &filter->dw_strength);
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
				filter->param_brightness = param;
			} else if (strcmp(info.name, "billow") == 0) {
				filter->param_billow = param;
			} else if (strcmp(info.name, "ridged") == 0) {
				filter->param_ridged = param;
			} else if (strcmp(info.name, "power") == 0) {
				filter->param_power = param;
			} else if (strcmp(info.name, "sum_influence") == 0) {
				filter->param_sum_influence = param;
			} else if (strcmp(info.name, "std_scale") == 0) {
				filter->param_std_scale = param;
			} else if (strcmp(info.name, "dw_iterations") == 0) {
				filter->param_dw_iterations = param;
			} else if (strcmp(info.name, "dw_strength") == 0) {
				filter->param_dw_strength = param;
			} else if (strcmp(info.name, "global_rotation") == 0) {
				filter->param_global_rotation = param;
			} else if (strcmp(info.name, "color_1") == 0) {
				filter->param_color_1 = param;
			} else if (strcmp(info.name, "color_2") == 0) {
				filter->param_color_2 = param;
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
			} else if (strcmp(info.name, "billow") == 0) {
				filter->param_billow = param;
			} else if (strcmp(info.name, "ridged") == 0) {
				filter->param_ridged = param;
			} else if (strcmp(info.name, "power") == 0) {
				filter->param_power = param;
			} else if (strcmp(info.name, "sum_influence") == 0) {
				filter->param_sum_influence = param;
			} else if (strcmp(info.name, "std_scale") == 0) {
				filter->param_std_scale = param;
			} else if (strcmp(info.name, "dw_iterations") == 0) {
				filter->param_dw_iterations = param;
			} else if (strcmp(info.name, "dw_strength") == 0) {
				filter->param_dw_strength = param;
			} else if (strcmp(info.name, "global_rotation") == 0) {
				filter->param_global_rotation = param;
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
