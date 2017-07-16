#ifndef __CANBOOSE_QUEUE_H__
#define __CANBOOSE_QUEUE_H__

#include "Arduino.h"

/* --------------------------------------------------------------------------------------------------------
 *  A dynamic queue to store frames to be sent. When using protocols like Simple Node Information
 *  the buffer inside FlexCan Libreary is not enough. It can store up to 16 CAN Frames.
 *  Application protocols use datagrams and can send up to 9 frames, so we need a FIFO queue to 
 *  store these messages and send them when FlexCAN TX buffer has capacity again.
 *
 *  We start a timer every 2 ms. to fill CAN TX ring buffer. We stop this timer when there are
 *  no more messages to send. So this queue uses memory in cases when we have a lot of traffic.
 */
struct queueNode {
  uint32_t header;
  uint8_t data[8];
  uint8_t len;
  queueNode *next;
};

class QueueClass {
  public:
    queueNode* push();
    queueNode* readFront();
    void deleteFront();
    
  private:
    queueNode *front = NULL;
    queueNode *rear = NULL;
};

#endif
