/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: mimir_policy_loader.c
 * Created: 2026-06-19 00:51:52
 * Last updated: 2026-06-19 00:59:39
 * Description:
 * License: $LICENSE
 */

static int mimir_load_policy_content(char *path, char **policy_content, size_t *policy_content_size) {
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
    *policy_content = (char*)mimir_arena_malloc(&g_mimir_arena, *policy_content_size);

    if (*policy_content == NULL) {
        mimir_error("Failed to allocate memory for backup policy");
        close(policy_fd);
        return -1;
    }

    read(policy_fd, *policy_content, *policy_content_size);

    close(policy_fd);

    return 0;
}
