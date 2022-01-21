#include <stdio.h>
#include "list.c"

typedef struct listCount
{
    int val;
    int count;
    struct listCount *next;
} listCount;

listCount *getListCount(int n)
{
    listCount *last = (listCount *)malloc(sizeof(listCount));
    last->val = n;
    last->count = n;
    last->next = NULL;
    for (int i = n - 1; i > 0; i--)
    {
        listCount *new = (listCount *)malloc(sizeof(listCount));
        new->val = i;
        new->count = n;
        new->next = last;
        last = new;
    }
    return last;
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

void destroyListCount(listCount **l)
{
    listCount *tmp = *l;
    while (*l != NULL)
    {
        tmp = *l;
        *l = (*l)->next;
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
        if (--(node->count) == 0)
        {
            removeListCount(l, &node, &prev);
        }
        return 1;
    }
    return 0;
}

void reduceListCount(listCount **possCount, list *poss, int i)
{
    listCount *nodeCount = possCount[i];
    listCount *prevCount = NULL;
    for (list *node = poss; node != NULL && nodeCount != NULL; node = node->next)
    {
        while (nodeCount->val < node->val)
        {
            prevCount = nodeCount;
            nodeCount = nodeCount->next;
        }
        if (nodeCount->val == node->val)
        {
            if (--(nodeCount->count) == 0)
            {
                listCount *nextCount = nodeCount->next;
                removeListCount(possCount + i, &nodeCount, &prevCount);
                nodeCount = nextCount;
            }
        }
    }
}
