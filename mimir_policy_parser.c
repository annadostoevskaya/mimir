/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: mimir_policy_parser.c
 * Created: 2026-06-16 02:48:16
 * Last updated: 2026-06-19 01:19:22
 * Description:
 * License: $LICENSE
 */

#define DEFAULT_POLICY_CONTENT_SIZE 5

struct mimir_policy_entry {
    char *section;
    char *key;
    char *value;
};

static int mimir_parse_next_line(struct mimir_slice *content, struct mimir_slice *line) {
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

static struct mimir_slice mimir_parse_trim_line(struct mimir_slice trimmed) {
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

static int mimir_parse_key_value(struct mimir_slice line, struct mimir_slice *key, struct mimir_slice *value) {
    char *c = NULL;

    for (c = line.start; c < line.end; c += 1) {
        if (*c == '=') {
            break;
        }
    }

    if (c == line.end) {
        mimir_error("Failed to find equals symbol");
        return -1;
    }

    key->start = line.start;
    key->end = c;

    value->start = c + 1;
    value->end = line.end;

    *key = mimir_parse_trim_line(*key);
    *value = mimir_parse_trim_line(*value);

    if (key->start == key->end) {
        mimir_error("Failed to find key");
        return -1;
    }

    if (value->start == value->end) {
        mimir_error("Failed to find value");
        return -1;
    }

    for (c = key->start; c < key->end; c += 1) {
        if (mimir_parse_is_alpha_numeric(*c) || *c == '-' || *c == '_') {
            continue;
        }

        mimir_error("Failed to parse key, you should use only specified symbols");
        return -1;
    }

    for (c = value->start; c < value->end; c += 1) {
        if (mimir_parse_is_alpha_numeric(*c) || *c == '-' || *c == '_' || *c == '/' || *c == '.') {
            continue;
        }

        mimir_error("Failed to parse value, you should use only specified symbols");
        return -1;
    }

    return 0;
}

static int mimir_parse_section(struct mimir_slice line, struct mimir_slice *section) {
    if (line.start >= line.end || line.end - line.start < 3) {
        mimir_error("Section syntax error");
        return -1;
    }

    if (*line.start != '[' || *(line.end - 1) != ']') {
        mimir_error("Section syntax error");
        return -1;
    }

    line.start += 1;
    line.end -= 1;

    for (char *c = line.start; c < line.end; c += 1) {
        if (!mimir_parse_is_alpha_numeric(*c)) {
            mimir_error("Section syntax error, you should use only alpha-numeric symbols");
            return -1;
        }
    }

    *section = line;

    return 0;
}

int mimir_parse_policy_content(char* policy_content, size_t policy_content_size, struct mimir_policy_entry **policy_entries, size_t *policy_entries_size, size_t *policy_entries_count, size_t *policy_sections_count) {
    struct mimir_slice content = {
        .start = policy_content,
        .end = policy_content + policy_content_size
    };

    struct mimir_slice line = {};
    struct mimir_slice current_section = {};

    *policy_entries_count = 0;
    *policy_sections_count = 0;
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

            *policy_sections_count += 1;

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
