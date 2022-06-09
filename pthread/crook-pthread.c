#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include "listCount.c"
#define MAX_RECURSION 4

int isSudokuSolved = 0;
int max_recursion_threads = MAX_RECURSION;
pthread_mutex_t max_recursion_threads_mutex = PTHREAD_MUTEX_INITIALIZER;

// Cell of a Sudoku
typedef struct cell
{
    int val;               // value of the cell
    int changed;           // 0 := not changed, 1 := changed, -1 := invalid
    list *poss;            // possible values list from 1 to n where n is size of sudoku
    pthread_mutex_t mutex; // mutex for the cell
} cell;

// Array of listCounts for each row/column/grid and their mutexes
typedef struct possRCG
{
    listCount *poss;       // possible values list from 1 to n where n is size of sudoku
    pthread_mutex_t mutex; // mutex for every list in poss
} possRCG;

// Params for solveSingleton function
typedef struct solveSingletonParams
{
    cell **sudoku;   // sudoku
    int n;           // size of sudoku
    int max_threads; // max number of threads
    int n_thread;    // number of current thread
    int *changed;    // 0 := not changed, 1 := changed, -1 := invalid
} solveSingletonParams;

// Params for markupSudoku function
typedef struct markupParams
{
    cell **sudoku;         // sudoku to be solved
    int n;                 // size of sudoku
    possRCG **possRows;    // possible values for each row
    possRCG **possColumns; // possible values for each column
    possRCG **possGrids;   // possible values for each grid
    int *isValid;          // 1 := is valid, 0 := is not valid
    int max_threads;       // max number of threads
    int n_thread;          // number of current thread
} markupParams;

// Params for loneRangers function
typedef struct loneRangerParams
{
    cell **sudoku;     // sudoku to be solved
    possRCG **possRcg; // possible values for each row/column/grid
    int n;             // size of sudoku
    int *changed;      // 0 := not changed, 1 := changed, -1 := invalid
    int max_threads;   // max number of threads
    int n_thread;      // number of current thread
} loneRangerParams;

typedef struct solveSudokuParams
{
    cell **sudoku;
    possRCG **possRows;
    possRCG **possColumns;
    possRCG **possGrids;
    int n;
} solveSudokuParams;

typedef struct isSolvedParams
{
    int *isSolved;   // 1 := solved, 0 := not solved
    cell **sudoku;   // sudoku
    int n;           // size of sudoku
    int max_threads; // max number of threads
    int n_thread;    // number of current thread
} isSolvedParams;

typedef struct cloneSudokuParams
{
    cell **sudoku; // sudoku
    int n;         // size of sudoku
    cell **clone;  // clone of sudoku
} cloneSudokuParams;

typedef struct clonePossRCGParams
{
    possRCG **possRcg; // possible values for each row/column/grid
    int n;             // size of sudoku
    possRCG **clone;   // clone of possRcg
} possRCGParams;

// Read sudoku from file and return a 2D array of cells
cell **readSudoku(int *n, possRCG ***possRows, possRCG ***possColumns, possRCG ***possGrids, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    cell **sudoku = NULL;
    char line[1024];

    // read the size of the sudoku
    if (fgets(line, sizeof(line), fp))
    {
        char *scan = line;
        sscanf(scan, "%d", n);
        sudoku = malloc(*n * sizeof(sudoku));

        *possRows = malloc(*n * sizeof(possRCG *));
        *possColumns = malloc(*n * sizeof(possRCG *));
        *possGrids = malloc(*n * sizeof(possRCG *));
        for (int i = 0; i < *n; i++)
        {
            (*possRows)[i] = malloc(*n * sizeof(possRCG));
            (*possColumns)[i] = malloc(*n * sizeof(possRCG));
            (*possGrids)[i] = malloc(*n * sizeof(possRCG));

            // create a listCount of all possible values (from 1 to n)
            (*possRows)[i]->poss = getListCount(*n);
            pthread_mutex_init(&(*possRows)[i]->mutex, NULL);

            // create the possible values listCount for each column
            (*possColumns)[i]->poss = getListCount(*n);
            pthread_mutex_init(&(*possColumns)[i]->mutex, NULL);

            // create the possible values listCount for each cell
            (*possGrids)[i]->poss = getListCount(*n);
            pthread_mutex_init(&(*possGrids)[i]->mutex, NULL);
        }

        // read the sudoku
        for (int i = 0; i < *n; ++i)
        {
            sudoku[i] = calloc(*n, sizeof(cell));
            for (int j = 0; j < *n; ++j)
            {
                int t = -1;
                if (fscanf(fp, "%X", &t) != 1)
                {
                    char c;
                    fscanf(fp, "%c", &c);
                    c = tolower(c);
                    t = (c - 'f') + 15;
                }
                // value of cell is the same as the value in the file
                (sudoku[i] + j)->val = t;
                // create a list of all possible values (from 1 to n)
                list *last = getList(*n);
                (sudoku[i] + j)->poss = last;
                // init changed to 0
                (sudoku[i] + j)->changed = 0;
                // init mutex for the cell
                pthread_mutex_init(&((sudoku[i] + j)->mutex), NULL);
            }
        }

        fclose(fp);

        return sudoku;
    }
    return NULL;
}

