 /*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: linux_mimira.c
 * Created: 2026-06-16 02:46:12
 * Last updated: 2026-06-24 03:18:03
 * Description:
 * License: $LICENSE
 */

#include <stddef.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* BEEGIN: mimir_core.c */

struct mimir_slice {
    char *start;
    char *end;
};

ssize_t mimir_write(int fd, const char* s) {
    ssize_t len = 0;

    while (s[len] != '\0')
        len++;

    return write(fd, s, len);
}

void mimir_error(const char* error_message) {
    mimir_write(STDERR_FILENO, "[mimir] ");
    mimir_write(STDERR_FILENO, error_message);
    mimir_write(STDERR_FILENO, "\n");
}

void *mimir_malloc(size_t size) {
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
        mimir_error("mmap() failed");
        return NULL;
    }

    return addr;
}

int mimir_free(void *addr, size_t size) {
    int success = 0;

    success = munmap(addr, size);

    if (success != 0) {
        mimir_error("munmap() failed");
        return -1;
    }

    return 0;
}

/* END: mimir_core.c */

#include "mimir_arena.c"

struct mimir_arena_allocator g_mimir_arena = {};

/* BEGIN: mimir_slice.c */

static char *mimir_slice_to_cstring(struct mimir_slice src) {
    size_t len = src.end - src.start;

    char *dest = mimir_arena_malloc(&g_mimir_arena, len + 1);
    if (dest == NULL) {
        return NULL;
    }

    memcpy(dest, src.start, len);
    dest[len] = '\0';

    return dest;
}

static void mimir_print_slice(int fd, struct mimir_slice s) {
    write(fd, s.start, s.end - s.start);
    write(fd, "\n", 1);
}

/* END: mimir_slice.c */

#include "mimir_policy_loader.c"
#include "mimir_policy_parser.c"

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

enum mimir_policy_driver_type {
    MIMIR_POLICY_DRIVER_NONE = 0,
    MIMIR_POLICY_DRIVER_RSYNC,

    MIMIR_POLICY_DRIVER_COUNT
};
/*
enum mimir_policy_artifact_type {
    MIMIR_ARTIFACT_NONE = 0,
    MIMIR_ARTIFACT_FS_PATH ,
    MIMIR_ARTIFACT_STREAM,
    MIMIR_ARTIFACT_BLOCK_DEVICE,

    MIMIR_ARTIFACT_COUNT
};

struct mimir_policy_artifact {
    enum mimir_policy_artifact_type type;

    union {
        struct {
            char *path;
        } fs;

        struct {
            int fd;
        } stream;

        struct {
            char *path;
        } block_device;
    };
};

struct mimir_policy     // struct mimir_policy_capture capture;
*/

struct mimir_policy_source_fs_config {
    char *path;
};

struct mimir_policy_repository_fs_config {
    char *path;
};

struct mimir_policy_driver_rsync_config {
    int archive;
    int delete;
    int hardlinks;
    int acl;
    int xattrs;
    int numeric_ids;
};

struct mimir_policy_source {
    enum mimir_policy_source_type type;
    void *config;
    const struct mimir_policy_source_vtable *vtable;
};

struct mimir_policy_repository {
    enum mimir_policy_repository_type type;
    void *config;
    const struct mimir_policy_repository_vtable *vtable;
};

struct mimir_policy_driver {
    enum mimir_policy_driver_type type;
    void *config;
    const struct mimir_policy_driver_vtable *vtable;
};

struct mimir_policy {
    char *title;

    struct mimir_policy_source source;
    struct mimir_policy_repository repository;
    struct mimir_policy_driver driver;
};

static int mimir_ini_section_is_manifest(char *section) {
    for (char *c = section; *c != '\0'; c += 1) {
        if (*c == '.') {
            return 0;
        }
    }

    return 1;
}

static int mimir_parse_bool(char *value, int *result) {
    if (
        !strcmp(value, "true") ||
        !strcmp(value, "yes") ||
        !strcmp(value, "1")
    ) {
        *result = 1;
        return 0;
    }

    if (
        !strcmp(value, "false") ||
        !strcmp(value, "no") ||
        !strcmp(value, "0")
    ) {
        *result = 0;
        return 0;
    }

    mimir_error("Invalid bool value");
    return -1;
}

