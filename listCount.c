#include <stdio.h>
#include <stdlib.h>
#include "listCount.h"

//Return a linked list with values from 1 to n and count of each node set to n
listCount *getListCount(const int n)
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

//Print the list l with the count of each element
void printPossListCount(listCount *l)
{
    if (l == NULL)
    {
        printf("ERROR (EMPTY LIST) \n");
        return;
    }

    listCount *tmp = l;
    while (tmp->next != NULL)
    {
        if (tmp->val <= 15)
        {
            printf("%X (%d) or ", tmp->val, tmp->count);
        }
        else
        {
            char c = 'F' + (tmp->val - 15);
            printf("%c (%d) or ", c, tmp->count);
        }
        tmp = tmp->next;
    }
    
    if (tmp->val <= 15)
    {
        printf("%X (%d) or ", tmp->val, tmp->count);
    }
    else
    {
        char c = 'F' + (tmp->val - 15);
        printf("%c (%d) \n", c, tmp->count);
    }
}

//Destroy the list l
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

//Find the node with value n in the list l and set the pointer to the node to the node before it
int findListCount(listCount *l, const int n, listCount **node, listCount **prev)
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

//Remove from list l the node
int removeListCount(listCount **l, listCount **node, listCount **prev)
{
    if (*node == NULL || *l == NULL)
    {
        return 0;
    }
    if (*prev == NULL)
    {
        *l = (*node)->next;
        free(*node);
    }
    else
    {
        (*prev)->next = (*node)->next;
        free(*node);
    }
    return 1;
}

// Find and force remove of node with value n from list l
int findAndRemoveListCount(listCount **l, const int n)
{
    listCount *node = NULL;
    listCount *prev = NULL;
    if (findListCount(*l, n, &node, &prev) == 1)
    {
        removeListCount(l, &node, &prev);
        return 1;
    }
    return 0;
}

//Find and subtract count (and eventually remove) of node with value n from list l
int findAndReduceListCount(listCount **l, const int n)
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

//Find the next node (if exists) with val > n in the list l
listCount *findNextListCount(listCount *l, const int n)
{
    listCount *node = l;
    while (node != NULL)
    {
        if (node->val > n)
        {
            return node;
        }
        node = node->next;
    }
    return node;
}

//Reduce the count (and eventually remove) of nodes in list possCount that are also in poss
void reduceListCount(listCount **possCount, list *poss)
{
    listCount *nodeCount = *possCount;
    listCount *prevCount = NULL;
    for (list *node = poss; node != NULL && nodeCount != NULL; node = node->next)
    {
        while (nodeCount->val < node->val)
        {
            prevCount = nodeCount;
            nodeCount = nodeCount->next;
            if (nodeCount == NULL)
            {
                return;
            }
        }
        if (nodeCount->val == node->val)
        {
            if (--(nodeCount->count) == 0)
            {
                listCount *nextCount = nodeCount->next;
                removeListCount(possCount, &nodeCount, &prevCount);
                nodeCount = nextCount;
            }
        }
    }
}

//Return a clone of l1
listCount *cloneListCount(const listCount *l1)
{
    listCount *l2 = NULL;
    listCount *prev = NULL;
    while (l1 != NULL)
    {
        listCount *new = (listCount *)malloc(sizeof(listCount));
        new->val = l1->val;
        new->count = l1->count;
        new->next = NULL;
        if (prev == NULL)
        {
            l2 = new;
        }
        else
        {
            prev->next = new;
        }
        prev = new;
        l1 = l1->next;
    }
    return l2;
}