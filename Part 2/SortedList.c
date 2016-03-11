#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    if (list == NULL || element == NULL)
        return;

    SortedListElement_t *curNode = list;

    // First node
    if (list->next == NULL)
    {
        list->next = element;
        list->prev = NULL;
        if (opt_yield & INSERT_YIELD)
            pthread_yield();
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
            if (opt_yield & INSERT_YIELD)
                pthread_yield();
            element->next = NULL;
            return;
        }
        else // Before last node
        {
            element->next = curNode;
            curNode->prev->next = element;
            if (opt_yield & INSERT_YIELD)
                pthread_yield();
            element->prev = curNode->prev;
            curNode->prev = element;
            return;
        }
    }
    else // Somewhere in the middle, place before curNode
    {
        element->next = curNode;
        curNode->prev->next = element;
        if (opt_yield & INSERT_YIELD)
                pthread_yield();
        element->prev = curNode->prev;
        curNode->prev = element;
        return;
    }
}

int SortedList_delete(SortedListElement_t *element)
{
    if (element == NULL || element->prev == NULL)
        return 1;

    // End of list
    if (element->next == NULL)
    {
        if (element->prev->next != element)
            return 1;
        element->prev->next = NULL;
        if (opt_yield & DELETE_YIELD)
            pthread_yield();
        element->prev = NULL;
    }
    else // Somewhere in middle, or beginning
    {
        if (element->prev->next != element ||
            element->next->prev != element)
            return 1;

        element->prev->next = element->next;
        if (opt_yield & SEARCH_YIELD)
            pthread_yield();
        element->next->prev = element->prev;
        element->prev = NULL;
        element->next = NULL;
    }

    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    // Can't find the element in a corrupted or empty list
    if (key == NULL || list == NULL || list->next == NULL)
        return NULL;

    SortedListElement_t *curNode = list->next;

    while (curNode != NULL)
    {
        if (curNode->key == NULL)
            return NULL;

        if (strcmp(key, curNode->key) == 0)
            return curNode;
        if (opt_yield & SEARCH_YIELD)
                pthread_yield();
        curNode = curNode->next;
    }

    return curNode;
}

int SortedList_length(SortedList_t *list)
{
    int i;
    int length = 0;
    SortedListElement_t *curNode;

    if (list == NULL)
        return -1;

    for(i = 0; i < num_lists; ++i)
    {
        curNode = &list[i];
        if (curNode == NULL)
            break;

        while (curNode->next != NULL)
        {
            length++;
            SortedListElement_t *prevNode = curNode;
            if (opt_yield & SEARCH_YIELD)
                pthread_yield();
            if (curNode == NULL)
                break;
            curNode = curNode->next;
            if (curNode == NULL || prevNode == NULL)
                break;
            if (prevNode->next == NULL || curNode->prev == NULL)
                return -1;
        }
    }

    return length;
}
