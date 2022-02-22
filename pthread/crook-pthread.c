//TODO: PARALLEL CLONE METHODS (and eventually destroy methods)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "listCount.c"

//Cell of a Sudoku
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
    int changed;           // 0 := not changed, 1 := changed, -1 := invalid
    listCount *poss;       // possible values list from 1 to n where n is size of sudoku
    pthread_mutex_t mutex; // mutex for every list in poss
} possRCG;

// Params for markupSudoku function
typedef struct markupParams
{
    cell **sudoku;         // sudoku to be solved
    int n;                 // size of sudoku
    int r;                 // r of current cell
    int c;                 // c of current cell
    possRCG **possRows;    // possible values for each row
    possRCG **possColumns; // possible values for each column
    possRCG **possGrids;   // possible values for each grid
} markupParams;

// Params for markupSudoku function
typedef struct loneRangerParams
{
    cell **sudoku;
    possRCG *possRcg;
    int r;
    int c;
    int n;
    int changed;
} loneRangerParams;

//Read sudoku from file and return a 2D array of cells
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

            //create a listCount of all possible values (from 1 to 9)
            (*possRows)[i]->poss = getListCount(*n);
            (*possRows)[i]->changed = 0;
            pthread_mutex_init(&(*possRows)[i]->mutex, NULL);

            //create the possible values listCount for each column
            (*possColumns)[i]->poss = getListCount(*n);
            ;
            (*possColumns)[i]->changed = 0;
            pthread_mutex_init(&(*possColumns)[i]->mutex, NULL);

            //create the possible values listCount for each cell
            (*possGrids)[i]->poss = getListCount(*n);
            (*possGrids)[i]->changed = 0;
            pthread_mutex_init(&(*possGrids)[i]->mutex, NULL);
        }

        // read the sudoku
        for (int i = 0; i < *n; ++i)
        {
            sudoku[i] = calloc(*n, sizeof(cell));
            for (int j = 0; j < *n; ++j)
            {
                int t;
                fscanf(fp, "%d", &t);
                //value of cell is the same as the value in the file
                (sudoku[i] + j)->val = t;
                //create a list of all possible values (from 1 to 9)
                list *last = getList(*n);
                (sudoku[i] + j)->poss = last;
                //init changed to 0
                (sudoku[i] + j)->changed = 0;
                //init mutex for the cell
                pthread_mutex_init(&((sudoku[i] + j)->mutex), NULL);
            }
        }

        fclose(fp);

        return sudoku;
    }
    return NULL;
}

//Return a copy of sudoku
cell **cloneSudoku(cell **sudoku, const int n)
{
    cell **clone = malloc(n * sizeof(cell *));
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
    return clone;
}

//Destroy and free memory of sudoku (2D array of cells)
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

//Destroy and free memory of l (array of possRCG)
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

//Return a copy of l (array of possRCG)
possRCG **clonePossRCGArray(possRCG **l, const int n)
{
    possRCG **clone = malloc(n * sizeof(possRCG *));
    for (int i = 0; i < n; i++)
    {
        clone[i]->poss = cloneListCount(l[i]->poss);
        clone[i]->changed = 0;
        pthread_mutex_init(&(clone[i]->mutex), NULL);
    }
    return clone;
}

//Print the sudoku. If debug = 1, print the possible values of each cell, else print the value of each cell.
void printSudoku(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n, int debug)
{
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
            if (i % 3 == 0)
            {
                printf("----------------------\n");
            }
            for (int j = 0; j < n; ++j)
            {
                if (j % 3 == 0)
                {
                    printf("|");
                }
                printf("%d ", (sudoku[i] + j)->val);
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

            printf("Column %i = ", i);
            printPossListCount(possColumns[i]->poss);
            printf("\n");
        }
    }
}

