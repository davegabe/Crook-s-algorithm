#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "listCount.c"

typedef struct cell
{
    int val;
    list *poss; // possible values list from 1 to n where n is size of sudoku
} cell;

//Read sudoku from file and return a 2D array of cells
cell **readSudoku(int *n, listCount ***possRows, listCount ***possColumns, listCount ***possGrids, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    cell **sudoku = NULL, **tmp;
    char line[1024];

    // read the size of the sudoku
    if (fgets(line, sizeof(line), fp))
    {
        char *scan = line;
        sscanf(scan, "%d", n);
        sudoku = malloc(*n * sizeof(sudoku));

        *possRows = malloc(*n * sizeof(listCount *));
        *possColumns = malloc(*n * sizeof(listCount *));
        *possGrids = malloc(*n * sizeof(listCount *));
        for (int i = 0; i < *n; i++)
        {
            //create a listCount of all possible values (from 1 to 9)
            listCount *last = getListCount(*n);
            (*possRows)[i] = last;

            //create the possible values listCount for each column
            last = getListCount(*n);
            (*possColumns)[i] = last;

            //create the possible values listCount for each cell
            last = getListCount(*n);
            (*possGrids)[i] = last;
        }

        // read the sudoku
        for (size_t i = 0; i < *n; ++i)
        {
            sudoku[i] = calloc(*n, sizeof(cell));
            for (size_t j = 0; j < *n; ++j)
            {
                int t;
                fscanf(fp, "%d", &t);
                //value of cell is the same as the value in the file
                (sudoku[i] + j)->val = t;
                //create a list of all possible values (from 1 to 9)
                list *last = getList(*n);
                (sudoku[i] + j)->poss = last;
            }
        }

        fclose(fp);

        return sudoku;
    }
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

//Destroy and free memory of l (array of listCount)
listCount **destroyListCountArray(listCount **l, int n)
{
    for (int i = 0; i < n; i++)
    {
        destroyListCount(&l[i]);
    }
    free(l);
    l = NULL;
}

//Return a copy of l (array of listCount)
listCount **cloneListCountArray(listCount **l, int n)
{
    listCount **clone = malloc(n * sizeof(listCount *));
    for (int i = 0; i < n; i++)
    {
        clone[i] = cloneListCount(l[i]);
    }
    return clone;
}

//Print the sudoku. If debug = 1, print the possible values of each cell, else print the value of each cell.
void printSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int debug)
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

    for (size_t i = 0; i < n; ++i)
    {
        if (debug == 0)
        {
            if (i % 3 == 0)
            {
                printf("----------------------\n");
            }
            for (size_t j = 0; j < n; ++j)
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
            for (size_t j = 0; j < n; ++j)
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
            printPossListCount(possRows[i]);

            printf("Grid %d = ", i);
            printPossListCount(possGrids[i]);

            printf("Column %i = ", i);
            printPossListCount(possColumns[i]);
            printf("\n");
        }
    }
}

//Return index of grid (from 0 to n-1) where cell (r,c) is located
int getIndexPossGrid(int n, int r, int c)
{
    int rowN = sqrt(n);
    return (r / rowN) * rowN + c / rowN;
}

