#include "gol.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* --- Test: creazione e distruzione --- */

static void test_create_null(void)
{
    /* gol_create con size <= 0 deve restituire NULL */
    assert(gol_create(0, 10) == NULL);
    assert(gol_create(10, 0) == NULL);
    assert(gol_create(-1, 10) == NULL);
    assert(gol_create(10, -5) == NULL);
    assert(gol_create(0, 0) == NULL);
    printf("PASS: test_create_null\n");
}

static void test_create_valid(void)
{
    GolBoard *b = gol_create(5, 10);
    assert(b != NULL);
    assert(b->rows == 5);
    assert(b->cols == 10);
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 10; j++)
            assert(b->grid[i][j] == 0);
    gol_free(b);
    printf("PASS: test_create_valid\n");
}

static void test_free_null(void)
{
    /* gol_free con NULL non deve crashare */
    gol_free(NULL);
    printf("PASS: test_free_null\n");
}

/* --- Test: pulisci --- */

static void test_clear(void)
{
    GolBoard *b = gol_create(4, 4);
    assert(b != NULL);

    /* Imposta alcune celle vive */
    b->grid[1][1] = 1;
    b->grid[2][2] = 1;
    assert(b->grid[1][1] == 1);

    /* Pulisci */
    gol_clear(b);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(b->grid[i][j] == 0);

    gol_free(b);
    printf("PASS: test_clear\n");
}

static void test_clear_null(void)
{
    gol_clear(NULL);
    printf("PASS: test_clear_null\n");
}

/* --- Test: randomizza --- */

static void test_randomize(void)
{
    GolBoard *b = gol_create(3, 3);
    assert(b != NULL);

    gol_randomize(b);

    /* Tutte le celle devono essere 0 o 1 */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            assert(b->grid[i][j] == 0 || b->grid[i][j] == 1);

    gol_free(b);
    printf("PASS: test_randomize\n");
}

static void test_randomize_null(void)
{
    gol_randomize(NULL);
    printf("PASS: test_randomize_null\n");
}

/* --- Test: step — block (still life) --- */

static void test_block(void)
{
    /* Block 2x2: deve rimanere invariato */
    GolBoard *b = gol_create(4, 4);
    assert(b != NULL);

    b->grid[1][1] = 1;
    b->grid[1][2] = 1;
    b->grid[2][1] = 1;
    b->grid[2][2] = 1;

    gol_step(b);

    assert(b->grid[1][1] == 1);
    assert(b->grid[1][2] == 1);
    assert(b->grid[2][1] == 1);
    assert(b->grid[2][2] == 1);

    gol_free(b);
    printf("PASS: test_block\n");
}

/* --- Test: step — blinker (periodo 2) --- */

static void test_blinker(void)
{
    /* Blinker orizzontale → verticale → orizzontale */
    GolBoard *b = gol_create(5, 5);
    assert(b != NULL);

    /* Orizzontale */
    b->grid[2][1] = 1;
    b->grid[2][2] = 1;
    b->grid[2][3] = 1;

    gol_step(b);

    /* Deve diventare verticale */
    assert(b->grid[1][2] == 1);
    assert(b->grid[2][2] == 1);
    assert(b->grid[3][2] == 1);
    assert(b->grid[2][1] == 0);
    assert(b->grid[2][3] == 0);

    gol_step(b);

    /* Deve tornare orizzontale */
    assert(b->grid[2][1] == 1);
    assert(b->grid[2][2] == 1);
    assert(b->grid[2][3] == 1);
    assert(b->grid[1][2] == 0);
    assert(b->grid[3][2] == 0);

    gol_free(b);
    printf("PASS: test_blinker\n");
}

/* --- Test: step — glider su board grande (nessun wrapping) --- */

static void test_glider(void)
{
    /* Glider standard su board 7x7: non tocca i bordi */
    /* Posizione iniziale:
     * .1.....
     * ..1....
     * .111...
     */
    GolBoard *b = gol_create(7, 7);
    assert(b != NULL);

    b->grid[0][1] = 1;
    b->grid[1][2] = 1;
    b->grid[2][0] = 1;
    b->grid[2][1] = 1;
    b->grid[2][2] = 1;

    /* Dopo 4 step il glider si sposta di (1, 1) diagonali */
    for (int i = 0; i < 4; i++)
        gol_step(b);

    assert(b->grid[1][2] == 1);
    assert(b->grid[2][3] == 1);
    assert(b->grid[3][1] == 1);
    assert(b->grid[3][2] == 1);
    assert(b->grid[3][3] == 1);

    gol_free(b);
    printf("PASS: test_glider\n");
}

/* --- Test: step — wrapping toroidale --- */

static void test_toroidal_wrap(void)
{
    /* Board 3x3: verifica che il wrapping funzioni correttamente */
    GolBoard *b = gol_create(3, 3);
    assert(b != NULL);

    /* Imposta solo le celle degli angoli */
    b->grid[0][0] = 1;
    b->grid[0][2] = 1;
    b->grid[2][0] = 1;
    b->grid[2][2] = 1;

    /* Step: ogni angolo ha 2 vicini (gli altri angoli via wrapping) → sopravvive */
    /* Il centro ha 4 vicini → non nasce (serve esattamente 3) */
    gol_step(b);

    /* Pattern still life: tutti e 4 gli angoli rimangono vivi */
    assert(b->grid[0][0] == 1);
    assert(b->grid[0][2] == 1);
    assert(b->grid[2][0] == 1);
    assert(b->grid[2][2] == 1);
    assert(b->grid[1][1] == 0);

    gol_free(b);
    printf("PASS: test_toroidal_wrap\n");
}

