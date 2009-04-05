/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

#ifdef AMIGA
#include "audio_amiga.h"
#endif
#ifdef ESD
#include "audio_esd.h"
#endif
#include "audio_none.h"
#ifdef SDL
#include "audio_sdl.h"
#endif
#ifdef WINMM
#include "audio_winmm.h"
#endif
#ifdef ALSA
#include "audio_alsa.h"
#endif

#include "audio.h"

#define MAX_NUM_PLUGINS		4
#define SNDSPEC_SUFFIX		".soundspec"

/* keep it open throughout */
static struct section_file tagstruct, *tagfile = NULL;

static struct audio_plugin plugins[MAX_NUM_PLUGINS];
static struct audio_plugin *selected_plugin = NULL;
static int num_plugins_used = 0;

/**********************************************************************
  Returns a static, NULL-terminated list of all sound plugins
  available on the system.  This function is unfortunately similar to
  audio_get_all_plugin_names().
***********************************************************************/
const char **get_soundplugin_list(void)
{
  static const char* plugin_list[MAX_NUM_PLUGINS + 1];
  int i;
  
  for (i = 0; i < num_plugins_used; i++) {
    plugin_list[i] = plugins[i].name;
  }
  assert(i <= MAX_NUM_PLUGINS);
  plugin_list[i] = NULL;

  return plugin_list;
}

/**********************************************************************
  Returns a static list of soundsets available on the system by
  searching all data directories for files matching SNDSPEC_SUFFIX.
  The list is NULL-terminated.
***********************************************************************/
const char **get_soundset_list(void)
{
  static const char **audio_list = NULL;

  if (!audio_list) {
    /* Note: this means you must restart the client after installing a new
       soundset. */
    audio_list = datafilelist(SNDSPEC_SUFFIX);
  }

  return audio_list;
}

/**************************************************************************
  Add a plugin.
**************************************************************************/
void audio_add_plugin(const struct audio_plugin *p)
{
  assert(num_plugins_used < MAX_NUM_PLUGINS);
  memcpy(&plugins[num_plugins_used], p, sizeof(struct audio_plugin));
  num_plugins_used++;
}

/**************************************************************************
  Choose plugin. Returns TRUE on success, FALSE if not
**************************************************************************/
bool audio_select_plugin(const char *const name)
{
  struct audio_plugin *plugin;
  int i;

  for (i = 0, plugin = plugins; i < num_plugins_used; i++, plugin++) {
    if (strcmp(plugin->name, name) == 0) {
      break;
    }
  }

  if (i >= num_plugins_used) {
    freelog(LOG_ERROR,
	    "Plugin '%s' isn't available. Available are %s", name,
	    audio_get_all_plugin_names());
    return FALSE;
  }

  if (plugin == selected_plugin) {
    /* We already selected this one */
    return TRUE;
  }

  if (selected_plugin) {
    freelog(LOG_DEBUG, "Shutting down %s", selected_plugin->name);
    selected_plugin->stop();
    selected_plugin->wait();
    selected_plugin->shutdown();
  }

  if (!plugin->init()) {
    freelog(LOG_ERROR, "Plugin %s found but can't be initialized.", name);
    return FALSE;
  }

  selected_plugin = plugin;
  freelog(LOG_NORMAL, _("Plugin '%s' is now selected"),
	  selected_plugin->name);
  return TRUE;
}

/**************************************************************************
  Initialize base audio system. Note that this function is called very
  early at the client startup. So for example logging isn't available.
**************************************************************************/
void audio_init()
{
  audio_none_init();
  assert(num_plugins_used == 1);

#ifdef ESD
  audio_esd_init();
#endif
#ifdef SDL
  audio_sdl_init();
#endif
#ifdef ALSA
  audio_alsa_init();
#endif
#ifdef WINMM
  audio_winmm_init();
#endif
#ifdef AMIGA
  audio_amiga_init();
#endif
}

/**************************************************************************
  Returns the filename for the given soundset. Returns NULL if
  soundset couldn't be found. Caller has to free the return value.
**************************************************************************/
static char *soundspec_fullname(char *soundset_name, size_t size)
{
  const char *soundset_default = "stdsounds";	/* Do not i18n! */
  char fname[strlen(soundset_name) + strlen(SNDSPEC_SUFFIX) + 1];
  char *dname;

  sprintf(fname, "%s%s", soundset_name, SNDSPEC_SUFFIX);

  dname = datafilename(fname);

  if (dname) {
    return mystrdup(dname);
  }

  if (strcmp(soundset_name, soundset_default) == 0) {
    /* avoid endless recursion */
    return NULL;
  }

  freelog(LOG_ERROR, "Couldn't find soundset \"%s\" trying \"%s\".",
	  soundset_name, soundset_default);
  mystrlcpy(soundset_name, soundset_default, size);
  return soundspec_fullname(soundset_name, size);
}