static int mimir_build_driver_rsync(
    struct mimir_policy_ini *ini,
    char *section,
    struct mimir_policy_driver *driver
) {
    struct mimir_policy_driver_rsync_config *config = mimir_arena_malloc(
        &g_mimir_arena,
        sizeof(*config)
    );

    if (config == NULL) {
        mimir_error("Failed to allocate rsync driver config");
        return -1;
    }

    config->archive = 0;
    config->delete = 0;
    config->hardlinks = 0;
    config->acl = 0;
    config->xattrs = 0;
    config->numeric_ids = 0;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (strcmp(entry->section, section)) {
            continue;
        }

        if (!strcmp(entry->key, "archive")) {
            if (mimir_parse_bool(entry->value, &config->archive) != 0) {
                return -1;
            }

            continue;
        }

        if (!strcmp(entry->key, "delete")) {
            if (mimir_parse_bool(entry->value, &config->delete) != 0) {
                return -1;
            }

            continue;
        }

        if (!strcmp(entry->key, "hardlinks")) {
            if (mimir_parse_bool(entry->value, &config->hardlinks) != 0) {
                return -1;
            }

            continue;
        }

        if (!strcmp(entry->key, "acl")) {
            if (mimir_parse_bool(entry->value, &config->acl) != 0) {
                return -1;
            }

            continue;
        }

        if (!strcmp(entry->key, "xattrs")) {
            if (mimir_parse_bool(entry->value, &config->xattrs) != 0) {
                return -1;
            }

            continue;
        }

        if (!strcmp(entry->key, "numeric_ids")) {
            if (mimir_parse_bool(entry->value, &config->numeric_ids) != 0) {
                return -1;
            }

            continue;
        }

        mimir_error("Unknown rsync driver key");
        return -1;
    }

    driver->type = MIMIR_POLICY_DRIVER_RSYNC;
    driver->config = config;
    driver->vtable = NULL;

    return 0;
}

// FIXME: Rewrite this shit with hashmap
static void mimir_count_policy_manifests(struct mimir_policy_ini *ini, size_t *count) {
    *count = 0;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (!mimir_ini_section_is_manifest(entry->section)) {
            continue;
        }

        int already_seen = 0;

        for (size_t j = 0; j < i; j += 1) {
            struct mimir_policy_entry *previous = ini->entries + j;

            if (mimir_ini_section_is_manifest(previous->section)
                && !strcmp(previous->section, entry->section)) {
                already_seen = 1;
                break;
            }
        }

        if (!already_seen) {
            *count += 1;
        }
    }
}

static int mimir_read_policy_manifest(
    struct mimir_policy_ini *ini,
    char *section,
    char **source,
    char **repository,
    char **driver
) {
    *source = NULL;
    *repository = NULL;
    *driver = NULL;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (strcmp(entry->section, section)) {
            continue;
        }

        if (!strcmp(entry->key, "source")) {
            *source = entry->value;
            continue;
        }

        if (!strcmp(entry->key, "repository")) {
            *repository = entry->value;
            continue;
        }

        if (!strcmp(entry->key, "driver")) {
            *driver = entry->value;
            continue;
        }

        mimir_error("Unknown policy manifest key");
        return -1;
    }

    if (*source == NULL) {
        mimir_error("Policy source is missing");
        return -1;
    }

    if (*repository == NULL) {
        mimir_error("Policy repository is missing");
        return -1;
    }

    if (*driver == NULL) {
        mimir_error("Policy driver is missing");
        return -1;
    }

    return 0;
}

