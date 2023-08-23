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
    new->value = value;
    new->head = NULL;
    return new;
}

int addElement(listHead* list, void* value){
    listEl* new = newElement(value);
    new->head = list;
    list->len++;
    int index = 0;
    listEl* el = list->first;
    if(el == NULL){
        list->first = new;
    }
    else{
        listEl* prev = NULL;
        while(el != NULL){
            prev = el;
            el = el->next;
            index++;
        }
        prev->next = new;
        new->prev = prev;
    }
    return index;
}

listEl* removeElement(listHead* list, int index){
    if(index < 0 || index > list->len){
        return NULL;
    }
    listEl* el = list->first;
    while(el != NULL){
        if(index == 0){
            break;
        }
        index--;
        el = el->next;
    }
    if(el != NULL){
        
    }
    return el;
}

listEl* unlinkElement(listEl* el){
    listEl* prev = el->prev;
    prev->next = el->next;
    el->next->prev = prev;
    el->head = NULL;
    el->prev = NULL;
    el->next = NULL;
    return prev;
}

void freeListElement(listEl* el, bool freeValue){
    if(freeValue){
        free(el->value);
    }
    free(el);
}

void freeList(listHead* list, bool freeValues){
    listEl* el = list->first;
    while(el != NULL){
        listEl* next = el->next;
        freeListElement(el, freeValues);
        el = next;
    }
    free(list);
}