//Set the value of cell (r,c) to val and remove val from the possible values of the cell and the row, column and grid
void setCell(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int r, int c, int val)
{
    //set value in sudoku
    (sudoku[r] + c)->val = val;
    //remove the poss values from the possible values of the row
    reduceListCount(&possRows[r], (sudoku[r] + c)->poss);
    //remove the poss values from the possible values of the column
    reduceListCount(&possColumns[c], (sudoku[r] + c)->poss);
    //remove the poss values from the possible values of the grid
    reduceListCount(&possGrids[getIndexPossGrid(n, r, c)], (sudoku[r] + c)->poss);
    //remove all poss values from the cell
    destroyList(&((sudoku[r] + c)->poss));

    //remove the value from the possible values of the cells in the same row
    for (size_t k = 0; k < n; ++k)
    {
        if (k == c)
        {
            continue;
        }
        //if the cell has val as a possible value, after removing it...
        if (findAndRemoveList(&(sudoku[r] + k)->poss, val) == 1)
        {
            //remove the value from the possible values of the row
            findAndRemoveListCount(&(possRows[r]), val);
            //remove the value from the possible values of the column
            findAndRemoveListCount(&(possColumns[k]), val);
            //remove the value from the possible values of the grid
            findAndRemoveListCount(&(possGrids[getIndexPossGrid(n, r, k)]), val);
        }
    }

    //remove the value from the possible values of the cells in the same column
    for (size_t k = 0; k < n; ++k)
    {
        if (k == r)
        {
            continue;
        }
        //if the cell has val as a possible value, after removing it...
        if (findAndRemoveList(&(sudoku[k] + c)->poss, val) == 1)
        {
            //remove the value from the possible values of the row
            findAndRemoveListCount(&(possRows[k]), val);
            //remove the value from the possible values of the column
            findAndRemoveListCount(&(possColumns[c]), val);
            //remove the value from the possible values of the grid
            findAndRemoveListCount(&(possGrids[getIndexPossGrid(n, k, c)]), val);
        }
    }

    //remove the value from the possible values of the cells in the same grid
    int grid = getIndexPossGrid(n, r, c);
    int rowN = sqrt(n);
    int startI = grid / rowN * rowN;
    int startJ = grid % rowN * rowN;
    for (size_t k = 0; k < rowN; ++k)
    {
        for (size_t m = 0; m < rowN; ++m)
        {
            if (startI + k == r && startJ + m == c)
            {
                continue;
            }
            //if the cell has val as a possible value, after removing it...
            if (findAndRemoveList(&(sudoku[startI + k] + startJ + m)->poss, val) == 1)
            {
                //remove the value from the possible values of the row
                findAndRemoveListCount(&(possRows[startI + k]), val);
                //remove the value from the possible values of the column
                findAndRemoveListCount(&(possColumns[startJ + m]), val);
                //remove the value from the possible values of the grid
                findAndRemoveListCount(&(possGrids[getIndexPossGrid(n, startI + k, startJ + m)]), val);
            }
        }
    }
}

//Check if exist a cell with only one possible value and set it. Return 1 if a cell has been set, 0 otherwise. If a cell has no possible values (sudoku unsolvable), return -1.
int solveSingleton(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0) //if it's a void cell
            {
                if ((sudoku[i] + j)->poss == NULL) //sudoku unsolvable
                {
                    return -1;
                }
                else
                {
                    if ((sudoku[i] + j)->poss->next == NULL) //if it's a singleton (has only 1 possible value)
                    {
                        changed = 1;
                        setCell(sudoku, possRows, possColumns, possGrids, n, i, j, (sudoku[i] + j)->poss->val);
                    }
                }
            }
        }
    }
    return changed;
}

