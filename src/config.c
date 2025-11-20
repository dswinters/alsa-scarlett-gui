#include "config.h"

// Create a hash table
static GHashTable* json_object_to_g_hash_table(JsonNode *json_node) {
  if (json_node_get_node_type(json_node) != JSON_NODE_OBJECT) {
    return NULL;
  }

  JsonObject *json_object = json_node_get_object(json_node);
  GHashTable *hash_table = g_hash_table_new_full(
                                                 g_str_hash,
                                                 g_str_equal,
                                                 g_free, // Function to free the key (g_strdup'd string)
                                                 g_free  // Function to free the value (g_strdup'd string)
                                                 );

  GList *members = json_object_get_members(json_object);
  GList *l;

  for (l = members; l != NULL; l = l->next) {
    const gchar *key = l->data;
    JsonNode *value_node = json_object_get_member(json_object, key);

    // only allow strings in the innermost level
    if (json_node_get_node_type(value_node) == JSON_NODE_VALUE &&
        json_node_get_value_type(value_node) == G_TYPE_STRING) {

      const gchar *value = json_node_get_string(value_node);

      // Insert copies of the key and value strings into the GHashTable
      g_hash_table_insert(
                          hash_table,
                          g_strdup(key),
                          g_strdup(value)
                          );
    } else {
      // Handle error or log unexpected value type
      fprintf(stderr, "Warning: Value for key '%s' is not a string, skipping.\n", key);
    }
  }

  g_list_free(members);
  return hash_table;
}


struct config *load_config() {

  const char *app_config_dir = "alsa-scarlett-gui";
  const char *config_filename = "config.json";
  gchar *config_path = g_build_filename(
    g_get_user_config_dir(),
    app_config_dir,
    config_filename,
    NULL
  );

  if (! g_file_test(config_path, G_FILE_TEST_EXISTS)) {
    return NULL;
  }

  JsonParser *parser = NULL;
  JsonNode *root = NULL;
  JsonObject *root_object = NULL;
  GError *error = NULL;
  struct config *config_ptr = NULL;

  // allocate memory for the main struct config
  config_ptr = (struct config *)g_malloc0(sizeof(struct config));

  if (config_ptr == NULL) {
    fprintf(stderr, "Error: Failed to allocate memory for config struct.\n");
    return NULL;
  }

  // load and parse the JSON file
  parser = json_parser_new();
  if (!json_parser_load_from_file(parser, config_path, &error)) {
    fprintf(stderr, "Error loading JSON file %s: %s\n", config_path, error->message);
    g_error_free(error);
    g_object_unref(parser);
    g_free(config_ptr);
    return NULL;
  }

  root = json_parser_get_root(parser);
  if (json_node_get_node_type(root) != JSON_NODE_OBJECT) {
    fprintf(stderr, "Error: JSON root is not an object.\n");
    goto cleanup;
  }
  root_object = json_node_get_object(root);

  // populate labels config
  JsonNode *labels_node = json_object_get_member(root_object, "labels");
  if (labels_node) {
    config_ptr->labels = json_object_to_g_hash_table(labels_node);
  }

 cleanup:
  g_object_unref(parser);

  return config_ptr;
}


void free_config(struct config *config_ptr) {
  if (config_ptr == NULL) return;

  // destroy the GHashTables and free their contents
  if (config_ptr->labels) {
    g_hash_table_destroy(config_ptr->labels);
  }

  // free the memory for the config struct
  g_free(config_ptr);
}

char *get_custom_label(const char *name) {
  struct config *config = (struct config *)
    g_object_get_data(G_OBJECT(g_application_get_default()), "config");

  if (config != NULL && config->labels != NULL) {
    gpointer alias = g_hash_table_lookup(config->labels, name);
    if (alias != NULL) {
      return g_strdup((const char*)alias);
    }
  }
  return NULL;
}
