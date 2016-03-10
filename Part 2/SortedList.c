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

    // Start at first element
    curNode = curNode->next;

    // One or more elements in list already
    while (curNode->next != NULL && strcmp(element->key, curNode->key) > 0)
        curNode = curNode->next;

    // End of list
    if (curNode->next == NULL)
    {
        // After last node
        if (strcmp(element->key, curNode->key) > 0)
        {
            curNode->next = element;
            element->prev = curNode;
            element->next = NULL;
            return;
        }
        else // Before last node
        {
            element->next = curNode;
            curNode->prev->next = element;
            element->prev = curNode->prev;
            curNode->prev = element;
            return;
        }
    }
    else // Somewhere in the middle, place before curNode
    {
        element->next = curNode;
        curNode->prev->next = element;
        element->prev = curNode->prev;
        curNode->prev = element;
        return;
    }

}

int SortedList_delete(SortedListElement_t *element)
{

    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{

    return NULL;
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
