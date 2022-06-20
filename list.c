#include <stdio.h>
#include <stdlib.h>
#include "list.h"

// Return a linked list with values from 1 to n
list *getList(int n)
{
    list *last = (list *)malloc(sizeof(list));
    last->val = n;
    last->next = NULL;
    for (int i = n - 1; i > 0; i--)
    {
        list *new = (list *)malloc(sizeof(list));
        new->val = i;
        new->next = last;
        last = new;
    }
    return last;
}

// Print the list l
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
        if (tmp->val <= 15)
        {
            printf("%X or ", tmp->val);
        }
        else
        {
            char c = 'F' + (tmp->val - 15);
            printf("%c or ", c);
        }
        tmp = tmp->next;
    }

    if (tmp->val <= 15)
    {
        printf("%X \n", tmp->val);
    }
    else
    {
        char c = 'F' + (tmp->val - 15);
        printf("%c \n", c);
    }
}

// Destroy the list l
void destroyList(list **l)
{
    list *prev;
    while (*l != NULL)
    {
        prev = *l;
        *l = (*l)->next;
        free(prev);
    }
}

// Find the node with value val in the list l and set the pointer to the node to the node before it
int findList(list *l, int val, list **node, list **prev)
{
    if (l == NULL)
    {
        return 0;
    }

    *prev = NULL;
    *node = l;
    do
    {
        if ((*node)->val == val)
        {
            return 1;
        }
        *prev = *node;
        *node = (*node)->next;
    } while ((*node) != NULL);

    return 0;
}

// Remove a node from the list
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

// Find and removes node with value n from list l
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

// Return length of the list l
int lengthList(list *l)
{
    int len = 0;
    while (l != NULL)
    {
        len++;
        l = l->next;
    }
    return len;
}

// Return 1 if l1 == l2, 0 otherwise
int isEqualList(list *l1, list *l2)
{
    while (l1 != NULL && l2 != NULL)
    {
        if (l1->val != l2->val)
        {
            return 0;
        }
        l1 = l1->next;
        l2 = l2->next;
    }
    if (l1 != l2)
    {
        return 0;
    }
    return 1;
}

// Return 1 if l1 is a superset of l2, 0 otherwise
int isContainedList(list *l1, list *l2)
{
    // for each value in l2, if it's not n l1 then return 0 else continue
    while (l2 != NULL)
    {
        list *node = NULL;
        list *prev = NULL;
        if (findList(l1, l2->val, &node, &prev) == 0)
        {
            return 0;
        }
        l1 = l1->next;
    }
    return 1;
}

// Remove elements from l1 that are in l2. Return 1 if removed something, 0 otherwise.
int reduceList(list **l1, list *l2)
{
    int changed = 0;
    list *node = *l1;
    list *prev = NULL;
    while (node != NULL && l2 != NULL)
    {
        if (node->val == l2->val)
        {
            list *next = node->next;
            removeList(l1, &node, &prev);
            node = next;
            l2 = l2->next;
            changed = 1;
        }
        else
        {
            if (node->val > l2->val)
            {
                l2 = l2->next;
            }
            else
            {
                node = node->next;
            }
        }
    }
    return changed;
}

// Return a clone of l1
list *cloneList(list *l1)
{
    list *l2 = NULL;
    list *prev = NULL;
    while (l1 != NULL)
    {
        list *new = (list *)malloc(sizeof(list));
        new->val = l1->val;
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