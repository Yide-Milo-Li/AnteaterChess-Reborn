#ifndef BOOK_H
#define BOOK_H

#include "alien_chess.h"

/*
 * Opening Book System
 *
 * File format (.book):
 *   Header: "ACHESS_BOOK\0" (12 bytes) + uint32_t entry_count
 *   Entries (sorted by hash for binary search):
 *     uint64_t  position_hash
 *     int16_t   from_r, from_c, to_r, to_c
 *     int16_t   score        (centipawns)
 *     int16_t   depth        (search depth used)
 *     int16_t   flags        (promotion, castle, ant_eating, chain_len)
 *
 * Generation strategy:
 *   - BFS from initial position
 *   - At each node, keep top-K candidate moves
 *   - Search each to specified depth
 *   - Store best move for each position
 */

#define BOOK_MAGIC "ACHESS_BOOK"
#define BOOK_MAX_ENTRIES 500000

/* Runtime book modes — which book(s) may be probed */
#define BOOK_MODE_DISABLED         0   /* levels 1-5: no book */
#define BOOK_MODE_CPU_ONLY         1   /* level 6: opening.book only */
#define BOOK_MODE_NEURAL_ONLY      2   /* neural.book only */
#define BOOK_MODE_NEURAL_THEN_CPU  3   /* level 7: neural.book first, fallback opening.book */

typedef struct {
    uint64_t hash;
    int16_t  fr, fc, tr, tc;
    int16_t  score;
    int16_t  depth;
    int16_t  flags;  /* bits: [0-2]=promotion, [3-4]=castle, [5]=ant_eating */
} BookEntry;

typedef struct {
    BookEntry *entries;
    int count;
    int capacity;
} Book;

/* Book I/O */
int   book_load(Book *book, const char *filename);
int   book_save(const Book *book, const char *filename);
void  book_free(Book *book);

/* Book lookup: returns 1 if found, fills move */
int   book_lookup(const Book *book, uint64_t hash, Move *move);

/* Runtime book mode control */
void  book_set_mode(int mode);
int   book_get_mode(void);

/* Book generation (supports --resume via existing file) */
void  book_generate(const char *outfile, int search_depth, int top_k, int ply_depth);

/* Parallel book generation: forks child processes for ply-0 branches */
void  book_generate_parallel(const char *outfile, int search_depth,
                             int top_k, int ply_depth, int workers);

#endif /* BOOK_H */
