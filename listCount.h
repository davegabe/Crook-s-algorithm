#include "list.h"
typedef struct listCount {
  int val;
  int count;
  struct listCount *next;
} listCount;

listCount *getListCount(const int n);
void printPossListCount(listCount *l);
void destroyListCount(listCount **l);
int findListCount(listCount *l, const int n, listCount **node,
                  listCount **prev);
int removeListCount(listCount **l, listCount **node, listCount **prev);
int findAndRemoveListCount(listCount **l, const int n);
int findAndReduceListCount(listCount **l, const int n);
listCount *findNextListCount(listCount *l, const int n);
void reduceListCount(listCount **possCount, list *poss);
listCount *cloneListCount(const listCount *l1);