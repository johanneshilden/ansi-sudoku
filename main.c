#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROW(X) X / 9
#define COL(X) X % 9

/* Compute absolute position offset from a row-column coordinate. */
#define OFFSET(ROW, COL) ROW * 9 + COL 

/* Return the index of the box which corresponds to a given position. */
#define BOX(P) lookup (ROW (P) / 3, COL (P) / 3)

/* Translate an absolute position to a position relative to its box. */
#define BOX_INDEX(P) lookup (ROW (P) % 3, COL (P) % 3)
 
#define SET_CANDIDATE(matrix, pos, n) \
    toggle_candidate (matrix, pos, n, 1);

#define CLEAR_CANDIDATE(matrix, pos, n) \
    toggle_candidate (matrix, pos, n, 0);

#define IS_CANDIDATE(matrix, pos, n) (*(matrix + pos) & (1 << (n + 3)))

#define TO_BIN_9(w) \
    (w & 0x1000 ? 1 : 0), \
    (w & 0x0800 ? 1 : 0), \
    (w & 0x0400 ? 1 : 0), \
    (w & 0x0200 ? 1 : 0), \
    (w & 0x0100 ? 1 : 0), \
    (w & 0x0080 ? 1 : 0), \
    (w & 0x0040 ? 1 : 0), \
    (w & 0x0020 ? 1 : 0), \
    (w & 0x0010 ? 1 : 0)

#define TO_BIN(w) \
    (w & 0x0100 ? 1 : 0), \
    (w & 0x0080 ? 1 : 0), \
    (w & 0x0040 ? 1 : 0), \
    (w & 0x0020 ? 1 : 0), \
    (w & 0x0010 ? 1 : 0), \
    (w & 0x0008 ? 1 : 0), \
    (w & 0x0004 ? 1 : 0), \
    (w & 0x0002 ? 1 : 0), \
    (w & 0x0001 ? 1 : 0)

#define ROW_OFFSET 0
#define COL_OFFSET 1
#define BOX_OFFSET 2

enum state
{
    STATE_EVAL,
    STATE_FORWARD,
    STATE_REVERSE
};

const char *
offs_type (int offs)
{
    switch (offs)
    {
        case ROW_OFFSET: 
            return "ROW";
            break;
        case COL_OFFSET:
            return "COL";
            break;
        case BOX_OFFSET:
            return "BOX";
            break;
        default:
            return "ERROR";
    }
}

void 
dump (int8_t *p)
{
    int8_t i, j;

    fprintf (stdout, "┌───────┬───────┬───────┐\n");
    for (i = 0; i < 9; ++i) 
    {
        fprintf (stdout, "│ ");
        for (j = 0; j < 9; ++j) 
        {
            int8_t o = OFFSET (i, j);
            int8_t x = p[o];

            if (x)
                fprintf (stdout, "%d ", x);
            else
                fprintf (stdout, "  ");

            if (j == 2 || j == 5) 
                fprintf (stdout, "│ ");
        }
        fprintf (stdout, "│\n");
        if (i == 2 || i == 5) 
            fprintf (stdout, "├───────┼───────┼───────┤\n");
    }
    fprintf (stdout, "└───────┴───────┴───────┘\n");
}

void 
dump_candidates (int16_t *candidates)
{
    int8_t i, j;

    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 9; j++)
        {
            int8_t  w = i * 9 + j;
            int16_t b = candidates[w];
            fprintf (stdout, "|%s%d %d:%d%d%d%d%d%d%d%d%d", (w < 10 ? " " : ""), w, b & 0b1111, TO_BIN_9 (b));
        }
        fprintf (stdout, "\n");
    }
}

int8_t 
lookup (int8_t a, int8_t b)
{
    switch (a)
    {
        case 0:
            switch (b)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
            }
        case 1:
            switch (b)
            {
                case 0: return 3;
                case 1: return 4;
                case 2: return 5;
            }
        case 2:
            switch (b)
            {
                case 0: return 6;
                case 1: return 7;
                case 2: return 8;
            }
        default:
            assert (0);
            return -1;
    }
}
 
