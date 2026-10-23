#include <stdlib.h>
#include "doubly_linked_list.h"

void add_node(DLL **list_ptr, void *data) {
  DLL *new_node = malloc(sizeof(DLL));
  new_node->data = data;
  new_node->prev = NULL;
  new_node->next = NULL;

  if (*list_ptr == NULL) {
    // no elements in list yet
    *list_ptr = new_node;
  } else {
    new_node->next = *list_ptr;
    new_node->prev = NULL; // will become head of list

    // current head of list now becomes second in list
    (*list_ptr)->prev = new_node;

    *list_ptr = new_node; // head of list is new_node
  }
}

void remove_node(DLL **list_ptr, DLL *node) {
  if (node->prev == NULL && node->next == NULL) {
    *list_ptr = NULL;
  } else if (node->prev == NULL) {
    *list_ptr = node->next;
    node->next->prev = NULL;
  } else if (node->next == NULL) {
    node->prev->next = NULL;
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
  free(node);
}
