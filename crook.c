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

void printSudokuDebug(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int possibleValues)
{
    printf("\n#############################\nSUDOKU ");
    if (possibleValues == 0)
    {
        printf("(only grid)\n#############################\n");
    }
    else
    {
        printf("(possible values)\n#############################\n");
    }

    for (size_t i = 0; i < n; ++i)
    {
        if (possibleValues == 0)
        {
            for (size_t j = 0; j < n; ++j)
            {
                printf("%d ", (sudoku[i] + j)->val);
            }
            printf("\n");
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

int getIndexPossGrid(int n, int i, int j)
{
    int rowN = sqrt(n);
    return (i / rowN) * rowN + j / rowN;
}

void setCell(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int r, int c, int val)
{
    //set value in sudoku
    (sudoku[r] + c)->val = val;
    //remove the poss values from the possible values of the row
    reduceListCount(possRows, (sudoku[r] + c)->poss, r);
    //remove the poss values from the possible values of the column
    reduceListCount(possColumns, (sudoku[r] + c)->poss, c);
    //remove the poss values from the possible values of the grid
    reduceListCount(possGrids, (sudoku[r] + c)->poss, getIndexPossGrid(n, r, c));
    //remove all poss values from the cell
    destroyList(&((sudoku[r] + c)->poss));

    //remove the value from the possible values of the cells in the same row
    for (size_t k = 0; k < n; ++k)
    {
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

int solveSingleton(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0) //if it's a void cell
            {
                if ((sudoku[i] + j)->poss == NULL) //sudoku not solvable
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

int solveLoneRangers(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        //for each row i
        //if row i has lone rangers (if possRows[i] has count 1 for some number)
        for (listCount *l = possRows[i]; l != NULL; l = l->next)
        {
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
                        }
                    }
                }
            }
        }

        //for each column i
        //if column i has lone rangers (if possColumns[i] has count 1 for some number)
        for (listCount *l = possColumns[i]; l != NULL; l = l->next)
        {
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
                        }
                    }
                }
            }
        }

        //for each grid i
        //if grid i has lone rangers (if possGrids[i] has count 1 for some number)
        for (listCount *l = possGrids[i]; l != NULL; l = l->next)
        {
            if (l->count == 1) //if it's a lone ranger
            {
                //find the lone ranger in the grid
                int rowN = sqrt(n);
                int startI = i / rowN * rowN;
                int startJ = i % rowN * rowN;
                for (size_t k = 0; k < rowN; ++k)
                {
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
                            }
                        }
                    }
                }
            }
        }
    }
    return changed;
}

void solveSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    int changed = 0;
    do
    {
        do
        {
            changed = solveSingleton(sudoku, possRows, possColumns, possGrids, n);
            if (changed == -1)
                return;
        } while (changed > 0);
        changed = solveLoneRangers(sudoku, possRows, possColumns, possGrids, n);
    } while (changed > 0);
}

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

    solveSudoku(sudoku, possRows, possColumns, possGrids, n);

    // printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 1);
    printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 0);

    if (isSolved(sudoku, possRows, possColumns, possGrids, n) == 1)
    {
        printf("Sudoku solved!\n");
    }
    else
    {
        printf("Sudoku not solved!\n");
    }

    //TODO: freeing memory

    return 0;
}