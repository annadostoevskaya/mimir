/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: linux_mimira.c
 * Created: 2026-06-16 02:46:12
 * Last updated: 2026-06-26 08:58:39
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

// static void mimir_print_slice(int fd, struct mimir_slice s) {
//     write(fd, s.start, s.end - s.start);
//     write(fd, "\n", 1);
// }

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

struct mimir_policy {
    char *title;

    // struct mimir_policy_source source;
    // struct mimir_policy_repo repo;
    // struct mimir_policy_driver driver;
};

static int mimir_match_root_policy_section(const char *section, size_t *section_size) {
    *section_size = 1;

    for (const char *c = section; *c != '\0'; c += 1) {
        if (*c == '.') {
            return 0;
        }

        *section_size += 1;
    }

    return 1;
}

static int mimir_build_policies(struct mimir_policy_ini *policy_ini, struct mimir_policy **policies) {

    // BUILD POLICY
    // 1. Create domain temporary structure (hash-map)
    // 1.1. O(1) access to every hm[section][key] = value
    // 1.2. policy[title][section][source][fs].path ?
    // 2. Parse this to mimir_policy

    // 1. count policy manifest
    // 2. alloc memory
    // 3. check is section, check is already build? if no: goto 4
    // 4. build one policy
    // 5.

    char **roots = NULL;
    size_t roots_count = 0;
    for (size_t i = 0; i < policy_ini->sections_count; i += 1) {
        size_t section_size = 0;
        if (mimir_match_root_policy_section(policy_ini->sections[i], &section_size)) {
            roots_count += 1;
        }
    }

    if (roots_count == 0) {
        mimir_error("Policies Not Found");
        return -1;
    }

    roots = mimir_arena_malloc(&g_mimir_arena, sizeof(*roots) * roots_count);
    if (roots == NULL) {
        mimir_error("Failed to allocate memory for root policies");
        return -1;
    }

    // TODO: Deduplication for roots
    size_t roots_iterator = 0;
    for (size_t i = 0; i < policy_ini->sections_count; i += 1) {
        size_t section_size = 0;
        if (mimir_match_root_policy_section(policy_ini->sections[i], &section_size)) {
            roots[roots_iterator] = mimir_arena_malloc(&g_mimir_arena, section_size);
            if (roots[roots_iterator] == NULL) {
                mimir_error("Failed to allocate memory for root policies");
                return -1;
            }

            strcpy(roots[roots_iterator], policy_ini->sections[i]);
            roots_iterator += 1;
        }
    }

#define DEBUG_CODE(code) code

    DEBUG_CODE(if (roots_iterator != roots_count) {
            // *(char*)0x0 = 1;
            mimir_error("DEBUG_CODE: Error");
        });

    // search keys for roots[1] or for roots[0] and use it for build tree, u know

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
    int success = mimir_policy_load_content(filepath, &policy_content, &policy_content_size);
    if (success != 0) {
        mimir_error("Failed to load backup policy");
        return -1;
    }

    struct mimir_policy_ini ini = { 0 };
    success = mimir_policy_parse_content(policy_content, policy_content_size, &ini);
    if (success != 0) {
        mimir_error("Failed to parse backup policy");
        return -1;
    }

    struct mimir_policy *policies = NULL;
    success = mimir_build_policies(&ini, &policies);
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
