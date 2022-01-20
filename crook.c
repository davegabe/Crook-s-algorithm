#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

typedef struct listCount
{
    int val;
    int count;
    struct listCount *next;
} listCount;

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

        //create the possible values listCount for each row
        *possRows = malloc(*n * sizeof(listCount *));
        for (int i = 0; i < *n; i++)
        {
            //create a listCount of all possible values (from 1 to 9)
            listCount *last = (listCount *)malloc(sizeof(listCount));
            last->val = *n;
            last->count = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                listCount *new = (listCount *)malloc(sizeof(listCount));
                new->val = i;
                new->count = *n;
                new->next = last;
                last = new;
            }

            (*possRows)[i] = last;
        }

        //create the possible values listCount for each column
        *possColumns = malloc(*n * sizeof(listCount *));
        for (int i = 0; i < *n; i++)
        {
            //create a listCount of all possible values (from 1 to 9)
            listCount *last = (listCount *)malloc(sizeof(listCount));
            last->val = *n;
            last->count = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                listCount *new = (listCount *)malloc(sizeof(listCount));
                new->val = i;
                new->count = *n;
                new->next = last;
                last = new;
            }

            (*possColumns)[i] = last;
        }

        //create the possible values listCount for each cell
        *possGrids = malloc(*n * sizeof(listCount *));
        for (int i = 0; i < *n; i++)
        {
            //create a listCount of all possible values (from 1 to 9)
            listCount *last = (listCount *)malloc(sizeof(listCount));
            last->val = *n;
            last->count = *n;
            last->next = NULL;
            for (int i = *n - 1; i > 0; i--)
            {
                listCount *new = (listCount *)malloc(sizeof(listCount));
                new->val = i;
                new->count = *n;
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
    }

    fclose(fp);

    return sudoku;
}

