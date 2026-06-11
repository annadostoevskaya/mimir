#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct mimir_string {
    char *buffer;
    size_t length;
};

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

size_t mimir_config_parser_read_line(const struct mimir_string *content, struct mimir_string *line, size_t cursor) {
    if (content->length - cursor > 0) { // TODO: Write assert
        line->buffer = content->buffer + cursor;
    }

    line->buffer = content->buffer + cursor;
    line->length = 1;

    while (line->buffer[line->length] != '\n') {
        line->length += 1;
    }

    return line->length - 1;
}

int mimir_loading_policy(const struct mimir_string *content, struct mimir_policy *policy) {
    mimir_string line = {};
    char c = '\0';
    mimir_string current_section = {};
    struct mimir_config_entry *mimir_config_entries = NULL; // TODO: Dynamic allocator (kinda vector)

    while (mimir_config_parser_read_line(content, &line, )) {
        mimir_config_parser_trim_line(line);

        if (line.length == 0) { // line is empty
            continue;
        }

        c = *line.buffer;

        if (c == ';') { // line is comment
            continue;
        }

        if (c == '[' ) {
            int error = mimir_config_parser_start_section(line, &current_section);
            if (error != 0) {
                mimir_error("Failed to parse config, section error: ");
                mimir_error(line.buffer); // TODO: concat("> " + line.buffer)
                return -1;
            }

            continue;
        }

        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '0')) {
            mimir_parser_parse_key_value(line, /* key, value */);
            config_entry = {};
            config_entry. // TODO: strcopy
        }
    }

    return 0;
}

void mimir_write(int fd, const char* s) {
    int len = 0;

    while (s[len] != '\0')
        len++;

    write(fd, s, len);
}

void mimir_error(const char* error_message) {
    mimir_write(STDERR_FILENO, "[mimir] ");
    mimir_write(STDERR_FILENO, error_message);
    mimir_write(STDERR_FILENO, "\n");
}

void *mimir_core_malloc(size_t size) {
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (addr == MAP_FAILED) {
        mimir_error("mmap() failed");
        return NULL;
    }

    return addr;
}

int mimir_core_free(void *addr, size_t size) {
    int success = 0;

    success = munmap(addr, size);

    if (success != 0) {
        mimir_error("munmap() failed");
        return -1;
    }

    return 0;
}

struct mimir_arena_allocator {
    unsigned char *memory;
    size_t size;
    size_t cursor;
};

struct mimir_arena_allocator mimir_arena_initialize(size_t size) {
    void *memory = mimir_core_malloc(size);

    if (memory == NULL) {
        mimir_error("Failed to initialize arena alloactor");
        return (struct mimir_arena_allocator){ 0 };
    }

    return (struct mimir_arena_allocator){
        .memory = memory,
        .size = size,
        .cursor = 0
    };
}

void *mimir_arena_malloc(struct mimir_arena_allocator *arena, size_t size) {
    void *memory;

    if (arena == NULL) {
        return NULL;
    }

    if (size > arena->size - arena->cursor) {
        return NULL;
    }

    memory = (void*)(arena->memory + arena->cursor);
    arena->cursor += size;

    return memory;
}

void mimir_arena_mreset(struct mimir_arena_allocator *arena) {
    if (arena != NULL) {
        arena->cursor = 0;
    }
}

int mimir_arena_destroy(struct mimir_arena_allocator *arena) {
    if (arena == NULL) {
        return -1;
    }

    if (arena->memory == NULL) {
        return 0;
    }

    if (mimir_core_free(arena->memory, arena->size) != 0) {
        mimir_error("Failed to destroy arena allocator, mimir_core_free() failed");
        return -1;
    }

    arena->memory = NULL;
    arena->size = 0;
    arena->cursor = 0;

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        mimir_error("Usage: ./mimir policy.ini");
        return -1;
    }

    int policy_fd = 0;
    struct stat policy_stat = {};

    policy_fd = open(argv[1], O_RDONLY);
    if (policy_fd < 0) {
        mimir_error("File not found or access denied");
        return -1;
    }

    if (fstat(policy_fd, &policy_stat) < 0) {
        mimir_error("fstat() failed");
        close(policy_fd);
        return -1;
    }

    // TODO: Write custom alloactor here!
    char *policy_content = NULL;
    policy_content = (char*)mimir_core_malloc(policy_stat.st_size);

    if (policy_content == NULL) {
        mimir_error("Failed to allocate memory for backup policy");
        return -1;
    }

    read(policy_fd, policy_content, policy_stat.st_size);

    close(policy_fd);

    struct mimir_policy policy = {};
    error = mimir_policy_parse({ policy_content, policy_stat.st_size }, &policy);

    if (error != 0) {
        mimir_error("Failed to parse backup policy");
        return -1;
    }

    int success = 0;
    success = mimir_core_free((void*)policy_content, policy_stat.st_size);
    if (success != 0) {
        mimir_error("Failed to free memory for backup policy");
        return -1;
    }

    return 0;
}
