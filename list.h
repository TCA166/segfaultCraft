#include <stdlib.h>
#include <stdbool.h>

//Our own list implementation since the cNBT one is a bit off

#ifndef LIST_OUR_H
#define LIST_OUR_H

//A wrapper struct for a doubly linked list
typedef struct listHead listHead;

//An element in a doubly linked list
typedef struct listEl listEl;

struct listHead{
    listEl* first;
    size_t len;
    listEl* last;
};

struct listEl{
    listEl* next; //If NULL then this is the last element
    listEl* prev; //If NULL then this is the first element
    void* value;
    listHead* head; //If NULL then next and prev should be NULL
};

/*!
 @brief Iterates over every element within a list.
 @param list the listHead* that wraps your list
 @param el the name of the provided listEl* variable. Freeing this will cause invalid reads
*/
#define foreachListElement(list, el) \
    for(listEl* el = list->first; el != NULL; el=el->next)

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
unsigned int addElement(listHead* list, void* value);

/*!
 @brief Removes element at index
 @param list the list head
 @param index the index of element to be removed
*/
listEl* removeElement(listHead* list, unsigned int index);

/*!
 @brief Unlinks the element from the list
 @param el the element to unlist
 @return the next element to the removed one
*/
listEl* unlinkElement(listEl* el);

/*!
 @brief Initializes the list
 @return the initialized list
*/
listHead* initList();

/*!
 @brief Frees the list
 @param list the list to be freed
 @param freeValFunc pointer to the function that should be used to free list values, or NULL
*/
void freeList(listHead* list, void (*freeValFunc)(void* val));

/*!
 @brief Frees the list element
 @param el the list element to be freed
 @param freeValue should the value of this element be freed
*/
void freeListElement(listEl* el, bool freeValue);

#endif
