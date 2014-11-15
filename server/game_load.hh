#ifndef WC_SERVER_GAME_LOAD_H
#define WC_SERVER_GAME_LOAD_H

#include "registry.h"

#define REQUIERED_GAME_VERSION 10900

void game_load(struct section_file *file);
bool game_loadmap(struct section_file *file);

#endif  /* WC_SERVER_GAME_LOAD_H */