/* Return absolute offset from a box index and relative position. */
int8_t
box_offset (int8_t box, int8_t pos)
{
    switch (box)
    {
        case 0:  box = 0;  break;
        case 1:  box = 3;  break;
        case 2:  box = 6;  break;
        case 3:  box = 27; break; 
        case 4:  box = 30; break;
        case 5:  box = 33; break;
        case 6:  box = 54; break;
        case 7:  box = 57; break;
        default: box = 60; 
    }
    switch (pos)
    {
        case 0:  return box + 0;
        case 1:  return box + 1;
        case 2:  return box + 2;
        case 3:  return box + 9;
        case 4:  return box + 10;
        case 5:  return box + 11;
        case 6:  return box + 18;
        case 7:  return box + 19;
        default: return box + 20;
    }
}

int
validate_pos (const int8_t *d, int8_t p) 
{
    int8_t i,
           c = COL (p),
           r = ROW (p),
           n = d[p],
           s = BOX (p),
           q = BOX_INDEX (p);

    for (i = 0; i < 9; ++i) 
    {
        /* Check horizontal uniqueness constraint */
        if (i != c && n == d[OFFSET (r, i)]) 
            return 0;
        /* Check vertical uniqueness constraint */
        if (i != r && n == d[OFFSET (i, c)]) 
            return 0;
        /* Check box */
        if (i != q && n == d[box_offset (s, i)]) 
            return 0;
    }

    return 1;
}

int
toggle_candidate (int16_t *matrix, int8_t pos, int8_t n, int set)
{
    int16_t bits;
    int16_t count;
    
    if ((set && IS_CANDIDATE (matrix, pos, n)) || (!set && !IS_CANDIDATE (matrix, pos, n)))
        return 0;

    bits = *(matrix + pos);

    /* The number of candidates is stored in the four least significant bits */
    count = bits & 0b1111;

    bits >>= 4;

    --n;

    count += set ? 1 : -1;
    bits = set ? bits | (1 << n) : bits ^ (1 << n);

    *(matrix + pos) = count | (bits << 4);

    return 1;
}

/* Each entry in the matrix corresponds to a grid element in the puzzle and 
 * carries two bytes of information, consistent with the bit pattern 
 * described below. 
 *
 * fedcba9876543210
 * ------------xxxx   The four least significant bits keep track of the
 *                    number of candidates.  
 *
 * ---xxxxxxxxx----   Bits 4 to 12 signify whether the corresponding number 
 *        ^           between 1 and 9 is a feasible candidate or not.
 *
 * xxx-------------   The highest 3 bits are redundant.
 */ 
void
init_candidates (int8_t *p, int16_t *candidates)
{
    int8_t i, j;

    memset (candidates, 0, sizeof (int16_t) * 81);

    for (i = 0; i < 81; i++)
    {
        if (0 == p[i])
        {
            for (j = 1; j <= 9; j++)
            {
                p[i] = j;
                if (validate_pos (p, i))
                    SET_CANDIDATE (candidates, i, j);
            }
            p[i] = 0;
        }
        else
        {
            SET_CANDIDATE (candidates, i, p[i]);
        }
    }
}

/* Run brute-force algorithm integration step.
 *
 * Return codes:
 *
 *    0 : Step complete without errors.
 *    1 : Final state: Valid solution found.
 *   -1 : Final state: No solution exists.
 */
int
step (int8_t *d, int16_t *candidates, int8_t *cursor, enum state *state)
{
    int8_t c = *cursor;

    if (81 == c && STATE_REVERSE != *state)
        return 1;
    else if (-1 == c)
        return -1;
    else if (-2 == c)  /* Initial state */
        c = -1;

    switch (*state)
    {
        case STATE_EVAL:
            {
                assert ((0b1111 & candidates[c]) > 1);

                while (10 != ++d[c])
                {
                    if (!IS_CANDIDATE (candidates, c, d[c]))
                        continue;
                    if (1 == validate_pos (d, c))
                        break;
                }
                if (10 == d[c])
                {
                    d[c] = 0;
                    *state = STATE_REVERSE;
                }
                else
                {
                    *state = STATE_FORWARD;
                }
            }
            break;
        case STATE_FORWARD:
            {
                if (++c < 81 && (0b1111 & candidates[c]) > 1)
                    *state = STATE_EVAL;
            }
            break;
        case STATE_REVERSE:
            {
                if (c-- > 0 && (0b1111 & candidates[c]) > 1)
                    *state = STATE_EVAL;
            }
            break;
        default:
            assert (0);
    }
    *cursor = c;
    return 0;
}

