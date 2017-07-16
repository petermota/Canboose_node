#include "canboose_linkedlist.h"

linkedListNode* LinkedListClass::insertNode() {
  // Create a new node at the end of the list
  linkedListNode *temp = (linkedListNode*) malloc(sizeof(linkedListNode));
  temp->next = NULL;

  // Is the list empty?  
  if (front == NULL) {
    front = temp;
  }
  else {
    linkedListNode *aux = front;
    while (aux->next != NULL) {
      aux = aux->next;
    }
    aux->next = temp;
  }

  // Return newly inserted node
  return temp;
}

linkedListNode* LinkedListClass::findNode(uint16_t alias) {
  // Beginning of list
  linkedListNode *temp = front;

  // Where is the node?
  while (temp != NULL) {
    if (temp->alias == alias) 
      return temp;
    else
      temp = temp->next;
  }

  return NULL;
}

void LinkedListClass::deleteNode(uint16_t alias) {
  // Find the node and delete it
  linkedListNode *temp = front;
  linkedListNode *previous = NULL;

  // Where is the node?
  while (temp != NULL) {
    if (temp->alias == alias) {
      // We found the node
      if (temp == front) { // Is the first node
        front = temp->next;  // Now front will point to the next node (or NULL if there are no more)
      }
      else {  // Middle or last node
        previous->next = temp->next;
      }

      // Free memory
      free(temp);
      
      // Quit loop
      break;
    }
    else {
      previous = temp;
      temp = temp->next;
    }
  }
}
