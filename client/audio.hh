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
#ifndef WC_CLIENT_AUDIO_H
#define WC_CLIENT_AUDIO_H

#ifdef SDL
/* SDL is currently the only plugin able to modify the volume. */
#define AUDIO_VOLUME
#endif

#include "shared.hh"            /* bool type */

#include "options.hh"           /* struct client_option */

#define MAX_AUDIO_NAME_LEN              20
#define MAX_AUDIO_DESCR_LEN             200

struct audio_plugin {
  char name[MAX_AUDIO_NAME_LEN];
  char descr[MAX_AUDIO_DESCR_LEN];
  bool (*init) (void);
  void (*shutdown) (void);
  void (*stop) (void);
  void (*wait) (void);
  bool (*play) (const char *const tag, const char *const path, bool repeat);

#ifdef AUDIO_VOLUME
  int min_volume;
  int max_volume;
  void (*set_volume) (int volume);
#endif /* AUDIO_VOLUME */
};

const char **get_soundplugin_list(void);
const char **get_soundset_list(void);

void audio_init(void);
void audio_real_init(void);
void audio_add_plugin(const struct audio_plugin *p);
void audio_shutdown(void);
void audio_stop(void);

void audio_change_soundset(struct client_option *option);
void audio_change_plugin(struct client_option *option);
void audio_play_sound(const char *const tag, char *const alt_tag);
void audio_play_music(const char *const tag, char *const alt_tag);

bool audio_select_plugin(const char *const name);
const char *audio_get_all_plugin_names(void);

#ifdef AUDIO_VOLUME
#define AUDIO_VOLUME_MIN 0
#define AUDIO_VOLUME_MAX 128

void audio_set_volume(int volume);
void audio_change_volume(struct client_option *option);
#endif /* AUDIO_VOLUME */

#endif  /* WC_CLIENT_AUDIO_H */