static char *mimir_join3(char *a, char *b, char *c) {
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);
    size_t c_len = strlen(c);

    size_t total = a_len + 1 + b_len + 1 + c_len + 1;

    char *result = mimir_arena_malloc(&g_mimir_arena, total);
    if (result == NULL) {
        mimir_error("Failed to allocate joined string");
        return NULL;
    }

    size_t cursor = 0;

    for (size_t i = 0; i < a_len; i += 1) {
        result[cursor] = a[i];
        cursor += 1;
    }

    result[cursor] = '.';
    cursor += 1;

    for (size_t i = 0; i < b_len; i += 1) {
        result[cursor] = b[i];
        cursor += 1;
    }

    result[cursor] = '.';
    cursor += 1;

    for (size_t i = 0; i < c_len; i += 1) {
        result[cursor] = c[i];
        cursor += 1;
    }

    result[cursor] = '\0';

    return result;
}

static int mimir_build_repository_fs(
    struct mimir_policy_ini *ini,
    char *section,
    struct mimir_policy_repository *repository
) {
    struct mimir_policy_repository_fs_config *config = mimir_arena_malloc(
        &g_mimir_arena,
        sizeof(*config)
    );

    if (config == NULL) {
        mimir_error("Failed to allocate fs repository config");
        return -1;
    }

    config->path = NULL;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (strcmp(entry->section, section)) {
            continue;
        }

        if (!strcmp(entry->key, "path")) {
            config->path = entry->value;
            continue;
        }

        mimir_error("Unknown fs repository key");
        return -1;
    }

    if (config->path == NULL) {
        mimir_error("fs repository path is missing");
        return -1;
    }

    repository->type = MIMIR_POLICY_REPOSITORY_TYPE_FILE_SYSTEM;
    repository->config = config;
    repository->vtable = NULL;

    return 0;
}

static int mimir_build_source_fs(
    struct mimir_policy_ini *ini,
    char *section,
    struct mimir_policy_source *source
) {
    struct mimir_policy_source_fs_config *config = mimir_arena_malloc(
        &g_mimir_arena,
        sizeof(*config)
    );

    if (config == NULL) {
        mimir_error("Failed to allocate fs source config");
        return -1;
    }

    config->path = NULL;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (strcmp(entry->section, section)) {
            continue;
        }

        if (!strcmp(entry->key, "path")) {
            config->path = entry->value;
            continue;
        }

        mimir_error("Unknown fs source key");
        return -1;
    }

    if (config->path == NULL) {
        mimir_error("fs source path is missing");
        return -1;
    }

    source->type = MIMIR_POLICY_SOURCE_TYPE_FILE_SYSTEM;
    source->config = config;
    source->vtable = NULL;

    return 0;
}

static int mimir_build_source(
    struct mimir_policy_ini *ini,
    char *source_name,
    char *section,
    struct mimir_policy_source *source
) {
    if (!strcmp(source_name, "fs")) {
        return mimir_build_source_fs(ini, section, source);
    }

    mimir_error("Unknown policy source");
    return -1;
}

