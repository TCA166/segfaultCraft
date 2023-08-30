#include <stdlib.h>
#include <stdbool.h>
#include "list.h"

listEl* getElement(listHead* list, int index){
    if(index < 0 || index > list->len){
        return NULL;
    }
    listEl* res = list->first;
    while(res != NULL){
        if(index == 0){
            break;
        }
        res = res->next;
        index--;
    }
    return res;
}

listHead* initList(){
    listHead* list = malloc(sizeof(listHead));
    list->len = 0;
    list->first = NULL;
    return list;
}

listEl* newElement(void* value){
    listEl* new = malloc(sizeof(listEl));
    new->next = NULL;
    new->prev = NULL;
    new->value = value;
    new->head = NULL;
    return new;
}

unsigned int addElement(listHead* list, void* value){
    listEl* new = newElement(value);
    new->head = list;
    list->len++;
    int index = 0;
    listEl* el = list->first;
    if(el == NULL){
        list->first = new;
    }
    else{
        listEl* prev = list->last;
        prev->next = new;
        new->prev = prev;
    }
    list->last = new;
    return index;
}

listEl* removeElement(listHead* list, unsigned int index){
    if(index > list->len){
        return NULL;
    }
    listEl* el = list->first;
    while(el != NULL && index != 0){
        index--;
        el = el->next;
    }
    if(el != NULL){
        unlinkElement(el);
    }
    return el;
}

listEl* unlinkElement(listEl* el){
    if(el->prev != NULL){ //if there is a previous element
        el->prev->next = el->next; //we need to bridge the gap
    }
    else{
        el->head->first = el->next; //else the list head is the previous element and needs to be set accordingly
    }
    if(el->next != NULL){ //if there is a next element
        el->next->prev = el->prev; //we need to bridge the gap
    }
    else{
        el->head->last = el->prev; //else we need to update the last pointer
    }
    el->head = NULL;
    el->prev = NULL;
    listEl* next = el->next;
    el->next = NULL;
    return next;
}

void freeListElement(listEl* el, bool freeValue){
    if(freeValue){
        free(el->value);
    }
    free(el);
}

void freeList(listHead* list, void (*freeValFunc)(void* val)){
    listEl* el = list->first;
    while(el != NULL){
        listEl* next = el->next;
        if(freeValFunc != NULL){
            (*freeValFunc)(el->value);
        }
        free(el);
        el = next;
    }
    free(list);
}