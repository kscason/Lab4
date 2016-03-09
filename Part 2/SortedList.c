#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    if (!list || !element)
        return;

    SortedListElement_t *curNode = list;

    // First node
    if (SortedList_length(list) == 0)
    {
        list->next = element;
        list->prev = NULL;
        element->prev = list;
        element->next = NULL;
        return;
    }

    while (strcmp(curNode))

}

int SortedList_delete(SortedListElement_t *element)
{

}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{

}

int SortedList_length(SortedList_t *list)
{
    // I think this is right???? Should probs test
    int length = 0;
    SortedListElement_t *curNode;

    if (!list)
        return -1;

    curNode = (SortedListElement_t *) list;

    while (curNode->next)
    {
        length++;
        SortedListElement_t *prevNode = curNode;
        curNode = curNode->next;
        if (prevNode->next != curNode->prev)
            return -1;
    }

    return length;
}
