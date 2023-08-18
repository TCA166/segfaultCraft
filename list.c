#include <stdlib.h>
#include <stdbool.h>
#include "list.h"

typedef struct listEl{
    listEl* next;
    void* value;
    listHead* head;
} listEl;

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
    }
    return index;
}

listEl* removeElement(listHead* list, int index){
    if(index < 0 || index > list->len){
        return NULL;
    }
    listEl* el = list->first;
    listEl* prev = NULL;
    while(el != NULL){
        if(index == 0){
            break;
        }
        index--;
        prev = el;
        el = el->next;
    }
    if(el != NULL){
        el->head = NULL;
        prev->next = el->next;
        el->next = NULL;
    }
    return el;
}