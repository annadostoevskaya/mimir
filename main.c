#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>


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
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (m  == MAP_FAILED) {
        mimir_error("mmap() failed");
        return NULL;
    }

    return m;
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

    mimir_error(policy_content);

    return 0;
}