/**************************************************************************
  Select the right plugin or set plugin_name to "none".
**************************************************************************/
static void audio_set_plugin(char *plugin_name, size_t size)
{
  if (*plugin_name == '\0' || !audio_select_plugin(plugin_name)) {
    mystrlcpy(plugin_name, "none", size);
    assert(audio_select_plugin(plugin_name)); /* Must always work! */
  }

  if (strcmp(plugin_name, "none") == 0) {
    freelog(LOG_VERBOSE, "Proceeding with sound support disabled");
  }
}

/**************************************************************************
  Select the right soundset spec file or set spec_name "".
**************************************************************************/
static void audio_set_soundset(char *spec_name, size_t size)
{
  char *filename;
  char *file_capstr;
  char us_capstr[] = "+soundspec";

  freelog(LOG_VERBOSE, "Initializing sound using %s...", spec_name);
  filename = soundspec_fullname(spec_name, size);

  if (tagfile) {
    section_file_free(tagfile);
  } else {
    tagfile = &tagstruct;
  }

  if (!filename) {
    freelog(LOG_ERROR, "Cannot find sound spec-file \"%s\".", spec_name);
    freelog(LOG_NORMAL, _("To get sound you need to download a sound set!"));
    freelog(LOG_NORMAL, _("Get sound sets from <%s>."),
	    "ftp://ftp.freeciv.org/freeciv/contrib/audio/soundsets");
    freelog(LOG_NORMAL, _("Will continue with disabled sounds."));
    *spec_name = '\0';
    tagfile = NULL;
    return;
  }

  if (!section_file_load(tagfile, filename)) {
    freelog(LOG_ERROR, "Could not load sound spec-file: %s", filename);
    free(filename);
    *spec_name = '\0';
    tagfile = NULL;
    return;
  }

  free(filename);

  file_capstr = secfile_lookup_str(tagfile, "soundspec.options");
  if (!has_capabilities(us_capstr, file_capstr)) {
    freelog(LOG_NORMAL, _("sound spec-file appears incompatible:"));
    freelog(LOG_NORMAL, _("file: \"%s\""), filename);
    freelog(LOG_NORMAL, _("file options: %s"), file_capstr);
    freelog(LOG_NORMAL, _("supported options: %s"), us_capstr);
    *spec_name = '\0';
    section_file_free(tagfile);
    tagfile = NULL;
    return;
  }

  if (!has_capabilities(file_capstr, us_capstr)) {
    freelog(LOG_NORMAL, _("sound spec-file claims required option(s)"
			 " which we don't support:"));
    freelog(LOG_NORMAL, _("file: \"%s\""), filename);
    freelog(LOG_NORMAL, _("file options: %s"), file_capstr);
    freelog(LOG_NORMAL, _("supported options: %s"), us_capstr);
    *spec_name = '\0';
    section_file_free(tagfile);
    tagfile = NULL;
    return;
  }

  freelog(LOG_VERBOSE, "soundset is %s.", spec_name);
}

/**************************************************************************
  Initialize audio system and autoselect a plugin
**************************************************************************/
void audio_real_init(void)
{
  if (strcmp(default_sound_plugin_name, "none") != 0 && num_plugins_used == 1) {
    /* We only have the dummy plugin, skip the code but issue an advertise */
    freelog(LOG_NORMAL, _("No real audio plugin present, "
      "proceeding with sound support disabled"));
    freelog(LOG_NORMAL,
      _("For sound support, install either esound or SDL_mixer"));
    freelog(LOG_NORMAL,
      _("Esound: http://www.tux.org/~ricdude/EsounD.html"));
    freelog(LOG_NORMAL, _("SDL_mixer: http://www.libsdl.org/"
      "projects/SDL_mixer/index.html"));
  }

  audio_set_plugin(default_sound_plugin_name,
		   sizeof(default_sound_plugin_name));
  audio_set_soundset(default_sound_set_name,
		     sizeof(default_sound_set_name));
#ifdef AUDIO_VOLUME
  audio_set_volume(sound_volume);
#endif /* AUDIO_VOLUME */

  atexit(audio_shutdown);
}

/**************************************************************************
  ...
**************************************************************************/
void audio_change_soundset(struct client_option *option)
{
  audio_set_soundset(option->string.pvalue, option->string.size);
}

/**************************************************************************
  ...
**************************************************************************/
void audio_change_plugin(struct client_option *option)
{
  audio_set_plugin(option->string.pvalue, option->string.size);
}