//Return index of grid (from 0 to n-1) where cell (r,c) is located
int getIndexPossGrid(const int n, const int r, const int c)
{
    int rowN = sqrt(n);
    return (r / rowN) * rowN + c / rowN;
}

//Markup sudoku and find possible values for each cell
void *markupSudoku(void *params)
{
    cell **sudoku = ((markupParams *)params)->sudoku;
    possRCG **possRows = ((markupParams *)params)->possRows;
    possRCG **possColumns = ((markupParams *)params)->possColumns;
    possRCG **possGrids = ((markupParams *)params)->possGrids;
    int n = ((markupParams *)params)->n;
    int r = ((markupParams *)params)->r;
    int c = ((markupParams *)params)->c;

    pthread_mutex_lock(&((sudoku[r] + c)->mutex));
    if ((sudoku[r] + c)->val != 0)
    {
        list *l = cloneList((sudoku[r] + c)->poss);
        pthread_mutex_unlock(&((sudoku[r] + c)->mutex));

        //remove the poss values from the possible values of the row
        pthread_mutex_lock(&(possRows[r]->mutex));
        reduceListCount(&(possRows[r]->poss), l);
        pthread_mutex_unlock(&(possRows[r]->mutex));

        //remove the poss values from the possible values of the column
        pthread_mutex_lock(&(possColumns[c]->mutex));
        reduceListCount(&(possColumns[c]->poss), l);
        pthread_mutex_unlock(&(possColumns[c]->mutex));

        //remove the poss values from the possible values of the grid
        int indexGrid = getIndexPossGrid(n, r, c);
        pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
        reduceListCount(&(possGrids[indexGrid]->poss), l);
        pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));

        //remove all poss values from the cell
        pthread_mutex_lock(&((sudoku[r] + c)->mutex));
        (sudoku[r] + c)->changed = 0;
        destroyList(&((sudoku[r] + c)->poss));
        pthread_mutex_unlock(&((sudoku[r] + c)->mutex));

        //remove the value from the possible values of the cells in the same row
        for (int k = 0; k < n; ++k)
        {
            if (k == c)
            {
                continue;
            }
            //if the cell has val as a possible value, remove it
            pthread_mutex_lock(&((sudoku[r] + k)->mutex));
            int found = findAndRemoveList(&(sudoku[r] + k)->poss, (sudoku[r] + c)->val);
            pthread_mutex_unlock(&((sudoku[r] + k)->mutex));
            if (found == 1)
            {
                //remove the value from the possible values of the row
                pthread_mutex_lock(&(possRows[r]->mutex));
                findAndRemoveListCount(&(possRows[r]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possRows[r]->mutex));

                //remove the value from the possible values of the column
                pthread_mutex_lock(&(possColumns[k]->mutex));
                findAndRemoveListCount(&(possColumns[k]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possColumns[k]->mutex));

                //remove the value from the possible values of the grid
                int indexGrid = getIndexPossGrid(n, r, k);
                pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                findAndRemoveListCount(&(possGrids[indexGrid]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));
            }
        }

        //remove the value from the possible values of the cells in the same column
        for (int k = 0; k < n; ++k)
        {
            if (k == r)
            {
                continue;
            }
            //if the cell has val as a possible value, remove it
            pthread_mutex_lock(&((sudoku[k] + c)->mutex));
            int found = findAndRemoveList(&(sudoku[k] + c)->poss, (sudoku[r] + c)->val);
            pthread_mutex_unlock(&((sudoku[k] + c)->mutex));
            if (found == 1)
            {
                //remove the value from the possible values of the row
                pthread_mutex_lock(&(possRows[k]->mutex));
                findAndRemoveListCount(&(possRows[k]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possRows[k]->mutex));

                //remove the value from the possible values of the column
                pthread_mutex_lock(&(possColumns[c]->mutex));
                findAndRemoveListCount(&(possColumns[c]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possColumns[c]->mutex));

                //remove the value from the possible values of the grid
                int indexGrid = getIndexPossGrid(n, k, c);
                pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                findAndRemoveListCount(&(possGrids[indexGrid]->poss), (sudoku[r] + c)->val);
                pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));
            }
        }

        //remove the value from the possible values of the cells in the same grid
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
                //if the cell has val as a possible value, remove it
                pthread_mutex_lock(&((sudoku[startI + k] + startJ + m)->mutex));
                int found = findAndRemoveList(&(sudoku[startI + k] + startJ + m)->poss, (sudoku[r] + c)->val);
                pthread_mutex_unlock(&((sudoku[startI + k] + startJ + m)->mutex));
                if (found == 1)
                {

                    //remove the value from the possible values of the row
                    pthread_mutex_lock(&(possRows[startI + k]->mutex));
                    findAndRemoveListCount(&(possRows[startI + k]->poss), (sudoku[r] + c)->val);
                    pthread_mutex_unlock(&(possRows[startI + k]->mutex));

                    //remove the value from the possible values of the column
                    pthread_mutex_lock(&(possColumns[startJ + m]->mutex));
                    findAndRemoveListCount(&(possColumns[startJ + m]->poss), (sudoku[r] + c)->val);
                    pthread_mutex_unlock(&(possColumns[startJ + m]->mutex));

                    //remove the value from the possible values of the grid
                    int indexGrid = getIndexPossGrid(n, startI + k, startJ + m);
                    pthread_mutex_lock(&(possGrids[indexGrid]->mutex));
                    findAndRemoveListCount(&(possGrids[indexGrid]->poss), (sudoku[r] + c)->val);
                    pthread_mutex_unlock(&(possGrids[indexGrid]->mutex));
                }
            }
        }
    }
    else
    {
        pthread_mutex_unlock(&((sudoku[r] + c)->mutex));
    }

    //free the memory allocated for the markupParams
    free(params);
    params = NULL;
    return 0;
}

