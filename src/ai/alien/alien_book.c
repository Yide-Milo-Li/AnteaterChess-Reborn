/*
 * Alien plugin — minimal runtime book.
 *
 * The original alien/src/book.c also contained a self-play book trainer
 * (book_generate, book_generate_parallel) that uses fork()/waitpid()
 * for parallel search. The trainer is intentionally NOT included here:
 *
 *   - It is never invoked at runtime (the plugin only reads existing
 *     opening.book files).
 *   - It pulls in <sys/wait.h>/<unistd.h>; while both are POSIX and
 *     compile fine on Linux, dropping them removes any cross-platform
 *     surface area we don't need.
 *   - It also drags in many auxiliary helpers (hashset_*, gen_*,
 *     explore, estimate_nodes, ...) that bloat the plugin.
 *
 * What's left here is the read path: load a .book file, binary-search
 * for the current position's hash, validate the candidate move against
 * the legal-move list, hand it back to the caller.
 */
#include "alien_namespace.h"
#include "alien_chess.h"
#include "alien_book.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sort helper for the binary search (kept for symmetry; load assumes
 * the file is already sorted as alien's book_save writes it). */
static int entry_cmp(const void *a, const void *b) {
    const BookEntry *ea = (const BookEntry *)a;
    const BookEntry *eb = (const BookEntry *)b;
    if (ea->hash < eb->hash) return -1;
    if (ea->hash > eb->hash) return  1;
    return 0;
}

int book_load(Book *book, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    char magic[12];
    if (fread(magic, 1, 12, f) != 12 || memcmp(magic, BOOK_MAGIC, 12) != 0) {
        fclose(f);
        return 0;
    }

    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    book->count = (int)count;
    book->capacity = book->count + 1;
    book->entries = (BookEntry *)malloc(sizeof(BookEntry) * (size_t)book->capacity);
    if (!book->entries) {
        fclose(f);
        return 0;
    }

    if ((int)fread(book->entries, sizeof(BookEntry), (size_t)book->count, f)
        != book->count) {
        free(book->entries);
        book->entries = NULL;
        fclose(f);
        return 0;
    }
    fclose(f);

    /* Defensive: re-sort in case the file wasn't sorted. */
    qsort(book->entries, (size_t)book->count, sizeof(BookEntry), entry_cmp);
    printf("Book loaded: %d entries from %s\n", book->count, filename);
    return 1;
}

void book_free(Book *book) {
    if (book && book->entries) {
        free(book->entries);
        book->entries = NULL;
        book->count = book->capacity = 0;
    }
}

int book_lookup(const Book *book, uint64_t hash, Move *move) {
    if (!book || !book->entries || book->count == 0) return 0;

    int lo = 0, hi = book->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (book->entries[mid].hash == hash) {
            const BookEntry *e = &book->entries[mid];
            memset(move, 0, sizeof(Move));
            move->fr = e->fr;  move->fc = e->fc;
            move->tr = e->tr;  move->tc = e->tc;
            move->promotion = (e->flags & 0x07);
            move->castle    = (e->flags >> 3) & 0x03;
            move->ant_eating = (e->flags >> 5) & 0x01;
            return 1;
        }
        if (book->entries[mid].hash < hash) lo = mid + 1;
        else                                hi = mid - 1;
    }
    return 0;
}

/* ---- Global book state ---- */

static Book global_book = {NULL, 0, 0};
static Book neural_book = {NULL, 0, 0};
static int  book_mode   = BOOK_MODE_DISABLED;

int book_load_global(const char *filename) {
    return book_load(&global_book, filename);
}

int book_load_neural(const char *filename) {
    return book_load(&neural_book, filename);
}

void book_set_mode(int mode) { book_mode = mode; }
int  book_get_mode(void)     { return book_mode; }

/* ---- Probe used by alien_ai.c at search root ---- */

int book_probe(const State *s, Move *move) {
    if (book_mode == BOOK_MODE_DISABLED) return 0;

    Move raw;
    int found = 0;

    if ((book_mode == BOOK_MODE_NEURAL_ONLY || book_mode == BOOK_MODE_NEURAL_THEN_CPU)
        && book_lookup(&neural_book, s->hash, &raw)) {
        found = 1;
    }
    if (!found
        && (book_mode == BOOK_MODE_CPU_ONLY || book_mode == BOOK_MODE_NEURAL_THEN_CPU)
        && book_lookup(&global_book, s->hash, &raw)) {
        found = 1;
    }
    if (!found) return 0;

    MoveList ml;
    gen_legal_moves(s, &ml);
    int match_count = 0;
    int match_idx = -1;

    for (int i = 0; i < ml.count; ++i) {
        Move *cand = &ml.moves[i];
        if (cand->fr != raw.fr || cand->fc != raw.fc ||
            cand->tr != raw.tr || cand->tc != raw.tc) continue;
        if (raw.promotion && cand->promotion != raw.promotion) continue;
        if (raw.castle    && cand->castle    != raw.castle)    continue;
        if (raw.ant_eating && !cand->ant_eating)               continue;
        match_idx = i;
        match_count++;
    }

    if (match_count == 1) {
        *move = ml.moves[match_idx];
        return 1;
    }
    /* Ambiguous or not-found legal match → fall back to search. */
    return 0;
}

/*
 * Stubs for symbols that the alien CLI provides via book.h but that the
 * plugin runtime doesn't need. Defining them as no-ops keeps the public
 * header self-consistent without dragging in fork()/wait().
 */
int book_save(const Book *book, const char *filename) {
    (void)book; (void)filename;
    return 0;
}
void book_generate(const char *outfile, int search_depth, int top_k, int ply_depth) {
    (void)outfile; (void)search_depth; (void)top_k; (void)ply_depth;
}
void book_generate_parallel(const char *outfile, int search_depth,
                            int top_k, int ply_depth, int workers) {
    (void)outfile; (void)search_depth; (void)top_k; (void)ply_depth; (void)workers;
}
