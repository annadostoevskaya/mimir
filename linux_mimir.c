/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: linux_mimira.c
 * Created: 2026-06-16 02:46:12
 * Last updated: 2026-06-18 01:26:08
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

void mimir_print_sv(int fd, struct mimir_slice sv) {
    write(fd, sv.start, sv.end - sv.start);
    write(fd, "\n", 1);
}

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
#include "mimir_policy_parser.c"

struct mimir_arena_allocator g_mimir_arena = {};

int mimir_load_policy_content(char *path, char **policy_content, size_t *policy_content_size) {
    int policy_fd = 0;
    struct stat policy_stat = {};

    policy_fd = open(path, O_RDONLY);

    if (policy_fd < 0) {
        mimir_error("File not found or access denied");
        return -1;
    }

    if (fstat(policy_fd, &policy_stat) < 0) {
        mimir_error("fstat() failed");
        close(policy_fd);
        return -1;
    }

    *policy_content_size = policy_stat.st_size;
    *policy_content = (char*)mimir_malloc(*policy_content_size);

    if (*policy_content == NULL) {
        mimir_error("Failed to allocate memory for backup policy");
        close(policy_fd);
        return -1;
    }

    read(policy_fd, *policy_content, *policy_content_size);

    close(policy_fd);

    return 0;
}


struct mimir_policy_entry {
    char *section;
    char *key;
    char *value;
};

#define DEFAULT_POLICY_CONTENT_SIZE 5

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

int mimir_parse_policy_content(char* policy_content, size_t policy_content_size, struct mimir_policy_entry **policy_entries, size_t *policy_entries_size, size_t *policy_entries_count) {
    struct mimir_slice content = {
        .start = policy_content,
        .end = policy_content + policy_content_size
    };

    struct mimir_slice line = {};
    struct mimir_slice current_section = {};

    *policy_entries_count = 0;
    if (*policy_entries_size == 0) {
        *policy_entries_size = DEFAULT_POLICY_CONTENT_SIZE;
    }

    *policy_entries = (struct mimir_policy_entry*)mimir_arena_malloc(&g_mimir_arena, sizeof(struct mimir_policy_entry) * (*policy_entries_size));

    if (*policy_entries == NULL) {
        mimir_error("Failed to allocate memory for policy entries");
        return -1;
    }

    while (mimir_parse_next_line(&content, &line)) {
        struct mimir_slice trimmed_line = mimir_parse_trim_line(line);

        if (trimmed_line.start == trimmed_line.end) {
            continue;
        }

        if (*trimmed_line.start == '[') {
            int success = mimir_parse_section(trimmed_line, &current_section);
            if (success != 0) {
                mimir_error("Failed to parse section (mimir_parse_current_section)");
                return -1;
            }

            continue;
        }

        char *c = trimmed_line.start;
        if (mimir_parse_is_alpha_numeric(*c)) {

            if (current_section.start == current_section.end) {
                mimir_error("Key-value outside of section");
                return -1;
            }

            struct mimir_slice key = {};
            struct mimir_slice value = {};
            int success = mimir_parse_key_value(trimmed_line, &key, &value);
            if (success != 0) {
                mimir_error("Failed to parse key-value (mimir_parse_key_value)");
                return -1;
            }

            if (*policy_entries_count >= *policy_entries_size) {
                mimir_error("No memory for entries left");
                return -1;
            }

            struct mimir_policy_entry *entry = (*policy_entries) + (*policy_entries_count);

            entry->section = mimir_slice_to_cstring(current_section);
            entry->key = mimir_slice_to_cstring(key);
            entry->value = mimir_slice_to_cstring(value);

            if (entry->section == NULL || entry->key == NULL || entry->value == NULL) {
                mimir_error("Failed to allocate policy entry strings");
                return -1;
            }

            *policy_entries_count +=  1;

            continue;
        }

        mimir_error("Unexpected token error");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    g_mimir_arena = mimir_arena_initialize(4096);

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

    struct mimir_policy_entry *policy_entries = NULL;
    size_t policy_entries_size = 0;
    size_t policy_entries_count = 0;
    success = mimir_parse_policy_content(policy_content, policy_content_size, &policy_entries, &policy_entries_size, &policy_entries_count);
    if (success != 0) {
        mimir_error("Failed to parse backup policy");
        return -1;
    }

    for (size_t i = 0; i < policy_entries_count; i += 1) {
        mimir_error(policy_entries[i].section);
        mimir_error(policy_entries[i].key);
        mimir_error(policy_entries[i].value);
    }

    success = mimir_free(policy_content, policy_content_size);
    if (success != 0) {
        mimir_error("Failed to free memory for backup policy");
        return -1;
    }

    return 0;
}
