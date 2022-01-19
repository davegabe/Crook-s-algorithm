#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

typedef struct list
{
    int val;
    struct list *next;
} list;

typedef struct cell
{
    int val;
    list *poss; // possible values list from 1 to n where n is size of sudoku
} cell;

cell **readSudoku(int *n, list ***possRows, list ***possColumns, list ***possGrids, const char *filename)
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

        //create the possible values list for each row
        *possRows = malloc(*n * sizeof(list *));
        for (int i = 0; i < *n; i++)
        {
            //create a list of all possible values (from 1 to 9)
            list *last = (list *)malloc(sizeof(list));
            last->val = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                list *new = (list *)malloc(sizeof(list));
                new->val = i;
                new->next = last;
                last = new;
            }

            (*possRows)[i] = last;
        }

        //create the possible values list for each column
        *possColumns = malloc(*n * sizeof(list *));
        for (int i = 0; i < *n; i++)
        {
            //create a list of all possible values (from 1 to 9)
            list *last = (list *)malloc(sizeof(list));
            last->val = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                list *new = (list *)malloc(sizeof(list));
                new->val = i;
                new->next = last;
                last = new;
            }

            (*possColumns)[i] = last;
        }

        //create the possible values list for each cell
        *possGrids = malloc(*n * sizeof(list *));
        for (int i = 0; i < *n; i++)
        {
            //create a list of all possible values (from 1 to 9)
            list *last = (list *)malloc(sizeof(list));
            last->val = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                list *new = (list *)malloc(sizeof(list));
                new->val = i;
                new->next = last;
                last = new;
            }

            (*possGrids)[i] = last;
        }
    }

    // read the sudoku
    for (size_t i = 0; i < *n; ++i)
    {
        sudoku[i] = calloc(*n, sizeof(cell));
        for (size_t j = 0; j < *n; ++j)
        {
            int t;
            fscanf(fp, "%d", &t);
            (sudoku[i] + j)->val = t; //value of cell is the same as the value in the file
            if (t == 0)               //if it's a void cell (has value 0)
            {
                //create a list of all possible values (from 1 to 9)
                list *last = (list *)malloc(sizeof(list));
                last->val = *n;
                last->next = NULL;
                for (int i = *n - 1; i > 0; i--)
                {
                    list *new = (list *)malloc(sizeof(list));
                    new->val = i;
                    new->next = last;
                    last = new;
                }

                //add all possible values (from 1 to 9)
                (sudoku[i] + j)->poss = last;
            }
            else //if the cell has a value
            {
                //possible values list is empty
                (sudoku[i] + j)->poss = NULL;
            }
        }
    }

    fclose(fp);

    return sudoku;
}

void printPossList(list *l)
{
    if (l == NULL)
    {
        return;
    }

    list *tmp = l;
    while (tmp->next != NULL)
    {
        printf("%d or ", tmp->val);
        tmp = tmp->next;
    }
    printf("%d \n", tmp->val);
}

void printSudokuDebug(cell **sudoku, list **possRows, list **possColumns, list **possGrids, int n, int possibleValues)
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
                if ((sudoku[i] + j)->poss == NULL)
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
            printPossList(possRows[i]);

            printf("Grid %d = ", i);
            printPossList(possGrids[i]);

            printf("Column %i = ", i);
            printPossList(possColumns[i]);
            printf("\n");
        }
    }
}

int findList(list *l, int n, list **node, list **prev)
{
    if (l == NULL)
    {
        return 0;
    }

    *prev = NULL;
    *node = l;
    do
    {
        if ((*node)->val == n)
        {
            return 1;
        }
        *prev = *node;
        *node = (*node)->next;
    } while ((*node) != NULL);

    return 0;
}

int removeList(list **l, list **node, list **prev)
{
    if (*node == NULL || *l == NULL)
    {
        return 0;
    }

    if (*prev == NULL)
    {
        if ((*node)->next != NULL)
        {
            *l = (*node)->next;
        }
        else
        {
            *l = NULL;
        }
        free(*node);
    }
    else
    {
        (*prev)->next = (*node)->next;
        free(*node);
    }
    return 1;
}

int findAndRemoveList(list **l, int n)
{
    list *node = NULL;
    list *prev = NULL;
    if (findList(*l, n, &node, &prev) == 1)
    {
        return removeList(l, &node, &prev);
    }
    return 0;
}

int getIndexPossGrid(int n, int i, int j)
{
    int rowN = sqrt(n);
    return i / rowN + (j / rowN) * rowN;
}

