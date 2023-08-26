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
    listEl* next;
    listEl* prev;
    void* value;
    listHead* head;
};

#define foreachListElement(list) \
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
int addElement(listHead* list, void* value);

/*!
 @brief Removes element at index
 @param list the list head
 @param index the index of element to be removed
*/
listEl* removeElement(listHead* list, int index);

/*!
 @brief Unlinks the element from the list
 @param el the element to unlist
 @return the previous element to the removed one
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
 @param freeValues whether free(el.value) should be called
*/
void freeList(listHead* list, bool freeValues);

/*!
 @brief Frees the list element
 @param el the list element to be freed
 @param freeValue should the value of this element be freed
*/
void freeListElement(listEl* el, bool freeValue);

#endif
