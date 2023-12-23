#include <obs-module.h>

#include "version.h"

extern struct obs_source_info obs_noise_source;
extern struct obs_source_info obs_noise_displace_filter;

OBS_DECLARE_MODULE();

OBS_MODULE_USE_DEFAULT_LOCALE("obs-noise", "en-US");

OBS_MODULE_AUTHOR("FiniteSingularity");

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Noise] loaded version %s", PROJECT_VERSION);
	obs_register_source(&obs_noise_source);
	obs_register_source(&obs_noise_displace_filter);
	return true;
}

void obs_module_unload(void) {}
