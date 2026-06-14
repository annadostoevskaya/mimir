
#include "mimir_core.c"
#include ""

struct mimir_string {
    char *buffer;
    size_t length;
};

struct mimir_arena_allocator global_arena = { 0 };

enum mimir_policy_source_type {
    MIMIR_POLICY_SOURCE_TYPE_NONE = 0,
    MIMIR_POLICY_SOURCE_TYPE_FILE_SYSTEM,

    MIMIR_POLICY_SOURCE_TYPE_COUNT
};

enum mimir_policy_repository_type {
    MIMIR_POLICY_REPOSITORY_TYPE_NONE = 0,
    MIMIR_POLICY_REPOSITORY_TYPE_FILE_SYSTEM,

    MIMIR_POLICY_REPOSITORY_TYPE_COUNT
};

enum mimir_policy_engine {
    MIMIR_POLICY_ENGINE_NONE = 0,
    MIMIR_POLICY_ENGINE_RSYNC,

    MIMIR_POLICY_ENGINE_COUNT
};

struct mimir_policy {
    enum mimir_policy_source_type source_type;
    enum mimir_policy_repository_type repository_type;
    enum mimir_policy_engine engine;
    struct mimir_string source_path;
    struct mimir_string repository_path;
};

struct mimir_config_entry {
    struct mimir_string section;
    struct mimir_string key;
    struct mimir_string value;
};

int mimir_loading_policy(const struct mimir_string *content, struct mimir_policy *policy) {

}

