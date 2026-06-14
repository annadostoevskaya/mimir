#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

int mimir_write(int fd, const char* s) {
    int len = 0;

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

struct string_view {
    char *start;
    char *end;
};

void mimir_print_sv(int fd, struct string_view sv) {
    write(fd, sv.start, sv.end - sv.start);
    write(fd, "\n", 1);
}

static int mimir_parse_next_line(struct string_view *content, struct string_view *line) {
    char *cursor = NULL;

    if (content->start >= content->end) {
        return 0;
    }

    cursor = content->start;

    while (cursor < content->end && *cursor != '\n') {
        cursor += 1;
    }

    line->start = content->start;
    line->end = cursor;

    if (cursor < content->end && *cursor == '\n') {
        cursor += 1;
    }

    content->start = cursor;

    return 1;
}

static int mimir_parse_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int mimir_parse_is_alpha_numeric(char c) {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9');
}

static struct string_view mimir_parse_trim_line(struct string_view trimmed) {
    for (char *c = trimmed.start; c < trimmed.end; c += 1) {
        if  (*c == ';') {
            trimmed.end = c;
            break;
        }
    }

    while (trimmed.start < trimmed.end && mimir_parse_is_space(*trimmed.start)) {
        trimmed.start += 1;
    }

    while (trimmed.end > trimmed.start && mimir_parse_is_space(*(trimmed.end - 1))) {
        trimmed.end -= 1;
    }

    return trimmed;
}

static int mimir_parse_key_value(struct string_view line, struct string_view *key, struct string_view *value) {
    char *c = NULL;

    key->start = line.start;
    value->end = line.end;

    for (c = line.start; c < line.end; c += 1) {
        if (*c == '=') {
            break;
        }
    }

    if (c == line.end - 1) {
        mimir_error("Failed to find equals symbols");
        return -1;
    }

    if (c + 1 == line.end) {
        mimir_error("Failed to find value");
        return -1;
    }

    value->start = c + 1;
    key->end = c - 1;

    *key = mimir_parse_trim_line(*key);
    *value = mimir_parse_trim_line(*value);

    for (c = key->start; c < key->end; c += 1) {
        if (mimir_parse_is_alpha_numeric(*c) || *c == '-' || *c == '_') {
            continue;
        }

        mimir_error("Failed to parse key, you should use only specified symbols");
        return -1;
    }

    for (c = value->start; c < value->end; c += 1) {
        if (mimir_parse_is_alpha_numeric(*c) || *c == '-' || *c == '_' || *c == '/') {
            continue;
        }

        mimir_error("Failed to parse value, you should use only specified symbols");
        return -1;
    }

    return 0;
}

static int mimir_parse_section(struct string_view line, struct string_view *section) {
    if (*line.start != '[' || *(line.end - 1) != ']'
        || line.start + 1 >= line.end
        || line.end - 1 <= line.start) {
        mimir_error("Section syntax error"); // TODO(annad): Error handling and logging
        return -1;
    }

    line.start += 1;
    line.end -= 1;

    for (char *c = line.start; c < line.end; c += 1) {
        if (!mimir_parse_is_alpha_numeric(*c)) {
            mimir_error("Section syntax error, you should use only alpha-numeric symbols");
            break;
        }
    }

    *section = line;

    return 0;
}

int mimir_parse_policy_content(char* policy_content, size_t policy_content_size) {
    struct string_view content = {
        .start = policy_content,
        .end = policy_content + policy_content_size
    };

    struct string_view line = {};
    struct string_view current_section = {};

    while (mimir_parse_next_line(&content, &line)) {
        struct string_view trimmed_line = mimir_parse_trim_line(line);

        mimir_print_sv(STDERR_FILENO, trimmed_line);

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
            struct string_view key = {};
            struct string_view value = {};
            int success = mimir_parse_key_value(trimmed_line, &key, &value);
            if (success != 0) {
                mimir_error("Failed to parse key-value (mimir_parse_key_value)");
                return -1;
            }

            // update entries array

            continue;
        }

        mimir_error("Unexpected token error");
    }

    return 0;
}

int main(int argc, char **argv) {
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

    success = mimir_parse_policy_content(policy_content, policy_content_size);
    if (success != 0) {
        mimir_error("Failed to parser backup policy");
        return -1;
    }

    success = mimir_free(policy_content, policy_content_size);
    if (success != 0) {
        mimir_error("Failed to free memory for backup policy");
        return -1;
    }

    return 0;
}
