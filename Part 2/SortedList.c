#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    if (!list || !element)
        return;

    int i = 0; // Start at 0 or 1?
    int length = SortedList_length(list);
    SortedListElement_t *curNode = list;

    while (i != length)
    {
        if (element->)


        i++;
    }

}

int SortedList_delete( SortedListElement_t *element)
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