void printPossList(list *l)
{
    if (l == NULL)
    {
        printf("ERROR (EMPTY LIST) \n");
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

void printPossListCount(listCount *l)
{
    if (l == NULL)
    {
        return;
    }

    listCount *tmp = l;
    while (tmp->next != NULL)
    {
        printf("%d (%d) or ", tmp->val, tmp->count);
        tmp = tmp->next;
    }
    printf("%d (%d) \n", tmp->val, tmp->count);
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

void destroyList(list **l)
{
    list *tmp = *l;
    while (*l != NULL)
    {
        tmp = *l;
        *l = (*l)->next;
        free(tmp);
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

void destroyListCount(listCount *l)
{
    listCount *tmp = l;
    while (l != NULL)
    {
        tmp = l;
        l = l->next;
        free(tmp);
    }
}

int findListCount(listCount *l, int n, listCount **node, listCount **prev)
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

int removeListCount(listCount **l, listCount **node, listCount **prev)
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

int findAndRemoveListCount(listCount **l, int n)
{
    listCount *node = NULL;
    listCount *prev = NULL;
    if (findListCount(*l, n, &node, &prev) == 1)
    {
        return removeListCount(l, &node, &prev);
    }
    return 0;
}

int getIndexPossGrid(int n, int i, int j)
{
    int rowN = sqrt(n);
    return (i / rowN) * rowN + j / rowN;
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

void reduceListCount(listCount **possCount, list *poss, int i)
{
    listCount *nodeCount = possCount[i];
    for (list *node = poss; node != NULL; node = node->next)
    {
        while (nodeCount->val < node->val)
        {
            nodeCount = nodeCount->next;
        }
        if (nodeCount->val == node->val)
        {
            if (--(nodeCount->count) == 0)
            {
                findAndRemoveListCount(&(possCount[i]), nodeCount->val);
            }
        }
    }
}

void reducePossRow(cell **sudoku, listCount **possRows, int n, int row, cell *cell)
{
    //remove the poss values from the possible values of the row
    reduceListCount(possRows, cell->poss, row);

    //remove the value from the possible values of the cells in the same row
    for (size_t k = 0; k < n; ++k)
    {
        findAndRemoveList(&((sudoku[row] + k)->poss), cell->val);
    }
}

void reducePossColumn(cell **sudoku, listCount **possColumns, int n, int col, cell *cell)
{
    //remove the poss values from the possible values of the column
    reduceListCount(possColumns, cell->poss, col);

    //remove the value from the possible values of the cells in the same column
    for (size_t k = 0; k < n; ++k)
    {
        findAndRemoveList(&((sudoku[k] + col)->poss), cell->val);
    }
}

void reducePossGrid(cell **sudoku, listCount **possGrids, int n, int grid, cell *cell)
{
    //remove the poss values from the possible values of the grid
    reduceListCount(possGrids, cell->poss, grid);

    //remove the value from the possible values of the cells in the same grid
    int rowN = sqrt(n);
    int startI = grid / rowN * rowN;
    int startJ = grid % rowN * rowN;
    for (size_t k = 0; k < rowN; ++k)
    {
        for (size_t m = 0; m < rowN; ++m)
        {
            findAndRemoveList(&((sudoku[startI + k] + startJ + m)->poss), cell->val);
        }
    }
}

void reducePoss(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n)
{
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if ((sudoku[i] + j)->val != 0)
            {
                //remove the value from the possible values of the row
                reducePossRow(sudoku, possRows, n, i, sudoku[i] + j);
                //remove the value from the possible values of the column
                reducePossColumn(sudoku, possColumns, n, j, sudoku[i] + j);
                //remove the value from the possible values of the grid
                reducePossGrid(sudoku, possGrids, n, getIndexPossGrid(n, i, j), sudoku[i] + j);
                //destroy poss list
                destroyList(&((sudoku[i] + j)->poss));
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
                if ((sudoku[i] + j)->poss == NULL) //!! ERORRE QUESTO NON DOVREBBE MAI CAPITARE
                {
                    printf("ERRORE SUDOKU                %d,%d\n", i, j);
                    //return 0;
                }
                else
                {
                    if ((sudoku[i] + j)->poss->next == NULL) //if it's a singleton (has only 1 possible value)
                    {
                        changed = 1;
                        (sudoku[i] + j)->val = (sudoku[i] + j)->poss->val;

                        //remove the value from the possible values of the row
                        reducePossRow(sudoku, possRows, n, i, sudoku[i] + j);
                        //remove the value from the possible values of the column
                        reducePossColumn(sudoku, possColumns, n, j, sudoku[i] + j);
                        //remove the value from the possible values of the grid
                        reducePossGrid(sudoku, possGrids, n, getIndexPossGrid(n, i, j), sudoku[i] + j);
                        //destroy poss list
                        destroyList(&((sudoku[i] + j)->poss));
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
                        if (findAndRemoveList(&(sudoku[i] + j)->poss, l->val) == 1) //if it's the lone ranger in the row
                        {
                            changed = 1;
                            (sudoku[i] + j)->val = l->val;

                            //remove the value from the possible values of the row
                            reducePossRow(sudoku, possRows, n, i, sudoku[i] + j);
                            //remove the value from the possible values of the column
                            reducePossColumn(sudoku, possColumns, n, j, sudoku[i] + j);
                            //remove the value from the possible values of the grid
                            reducePossGrid(sudoku, possGrids, n, getIndexPossGrid(n, i, j), sudoku[i] + j);
                            //destroy poss list
                            destroyList(&(sudoku[i] + j)->poss);
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
                        if (findAndRemoveList(&(sudoku[j] + i)->poss, l->val) == 1) //if it's the lone ranger in the row
                        {
                            changed = 1;
                            (sudoku[j] + i)->val = l->val;

                            //remove the value from the possible values of the row
                            reducePossRow(sudoku, possRows, n, j, sudoku[i] + j);
                            //remove the value from the possible values of the column
                            reducePossColumn(sudoku, possRows, n, i, sudoku[j] + i);
                            //remove the value from the possible values of the grid
                            reducePossGrid(sudoku, possGrids, n, getIndexPossGrid(n, j, i), sudoku[j] + i);
                            //destroy poss list
                            destroyList(&(sudoku[j] + i)->poss);
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
                            if (findAndRemoveList(&(sudoku[startI + k] + startJ + m)->poss, l->val) == 1) //if it's the lone ranger in the grid
                            {
                                changed = 1;
                                (sudoku[startI + k] + startJ + m)->val = l->val;

                                //remove the value from the possible values of the row
                                reducePossRow(sudoku, possRows, n, startI + k, sudoku[startI + k] + startJ + m);
                                //remove the value from the possible values of the column
                                reducePossColumn(sudoku, possGrids, n, startJ + m, sudoku[startI + k] + startJ + m);
                                //remove the value from the possible values of the grid
                                reducePossGrid(sudoku, possGrids, n, getIndexPossGrid(n, startI + k, startJ + m), sudoku[startI + k] + startJ + m);
                                //destroy poss list
                                destroyList(&(sudoku[startI + k] + startJ + m)->poss);
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
    int changed;
    do
    {
        changed = 0;
        changed += solveSingleton(sudoku, possRows, possColumns, possGrids, n);
        changed += solveLoneRangers(sudoku, possRows, possColumns, possGrids, n);
        printf("%d\n", changed);
    } while (changed != 0);
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

    reducePoss(sudoku, possRows, possColumns, possGrids, n);
    solveSudoku(sudoku, possRows, possColumns, possGrids, n);

    printSudokuDebug(sudoku, possRows, possColumns, possGrids, n, 1);

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