// Copy a sudoku
void *cloneSudoku(void *params)
{
    cell **sudoku = ((cloneSudokuParams *)params)->sudoku;
    int n = ((cloneSudokuParams *)params)->n;
    cell **clone = ((cloneSudokuParams *)params)->clone;

    for (int i = 0; i < n; i++)
    {
        clone[i] = malloc(n * sizeof(cell));
        for (int j = 0; j < n; j++)
        {
            (clone[i] + j)->val = (sudoku[i] + j)->val;
            (clone[i] + j)->poss = cloneList((sudoku[i] + j)->poss);
            (clone[i] + j)->changed = (sudoku[i] + j)->changed;
            pthread_mutex_init(&((clone[i] + j)->mutex), NULL);
        }
    }
    free(params);
    return 0;
}

// Destroy and free memory of sudoku (2D array of cells)
void destroySudoku(cell **sudoku, const int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            destroyList(&(sudoku[i] + j)->poss);
        }
        free(sudoku[i]);
        sudoku[i] = NULL;
    }
    free(sudoku);
    sudoku = NULL;
}

// Destroy and free memory of l (array of possRCG)
void destroyPossRCGArray(possRCG **l, const int n)
{
    for (int i = 0; i < n; i++)
    {
        destroyListCount(&(l[i]->poss));
        pthread_mutex_destroy(&(l[i]->mutex));
    }
    free(l);
    l = NULL;
}

// Copy an array of possRCG
void *clonePossRCGArray(void *params)
{
    possRCG **l = ((possRCGParams *)params)->possRcg;
    int n = ((possRCGParams *)params)->n;
    possRCG **clone = ((possRCGParams *)params)->clone;

    for (int i = 0; i < n; i++)
    {
        clone[i] = malloc(sizeof(possRCG));
        clone[i]->poss = cloneListCount(l[i]->poss);
        pthread_mutex_init(&(clone[i]->mutex), NULL);
    }
    free(params);
    return 0;
}

// Print the sudoku. If debug = 1, print the possible values of each cell, else print the value of each cell.
void printSudoku(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n, int debug)
{
    int sqrtN = sqrt(n);
    printf("#############################\nSUDOKU ");
    if (debug == 0)
    {
        printf("\n#############################\n");
    }
    else
    {
        printf("(possible values)\n#############################\n");
    }

    for (int i = 0; i < n; ++i)
    {
        if (debug == 0)
        {
            if (i % sqrtN == 0)
            {
                printf("----------------------\n");
            }
            for (int j = 0; j < n; ++j)
            {
                if (j % sqrtN == 0)
                {
                    printf("|");
                }
                if ((sudoku[i] + j)->val <= 15)
                {
                    printf("%X ", (sudoku[i] + j)->val);
                }
                else
                {
                    char c = 'F' + ((sudoku[i] + j)->val - 15);
                    printf("%c ", c);
                }
            }
            printf("|\n");
            if (i == n - 1)
            {
                printf("----------------------\n\n");
            }
        }
        else
        {
            for (int j = 0; j < n; ++j)
            {
                if ((sudoku[i] + j)->val != 0)
                {
                    printf("(%d,%d) = %d \n", i, j, (sudoku[i] + j)->val);
                }
                else
                {
                    printf("(%d,%d) poss = ", i, j);
                    printPossList((sudoku[i] + j)->poss);
                }
            }
            printf("Row %d = ", i);
            printPossListCount(possRows[i]->poss);

            printf("Grid %d = ", i);
            printPossListCount(possGrids[i]->poss);

            printf("Column %d = ", i);
            printPossListCount(possColumns[i]->poss);
            printf("\n");
        }
    }
}