/**************************************************************************
  INTERNAL. Returns TRUE for success.
**************************************************************************/
static bool audio_play_tag(const char *tag, bool repeat)
{
  char *soundfile, *fullpath = NULL;

  if (!tag || strcmp(tag, "-") == 0) {
    return FALSE;
  }

  if (tagfile) {
    soundfile = secfile_lookup_str_default(tagfile, "-", "files.%s", tag);
    if (strcmp(soundfile, "-") == 0) {
      freelog(LOG_VERBOSE, "No sound file for tag %s (file %s)", tag,
	      soundfile);
    } else {
      fullpath = datafilename(soundfile);
      if (!fullpath) {
	freelog(LOG_ERROR, _("Cannot find audio file %s"), soundfile);
      }
    }
  }

  return selected_plugin->play(tag, fullpath, repeat);
}

/**************************************************************************
  Play an audio sample as suggested by sound tags
**************************************************************************/
void audio_play_sound(const char *const tag, char *const alt_tag)
{
  char *pretty_alt_tag = alt_tag ? alt_tag : "(null)";

  assert(tag != NULL);

  freelog(LOG_DEBUG, "audio_play_sound('%s', '%s')", tag, pretty_alt_tag);

  /* try playing primary tag first, if not go to alternative tag */
  if (!audio_play_tag(tag, FALSE) && !audio_play_tag(alt_tag, FALSE)) {
    freelog(LOG_VERBOSE, "Neither of tags %s or %s found", tag,
	    pretty_alt_tag);
  }
}

/**************************************************************************
  Loop sound sample as suggested by sound tags
**************************************************************************/
void audio_play_music(const char *const tag, char *const alt_tag)
{
  char *pretty_alt_tag = alt_tag ? alt_tag : "(null)";

  assert(tag != NULL);

  freelog(LOG_DEBUG, "audio_play_music('%s', '%s')", tag, pretty_alt_tag);

  /* try playing primary tag first, if not go to alternative tag */
  if (!audio_play_tag(tag, TRUE) && !audio_play_tag(alt_tag, TRUE)) {
    freelog(LOG_VERBOSE, "Neither of tags %s or %s found", tag,
	    pretty_alt_tag);
  }
}

/**************************************************************************
  Stop looping sound. Music should die down in a few seconds.
**************************************************************************/
void audio_stop()
{
  selected_plugin->stop();
}

/**************************************************************************
  Call this at end of program only.
**************************************************************************/
void audio_shutdown()
{
  /* avoid infinite loop at end of game */
  audio_stop();

  audio_play_sound("e_game_quit", NULL);
  selected_plugin->wait();
  selected_plugin->shutdown();

  if (tagfile) {
    section_file_free(tagfile);
    tagfile = NULL;
  }
}

/**************************************************************************
  Returns a string which list all available plugins. You don't have to
  free the string.
**************************************************************************/
const char *audio_get_all_plugin_names()
{
  static char buffer[100];
  int i;

  sz_strlcpy(buffer, "[");

  for (i = 0; i < num_plugins_used; i++) {
    sz_strlcat(buffer, plugins[i].name);
    if (i != num_plugins_used - 1) {
      sz_strlcat(buffer, ", ");
    }
  }
  sz_strlcat(buffer, "]");
  return buffer;
}

#ifdef AUDIO_VOLUME
/**************************************************************************
  Set the volume level.
**************************************************************************/
void audio_set_volume(int volume)
{
  if (!selected_plugin || !selected_plugin->set_volume) {
    /* Cannot set volume */
    return;
  }

  assert(AUDIO_VOLUME_MIN < AUDIO_VOLUME_MAX);
  assert(selected_plugin->min_volume < selected_plugin->max_volume);

  /* Unsure we are in a correct scale */
  if (volume > AUDIO_VOLUME_MAX) {
    volume = AUDIO_VOLUME_MAX;
  } else if (volume < AUDIO_VOLUME_MIN) {
    volume = AUDIO_VOLUME_MIN;
  }

  /* Modify the value to be in the plugin scale */
  volume = selected_plugin->min_volume
    + ((volume - AUDIO_VOLUME_MIN)
       * (selected_plugin->max_volume - selected_plugin->min_volume))
      / (AUDIO_VOLUME_MAX - AUDIO_VOLUME_MIN);

  assert(volume >= selected_plugin->min_volume
	 && volume <= selected_plugin->max_volume);

  /* Really set the volume now */
  selected_plugin->set_volume(volume);
}

/**************************************************************************
  Set the volume level, option callback.
**************************************************************************/
void audio_change_volume(struct client_option *option)
{
  audio_set_volume(*option->integer.pvalue);
}
#endif /* AUDIO_VOLUME */
