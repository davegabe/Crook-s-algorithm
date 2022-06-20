#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include "crook-unoptimized.h"

// Read sudoku from file and return a 2D array of cells
cell **readSudoku(int *n, listCount ***possRows, listCount ***possColumns, listCount ***possGrids, const char *filename)
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

        *possRows = malloc(*n * sizeof(listCount *));
        *possColumns = malloc(*n * sizeof(listCount *));
        *possGrids = malloc(*n * sizeof(listCount *));
        for (int i = 0; i < *n; i++)
        {
            // create a listCount of all possible values (from 1 to n)
            listCount *last = getListCount(*n);
            (*possRows)[i] = last;

            // create the possible values listCount for each column
            last = getListCount(*n);
            (*possColumns)[i] = last;

            // create the possible values listCount for each cell
            last = getListCount(*n);
            (*possGrids)[i] = last;
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
                (sudoku[i] + j)->changed = 0;
                // create a list of all possible values (from 1 to n)
                list *last = getList(*n);
                (sudoku[i] + j)->poss = last;
            }
        }

        fclose(fp);

        return sudoku;
    }
    return NULL;
}

// Return a copy of sudoku
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
        }
    }
    return clone;
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

// Destroy and free memory of l (array of listCount)
void destroyListCountArray(listCount **l, int n)
{
    for (int i = 0; i < n; i++)
    {
        destroyListCount(&l[i]);
    }
    free(l);
    l = NULL;
}

// Return a copy of l (array of listCount)
listCount **cloneListCountArray(listCount **l, int n)
{
    listCount **clone = malloc(n * sizeof(listCount *));
    for (int i = 0; i < n; i++)
    {
        clone[i] = cloneListCount(l[i]);
    }
    return clone;
}

// Print the sudoku. If debug = 1, print the possible values of each cell, else print the value of each cell.
void printSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int debug)
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
            printPossListCount(possRows[i]);

            printf("Grid %d = ", i);
            printPossListCount(possGrids[i]);

            printf("Column %d = ", i);
            printPossListCount(possColumns[i]);
            printf("\n");
        }
    }
}

// Return index of grid (from 0 to n-1) where cell (r,c) is located
int getIndexPossGrid(int n, int r, int c)
{
    int rowN = sqrt(n);
    return (r / rowN) * rowN + c / rowN;
}

// Markup sudoku and find possible values for each cell
int markupSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    for (int r = 0; r < n; r++)
    {
        for (int c = 0; c < n; c++)
        {
            if ((sudoku[r] + c)->changed == 0)
            {
                if ((sudoku[r] + c)->val != 0)
                {
                    list *l = cloneList((sudoku[r] + c)->poss);

                    // remove the value of the cell from the possible values of the row, column and grid
                    findAndRemoveListCount(&(possRows[r]), (sudoku[r] + c)->val);
                    findAndRemoveListCount(&(possColumns[c]), (sudoku[r] + c)->val);
                    findAndRemoveListCount(&(possGrids[getIndexPossGrid(n, r, c)]), (sudoku[r] + c)->val);

                    // remove the poss values from the possible values of the row
                    reduceListCount(&(possRows[r]), l);

                    // remove the poss values from the possible values of the column
                    reduceListCount(&(possColumns[c]), l);

                    // remove the poss values from the possible values of the grid
                    int indexGrid = getIndexPossGrid(n, r, c);
                    reduceListCount(&(possGrids[indexGrid]), l);

                    // remove all poss values from the cell
                    (sudoku[r] + c)->changed = 1;
                    destroyList(&((sudoku[r] + c)->poss));

                    // remove the value from the possible values of the cells in the same row
                    for (int k = 0; k < n; ++k)
                    {
                        if (k == c)
                        {
                            continue;
                        }

                        if ((sudoku[r] + c)->val == (sudoku[r] + k)->val) // not valid
                        {
                            destroyList(&l);
                            return -1;
                        }

                        // if the cell has val as a possible value, remove it
                        int found = findAndRemoveList(&(sudoku[r] + k)->poss, (sudoku[r] + c)->val);
                        if (found == 1)
                        {
                            // remove the value from the possible values of the row
                            findAndReduceListCount(&(possRows[r]), (sudoku[r] + c)->val);

                            // remove the value from the possible values of the column
                            findAndReduceListCount(&(possColumns[k]), (sudoku[r] + c)->val);

                            // remove the value from the possible values of the grid
                            int indexGrid = getIndexPossGrid(n, r, k);
                            findAndReduceListCount(&(possGrids[indexGrid]), (sudoku[r] + c)->val);
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
                            destroyList(&l);
                            return -1;
                        }

                        // if the cell has val as a possible value, remove it
                        int found = findAndRemoveList(&(sudoku[k] + c)->poss, (sudoku[r] + c)->val);
                        if (found == 1)
                        {
                            // remove the value from the possible values of the row
                            findAndReduceListCount(&(possRows[k]), (sudoku[r] + c)->val);

                            // remove the value from the possible values of the column
                            findAndReduceListCount(&(possColumns[c]), (sudoku[r] + c)->val);

                            // remove the value from the possible values of the grid
                            int indexGrid = getIndexPossGrid(n, k, c);
                            findAndReduceListCount(&(possGrids[indexGrid]), (sudoku[r] + c)->val);
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
                                destroyList(&l);
                                return -1;
                            }

                            // if the cell has val as a possible value, remove it
                            int found = findAndRemoveList(&(sudoku[startI + k] + startJ + m)->poss, (sudoku[r] + c)->val);
                            if (found == 1)
                            {

                                // remove the value from the possible values of the row
                                findAndReduceListCount(&(possRows[startI + k]), (sudoku[r] + c)->val);

                                // remove the value from the possible values of the column
                                findAndReduceListCount(&(possColumns[startJ + m]), (sudoku[r] + c)->val);

                                // remove the value from the possible values of the grid
                                int indexGrid = getIndexPossGrid(n, startI + k, startJ + m);
                                findAndReduceListCount(&(possGrids[indexGrid]), (sudoku[r] + c)->val);
                            }
                        }
                    }
                    destroyList(&l);
                }
                else
                {
                    if ((sudoku[r] + c)->poss == NULL) // not valid
                    {
                        return -1;
                    }
                }
            }
        }
    }
    // free the memory allocated for the markupParams
    // free(params);
    // params = NULL;
    return 0;
}

