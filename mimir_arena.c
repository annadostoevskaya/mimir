/*
 * Author: Anna Dostoevskaya
 * Email: iwantknow.aboutjt68h43@gmail.com
 * File: mimir_arena.c
 * Created: 2026-06-16 02:46:12
 * Last updated: 2026-06-16 02:56:49
 * Description:
 * License: $LICENSE
 */

struct mimir_arena_allocator {
    unsigned char *memory;
    size_t size;
    size_t cursor;
};

struct mimir_arena_allocator mimir_arena_initialize(size_t size) {
    void *memory = mimir_malloc(size);

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

    if (size > arena->size - arena->cursor) {
        mimir_error("Failed to allocate memory from mimir_arena, not enough memory (mimir_arena_malloc)");
        return NULL;
    }

    memory = (void*)(arena->memory + arena->cursor);
    arena->cursor += size;

    return memory;
}

void mimir_arena_mreset(struct mimir_arena_allocator *arena) {
    arena->cursor = 0;
}

int mimir_arena_destroy(struct mimir_arena_allocator *arena) {
    if (mimir_free(arena->memory, arena->size) != 0) {
        mimir_error("Failed to destroy arena allocator, mimir_free() failed");
        return -1;
    }

    arena->memory = NULL;
    arena->size = 0;
    arena->cursor = 0;

    return 0;
}