int8_t 
log2_plus1 (int16_t bits)
{
    switch (bits)
    {
        case 0x001: return 1;
        case 0x002: return 2;
        case 0x004: return 3;
        case 0x008: return 4;
        case 0x010: return 5;
        case 0x020: return 6;
        case 0x040: return 7;
        case 0x080: return 8;
        case 0x100: return 9;
        default:    return 0;
    }
}

int8_t
get_offset (int8_t i, int8_t j, int offstype)
{
    switch (offstype)
    {
        case ROW_OFFSET:    
            return OFFSET (i, j);
        case COL_OFFSET:    
            return OFFSET (j, i);
        case BOX_OFFSET: 
            return box_offset (i, j);
        default:
            assert (0);
            return -1;
    }
}

int8_t
unset_bits (int16_t *matrix, int8_t pos, int16_t bits)
{
    int8_t i, 
           n = 0;

    for (i = 0; i < 9; i++)
        if (bits & (1 << i))
            n += toggle_candidate (matrix, pos, i + 1, 0);

    return n;
}

int8_t
bitcount (int16_t bits)
{
    int8_t i, c = 0;

    for (i = 0; i < 9; i++)
        if ((1 << i) & bits)
            c++;
    return c;
}

void
init_deep_loop (int8_t p[], int8_t n)
{
    int8_t i;

    for (i = 0; i < n; i++)
        p[i] = n - i - 1;
}

int
deep_loop (int8_t p[], int8_t n)
{
    int8_t q = 0;

    while (q >= 0 && (9 - q) == ++p[q])
        if (++q == n)
            q = -2;
    while (--q >= 0)
        p[q] = p[q + 1] + 1;
 
    return (-2 < q);
}

void
remove_naked_subset (int16_t *candidates, int8_t i, int offs, int8_t n, int *f)
{
    int8_t  k, j[5], s;
    int16_t c[5], 
            bits;

    for (k = 0; k < n; k++)
        j[k] = n - k - 1;

    init_deep_loop (j, n);

    do
    {
        /* Take the n candidate sets which correspond to the current 
         * permutation of {1..n}. If these sets have no less than two
         * and no more than n elements, they may qualify for subset
         * elimination.
         *
         * To determine if the union of the selected sets form an
         * n-element set, we count the number of bits in the result 
         * obtained by a bitwise OR operation.
         */
        bits = 0;
        for (k = 0; k < n; k++)
        {
            c[k] = candidates[get_offset (i, j[n - k - 1], offs)];
            s = c[k] & 0b1111;
            
            /* Count the number of elements in this set */
            if (2 <= s && s <= n)
            {
                bits |= (c[k] >> 4);
            }
            else
            {
                k = -1;
                break;
            }
        }
        if (-1 != k && n == bitcount (bits))
        {
            for (k = 0; k < 9; k++)
            {
                int8_t pos;

                /* Make sure not to include the n-selection itself */
                for (s = 0; s < n; s++)
                    if (k == j[s])
                        break;

                if (s == n)
                {
                    pos = get_offset (i, k, offs);

                    if (unset_bits (candidates, pos, bits))
                    {
                        *f = 1;
#ifndef NDEBUG
                        printf ("Naked subset (n = %d) (offs. type = %s) elimination: %d\n", n, offs_type (offs), pos);
#endif
                    }
                }
            }
        }

    } while (deep_loop (j, n));
}