static int mimir_build_single_policy(
    struct mimir_policy_ini *ini,
    char *title,
    struct mimir_policy *policy
) {
    char *source_name = NULL;
    char *repository_name = NULL;
    char *driver_name = NULL;

    int success = mimir_read_policy_manifest(
        ini,
        title,
        &source_name,
        &repository_name,
        &driver_name
    );

    if (success != 0) {
        return -1;
    }

    policy->title = title;

    char *source_section = mimir_join3(title, "source", source_name);
    if (source_section == NULL) {
        return -1;
    }

    success = mimir_build_source(
                                 ini,
                                 source_name,
                                 source_section,
                                 &policy->source
                                 );

    if (success != 0) {
        mimir_error("Failed to build policy source");
        return -1;
    }

    char *repository_section = mimir_join3(title, "repository", repository_name);
    if (repository_section == NULL) {
        return -1;
    }

    success = mimir_build_repository_fs(
                                        ini,
                                        repository_section,
                                        &policy->repository
                                        );

    if (success != 0) {
        mimir_error("Failed to build fs repository");
        return -1;
    }

    char *driver_section = mimir_join3(title, "driver", driver_name);
    if (driver_section == NULL) {
        return -1;
    }

    success = mimir_build_driver_rsync(
                                       ini,
                                       driver_section,
                                       &policy->driver
                                       );

    if (success != 0) {
        mimir_error("Failed to build rsync driver");
        return -1;
    }

    mimir_write(STDOUT_FILENO, "policy: ");
    mimir_write(STDOUT_FILENO, title);
    mimir_write(STDOUT_FILENO, "\n");

    mimir_write(STDOUT_FILENO, "source: ");
    mimir_write(STDOUT_FILENO, source_name);
    mimir_write(STDOUT_FILENO, "\n");

    mimir_write(STDOUT_FILENO, "repository: ");
    mimir_write(STDOUT_FILENO, repository_name);
    mimir_write(STDOUT_FILENO, "\n");

    mimir_write(STDOUT_FILENO, "driver: ");
    mimir_write(STDOUT_FILENO, driver_name);
    mimir_write(STDOUT_FILENO, "\n");

    struct mimir_policy_source_fs_config *fs_config =
        policy->source.config;

    mimir_write(STDOUT_FILENO, "source.path: ");
    mimir_write(STDOUT_FILENO, fs_config->path);
    mimir_write(STDOUT_FILENO, "\n");

    struct mimir_policy_repository_fs_config *repository_config =
        policy->repository.config;

    mimir_write(STDOUT_FILENO, "repository.path: ");
    mimir_write(STDOUT_FILENO, repository_config->path);
    mimir_write(STDOUT_FILENO, "\n");

    struct mimir_policy_driver_rsync_config *driver_config =
        policy->driver.config;

    mimir_write(STDOUT_FILENO, "driver.archive: ");
    mimir_write(STDOUT_FILENO, driver_config->archive ? "true" : "false");
    mimir_write(STDOUT_FILENO, "\n");

    mimir_write(STDOUT_FILENO, "driver.delete: ");
    mimir_write(STDOUT_FILENO, driver_config->delete ? "true" : "false");
    mimir_write(STDOUT_FILENO, "\n");

    return 0;
}

static int mimir_build_policies(struct mimir_policy_ini *ini, struct mimir_policy **policies, size_t *policies_count) {
    mimir_count_policy_manifests(ini, policies_count);

    if (*policies_count == 0) {
        mimir_error("Policies Not Found");
        return -1;
    }

    *policies = mimir_arena_malloc(&g_mimir_arena, sizeof(**policies) * (*policies_count));

    if (*policies == NULL) {
        mimir_error("Failed to allocate policies");
        return -1;
    }

    size_t policy_index = 0;

    for (size_t i = 0; i < ini->entries_count; i += 1) {
        struct mimir_policy_entry *entry = ini->entries + i;

        if (!mimir_ini_section_is_manifest(entry->section)) {
            continue;
        }

        int already_built = 0;

        for (size_t j = 0; j < policy_index; j += 1) {
            if (!strcmp((*policies)[j].title, entry->section)) {
                already_built = 1;
                break;
            }
        }

        if (already_built) {
            continue;
        }

        struct mimir_policy *policy = (*policies) + policy_index;
        policy->title = entry->section;

        int success = mimir_build_single_policy(ini, entry->section, policy);
        if (success != 0) {
            mimir_error("Failed to build policy");
            return -1;
        }

        policy_index += 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    g_mimir_arena = mimir_arena_initialize(8192);

    if (argc != 2) {
        mimir_error("Usage: ./mimir policy.ini");
        return -1;
    }

    char *filepath = argv[1]; // TODO: Validation
    char *policy_content = NULL;
    size_t policy_content_size = 0;
    int success = mimir_load_policy_content(filepath, &policy_content, &policy_content_size);
    if (success != 0) {
        mimir_error("Failed to load backup policy");
        return -1;
    }

    struct mimir_policy_ini ini = { 0 };
    success = mimir_parse_policy_content(policy_content, policy_content_size, &ini);
    if (success != 0) {
        mimir_error("Failed to parse backup policy");
        return -1;
    }

    struct mimir_policy *policies = NULL;
    size_t policies_count = 0;
    success = mimir_build_policies(&ini, &policies, &policies_count);
    if (success != 0) {
        mimir_error("Failed to build policies (mimir_build_policies)");
        return -1;
    }

    success = mimir_arena_destroy(&g_mimir_arena);
    if (success != 0) {
        mimir_error("Failed to free memory for backup policy");
        return -1;
    }

    return 0;
}
