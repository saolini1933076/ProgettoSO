#include "linked_list.h"
#include <assert.h>
#include <stddef.h>


void List_init(ListHead* head) {
  head->first=0;
  head->last=0;
  head->size=0;
}

ListItem* List_find(ListHead* head, ListItem* item) {
  // linear scanning of list
  ListItem* aux=head->first;
  while(aux){
    if (aux==item)
      return item;
    aux=aux->next;
  }
  return 0;
}
//LIST INSERT MODIFICATA PER ELIMINARE UN ERRORE
ListItem* List_insert(ListHead* head, ListItem* prev, ListItem* item) {
  if (item->next || item->prev)
    return 0;
  
#ifdef _LIST_DEBUG_
  // Verifica che l'elemento non sia già presente nella lista
  ListItem* instance = List_find(head, item);
  assert(!instance);

  // Verifica che il elemento precedente sia nella lista
  if (prev) {
    ListItem* prev_instance = List_find(head, prev);
    assert(prev_instance);
  }
#endif

  ListItem* next = prev ? prev->next : head->first;
  if (prev) {
    item->prev = prev;
    prev->next = item;
  }
  if (next) {
    item->next = next;
    next->prev = item;
  }
  if (!prev)
    head->first = item;
  if (!next)
    head->last = item;
  ++head->size;
  return item;
}

ListItem* List_detach(ListHead* head, ListItem* item) {

#ifdef _LIST_DEBUG_
  // we check that the element is in the list
  ListItem* instance=List_find(head, item);
  assert(instance);
#endif

  ListItem* prev=item->prev;
  ListItem* next=item->next;
  if (prev){
    prev->next=next;
  }
  if(next){
    next->prev=prev;
  }
  if (item==head->first)
    head->first=next;
  if (item==head->last)
    head->last=prev;
  head->size--;
  item->next=item->prev=0;
  return item;
}

ListItem* List_pushBack(ListHead* head, ListItem* item) {
  return List_insert(head, head->last, item);
};

ListItem* List_pushFront(ListHead* head, ListItem* item) {
  return List_insert(head, 0, item);
};

ListItem* List_popFront(ListHead* head) {
  return List_detach(head, head->first);
}

//AGGIUNTA LA FUNZIONE
int List_size(const ListHead* head) {
  return head->size;
}

void* List_getByIndex(ListHead* list, int index) {
    if (list == NULL || index < 0 || index >= list->size) {
        return NULL;  // Indice non valido o lista vuota
    }

    ListItem* current = list->first;
    for (int i = 0; i < index; ++i) {
        current = current->next;
    }

    return current;
}
