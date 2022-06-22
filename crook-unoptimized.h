#include "listCount.h"

// Cell of a Sudoku
typedef struct cell
{
    int val;
    list *poss;  // possible values list from 1 to n where n is size of sudoku
    int changed; // 0 if not changed, 1 if changed, -1 if invalid
} cell;

cell **readSudoku(int *n, listCount ***possRows, listCount ***possColumns, listCount ***possGrids, const char *filename);
cell **cloneSudoku(cell **sudoku, const int n);
void destroySudoku(cell **sudoku, const int n);
void destroyListCountArray(listCount **l, int n);
listCount **cloneListCountArray(listCount **l, int n);
void printSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n, int debug);
int getIndexPossGrid(int n, int r, int c);
int markupSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n);
void setCell(cell **sudoku, int r, int c, int val);
int solveSingleton(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n);
int solveLoneRangers(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n);
int isSolved(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n);
cell **solveSudoku(cell **sudoku, listCount **possRows, listCount **possColumns, listCount **possGrids, int n);