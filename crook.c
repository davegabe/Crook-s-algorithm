#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct list
{
    int val;
    struct list *next;
} list;

typedef struct cell
{
    int val;
    list poss; // possible values list from 0 to n where n is size of sudoku (0 means no possible value)
} cell;

cell **readSudoku(int *n, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    cell **matrix = NULL, **tmp;
    char line[1024];

    // read the size of the sudoku
    if (fgets(line, sizeof(line), fp))
    {
        char *scan = line;
        sscanf(scan, "%d", n);
        matrix = malloc(*n * sizeof(matrix));
    }

    // read the sudoku
    for (size_t i = 0; i < *n; ++i)
    {
        matrix[i] = calloc(*n, sizeof(cell));
        for (size_t j = 0; j < *n; ++j)
        {
            int t;
            fscanf(fp, "%d", &t);
            (matrix[i] + j)->val = t; //value of cell is the same as the value in the file
            if (t == 0)               //if it's a void cell (has value 0)
            {
                //create list of possible values (from 2 to 9)
                list *last = (list *)malloc(sizeof(list));
                last->val = *n;
                last->next = NULL;
                for (int i = *n; i > 1; i--)
                {
                    list *new = (list *)malloc(sizeof(list));
                    new->val = i;
                    new->next = last;
                    last = new;
                }

                //add all possible values (from 1 to 9)
                (matrix[i] + j)->poss.val = 1;
                (matrix[i] + j)->poss.next = last;
            }
            else //if the cell has a value
            {
                //possible values list is empty
                (matrix[i] + j)->poss.val = 0;
                (matrix[i] + j)->poss.next = NULL;
            }
        }
    }

    fclose(fp);

    return matrix;
}

void printSudokuDebug(cell **matrix, int n, int possibleValues)
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
                printf("%d ", (matrix[i] + j)->val);
            }
            printf("\n");
        }
        else
        {
            for (size_t j = 0; j < n; ++j)
            {
                if ((matrix[i] + j)->poss.val == 0)
                {
                    printf("(%d,%d) = %d \n", i, j, (matrix[i] + j)->val);
                }
                else
                {
                    printf("(%d,%d) = %d ", i, j, (matrix[i] + j)->poss.val);
                    list *tmp = (matrix[i] + j)->poss.next;
                    while (tmp != NULL)
                    {
                        printf("or %d ", tmp->val);
                        tmp = tmp->next;
                    }
                    printf("\n");
                }
            }
            printf("\n");
        }
    }
}

int main(void)
{
    int n = 0;
    cell **matrix = readSudoku(&n, "sudoku.txt");

    if (matrix == NULL)
    {
        fprintf(stderr, "could not read matrix\n");
        return 1;
    }
    else
    {
        printSudokuDebug(matrix, n, 0);
        printSudokuDebug(matrix, n, 1);
    }

    //TODO: freeing memory

    return 0;
}