/* --- Test: step — null --- */

static void test_step_null(void)
{
    gol_step(NULL);
    printf("PASS: test_step_null\n");
}

/* --- Test: step — board 1x1 --- */

static void test_step_1x1(void)
{
    GolBoard *b = gol_create(1, 1);
    assert(b != NULL);

    b->grid[0][0] = 1;
    gol_step(b);
    /* 1x1: la cella ha 0 vicini → muore */
    assert(b->grid[0][0] == 0);

    b->grid[0][0] = 1;
    gol_step(b);
    assert(b->grid[0][0] == 0);

    gol_free(b);
    printf("PASS: test_step_1x1\n");
}

/* --- Test: step — board 1xN --- */

static void test_step_1xn(void)
{
    GolBoard *b = gol_create(1, 5);
    assert(b != NULL);

    /* Linea orizzontale: tutte le celle interne hanno 2 vicini */
    b->grid[0][1] = 1;
    b->grid[0][2] = 1;
    b->grid[0][3] = 1;

    gol_step(b);

    /* In 1D toroidale, le righe sopra/sotto sono la stessa riga,
     * quindi ogni cella interna ha 4 vicini (doppi).
     * Le celle 1 e 3 hanno 2 vicini ciascuna (0 e 2, o 2 e 4) → muoiono.
     * La cella 2 ha 4 vicini → muore.
     * Le celle 0 e 4 hanno 2 vicini ciascuna (1 e 3) → muoiono.
     * Risultato: tutti morti, MA il wrapping 1D fa sì che
     * le celle 0 e 4 siano vicine tra loro (toroidale),
     * quindi ognuna ha 2 vicini (1 e 4 per la 0, 0 e 3 per la 4). */
    assert(b->grid[0][0] == 1);
    assert(b->grid[0][4] == 1);
    assert(b->grid[0][1] == 0);
    assert(b->grid[0][2] == 0);
    assert(b->grid[0][3] == 0);

    gol_free(b);
    printf("PASS: test_step_1xn\n");
}

/* --- Test: step — board Nx1 --- */

static void test_step_nx1(void)
{
    GolBoard *b = gol_create(5, 1);
    assert(b != NULL);

    b->grid[1][0] = 1;
    b->grid[2][0] = 1;
    b->grid[3][0] = 1;

    gol_step(b);

    /* Simile a 1xn: colonne a destra/sinistra sono la stessa colonna.
     * Le celle 1 e 3 hanno 2 vicini ciascuna → muoiono.
     * La cella 2 ha 4 vicini → muore.
     * Le celle 0 e 4 hanno 2 vicini ciascuna (1 e 3) → muoiono.
     * Risultato: tutti morti, MA il wrapping 1D fa sì che
     * le celle 0 e 4 siano vicine tra loro (toroidale),
     * quindi ognuna ha 2 vicini. */
    assert(b->grid[0][0] == 1);
    assert(b->grid[4][0] == 1);
    assert(b->grid[1][0] == 0);
    assert(b->grid[2][0] == 0);
    assert(b->grid[3][0] == 0);

    gol_free(b);
    printf("PASS: test_step_nx1\n");
}

/* --- Test: step — board 2x2 tutto vivo --- */

static void test_step_2x2_full(void)
{
    GolBoard *b = gol_create(2, 2);
    assert(b != NULL);

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            b->grid[i][j] = 1;

    gol_step(b);

    /* Su 2x2 toroidale, ogni cella ha 8 vicini (con duplicati dal wrapping)
     * → muore per sovraffollamento */
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            assert(b->grid[i][j] == 0);

    gol_free(b);
    printf("PASS: test_step_2x2_full\n");
}

/* --- Test: step — board 2x2 tutto morto --- */

static void test_step_2x2_empty(void)
{
    GolBoard *b = gol_create(2, 2);
    assert(b != NULL);

    gol_step(b);

    /* Nessuna cella viva → nessuna nascita */
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            assert(b->grid[i][j] == 0);

    gol_free(b);
    printf("PASS: test_step_2x2_empty\n");
}

/* --- Test: step — board 3x3 tutto vivo --- */

static void test_step_3x3_full(void)
{
    GolBoard *b = gol_create(3, 3);
    assert(b != NULL);

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            b->grid[i][j] = 1;

    gol_step(b);

    /* Ogni cella ha 8 vicini → muore per sovraffollamento */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            assert(b->grid[i][j] == 0);

    gol_free(b);
    printf("PASS: test_step_3x3_full\n");
}

/* --- Main --- */

int main(void)
{
    test_create_null();
    test_create_valid();
    test_free_null();
    test_clear();
    test_clear_null();
    test_randomize();
    test_randomize_null();
    test_block();
    test_blinker();
    test_glider();
    test_toroidal_wrap();
    test_step_null();
    test_step_1x1();
    test_step_1xn();
    test_step_nx1();
    test_step_2x2_full();
    test_step_2x2_empty();
    test_step_3x3_full();

    printf("\n=== Tutti i test passati ===\n");
    return 0;
}