// Set the value of cell (r,c) to val and remove val from the possible values of the cell
void setCell(cell **sudoku, int r, int c, int val)
{
    // set value in sudoku
    (sudoku[r] + c)->val = val;
}

// Check if exist a cell with only one possible value and set it. Return 1 if a cell has been set, 0 otherwise. If a cell has no possible values (sudoku unsolvable), return -1.
int solveSingleton(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0) // if it's a void cell
            {
                if ((sudoku[i] + j)->poss == NULL) // sudoku unsolvable
                {
                    return -1;
                }
                else
                {
                    if ((sudoku[i] + j)->poss->next == NULL) // if it's a singleton (has only 1 possible value)
                    {
                        changed = 1;
                        setCell(sudoku, i, j, (sudoku[i] + j)->poss->val);
                    }
                }
            }
        }
    }
    return changed;
}

// Check if exist (for every row, column and grid) a possible value where only one cell is possible. If so, set the value of the cell. Return 1 if a cell has been set, 0 otherwise.
int solveLoneRangers(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (int i = 0; i < n; ++i)
    {
        // for each row i
        // if row i has lone rangers (if possRows[i] has count 1 for some number)
        int lastVal = 0;
        for (listCount *l = findNextListCount(possRows[i], lastVal); l != NULL; l = findNextListCount(possRows[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                // find the lone ranger in the row
                for (int j = 0; j < n; ++j)
                {
                    if ((sudoku[i] + j)->val == 0)
                    {
                        list *node = NULL;
                        list *prev = NULL;
                        if (findList((sudoku[i] + j)->poss, l->val, &node, &prev) == 1) // if it's the lone ranger in the row
                        {
                            changed = 1;
                            setCell(sudoku, i, j, l->val);
                            break;
                        }
                    }
                }
            }
        }

        // for each column i
        // if column i has lone rangers (if possColumns[i] has count 1 for some number)
        lastVal = 0;
        for (listCount *l = findNextListCount(possColumns[i], lastVal); l != NULL; l = findNextListCount(possColumns[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                // find the lone ranger in the row
                for (int j = 0; j < n; ++j)
                {
                    if ((sudoku[j] + i)->val == 0)
                    {
                        list *node = NULL;
                        list *prev = NULL;
                        if (findList((sudoku[j] + i)->poss, l->val, &node, &prev) == 1) // if it's the lone ranger in the column
                        {
                            changed = 1;
                            setCell(sudoku, j, i, l->val);
                            break;
                        }
                    }
                }
            }
        }

        // for each grid i
        // if grid i has lone rangers (if possGrids[i] has count 1 for some number)
        lastVal = 0;
        for (listCount *l = findNextListCount(possGrids[i], lastVal); l != NULL; l = findNextListCount(possGrids[i], lastVal))
        {
            lastVal = l->val;
            if (l->count == 1) // if it's a lone ranger
            {
                // find the lone ranger in the grid
                int rowN = sqrt(n);
                int startI = i / rowN * rowN;
                int startJ = i % rowN * rowN;
                for (int k = 0; k < rowN; ++k)
                {
                    int br = 0;
                    for (int m = 0; m < rowN; ++m)
                    {
                        if ((sudoku[startI + k] + startJ + m)->val == 0)
                        {
                            list *node = NULL;
                            list *prev = NULL;
                            if (findList((sudoku[startI + k] + startJ + m)->poss, l->val, &node, &prev) == 1) // if it's the lone ranger in the grid
                            {
                                changed = 1;
                                setCell(sudoku, startI + k, startJ + m, l->val);
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

// Check if the sudoku is solved
int isSolved(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    for (int i = 0; i < n; ++i)
    {
        // if (possRows[i] != NULL || possColumns[i] != NULL || possGrids[i] != NULL)
        // {
        //     return 0;
        // }

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

// Try to solve the sudoku. Apply solveSingleton, solveLoneRangers, solveTwins repeating every time changes something. If it doesn't change anything, check if it's solved and return the solved sudoku. If it's not solved, try guessing a value and solve again. If sudoku is unsolvable, return NULL.
cell **solveSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    srand(time(NULL));
    int changed;
    do
    {
        do
        {
            changed = 0;
            if (markupSudoku(sudoku, possRows, possColumns, possGrids, n) == -1)
                return NULL;

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
        } while (changed > 0);

        int changedLoneRangers = solveLoneRangers(sudoku, possRows, possColumns, possGrids, n);
        if (changedLoneRangers == 1)
        {
            changed = 1;
            // printf("LONE RANGERS\n");
        }
    } while (changed > 0);

    if (isSolved(sudoku, possRows, possColumns, possGrids, n) == 1)
    {
        return sudoku;
    }
    else
    {
        // pick the first cell with value 0
        // printf("RANDOM\n");
        int r, c;
        for (int index = 0; index < n * n; ++index)
        {
            r = index / n;
            c = index % n;
            if ((sudoku[r] + c)->val == 0)
            {
                break;
            }
        }

        // for each possible value, try to solve the sudoku
        for (list *l = (sudoku[r] + c)->poss; l != NULL; l = l->next)
        {
            cell **sudokuClone = cloneSudoku(sudoku, n);
            listCount **possRowsClone = cloneListCountArray(possRows, n);
            listCount **possColumnsClone = cloneListCountArray(possColumns, n);
            listCount **possGridsClone = cloneListCountArray(possGrids, n);
            setCell(sudokuClone, r, c, l->val);

            cell **solvedSudoku = solveSudoku(sudokuClone, possRowsClone, possColumnsClone, possGridsClone, n);
            destroyListCountArray(possRowsClone, n);
            destroyListCountArray(possColumnsClone, n);
            destroyListCountArray(possGridsClone, n);
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
    DIR *d;
    struct dirent *dir;
    char *path = "sudoku-examples/hex/";
    d = opendir(path);
    long total_time = 0;
    int n_sudokus = 0;
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
            listCount **possRows = NULL;
            listCount **possColumns = NULL;
            listCount **possGrids = NULL;
            cell **sudoku = readSudoku(&n, &possRows, &possColumns, &possGrids, fileSudoku);

            if (sudoku == NULL)
            {
                fprintf(stderr, "Could not read sudoku\n");
                return 1;
            }

            clock_t begin = clock();

            cell **solvedSudoku = solveSudoku(sudoku, possRows, possColumns, possGrids, n);

            clock_t end = clock();
            total_time += (end - begin);

            if (solvedSudoku != NULL)
            {
                printSudoku(solvedSudoku, possRows, possColumns, possGrids, n, 0);
                printf("Sudoku solved in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
                destroySudoku(solvedSudoku, n);
            }
            else
            {
                printf("Sudoku not solved!\n");
                destroySudoku(sudoku, n);
            }

            destroyListCountArray(possRows, n);
            destroyListCountArray(possColumns, n);
            destroyListCountArray(possGrids, n);
        }
        closedir(d);
        printf("############################################################\n");
        printf("Solved %d sudokus in %f seconds\n", n_sudokus, (double)total_time / CLOCKS_PER_SEC);
        printf("Average time: %f seconds\n", (double)total_time / CLOCKS_PER_SEC / n_sudokus);
    }
    return 0;
}