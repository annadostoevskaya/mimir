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