int isSolved(cell **sudoku, int n)
{
    for (size_t i = 0; i < n; ++i)
    {
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

void reducePossRow(cell **sudoku, list **possRows, int n, int row, int val)
{
    //remove the value from the possible values of the row
    findAndRemoveList(&(possRows[row]), val);

    //remove the value from the possible values of the cells in the same row
    for (size_t k = 0; k < n; ++k)
    {
        findAndRemoveList(&((sudoku[row] + k)->poss), val);
    }
}

void reducePossColumn(cell **sudoku, list **possColumns, int n, int col, int val)
{
    //remove the value from the possible values of the column
    findAndRemoveList(&(possColumns[col]), val);

    //remove the value from the possible values of the cells in the same column
    for (size_t k = 0; k < n; ++k)
    {
        findAndRemoveList(&((sudoku[k] + col)->poss), val);
    }
}

void reducePossGrid(cell **sudoku, list **possGrids, int n, int grid, int val)
{
    //remove the value from the possible values of the grid
    findAndRemoveList(&(possGrids[grid]), val);

    //remove the value from the possible values of the cells in the same grid
    int rowN = sqrt(n);
    int startI = grid / rowN * rowN;
    int startJ = grid % rowN * rowN;
    for (size_t k = 0; k < rowN; ++k)
    {
        for (size_t l = 0; l < rowN; ++l)
        {
            findAndRemoveList(&((sudoku[startI + k] + startJ + l)->poss), val);
        }
    }
}

void reducePoss(cell **sudoku, list **possRows, list **possColumns, list **possGrids, int n)
{
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val != 0)
            {
                //remove the value from the possible values of the row
                reducePossRow(sudoku, possRows, n, i, (sudoku[i] + j)->val);
                //remove the value from the possible values of the column
                reducePossColumn(sudoku, possColumns, n, j, (sudoku[i] + j)->val);
                //remove the value from the possible values of the grid
                reducePossColumn(sudoku, possGrids, n, getIndexPossGrid(n, i, j), (sudoku[i] + j)->val);
            }
        }
    }
}

int solveSingleton(cell **sudoku, list **possRows, list **possColumns, list **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0 && (sudoku[i] + j)->poss != NULL) //if it's a void cell and has a list of possible values
            {
                if ((sudoku[i] + j)->poss->next == NULL) //if it's a singleton (has only 1 possible value)
                {
                    (sudoku[i] + j)->val = (sudoku[i] + j)->poss->val;
                    findAndRemoveList(&(possRows[i]), (sudoku[i] + j)->val);
                    changed = 1;

                    //remove the value from the possible values of the row
                    reducePossRow(sudoku, possRows, n, i, (sudoku[i] + j)->val);
                    //remove the value from the possible values of the column
                    reducePossColumn(sudoku, possColumns, n, j, (sudoku[i] + j)->val);
                    //remove the value from the possible values of the grid
                    reducePossColumn(sudoku, possGrids, n, getIndexPossGrid(n, i, j), (sudoku[i] + j)->val);
                }
            }
        }
    }
    return changed;
}

int solveLoneRangers(cell **sudoku, list **possRows, list **possColumns, list **possGrids, int n)
{
    int changed = 0;
    for (size_t i = 0; i < n; ++i)
    {
        //for each row: check if there is only one cell for a possRows value
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val == 0) //if it's a void cell
            {
            }
        }
    }
    return changed;
}

void solveSudoku(cell **sudoku, list **possRows, list **possColumns, list **possGrids, int n)
{
    int changed;
    do
    {
        changed = 0;
        changed += solveSingleton(sudoku, possRows, possColumns, possGrids, n);
        changed += solveLoneRangers(sudoku, possRows, possColumns, possGrids, n);
    } while (changed != 0);
}

int main(void)
{
    int n = 0;
    list **possRows = NULL;
    list **possColumns = NULL;
    list **possGrids = NULL;
    cell **sudoku = readSudoku(&n, &possRows, &possColumns, &possGrids, "sudoku.txt");

    if (sudoku == NULL)
    {
        fprintf(stderr, "could not read sudoku\n");
        return 1;
    }

    //printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 0);
    //printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 1);
    reducePoss(sudoku, possRows, possColumns, possGrids, n);
    solveSudoku(sudoku, possRows, possColumns, possGrids, n);
    //printf("%d", findAndRemoveList(&sudoku[8][7].poss, 7));
    printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 0);

    if (isSolved(sudoku, n) == 1)
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