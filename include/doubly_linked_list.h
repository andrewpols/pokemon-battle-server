#ifndef _DOUBLY_LINKED_LIST_H_
#define _DOUBLY_LINKED_LIST_H_

typedef struct DoublyLinkedList {
  void *data;
  struct DoublyLinkedList *prev;
  struct DoublyLinkedList *next;
} DLL;

void add_node(DLL **list_ptr, void *data);
void remove_node(DLL **list_ptr, DLL *node);

#endif