void
remove_hidden_subset (int16_t *candidates, int8_t i, int offs, int8_t n, int *f)
{
    int8_t j, k, p[5];
    int16_t l[9], b;

    /* First, we translate the row, column, or box data from a list of candidate
     * sets (location => candidate mappings) into a candidate => location
     * mapping.
     * 
     * E.g., given a row with the following candidate sets,
     *             +---+-----+---+-------+-----+-------+---+-----+---+
     *    location : 0 :  1  : 2 :   3   :  4  :   5   : 6 :  7  : 8 :
     *             +---+-----+---+-------+-----+-------+---+-----+---+
     *  candidates : 2 | 479 | 5 | 14789 | 479 | 14789 | 3 | 479 | 6 |
     *             +---+-----+---+-------+-----+-------+---+-----+---+
     * we obtain;
     *             +----+---+---+-------+---+----+-------+----+-------+
     *   candidate : 1  : 2 : 3 :   4   : 5 : 6  :   7   : 8  :   9   :
     *             +----+---+---+-------+---+----+-------+----+-------+
     *    location | 35 | 0 | 6 | 13457 | 2 | 8  | 13457 | 35 | 13457 |
     *             +----+---+---+-------+---+----+-------+----+-------+
     */
    memset (l, 0, 18);
    
    for (k = 0; k < 9; k++)
    {
        b = candidates[get_offset (i, k, offs)];

        b >>= 4;

        for (j = 0; j < 9; j++)
            if (b & (1 << j))
                l[j] |= (1 << k);
    }
 
    init_deep_loop (p, n);

    do 
    {
        int16_t x = l[p[0]];

        if (n == bitcount (x))
        {
            /* Are the n location sets identical? */
            for (j = 1; j < n; j++)
                if (l[p[j]] != x)
                    break;
    
            if (n == j)
            {
                int16_t v = n;
                for (k = 0; k < n; k++)
                    v |= (1 << (p[k] + 4));

                for (k = 0; k < 9; k++)
                {
                    if (x & (1 << k))
                    {
                        int8_t o = get_offset (i, k, offs);            

                        if (candidates[o] != v)
                        {
                            candidates[o] = v;
                            *f = 1;
#ifndef NDEBUG
                            printf ("Hidden subset (n = %d) (offs. type = %s): %d\n", n, offs_type (offs), get_offset (i, k, offs));
#endif
                        }
                    }
                }
            }
        }

    } while (deep_loop (p, n));
}

/* This procedure returns 1 if any change took place on the candidate matrix.
 * Thus, a 0 is to be interpreted such that there are no further optimizations
 * possible. 
 */
int
saturate (int8_t *d, int16_t *candidates)
{
    int8_t i, j;
    int f = 0;

    for (i = 0; i < 81; i++)
    {
        if (!d[i] && 1 == (candidates[i] & 0b1111))
        {
            /* === Singleton elimination ======================================
             *
             * Only one candidate remains, hence we can assign this value 
             * to the cell, without further ado.
             */
            d[i] = log2_plus1 (candidates[i] >> 4);
            f = 1;

#ifndef NDEBUG
            printf ("Singleton elimination: %d\n", i);
#endif
        }
    }

    for (i = 0; i < 9; i++)
    {
        /* === Naked pairs ====================================================
         *
         * A "naked pair" is a pair of cells in a row, column or box
         * that contain only the same two candidates and therefore can 
         * be eliminated from all other candidate sets in the same row, 
         * column or box.  
         */
        remove_naked_subset (candidates, i, ROW_OFFSET, 2, &f);
        remove_naked_subset (candidates, i, COL_OFFSET, 2, &f);
        remove_naked_subset (candidates, i, BOX_OFFSET, 2, &f);
    }

    /* === Naked subsets ======================================================
     *
     */
    for (j = 3; j <= 5; j++)
    {
        for (i = 0; i < 9; i++)
        {
            remove_naked_subset (candidates, i, ROW_OFFSET, j, &f);
            remove_naked_subset (candidates, i, COL_OFFSET, j, &f);
            remove_naked_subset (candidates, i, BOX_OFFSET, j, &f);
        }
    }

    /* === Hidden singles =====================================================
     *
     */
    for (i = 0; i < 9; i++)
    {
        remove_hidden_subset (candidates, i, ROW_OFFSET, 1, &f);
        remove_hidden_subset (candidates, i, COL_OFFSET, 1, &f);
        remove_hidden_subset (candidates, i, BOX_OFFSET, 1, &f);
    }

    /* === Hidden subsets =====================================================
     *
     */
    for (j = 2; j <= 5; j++)
    {
        for (i = 0; i < 9; i++)
        {
            remove_hidden_subset (candidates, i, ROW_OFFSET, j, &f);
            remove_hidden_subset (candidates, i, COL_OFFSET, j, &f);
            remove_hidden_subset (candidates, i, BOX_OFFSET, j, &f);
        }
    }

    /* === Pointing pairs =====================================================
     *
     * @todo
     */

    /* === Pointing subsets ===================================================
     *
     * @todo
     */

    return f;
}

