#include <stdio.h>

typedef struct list
{
    int val;
    struct list *next;
} list;

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

//Remove elements from l1 that are in l2. Return 1 if removed something, 0 otherwise.
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
        return changed;
    }
}