// Return index of grid (from 0 to n-1) where cell (r,c) is located
int getIndexPossGrid(const int n, const int r, const int c)
{
    int rowN = sqrt(n);
    return (r / rowN) * rowN + c / rowN;
}

// Markup sudoku and find possible values for each cell
void *markupSudoku(void *params)
{
    cell **sudoku = ((markupParams *)params)->sudoku;
    possRCG **possRows = ((markupParams *)params)->possRows;
    possRCG **possColumns = ((markupParams *)params)->possColumns;
    possRCG **possGrids = ((markupParams *)params)->possGrids;
    int n = ((markupParams *)params)->n;
    int n_thread = ((markupParams *)params)->n_thread;
    int max_threads = ((markupParams *)params)->max_threads;

    int portion = n * n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n * n;
    }

    for (int i = start; i < end; ++i)
    {
        int r = i / n;
        int c = i % n;
        if (((markupParams *)params)->isValid == 0)
            break;
        if ((sudoku[r] + c)->val != 0)
        {
            if ((sudoku[r] + c)->changed == 0)
            {
                pthread_mutex_lock(&((sudoku[r] + c)->mutex));
                list *l = cloneList((sudoku[r] + c)->poss);
                (sudoku[r] + c)->changed = 1;
                destroyList(&((sudoku[r] + c)->poss));
                pthread_mutex_unlock(&((sudoku[r] + c)->mutex));

                // remove the value of the cell from the possible values of the row, column and grid
                pthread_mutex_lock(&(possRows[r]->mutex));
                findAndRemoveListCount(&(possRows[r]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possRows[r]->mutex));

                pthread_mutex_lock(&(possColumns[c]->mutex));
                findAndRemoveListCount(&(possColumns[c]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possColumns[c]->mutex));

                pthread_mutex_lock(&(possGrids[getIndexPossGrid(n, r, c)]->mutex));
                findAndRemoveListCount(&(possGrids[getIndexPossGrid(n, r, c)]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possGrids[getIndexPossGrid(n, r, c)]->mutex));

                // remove the poss values from the possible values of the row
                pthread_mutex_lock(&(possRows[r]->mutex));
                reduceListCount(&(possRows[r]->poss), l);
                pthread_mutex_unlock(&(possRows[r]->mutex));

                // remove the poss values from the possible values of the column
                pthread_mutex_lock(&(possColumns[c]->mutex));
                reduceListCount(&(possColumns[c]->poss), l);
                pthread_mutex_unlock(&(possColumns[c]->mutex));

                // remove the poss values from the possible values of the grid
                int indexGrid = getIndexPossGrid(n, r, c);
                pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                reduceListCount(&(possGrids[indexGrid]->poss), l);
                pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));

                // remove the value from the possible values of the cells in the same row
                for (int k = 0; k < n; ++k)
                {
                    if (k == c)
                    {
                        continue;
                    }

                    if ((sudoku[r] + c)->val == (sudoku[r] + k)->val) // not valid
                    {
                        *(((markupParams *)params)->isValid) = 0;
                        break;
                    }

                    // if the cell has val as a possible value, remove it
                    if ((sudoku[r] + k)->val == 0)
                    {
                        pthread_mutex_lock(&((sudoku[r] + k)->mutex));
                        int found = findAndRemoveList(&(sudoku[r] + k)->poss, (sudoku[r] + c)->val);
                        pthread_mutex_unlock(&((sudoku[r] + k)->mutex));
                        if (found == 1)
                        {
                            // remove the value from the possible values of the column
                            pthread_mutex_lock(&(possColumns[k]->mutex));
                            findAndReduceListCount(&(possColumns[k]->poss), (sudoku[r] + c)->val);
                            pthread_mutex_unlock(&(possColumns[k]->mutex));

                            // remove the value from the possible values of the grid
                            int indexGrid = getIndexPossGrid(n, r, k);
                            pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                            findAndReduceListCount(&(possGrids[indexGrid]->poss), (sudoku[r] + c)->val);
                            pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));
                        }
                    }
                }

                // remove the value from the possible values of the cells in the same column
                for (int k = 0; k < n; ++k)
                {
                    if (k == r)
                    {
                        continue;
                    }

                    if ((sudoku[r] + c)->val == (sudoku[k] + c)->val) // not valid
                    {
                        *(((markupParams *)params)->isValid) = 0;
                        break;
                    }

                    // if the cell has val as a possible value, remove it
                    if ((sudoku[k] + c)->val == 0)
                    {
                        pthread_mutex_lock(&((sudoku[k] + c)->mutex));
                        int found = findAndRemoveList(&(sudoku[k] + c)->poss, (sudoku[r] + c)->val);
                        pthread_mutex_unlock(&((sudoku[k] + c)->mutex));
                        if (found == 1)
                        {
                            // remove the value from the possible values of the row
                            pthread_mutex_lock(&(possRows[k]->mutex));
                            findAndReduceListCount(&(possRows[k]->poss), (sudoku[r] + c)->val);
                            pthread_mutex_unlock(&(possRows[k]->mutex));

                            // remove the value from the possible values of the grid
                            int indexGrid = getIndexPossGrid(n, k, c);
                            pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                            findAndReduceListCount(&(possGrids[indexGrid]->poss), (sudoku[r] + c)->val);
                            pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));
                        }
                    }
                }

                // remove the value from the possible values of the cells in the same grid
                int grid = getIndexPossGrid(n, r, c);
                int rowN = sqrt(n);
                int startI = grid / rowN * rowN;
                int startJ = grid % rowN * rowN;
                for (int k = 0; k < rowN; ++k)
                {
                    for (int m = 0; m < rowN; ++m)
                    {
                        if (startI + k == r && startJ + m == c)
                        {
                            continue;
                        }

                        if ((sudoku[r] + c)->val == (sudoku[startI + k] + startJ + m)->val) // not valid
                        {
                            *(((markupParams *)params)->isValid) = 0;
                            break;
                        }

                        // if the cell has val as a possible value, remove it
                        if ((sudoku[startI + k] + startJ + m)->val == 0)
                        {
                            pthread_mutex_lock(&((sudoku[startI + k] + startJ + m)->mutex));
                            int found = findAndRemoveList(&(sudoku[startI + k] + startJ + m)->poss, (sudoku[r] + c)->val);
                            pthread_mutex_unlock(&((sudoku[startI + k] + startJ + m)->mutex));
                            if (found == 1)
                            {

                                // remove the value from the possible values of the row
                                pthread_mutex_lock(&(possRows[startI + k]->mutex));
                                findAndReduceListCount(&(possRows[startI + k]->poss), (sudoku[r] + c)->val);
                                pthread_mutex_unlock(&(possRows[startI + k]->mutex));

                                // remove the value from the possible values of the column
                                pthread_mutex_lock(&(possColumns[startJ + m]->mutex));
                                findAndReduceListCount(&(possColumns[startJ + m]->poss), (sudoku[r] + c)->val);
                                pthread_mutex_unlock(&(possColumns[startJ + m]->mutex));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if ((sudoku[r] + c)->poss == NULL) // not valid
            {
                *(((markupParams *)params)->isValid) = 0;
                break;
            }
        }
    }
    free(params);
    return 0;
}

// Check for every cell in row, if the cells (r,c) is a singleton and set it. Return 1 if a cell has been set, 0 otherwise. If the cell has no possible values (sudoku unsolvable), return -1.
void *solveSingleton(void *params)
{
    cell **sudoku = ((solveSingletonParams *)params)->sudoku;
    int n = ((solveSingletonParams *)params)->n;
    int n_thread = ((solveSingletonParams *)params)->n_thread;
    int max_threads = ((solveSingletonParams *)params)->max_threads;

    int portion = n * n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n * n;
    }

    for (int i = start; i < end; ++i)
    {
        int r = i / n;
        int c = i % n;
        if (*(((solveSingletonParams *)params)->changed) == -1)
            break;
        if ((sudoku[r] + c)->val == 0) // if it's a void cell
        {
            if ((sudoku[r] + c)->poss == NULL) // sudoku unsolvable
            {
                *(((solveSingletonParams *)params)->changed) = -1;
            }
            else
            {
                if ((sudoku[r] + c)->poss->next == NULL) // if it's a singleton (has only 1 possible value)
                {
                    (sudoku[r] + c)->val = (sudoku[r] + c)->poss->val;
                    *(((solveSingletonParams *)params)->changed) = 1;
                }
            }
        }
    }
    free(params);
    return 0;
}

