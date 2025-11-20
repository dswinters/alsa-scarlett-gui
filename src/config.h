#pragma once

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <json-glib/json-glib.h>

// user config struct
struct config{
  GHashTable* labels;
};

struct config *load_config();

void free_config(struct config *config_ptr);

char *get_custom_label(const char *name);