//Check if the cell (r,c) is a singleton and set it. Return 1 if a cell has been set, 0 otherwise. If the cell has no possible values (sudoku unsolvable), return -1.
void *solveSingleton(void *paramCell)
{
    cell *c = ((cell *)paramCell);
    if (c->val == 0) //if it's a void cell
    {
        if (c->poss == NULL) //sudoku unsolvable
        {
            c->changed = -1;
        }
        else
        {
            if (c->poss->next == NULL) //if it's a singleton (has only 1 possible value)
            {
                c->val = c->poss->val;
                c->changed = 1;
            }
        }
    }
    c->changed = 0;
    return 0;
}

//Check if exist for a row/columns/grid a possible value where only one cell is possible.
void *solveLoneRangerRCG(void *params)
{
    cell **sudoku = ((loneRangerParams *)params)->sudoku;
    possRCG *possRcg = ((loneRangerParams *)params)->possRcg;
    int n = ((markupParams *)params)->n;
    int r = ((markupParams *)params)->r;
    int c = ((markupParams *)params)->c;

    //if has lone rangers (if possRcg has count 1 for some number)
    int lastVal = 0;
    for (listCount *l = findNextListCount(possRcg->poss, lastVal); l != NULL; l = findNextListCount(possRcg->poss, lastVal))
    {
        lastVal = l->val;
        if (l->count == 1) //if it's a lone ranger
        {
            if (r != -1 && c == -1) //find the lone ranger in the row
            {
                for (int c = 0; c < n; ++c)
                {
                    if ((sudoku[r] + c)->val == 0 && findAndRemoveList(&(sudoku[r] + c)->poss, l->val) == 1) //if it's the lone ranger in the row
                    {
                        possRcg->changed = 1;
                        ((loneRangerParams *)params)->changed = 1;
                        pthread_mutex_lock(&((sudoku[r] + c)->mutex));
                        (sudoku[r] + c)->val = l->val;
                        pthread_mutex_unlock(&((sudoku[r] + c)->mutex));
                        break;
                    }
                }
            }
            else if (r == -1 && c != -1) //find the lone ranger in the column
            {
                for (int r = 0; r < n; ++r)
                {
                    if ((sudoku[r] + c)->val == 0 && findAndRemoveList(&(sudoku[r] + c)->poss, l->val) == 1) //if it's the lone ranger in the column
                    {
                        possRcg->changed = 1;
                        ((loneRangerParams *)params)->changed = 1;
                        pthread_mutex_lock(&((sudoku[r] + c)->mutex));
                        (sudoku[r] + c)->val = l->val;
                        pthread_mutex_unlock(&((sudoku[r] + c)->mutex));
                        break;
                    }
                }
            }
            else //find the lone ranger in the grid
            {
                int rowN = sqrt(n);
                for (int rDelta = 0; rDelta < rowN; ++rDelta)
                {
                    for (int cDelta = 0; cDelta < rowN; ++cDelta)
                    {
                        if ((sudoku[r + rDelta] + c + cDelta)->val == 0 && findAndRemoveList(&(sudoku[r + rDelta] + c + cDelta)->poss, l->val) == 1) //if it's the lone ranger in the column
                        {
                            possRcg->changed = 1;
                            ((loneRangerParams *)params)->changed = 1;
                            pthread_mutex_lock(&((sudoku[r + rDelta] + c + cDelta)->mutex));
                            (sudoku[r + rDelta] + c + cDelta)->val = l->val;
                            pthread_mutex_unlock(&((sudoku[r + rDelta] + c + cDelta)->mutex));
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

//Check if exist (for every row, column and grid) a twinSize-tuple of cells with same possibilities of size twinSize
int solveTwins(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int twinSize)
{
    int changed = 0;
    for (int i = 0; i < n; ++i)
    {
        //for each row i, column j
        for (int j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0 && lengthList((sudoku[i] + j)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
            {
                int *colTwins = malloc(sizeof(int) * twinSize);
                int foundTwins = 1;
                colTwins[0] = j;
                for (int j1 = j + 1; j1 < n && foundTwins != twinSize; ++j1) //check other cells in the row
                {
                    if (isEqualList((sudoku[i] + j)->poss, (sudoku[i] + j1)->poss) == 1) //if it's equal, it's a twin
                    {
                        colTwins[foundTwins] = j1;
                        foundTwins++;
                    }
                }
                if (foundTwins == twinSize) //if we found all the twins
                {
                    //remove the values from the possible values of the cells in the same row
                    int index = 0;
                    for (int k = 0; k < n; ++k)
                    {
                        if ((sudoku[i] + k)->val == 0)
                        {
                            if (k == colTwins[index]) //if it's a twin, ignore
                            {
                                if (index < foundTwins - 1)
                                    index++;
                            }
                            else //if it's not a twin, remove poss
                            {
                                //for each possible value in the twin cell, remove it from the possible values of the other cells in the same row
                                for (list *l = (sudoku[i] + j)->poss; l != NULL; l = l->next)
                                {
                                    if (findAndRemoveList(&(sudoku[i] + k)->poss, l->val) == 1) //if removed something
                                    {
                                        changed = 1;
                                        //remove the poss values from the possible values of the row
                                        findAndRemoveListCount(&possRows[i], l->val);
                                        //remove the poss values from the possible values of the column
                                        findAndRemoveListCount(&possColumns[k], l->val);
                                        //remove the poss values from the possible values of the grid
                                        findAndRemoveListCount(&possGrids[getIndexPossGrid(n, i, k)], l->val);
                                    }
                                }
                            }
                        }
                    }
                }
                free(colTwins);
            }
        }

        //for each column i, row j
        for (int j = 0; j < n; ++j)
        {
            if ((sudoku[j] + i)->val == 0 && lengthList((sudoku[j] + i)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
            {
                int *rowTwins = malloc(sizeof(int) * twinSize);
                int foundTwins = 1;
                rowTwins[0] = j;
                for (int j1 = j + 1; j1 < n && foundTwins != twinSize; ++j1) //check other cells in the column
                {
                    if (isEqualList((sudoku[j] + i)->poss, (sudoku[j1] + i)->poss) == 1) //if it's equal, it's a twin
                    {
                        rowTwins[foundTwins] = j1;
                        foundTwins++;
                    }
                }
                if (foundTwins == twinSize) //if we found all the twins
                {
                    //remove the value from the possible values of the cells in the same column
                    int index = 0;
                    for (int k = 0; k < n; ++k)
                    {
                        if ((sudoku[k] + i)->val == 0)
                        {
                            if (k == rowTwins[index]) //if it's a twin, ignore
                            {
                                if (index < foundTwins - 1)
                                    index++;
                            }
                            else //if it's not a twin, remove poss
                            {
                                //for each possible value in the twin cell, remove it from the possible values of the other cells in the same column
                                for (list *l = (sudoku[j] + i)->poss; l != NULL; l = l->next)
                                {
                                    if (findAndRemoveList(&(sudoku[k] + i)->poss, l->val) == 1) //if removed something
                                    {
                                        changed = 1;
                                        //remove the poss values from the possible values of the row
                                        findAndRemoveListCount(&possRows[k], l->val);
                                        //remove the poss values from the possible values of the column
                                        findAndRemoveListCount(&possColumns[i], l->val);
                                        //remove the poss values from the possible values of the grid
                                        findAndRemoveListCount(&possGrids[getIndexPossGrid(n, k, i)], l->val);
                                    }
                                }
                            }
                        }
                    }
                }
                free(rowTwins);
            }
        }

        //for each grid i
        int rowN = sqrt(n);
        int startI = i / rowN * rowN;
        int startJ = i % rowN * rowN;
        for (int k = 0; k < rowN; ++k)
        {
            for (int m = 0; m < rowN; ++m)
            {
                if ((sudoku[startI + k] + startJ + m)->val == 0 && lengthList((sudoku[startI + k] + startJ + m)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
                {
                    int *gridTwinsR = malloc(sizeof(int) * twinSize);
                    int *gridTwinsC = malloc(sizeof(int) * twinSize);
                    int foundTwins = 1;
                    gridTwinsR[0] = k;
                    gridTwinsC[0] = m;
                    //check other cells in the grid
                    for (int k1 = k; k1 < rowN; ++k1)
                    {
                        for (int m1 = (k1 == k ? m + 1 : 0); m1 < rowN; ++m1)
                        {
                            if (isEqualList((sudoku[startI + k] + startJ + m)->poss, (sudoku[startI + k1] + startJ + m1)->poss) == 1) //if it's equal, it's a twin
                            {
                                gridTwinsR[foundTwins] = k1;
                                gridTwinsC[foundTwins] = m1;
                                foundTwins++;
                            }
                        }
                    }
                    if (foundTwins == twinSize) //if we found all the twins
                    {
                        //remove the value from the possible values of the cells in the same row
                        int index = 0;
                        for (int k1 = 0; k1 < rowN; ++k1)
                        {
                            for (int m1 = 0; m1 < rowN; ++m1)
                            {
                                if (k1 == gridTwinsR[index] && m1 == gridTwinsC[index]) //if it's a twin, ignore
                                {
                                    if (index < foundTwins - 1)
                                        index++;
                                }
                                else //if it's not a twin, remove poss
                                {
                                    //for each possible value in the twin cell, remove it from the possible values of the other cells in the same grid
                                    for (list *l = (sudoku[startI + k] + startJ + m)->poss; l != NULL; l = l->next)
                                    {
                                        if (findAndRemoveList(&(sudoku[startI + k1] + startJ + m1)->poss, l->val) == 1) //if removed something
                                        {
                                            changed = 1;
                                            //remove the poss values from the possible values of the row
                                            findAndRemoveListCount(&possRows[startI + k1], l->val);
                                            //remove the poss values from the possible values of the column
                                            findAndRemoveListCount(&possColumns[startJ + m1], l->val);
                                            //remove the poss values from the possible values of the grid
                                            findAndRemoveListCount(&possGrids[getIndexPossGrid(n, startI + k1, startJ + m1)], l->val);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    free(gridTwinsR);
                    free(gridTwinsC);
                }
            }
        }
    }
    return changed;
}

//Check if the sudoku is solved
int isSolved(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (possRows[i] != NULL || possColumns[i] != NULL || possGrids[i] != NULL)
        {
            return 0;
        }

        for (int j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

//Try to solve the sudoku. Apply solveSingleton, solveLoneRangers, solveTwins repeating every time changes something. If it doesn't change anything, check if it's solved and return the solved sudoku. If it's not solved, try guessing a value and solve again. If sudoku is unsolvable, return NULL.
cell **solveSudoku(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n)
{
    int changed;
    do
    {
        do
        {
            do
            {
                changed = 0;

                // ### MARKUP ###
                pthread_t *threads = malloc(sizeof(pthread_t) * n * n);
                for (int i = 0; i < n; ++i)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        markupParams *markup = malloc(sizeof(markupParams));
                        markup->sudoku = sudoku;
                        markup->possRows = possRows;
                        markup->possColumns = possColumns;
                        markup->possGrids = possGrids;
                        markup->n = n;
                        markup->r = i;
                        markup->c = j;
                        pthread_create(&threads[i * n + j], NULL, markupSudoku, (void *)markup);
                    }
                }
                //wait threads to finish
                for (int i = 0; i < n; ++i)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        pthread_join(threads[i * n + j], NULL);
                    }
                }

                // ### SINGLETON ###
                //spawn threads solving singleton on every cell
                for (int i = 0; i < n; ++i)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        pthread_create(&threads[i * n + j], NULL, solveSingleton, (void *)(sudoku[i] + j));
                    }
                }
                //wait threads to finish
                for (int i = 0; i < n; ++i)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        pthread_join(threads[i * n + j], NULL);
                        if ((sudoku[i] + j)->changed == 1)
                        {
                            changed = 1;
                            printf("SINGLETON\n");
                        }
                        else if ((sudoku[i] + j)->changed == -1)
                        {
                            return NULL;
                        }
                    }
                }
                free(threads);
            } while (changed > 0);
            changed = 0;

            // ### LONE RANGERS ###
            pthread_t *threads = malloc(sizeof(pthread_t) * n * 3);
            for (int i = 0; i < n; ++i)
            {
                //spawn threads solving lone rangers on every row
                loneRangerParams *loneranger = malloc(sizeof(loneRangerParams));
                loneranger->sudoku = sudoku;
                loneranger->possRcg = possRows[i];
                loneranger->changed = 0;
                loneranger->n = n;
                loneranger->r = i;
                loneranger->c = -1;
                pthread_create(&threads[i], NULL, solveLoneRangerRCG, (void *)loneranger);

                //spawn threads solving lone rangers on every column
                loneranger = malloc(sizeof(loneRangerParams));
                loneranger->sudoku = sudoku;
                loneranger->possRcg = possColumns[i];
                loneranger->changed = 0;
                loneranger->n = n;
                loneranger->r = -1;
                loneranger->c = i;
                pthread_create(&threads[i], NULL, solveLoneRangerRCG, (void *)loneranger);

                //spawn threads solving lone rangers on every grid
                int rowN = sqrt(n);
                loneranger = malloc(sizeof(loneRangerParams));
                loneranger->sudoku = sudoku;
                loneranger->possRcg = possGrids[i];
                loneranger->changed = 0;
                loneranger->n = n;
                loneranger->r = i / rowN * rowN;
                loneranger->c = i % rowN * rowN;
                pthread_create(&threads[i], NULL, solveLoneRangerRCG, (void *)loneranger);
            }
            //wait threads to finish
            for (int i = 0; i < 3 * n; ++i)
            {
                pthread_join(threads[i], NULL);
                if (i / n == 0) //possRows
                {
                    if ((possRows[i % n])->changed == 1)
                    {
                        changed = 1;
                        printf("LONE RANGERS\n");
                    }
                }
                else if (i / n == 1) //possColumns
                {
                    if ((possColumns[i % n])->changed == 1)
                    {
                        changed = 1;
                        printf("LONE RANGERS\n");
                    }
                }
                else //possGrids
                {
                    if ((possGrids[i % n])->changed == 1)
                    {
                        changed = 1;
                        printf("LONE RANGERS\n");
                    }
                }
            }
        } while (changed > 0);
        // for (int twinSize = 2; twinSize < n - 1; twinSize++)
        // {
        //     int changedTwins = solveTwins(sudoku, possRows, possColumns, possGrids, n, twinSize);
        //     if (changedTwins == 1)
        //     {
        //         changed = 1;
        //         // printf("TWINS %d\n", twinSize);
        //         break;
        //     }
        // }
    } while (changed > 0);

    if (isSolved(sudoku, possRows, possColumns, possGrids, n) == 1)
    {
        return sudoku;
    }
    else
    {
        //pick the first cell with value 0
        printf("RANDOM\n");
        int r, c;
        for (int index = 0; index < n * n; index++)
        {
            r = index / n;
            c = index % n;
            if ((sudoku[r] + c)->val == 0)
            {
                break;
            }
        }

        //for each possible value, try to solve the sudoku
        for (list *l = (sudoku[r] + c)->poss; l != NULL; l = l->next)
        {
            cell **sudokuClone = cloneSudoku(sudoku, n);
            possRCG **possRowsClone = clonePossRCGArray(possRows, n);
            possRCG **possColumnsClone = clonePossRCGArray(possColumns, n);
            possRCG **possGridsClone = clonePossRCGArray(possGrids, n);

            (sudokuClone[r] + c)->val = l->val;

            cell **solvedSudoku = solveSudoku(sudokuClone, possRowsClone, possColumnsClone, possGridsClone, n);
            destroyPossRCGArray(possRowsClone, n);
            destroyPossRCGArray(possColumnsClone, n);
            destroyPossRCGArray(possGridsClone, n);
            if (solvedSudoku != NULL)
            {
                destroySudoku(sudoku, n);
                return solvedSudoku;
            }
            else
            {
                destroySudoku(sudokuClone, n);
            }
        }

        return NULL;
    }
}

int main(void)
{
    int n = 0;
    possRCG **possRows = NULL;
    possRCG **possColumns = NULL;
    possRCG **possGrids = NULL;
    cell **sudoku = readSudoku(&n, &possRows, &possColumns, &possGrids, "../sudoku-examples/sudoku.txt");

    if (sudoku == NULL)
    {
        fprintf(stderr, "Could not read sudoku\n");
        return 1;
    }

    clock_t begin = clock();

    cell **solvedSudoku = solveSudoku(sudoku, possRows, possColumns, possGrids, n);

    clock_t end = clock();

    if (solvedSudoku != NULL)
    {
        printSudoku(solvedSudoku, possRows, possColumns, possGrids, n, 0);
        printf("Sudoku solved in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
        destroySudoku(solvedSudoku, n);
    }
    else
    {
        printf("Sudoku not solved!\n");
        //printSudoku(sudoku, possRows, possColumns, possGrids, n, 1);
        destroySudoku(sudoku, n);
    }

    destroyPossRCGArray(possRows, n);
    destroyPossRCGArray(possColumns, n);
    destroyPossRCGArray(possGrids, n);

    return 0;
}