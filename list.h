#include <stdlib.h>
#include <stdbool.h>

//Our own list implementation since the cNBT one is a bit off

#ifndef LIST_OUR_H
#define LIST_OUR_H

//Abstracted list element struct
typedef struct listEl listEl;

//Head for list
typedef struct listHead{
    listEl* first;
    size_t len;
} listHead;

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

#endif
