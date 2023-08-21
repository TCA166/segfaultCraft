#include <stdlib.h>
#include <stdbool.h>

//Our own list implementation since the cNBT one is a bit off

#ifndef LIST_OUR_H
#define LIST_OUR_H

typedef struct listHead listHead;

typedef struct listEl listEl;

//Head for list
struct listHead{
    listEl* first;
    size_t len;
};

struct listEl{
    listEl* next;
    void* value;
    listHead* head;
};

/*!
 @brief Gets the element in list at index
 @param list the list head
 @param index the element index
 @return pointer to the element 
*/
listEl* getElement(listHead* list, int index);

/*!
 @brief Appends a new element with the provided value
 @param list the list head
 @param value pointer to the new element value
 @return the new element index
*/
int addElement(listHead* list, void* value);

/*!
 @brief Removes element at index
 @param list the list head
 @param index the index of element to be removed
*/
listEl* removeElement(listHead* list, int index);

/*!
 @brief Initializes the list
 @return the initialized list
*/
listHead* initList();

/*!
 @brief Frees the list
 @param list the list to be freed
 @param freeValues whether free(el.value) should be called
*/
void freeList(listHead* list, bool freeValues);

#endif
