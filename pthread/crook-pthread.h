#include "../listCount.h"
#include <pthread.h>

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

// Params for solveSudoku function
typedef struct solveSudokuParams
{
  cell **sudoku;
  int isSolved;
  possRCG **possRows;
  possRCG **possColumns;
  possRCG **possGrids;
  int n;
} solveSudokuParams;

// Params for isSolved function
typedef struct isSolvedParams
{
  int *isSolved;   // 1 := solved, 0 := not solved
  cell **sudoku;   // sudoku
  int n;           // size of sudoku
  int max_threads; // max number of threads
  int n_thread;    // number of current thread
} isSolvedParams;

// Params for cloneSudoku function
typedef struct cloneSudokuParams
{
  cell **sudoku; // sudoku
  int n;         // size of sudoku
  cell **clone;  // clone of sudoku
} cloneSudokuParams;

// Params for clonePossRCG function
typedef struct clonePossRCGParams
{
  possRCG **possRcg; // possible values for each row/column/grid
  int n;             // size of sudoku
  possRCG **clone;   // clone of possRcg
} possRCGParams;

cell **readSudoku(int *n, possRCG ***possRows, possRCG ***possColumns,
                  possRCG ***possGrids, const char *filename);
void *cloneSudoku(void *params);
void destroySudoku(cell **sudoku, const int n);
void destroyPossRCGArray(possRCG **l, const int n);
void *clonePossRCGArray(void *params);
void printSudoku(cell **sudoku, possRCG **possRows, possRCG **possColumns,
                 possRCG **possGrids, int n, int debug);
int getIndexPossGrid(const int n, const int r, const int c);
void *markupSudoku(void *params);
void *solveSingleton(void *params);
void *solveLoneRangerR(void *params);
void *solveLoneRangerC(void *params);
void *solveLoneRangerG(void *params);
void *isSolved(void *params);
int singletonMT(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n, int max_threads);
int loneRangerMT(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n, int max_threads);
int isSudokuSolvedMT(cell **sudoku, int n, int max_threads);
cell **randomMT(cell **sudoku, possRCG **possRows, possRCG **possColumns, possRCG **possGrids, int n);
void *solveSudoku(void *params);