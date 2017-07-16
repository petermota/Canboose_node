/*
 * A dynamic linked list to store datagrams and to reconstruct the original data.
 * Also used to store sent datagram before the OK or Rejected messages
 * And a class to implement it
 */

#ifndef __CANBOOSE_LINKEDLIST_H__
#define __CANBOOSE_LINKEDLIST_H__

#include "Arduino.h"

struct linkedListNode {
  uint16_t  alias;
  uint8_t   data[72];
  uint8_t   len;
  linkedListNode *next;
};

class LinkedListClass {
  public:
    linkedListNode* insertNode();
    linkedListNode* findNode(uint16_t alias);
    void deleteNode(uint16_t alias);

  private:
    linkedListNode *front = NULL;
};

#endif