// Check if exist for a row a possible value where only one cell is possible.
void *solveLoneRangerR(void *params)
{
    cell **sudoku = ((loneRangerParams *)params)->sudoku;
    possRCG **possRcg = ((loneRangerParams *)params)->possRcg;
    int n = ((loneRangerParams *)params)->n;
    int n_thread = ((loneRangerParams *)params)->n_thread;
    int max_threads = ((loneRangerParams *)params)->max_threads;

    int portion = n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n;
    }

    for (int i = start; i < end; i++) // for every row
    {
        int lastVal = 0;
        pthread_mutex_lock(&(possRcg[i]->mutex));
        for (listCount *l = findNextListCount(possRcg[i]->poss, lastVal); l != NULL; l = findNextListCount(possRcg[i]->poss, lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                for (int c = 0; c < n; ++c)
                {
                    if ((sudoku[i] + c)->val == 0)
                    {
                        pthread_mutex_lock(&((sudoku[i] + c)->mutex));
                        if (findAndRemoveList(&(sudoku[i] + c)->poss, l->val) == 1) // if it's the lone ranger in the row
                        {
                            (*((loneRangerParams *)params)->changed) = 1;
                            (sudoku[i] + c)->val = l->val;
                            pthread_mutex_unlock(&((sudoku[i] + c)->mutex));
                            break;
                        }
                        else
                        {
                            pthread_mutex_unlock(&((sudoku[i] + c)->mutex));
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&(possRcg[i]->mutex));
    }
    free(params);
    return 0;
}

// Check if exist for a column a possible value where only one cell is possible.
void *solveLoneRangerC(void *params)
{
    cell **sudoku = ((loneRangerParams *)params)->sudoku;
    possRCG **possRcg = ((loneRangerParams *)params)->possRcg;
    int n = ((loneRangerParams *)params)->n;
    int n_thread = ((loneRangerParams *)params)->n_thread;
    int max_threads = ((loneRangerParams *)params)->max_threads;

    int portion = n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n;
    }

    for (int i = start; i < end; i++) // for every column
    {
        int lastVal = 0;
        pthread_mutex_lock(&(possRcg[i]->mutex));
        for (listCount *l = findNextListCount(possRcg[i]->poss, lastVal); l != NULL; l = findNextListCount(possRcg[i]->poss, lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                for (int r = 0; r < n; ++r)
                {
                    if ((sudoku[r] + i)->val == 0) // if it's the lone ranger in the column
                    {
                        pthread_mutex_lock(&((sudoku[r] + i)->mutex));
                        if (findAndRemoveList(&(sudoku[r] + i)->poss, l->val) == 1) // if it's the lone ranger in the column
                        {
                            (*((loneRangerParams *)params)->changed) = 1;
                            (sudoku[r] + i)->val = l->val;
                            pthread_mutex_unlock(&((sudoku[r] + i)->mutex));
                            break;
                        }
                        else
                        {
                            pthread_mutex_unlock(&((sudoku[r] + i)->mutex));
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&(possRcg[i]->mutex));
    }
    free(params);
    return 0;
}

// Check if exist for every grid a possible value where only one cell is possible.
void *solveLoneRangerG(void *params)
{
    cell **sudoku = ((loneRangerParams *)params)->sudoku;
    possRCG **possRcg = ((loneRangerParams *)params)->possRcg;
    int n = ((loneRangerParams *)params)->n;
    int n_thread = ((loneRangerParams *)params)->n_thread;
    int max_threads = ((loneRangerParams *)params)->max_threads;

    int portion = n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n;
    }

    int sqrtN = sqrt(n);
    for (int i = start; i < end; i++) // for every grid
    {
        int r = i / sqrtN * sqrtN;
        int c = i % sqrtN * sqrtN;
        int lastVal = 0;
        pthread_mutex_lock(&(possRcg[i]->mutex));
        for (listCount *l = findNextListCount(possRcg[i]->poss, lastVal); l != NULL; l = findNextListCount(possRcg[i]->poss, lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                for (int rDelta = 0; rDelta < sqrtN; ++rDelta)
                {
                    for (int cDelta = 0; cDelta < sqrtN; ++cDelta)
                    {
                        if ((sudoku[r + rDelta] + c + cDelta)->val == 0)
                        {
                            pthread_mutex_lock(&((sudoku[r + rDelta] + c + cDelta)->mutex));
                            if (findAndRemoveList(&(sudoku[r + rDelta] + c + cDelta)->poss, l->val) == 1) // if it's the lone ranger in the column
                            {
                                (*((loneRangerParams *)params)->changed) = 1;
                                (sudoku[r + rDelta] + c + cDelta)->val = l->val;
                                pthread_mutex_unlock(&((sudoku[r + rDelta] + c + cDelta)->mutex));
                                break;
                            }
                            else
                            {
                                pthread_mutex_unlock(&((sudoku[r + rDelta] + c + cDelta)->mutex));
                            }
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&(possRcg[i]->mutex));
    }
    free(params);
    return 0;
}

// Check if the sudoku is solved
void *isSolved(void *params)
{
    cell **sudoku = ((isSolvedParams *)params)->sudoku;
    int n = ((isSolvedParams *)params)->n;
    int n_thread = ((isSolvedParams *)params)->n_thread;
    int max_threads = ((isSolvedParams *)params)->max_threads;

    int portion = n * n / max_threads;
    int start = portion * n_thread;
    int end = portion * (n_thread + 1);
    if (n_thread == max_threads - 1)
    {
        end = n * n;
    }

    for (int i = start; i < end; ++i)
    {
        int r = i / n;
        int c = i % n;
        if ((sudoku[r] + c)->val == 0 || (*((isSolvedParams *)params)->isSolved) == 0)
        {
            *(((isSolvedParams *)params)->isSolved) = 0;
            break;
        }
    }
    free(params);
    return 0;
}

// Try to solve the sudoku. Apply solveSingleton, solveLoneRangers, solveTwins repeating every time changes something. If it doesn't change anything, check if it's solved and return the solved sudoku. If it's not solved, try guessing a value and solve again. If sudoku is unsolvable, return NULL.
void *solveSudoku(void *params)
{
    srand(time(NULL));
    int n = ((solveSudokuParams *)params)->n;
    cell **sudoku = ((solveSudokuParams *)params)->sudoku;
    possRCG **possRows = ((solveSudokuParams *)params)->possRows;
    possRCG **possColumns = ((solveSudokuParams *)params)->possColumns;
    possRCG **possGrids = ((solveSudokuParams *)params)->possGrids;

    int changed;
    do
    {
        do
        {
            changed = 0;
            if (isSudokuSolved == 1)
            {
                return 0;
            }

            // ### MARKUP ###
            int max_threads = 1;
            int isValid = 1;
            pthread_t *threads = malloc(sizeof(pthread_t) * max_threads);
            for (int i = 0; i < max_threads; ++i)
            {
                markupParams *markup = malloc(sizeof(markupParams));
                markup->sudoku = sudoku;
                markup->possRows = possRows;
                markup->possColumns = possColumns;
                markup->possGrids = possGrids;
                markup->n = n;
                markup->isValid = &isValid;
                markup->max_threads = max_threads;
                markup->n_thread = i;
                pthread_create(&threads[i], NULL, markupSudoku, (void *)markup);
            }
            // wait threads to finish
            for (int i = 0; i < max_threads; ++i)
            {
                pthread_join(threads[i], NULL);
            }
            free(threads);
            if (isValid == 0) // if sudoku is not valid
            {
                ((solveSudokuParams *)params)->sudoku = NULL;
                return 0;
            }

            // ### SINGLETON ###
            // spawn threads solving singleton
            max_threads = 2;
            int changed_singleton = 0;
            threads = malloc(sizeof(pthread_t) * max_threads);
            for (int i = 0; i < max_threads; ++i)
            {
                solveSingletonParams *singletonParam = malloc(sizeof(solveSingletonParams));
                singletonParam->sudoku = sudoku;
                singletonParam->n = n;
                singletonParam->n_thread = i;
                singletonParam->max_threads = max_threads;
                singletonParam->changed = &changed_singleton;
                pthread_create(&threads[i], NULL, solveSingleton, (void *)singletonParam);
            }
            // wait threads to finish
            for (int i = 0; i < max_threads; ++i)
            {
                pthread_join(threads[i], NULL);
            }
            changed += changed_singleton;
            free(threads);
        } while (changed > 0);
        changed = 0;

        // ### LONE RANGERS ###
        int changed_loneranger = 0;
        int max_threads = 1;
        pthread_t *threads = malloc(sizeof(pthread_t) * 3 * max_threads);
        for (int i = 0; i < max_threads; ++i)
        {
            // spawn thread solving lone rangers on every row
            loneRangerParams *loneranger = malloc(sizeof(loneRangerParams));
            loneranger->sudoku = sudoku;
            loneranger->possRcg = possRows;
            loneranger->n = n;
            loneranger->changed = &changed_loneranger;
            loneranger->n_thread = i;
            loneranger->max_threads = max_threads;
            pthread_create(&threads[i * 3], NULL, solveLoneRangerR, (void *)loneranger);

            // spawn threads solving lone rangers on every column
            loneranger = malloc(sizeof(loneRangerParams));
            loneranger->sudoku = sudoku;
            loneranger->possRcg = possColumns;
            loneranger->n = n;
            loneranger->changed = &changed_loneranger;
            loneranger->n_thread = i;
            loneranger->max_threads = max_threads;
            pthread_create(&threads[i * 3 + 1], NULL, solveLoneRangerC, (void *)loneranger);

            // spawn threads solving lone rangers on every grid
            loneranger = malloc(sizeof(loneRangerParams));
            loneranger->sudoku = sudoku;
            loneranger->possRcg = possGrids;
            loneranger->n = n;
            loneranger->changed = &changed_loneranger;
            loneranger->n_thread = i;
            loneranger->max_threads = max_threads;
            pthread_create(&threads[i * 3 + 2], NULL, solveLoneRangerG, (void *)loneranger);
        }
        // wait threads to finish
        for (int i = 0; i < max_threads * 3; ++i)
        {
            pthread_join(threads[i], NULL);
        }
        changed += changed_loneranger;
        free(threads);
    } while (changed > 0);

    // ### CHECK IF IS SOLVED ###
    int isSolvedV = 1;
    int max_threads = 3;
    pthread_t *threads = malloc(sizeof(pthread_t) * max_threads);
    for (int i = 0; i < max_threads; ++i)
    {
        isSolvedParams *isSolvedParam = malloc(sizeof(isSolvedParams));
        isSolvedParam->sudoku = sudoku;
        isSolvedParam->n = n;
        isSolvedParam->isSolved = &isSolvedV;
        isSolvedParam->n_thread = i;
        isSolvedParam->max_threads = max_threads;
        pthread_create(&threads[i], NULL, isSolved, (void *)isSolvedParam);
    }
    // wait threads to finish
    for (int i = 0; i < max_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    free(threads);

    if (isSolvedV == 1)
    {
        isSudokuSolved = 1;
        return 0;
    }
    else
    {
        // pick a random cell with value 0
        int r, c;
        do
        {
            int index = rand() % (n * n);
            r = index / n;
            c = index % n;
        } while ((sudoku[r] + c)->val != 0);

        // for each possible value, try to solve the sudoku
        int len = lengthList((sudoku[r] + c)->poss);
        solveSudokuParams *sudokuParams = malloc(sizeof(solveSudokuParams) * len);
        pthread_t *threads = malloc(sizeof(pthread_t) * len);
        int *thread_params = malloc(sizeof(int) * len);
        int n_threads = 0;
        int n_params = 0;
        for (list *l = (sudoku[r] + c)->poss; l != NULL; l = l->next)
        {
            cell **sudokuClone = malloc(n * sizeof(cell *));

            possRCG **possRowsClone = malloc(n * sizeof(possRCG *));
            possRCG **possColumnsClone = malloc(n * sizeof(possRCG *));
            possRCG **possGridsClone = malloc(n * sizeof(possRCG *));
            pthread_t *threadsB = malloc(sizeof(pthread_t) * 4);

            cloneSudokuParams *sudokuParam = malloc(sizeof(cloneSudokuParams));
            sudokuParam->sudoku = sudoku;
            sudokuParam->n = n;
            sudokuParam->clone = sudokuClone;
            pthread_create(&threadsB[0], NULL, cloneSudoku, (void *)sudokuParam);
            // cloneSudoku((void *)sudokuParam);

            possRCGParams *possRCGParam = malloc(sizeof(possRCGParams));
            possRCGParam->possRcg = possRows;
            possRCGParam->n = n;
            possRCGParam->clone = possRowsClone;
            pthread_create(&threadsB[1], NULL, clonePossRCGArray, (void *)possRCGParam);
            // clonePossRCGArray((void *)possRCGParam);

            possRCGParam = malloc(sizeof(possRCGParams));
            possRCGParam->possRcg = possColumns;
            possRCGParam->n = n;
            possRCGParam->clone = possColumnsClone;
            pthread_create(&threadsB[2], NULL, clonePossRCGArray, (void *)possRCGParam);
            // clonePossRCGArray((void *)possRCGParam);

            possRCGParam = malloc(sizeof(possRCGParams));
            possRCGParam->possRcg = possGrids;
            possRCGParam->n = n;
            possRCGParam->clone = possGridsClone;
            pthread_create(&threadsB[3], NULL, clonePossRCGArray, (void *)possRCGParam);
            // clonePossRCGArray((void *)possRCGParam);

            pthread_join(threadsB[0], NULL);
            pthread_join(threadsB[1], NULL);
            pthread_join(threadsB[2], NULL);
            pthread_join(threadsB[3], NULL);
            free(threadsB);

            (sudokuClone[r] + c)->val = l->val;

            (sudokuParams + n_params)->sudoku = sudokuClone;
            (sudokuParams + n_params)->possRows = possRowsClone;
            (sudokuParams + n_params)->possColumns = possColumnsClone;
            (sudokuParams + n_params)->possGrids = possGridsClone;
            (sudokuParams + n_params)->n = n;

            if (max_recursion_threads > 0)
            {
                pthread_create(&threads[n_threads], NULL, solveSudoku, (void *)(sudokuParams + n_params));
                thread_params[n_threads] = n_params;
                n_threads++;
                pthread_mutex_lock(&max_recursion_threads_mutex);
                max_recursion_threads--;
                pthread_mutex_unlock(&max_recursion_threads_mutex);
            }
            else
            {
                solveSudoku((void *)(sudokuParams + n_params));
                if ((sudokuParams + n_params)->sudoku != NULL)
                {
                    // destroySudoku(sudoku, n);
                    ((solveSudokuParams *)params)->sudoku = (sudokuParams + n_params)->sudoku;
                    return 0;
                }
            }
            n_params++;
        }
        for (int i = 0; i < n_threads; i++)
        {
            pthread_join(threads[i], NULL);
            int n_params = thread_params[i];
            destroyPossRCGArray((sudokuParams + n_params)->possRows, n);
            destroyPossRCGArray((sudokuParams + n_params)->possColumns, n);
            destroyPossRCGArray((sudokuParams + n_params)->possGrids, n);
            if ((sudokuParams + i)->sudoku != NULL)
            {
                // destroySudoku(sudoku, n);
                ((solveSudokuParams *)params)->sudoku = (sudokuParams + n_params)->sudoku;
                return 0;
            }
            else
            {
                // destroySudoku(sudoku, n);
            }
        }
        pthread_mutex_lock(&max_recursion_threads_mutex);
        max_recursion_threads += n_threads;
        pthread_mutex_unlock(&max_recursion_threads_mutex);
        free(threads);
        ((solveSudokuParams *)params)->sudoku = NULL;
    }
    return 0;
}

int main(void)
{
    DIR *d;
    struct dirent *dir;
    char *path = "../sudoku-examples/hex/";
    for (int i=0; i<100;i++) {
    d = opendir(path);
    long total_time = 0;
    int n_sudokus = 0;
        printf("%d\n", i);
        if (d)
        {
            while ((dir = readdir(d)) != NULL)
            {
                if (dir->d_name[0] == '.')
                    continue;

                n_sudokus++;
                char fileSudoku[100];
                strcpy(fileSudoku, path);
                strcat(fileSudoku, dir->d_name);
                printf("%s\n", fileSudoku);
                int n = 0;
                possRCG **possRows = NULL;
                possRCG **possColumns = NULL;
                possRCG **possGrids = NULL;
                cell **sudoku = readSudoku(&n, &possRows, &possColumns, &possGrids, fileSudoku);
                isSudokuSolved = 0;
                max_recursion_threads = MAX_RECURSION;

                if (sudoku == NULL)
                {
                    fprintf(stderr, "Could not read sudoku\n");
                    return 1;
                }

                clock_t begin = clock();

                solveSudokuParams *sudokuParams = malloc(sizeof(solveSudokuParams));
                sudokuParams->sudoku = sudoku;
                sudokuParams->possRows = possRows;
                sudokuParams->possColumns = possColumns;
                sudokuParams->possGrids = possGrids;
                sudokuParams->n = n;
                solveSudoku(sudokuParams);

                clock_t end = clock();
                total_time += (end - begin);

                if (sudokuParams->sudoku != NULL)
                {
                    printSudoku(sudokuParams->sudoku, possRows, possColumns, possGrids, n, 0);
                    printf("Sudoku solved in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
                }
                else
                {
                    printf("Sudoku not solved!\n");
                }

                destroySudoku(sudokuParams->sudoku, n);
                destroyPossRCGArray(possRows, n);
                destroyPossRCGArray(possColumns, n);
                destroyPossRCGArray(possGrids, n);
            }
            closedir(d);
            printf("############################################################\n");
            printf("Solved %d sudokus in %f seconds\n", n_sudokus, (double)total_time / CLOCKS_PER_SEC);
            printf("Average time: %f seconds\n", (double)total_time / CLOCKS_PER_SEC / n_sudokus);
        }
    }
    return 0;
}