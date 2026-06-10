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
    mimir_policy_source_type source_type;
    mimir_policy_repository_type repository_type;
    mimir_policy_engine engine;
    struct mimir_string source_path;
    struct mimir_string repository_path;
};

struct mimir_config_entry {
    mimir_string section;
    mimir_string key;
    mimir_string value;
};

int mimir_loading_policy(const mimir_string content, struct mimir_policy *policy) {
    mimir_string line = {};
    char c = '\0';
    mimir_string current_section = {};
    struct mimir_config_entry *mimir_config_entries = NULL; // TODO: Dynamic allocator (kinda vector)

    while (mimir_config_parser_read_line(content, line)) {
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

void *mimir_malloc(size_t size) {
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

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
    policy_content = (char*)mimir_malloc(policy_stat.st_size);

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
    success = mimir_free((void*)policy_content, policy_stat.st_size);
    if (success != 0) {
        mimir_error("Failed to free memory for backup policy");
        return -1;
    }

    return 0;
}
