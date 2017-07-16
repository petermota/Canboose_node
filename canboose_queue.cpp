/* -----------------------------------------------------------------
 * Implementation of a dynamic queue
 */

#include "canboose_queue.h"

queueNode* QueueClass::push() {
  queueNode *temp = (queueNode*) malloc(sizeof(queueNode));
  temp->next = NULL;
  
  if (rear == NULL) {
    front = temp;
    rear = temp;
  }
  else {
    rear->next = temp;
    rear = rear->next;
  }
  
  return temp;
}

queueNode* QueueClass::readFront() {
  return front;
}

void QueueClass::deleteFront() {
  if (front != NULL) {
    queueNode *temp;
    temp = front;
    front = front->next;
    if (front == NULL) rear = NULL;
    free(temp);
  }
}
