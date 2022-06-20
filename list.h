typedef struct list {
  int val;
  struct list *next;
} list;
list *getList(int n);
void printPossList(list *l);
void destroyList(list **l);
int findList(list *l, int val, list **node, list **prev);
int removeList(list **l, list **node, list **prev);
int findAndRemoveList(list **l, int n);
int lengthList(list *l);
int isEqualList(list *l1, list *l2);
int isContainedList(list *l1, list *l2);
int reduceList(list **l1, list *l2);
list *cloneList(list *l1);