void
tests ()
{
    int16_t candidates[81];
    int f;

    /*                xxxx987654321ssss */
    candidates[0] = 0b00000000000100001;
    candidates[1] = 0b00001010010000011;
    candidates[2] = 0b00000000100000001;
    candidates[3] = 0b00001110010010101;
    candidates[4] = 0b00001010010000011;
    candidates[5] = 0b00001110010010101;
    candidates[6] = 0b00000000001000001;
    candidates[7] = 0b00001010010000011;
    candidates[8] = 0b00000001000000001;

    remove_hidden_subset (candidates, 0, ROW_OFFSET, 2, &f);

    assert (0b00000000000100001 == candidates[0]);
    assert (0b00001010010000011 == candidates[1]);
    assert (0b00000000100000001 == candidates[2]);
    assert (0b00000100000010010 == candidates[3]);
    assert (0b00001010010000011 == candidates[4]);
    assert (0b00000100000010010 == candidates[5]);
    assert (0b00000000001000001 == candidates[6]);
    assert (0b00001010010000011 == candidates[7]);
    assert (0b00000001000000001 == candidates[8]);

    /*                xxxx987654321ssss */
    candidates[0] = 0b00000000100110011;
    candidates[1] = 0b00000001101010100;
    candidates[2] = 0b00000100000000001;
    candidates[3] = 0b00000000010000001;
    candidates[4] = 0b00000011100010100;
    candidates[5] = 0b00001011000000011;
    candidates[6] = 0b00000010000100010;
    candidates[7] = 0b00000011000000010;
    candidates[8] = 0b00001011000000011;

    remove_hidden_subset (candidates, 0, ROW_OFFSET, 1, &f);

    assert (candidates[0] == 0b00000000100110011);
    assert (candidates[1] == 0b00000000001000001);
    assert (candidates[2] == 0b00000100000000001);
    assert (candidates[3] == 0b00000000010000001);
    assert (candidates[4] == 0b00000011100010100);
    assert (candidates[5] == 0b00001011000000011);
    assert (candidates[6] == 0b00000010000100010);
    assert (candidates[7] == 0b00000011000000010);
    assert (candidates[8] == 0b00001011000000011);
}

