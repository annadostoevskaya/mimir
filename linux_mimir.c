/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: linux_mimira.c
 * Created: 2026-06-16 02:46:12
 * Last updated: 2026-06-21 03:52:23
 * Description:
 * License: $LICENSE
 */

#include <stddef.h>
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

    for (size_t i = 0; i < len; i += 1) {
        dest[i] = src.start[i];
    }

    dest[len] = '\0';
    return dest;
}

static int mimir_cstring_equals(const char* a, const char *b) {
    size_t i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }

        i += 1;
    }

    return a[i] == b[i];
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

enum mimir_policy_driver {
    MIMIR_POLICY_DRIVER_NONE = 0,
    MIMIR_POLICY_DRIVER_RSYNC,

    MIMIR_POLICY_DRIVER_COUNT
};

struct mimir_policy {
    enum mimir_policy_source_type source_type;
    enum mimir_policy_repository_type repository_type;
    enum mimir_policy_driver driver;
    char *title;
    char *source_path;
    char *repository_path;
};
/*
static int mimir_build_policies(struct mimir_policy_ini *policy_ini, struct mimir_policy **policies) {
    *policies = (struct mimir_policy*)mimir_arena_malloc(&g_mimir_arena, sections_count);

    if (*policies == NULL) {
        mimir_error("Failed to allocate memory for building policies");
        return -1;
    }

    for (size_t i = 0; i < sections_count; i += 1) {
        struct mimir_policy *policy = (*policies) + i;

        policy->title = entries[i * entries_count / sections_count].section;

        for (size_t j = 0; j < entries_count / sections_count; j += 1) {
            if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].key, "source_type")) {
                if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].value, "fs")) {
                    policy->source_type = MIMIR_POLICY_SOURCE_TYPE_FILE_SYSTEM;
                }
            }

            if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].key, "source_path")) {
                policy->source_path = entries[i * entries_count / sections_count + j].value; // TODO: Maybe we should copy this value?
            }

            if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].key, "repository_type")) {
                if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].value, "fs")) {
                    policy->repository_type = MIMIR_POLICY_REPOSITORY_TYPE_FILE_SYSTEM;
                }
            }

            if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].key, "repository_path")) {
                policy->repository_path = entries[i * entries_count / sections_count + j].value;
            }

            if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].key, "driver")) {
                if (mimir_cstring_equals(entries[i * entries_count / sections_count + j].value, "rsync")) {
                    policy->driver = MIMIR_POLICY_DRIVER_RSYNC;
                }
            }
        }
    }

    return 0;
}
*/
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
    // success = mimir_build_policies(ini, &policies);
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