//Check if exist (for every row, column and grid) a possible value where only one cell is possible. If so, set the value of the cell. Return 1 if a cell has been set, 0 otherwise.
int solveLoneRangers(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        //for each row i
        //if row i has lone rangers (if possRows[i] has count 1 for some number)
        int lastVal = 0;
        for (listCount *l = findNextListCount(possRows[i], lastVal); l != NULL; l = findNextListCount(possRows[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) //if it's a lone ranger
            {
                //find the lone ranger in the row
                for (size_t j = 0; j < n; ++j)
                {
                    if ((sudoku[i] + j)->val == 0)
                    {
                        list *node = NULL;
                        list *prev = NULL;
                        if (findList((sudoku[i] + j)->poss, l->val, &node, &prev) == 1) //if it's the lone ranger in the row
                        {
                            changed = 1;
                            setCell(sudoku, possRows, possColumns, possGrids, n, i, j, l->val);
                            break;
                        }
                    }
                }
            }
        }

        //for each column i
        //if column i has lone rangers (if possColumns[i] has count 1 for some number)
        lastVal = 0;
        for (listCount *l = findNextListCount(possColumns[i], lastVal); l != NULL; l = findNextListCount(possColumns[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) //if it's a lone ranger
            {
                //find the lone ranger in the row
                for (size_t j = 0; j < n; ++j)
                {
                    if ((sudoku[j] + i)->val == 0)
                    {
                        list *node = NULL;
                        list *prev = NULL;
                        if (findList((sudoku[j] + i)->poss, l->val, &node, &prev) == 1) //if it's the lone ranger in the column
                        {
                            changed = 1;
                            setCell(sudoku, possRows, possColumns, possGrids, n, j, i, l->val);
                            break;
                        }
                    }
                }
            }
        }

        //for each grid i
        //if grid i has lone rangers (if possGrids[i] has count 1 for some number)
        lastVal = 0;
        for (listCount *l = findNextListCount(possGrids[i], lastVal); l != NULL; l = findNextListCount(possGrids[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) //if it's a lone ranger
            {
                //find the lone ranger in the grid
                int rowN = sqrt(n);
                int startI = i / rowN * rowN;
                int startJ = i % rowN * rowN;
                for (size_t k = 0; k < rowN; ++k)
                {
                    int br = 0;
                    for (size_t m = 0; m < rowN; ++m)
                    {
                        if ((sudoku[startI + k] + startJ + m)->val == 0)
                        {
                            list *node = NULL;
                            list *prev = NULL;
                            if (findList((sudoku[startI + k] + startJ + m)->poss, l->val, &node, &prev) == 1) //if it's the lone ranger in the grid
                            {
                                changed = 1;
                                setCell(sudoku, possRows, possColumns, possGrids, n, startI + k, startJ + m, l->val);
                                br = 1;
                                break;
                            }
                        }
                    }
                    if (br == 1)
                    {
                        break;
                    }
                }
            }
        }
    }
    return changed;
}

//Check if exist (for every row, column and grid) a twinSize-tuple of cells with same possibilities of size twinSize
int solveTwins(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int twinSize)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        //for each row i, column j
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0 && lengthList((sudoku[i] + j)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
            {
                int *colTwins = malloc(sizeof(int) * twinSize);
                int foundTwins = 1;
                colTwins[0] = j;
                for (size_t j1 = j + 1; j1 < n && foundTwins != twinSize; ++j1) //check other cells in the row
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
                    for (size_t k = 0; k < n; ++k)
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
            }
        }

        //for each column i, row j
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[j] + i)->val == 0 && lengthList((sudoku[j] + i)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
            {
                int *rowTwins = malloc(sizeof(int) * twinSize);
                int foundTwins = 1;
                rowTwins[0] = j;
                for (size_t j1 = j + 1; j1 < n && foundTwins != twinSize; ++j1) //check other cells in the column
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
                    for (size_t k = 0; k < n; ++k)
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
            }
        }

        //for each grid i
        int rowN = sqrt(n);
        int startI = i / rowN * rowN;
        int startJ = i % rowN * rowN;
        for (size_t k = 0; k < rowN; ++k)
        {
            for (size_t m = 0; m < rowN; ++m)
            {
                if ((sudoku[k] + m)->val == 0 && lengthList((sudoku[k] + m)->poss) == twinSize) //if cell has twinSize poss, it MAY be a twinSize twin
                {
                    int *gridTwinsR = malloc(sizeof(int) * twinSize);
                    int *gridTwinsC = malloc(sizeof(int) * twinSize);
                    int foundTwins = 1;
                    gridTwinsR[0] = k;
                    gridTwinsC[0] = m;
                    //check other cells in the grid
                    for (size_t k1 = k; k1 < rowN; ++k1)
                    {
                        for (size_t m1 = (k1 == k ? m + 1 : 0); m1 < rowN; ++m1)
                        {
                            if (isEqualList((sudoku[k] + m)->poss, (sudoku[k1] + m1)->poss) == 1) //if it's equal, it's a twin
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
                        for (size_t k1 = 0; k1 < rowN; ++k1)
                        {
                            for (size_t m1 = 0; m1 < rowN; ++m1)
                            {
                                if (k1 == gridTwinsR[index] && m1 == gridTwinsC[index]) //if it's a twin, ignore
                                {
                                    if (index < foundTwins - 1)
                                        index++;
                                }
                                else //if it's not a twin, remove poss
                                {
                                    //for each possible value in the twin cell, remove it from the possible values of the other cells in the same grid
                                    for (list *l = (sudoku[k] + m)->poss; l != NULL; l = l->next)
                                    {
                                        if (findAndRemoveList(&(sudoku[k1] + m1)->poss, l->val) == 1) //if removed something
                                        {
                                            changed = 1;
                                            //remove the poss values from the possible values of the row
                                            findAndRemoveListCount(&possRows[k1], l->val);
                                            //remove the poss values from the possible values of the column
                                            findAndRemoveListCount(&possColumns[m1], l->val);
                                            //remove the poss values from the possible values of the grid
                                            findAndRemoveListCount(&possGrids[getIndexPossGrid(n, k1, m1)], l->val);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return changed;
}

//Check if the sudoku is solved
int isSolved(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (possRows[i] != NULL || possColumns[i] != NULL || possGrids[i] != NULL)
        {
            return 0;
        }

        for (size_t j = 0; j < n; ++j)
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
cell **solveSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed;
    do
    {
        changed = 0;
        int changedSingleton = solveSingleton(sudoku, possRows, possColumns, possGrids, n);
        if (changedSingleton == 1)
        {
            changed = 1;
            // printf("SINGLETON\n");
        }
        else if (changed == -1)
        {
            return NULL;
        }

        int changedLoneRangers = solveLoneRangers(sudoku, possRows, possColumns, possGrids, n);
        if (changedLoneRangers == 1)
        {
            changed = 1;
            // printf("LONE RANGERS\n");
        }

        for (int twinSize = 2; twinSize < n - 1; twinSize++)
        {
            int changedTwins = solveTwins(sudoku, possRows, possColumns, possGrids, n, twinSize);
            if (changedTwins == 1)
            {
                changed = 1;
                // printf("TWINS %d\n", twinSize);
                break;
            }
        }
    } while (changed > 0);

    if (isSolved(sudoku, possRows, possColumns, possGrids, n) == 1)
    {
        return sudoku;
    }
    else
    {
        //pick the first cell with value 0
        // printf("RANDOM\n");
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
            listCount **possRowsClone = cloneListCountArray(possRows, n);
            listCount **possColumnsClone = cloneListCountArray(possColumns, n);
            listCount **possGridsClone = cloneListCountArray(possGrids, n);
            setCell(sudokuClone, possRowsClone, possColumnsClone, possGridsClone, n, r, c, l->val);

            cell **solvedSudoku = solveSudoku(sudokuClone, possRowsClone, possColumnsClone, possGridsClone, n);
            destroyListCountArray(possRowsClone, n);
            destroyListCountArray(possColumnsClone, n);
            destroyListCountArray(possGridsClone, n);
            if (solvedSudoku != NULL)
            {
                // destroySudoku(sudoku, n);
                return solvedSudoku;
            }
            destroySudoku(sudokuClone, n);
        }
    }
    return NULL;
}

int main(void)
{
    int n = 0;
    listCount **possRows = NULL;
    listCount **possColumns = NULL;
    listCount **possGrids = NULL;
    cell **sudoku = readSudoku(&n, &possRows, &possColumns, &possGrids, "sudoku.txt");

    if (sudoku == NULL)
    {
        fprintf(stderr, "could not read sudoku\n");
        return 1;
    }

    for (size_t r = 0; r < n; ++r)
    {
        for (size_t c = 0; c < n; ++c)
        {
            if ((sudoku[r] + c)->val != 0)
            {
                setCell(sudoku, possRows, possColumns, possGrids, n, r, c, (sudoku[r] + c)->val);
            }
        }
    }

    cell **solvedSudoku = solveSudoku(sudoku, possRows, possColumns, possGrids, n);

    if (solvedSudoku != NULL)
    {
        printf("Sudoku solved!\n");
        printSudoku(solvedSudoku, possRows, possColumns, possGrids, n, 0);
    }
    else
    {
        printf("Sudoku not solved!\n");
        //printSudoku(sudoku, possRows, possColumns, possGrids, n, 1);
    }

    destroySudoku(sudoku, n);
    destroyListCountArray(possRows, n);
    destroyListCountArray(possColumns, n);
    destroyListCountArray(possGrids, n);
    return 0;
}