void
tests2 ()
{
    /*
    {
        int16_t candidates[81];
    
        int8_t p[] = 
        {
            0,0,0, 0,0,2, 0,0,0,
            0,8,0, 0,6,0, 0,1,0,
            2,0,6, 0,0,0, 5,0,9,
    
            0,0,8, 0,0,0, 0,0,2,
            0,1,0, 0,8,0, 0,7,0,
            5,0,0, 0,0,0, 1,0,0,
    
            3,0,1, 0,0,0, 9,0,5,
            0,4,0, 0,7,0, 0,6,0,
            0,0,0, 1,0,0, 0,0,0
        };
    
        init_candidates (p, candidates);
    
        while (saturate (p, candidates))
            printf ("-\n");
    
        {
            int8_t r, cursor = -2;
            enum state state = STATE_FORWARD;
    
            r = 0;
            while (0 == r)
                r = step (p, candidates, &cursor, &state);
        }
    
        dump (p);
    }
    {
        int16_t candidates[81];
    
        int8_t p[] = 
        {
            8,0,0, 0,0,0, 0,0,0,
            0,0,3, 6,0,0, 0,0,0,
            0,7,0, 0,9,0, 2,0,0,
    
            0,5,0, 0,0,7, 0,0,0,
            0,0,0, 0,4,5, 7,0,0,
            0,0,0, 1,0,0, 0,3,0,

            0,0,1, 0,0,0, 0,6,8,
            0,0,8, 5,0,0, 0,1,0,
            0,9,0, 0,0,0, 4,0,0
        };
    
        init_candidates (p, candidates);
    
        while (saturate (p, candidates))
            printf ("-\n");
    
        {
            int8_t r, cursor = -2;
            enum state state = STATE_FORWARD;
    
            r = 0;
            while (0 == r)
                r = step (p, candidates, &cursor, &state);
        }
    
        dump (p);
    }
    {
        int16_t candidates[81];
    
        int8_t p[] = 
        {
            7,0,0, 0,0,0, 4,0,0,
            0,2,0, 0,7,0, 0,8,0,
            0,0,3, 0,0,8, 0,0,9,
    
            0,0,0, 5,0,0, 3,0,0,
            0,6,0, 0,2,0, 0,9,0,
            0,0,1, 0,0,7, 0,0,6,

            0,0,0, 3,0,0, 9,0,0,
            0,3,0, 0,4,0, 0,6,0,
            0,0,9, 0,0,1, 0,3,5
        };
    
        init_candidates (p, candidates);
    
        while (saturate (p, candidates))
            printf ("-\n");
    
        {
            int8_t r, cursor = -2;
            enum state state = STATE_FORWARD;
    
            r = 0;
            while (0 == r)
                r = step (p, candidates, &cursor, &state);
        }
    
        dump (p);
    }
    */
    {
        int16_t candidates[81];
    
        int8_t p[] = 
        {
            5,3,0, 0,7,0, 0,0,0,
            6,0,0, 1,9,5, 0,0,0,
            0,9,8, 0,0,0, 0,6,0,
    
            8,0,0, 0,6,0, 0,0,3,
            4,0,0, 8,0,3, 0,0,1,
            7,0,0, 0,2,0, 0,0,6,

            0,6,0, 0,0,0, 2,8,0,
            0,0,0, 4,1,9, 0,0,5,
            0,0,0, 0,8,0, 0,7,9 
        };
    
        init_candidates (p, candidates);
    
        while (saturate (p, candidates))
            printf ("-\n");
    
        {
            int8_t r, cursor = -2;
            enum state state = STATE_FORWARD;
    
            r = 0;
            while (0 == r)
                r = step (p, candidates, &cursor, &state);
        }
    
        dump (p);
    }
}

int 
main (int argc, char *argv[])
{
    tests2 ();

    /*
    int16_t candidates[81];

    int8_t p[] = 
    {
        3,0,0, 0,0,2, 0,0,0,
        0,2,9, 0,3,0, 0,0,0,
        0,0,6, 0,4,0, 0,3,2,

        9,0,1, 0,0,8, 0,0,0,
        0,8,7, 0,0,0, 3,4,0,
        0,0,0, 3,0,0, 2,0,1,

        6,1,0, 0,8,0, 4,0,0,
        0,0,0, 0,6,0, 1,7,0,
        0,0,0, 2,0,0, 0,0,5
    };

    tests ();

    init_candidates (p, candidates);

    dump_candidates (candidates);

    while (saturate (p, candidates))
        dump_candidates (candidates);

    dump_candidates (candidates);

    {
        int8_t r, cursor = -2;
        enum state state = STATE_FORWARD;

        r = 0;
        while (0 == r)
        {
            r = step (p, candidates, &cursor, &state);
        }

        dump (p);
        printf ("%d\n", r);
    }
    */

    return 0;
}
