#include "canboose_frametransferlayer.h"

void FrameTransferLayer::init(NetworkTransportListener *listener) {
  // Timer to send messages not running
  queueTimerRunning = false;
  
  // A listener to notify LCC message to the above layer
  netListener = listener;
  
  // We want to transmit in order so number of TX mailboxes = 1
  Can0.begin(125000);
  Can0.attachObj(this);
  Can0.setNumTxBoxes(1);
  
  // Teensy 3.x has 16 mailboxes. We will configure like this:
  // 0 -    -> Receive standard frames
  // 1 - 14 -> Receive extended frames
  // 15     -> Transmit frames
  CAN_filter_t extFilter;
  extFilter.id=0;
  extFilter.ext=1;
  extFilter.rtr=0;
  for (int filterNum = 1; filterNum < 15; filterNum++) {
    Can0.setFilter(extFilter, filterNum);
  }
  
  // This instance will process/receive all frames
  attachGeneralHandler();
  
  // OK. Let's start generating a Node source alias
  // Initialize a new ramdom seed. Pin A0 Teensy 3.6 should be empty
  randomSeed(analogRead(A0));
  checkID();
}

void FrameTransferLayer::checkID() {
  checkID_timer.end();
  
  // Generate a tentative sourceNodeID
  sourceNodeID = random(1, 4095);
  
  // We are in inhibited state
  permitted = false;
  collisionNodeID = false;
  
  // Send 4 CheckID messages
  sendFrame(CID_FIRST  | (uint32_t) ((UID & 0xFFF000000000) >> 24), NULL, 0);
  sendFrame(CID_SECOND | (uint32_t) ((UID & 0x000FFF000000) >> 12), NULL, 0);
  sendFrame(CID_THIRD  | (uint32_t) (UID & 0x000000FFF000), NULL, 0);
  sendFrame(CID_FOURTH | (uint32_t) (UID & 0x000000000FFF) << 12, NULL, 0);
  
  // Wait 250 milliseconds. Standard says a minimum of 200
  checkID_timer.begin(proxyReserveID, 250000);
}

void FrameTransferLayer::reserveID() {
  checkID_timer.end();
  
  // If there was no collision we have to send a ReserveID and Alias Mad Definition message
  // Then we have a valid sourceID
  if (!collisionNodeID && sendFrame(RID, NULL, 0) && sendFrame(AMD, UID_array, 6)) {
    permitted = true;
    if (netListener != NULL) netListener->initializationComplete();
  }
  else {
    // Wait another 250 milliseconds before starting again
    checkID_timer.begin(proxyCheckID, 250000);
  }
}

bool FrameTransferLayer::sendFrame(uint32_t header, uint8_t data[], uint8_t len) {
  if (len >= 0 && len <= 8) {
    // In the inhibited state we can send only CID, RID and AMD frames
    if (!permitted) {
      uint16_t cid_header     = header >> 24;
      uint32_t rid_amd_header = header >> 12;
      bool allowed_to_send    = cid_header == 0x17 || cid_header == 0x16 ||
                                cid_header == 0x15 || cid_header == 0x14 ||
                                rid_amd_header == 0x10700 || rid_amd_header == 0x10701;
      if (!allowed_to_send) {
        return false;
      }
    }
    
    // Just to be sure, 12 lower bits will be zeroed and we will add sourceNodeID
    CAN_message_t msg = {0, 0, {1, 0, 0, 0}, 0};
    msg.id = (header & 0x3FFFF000) | sourceNodeID;
    msg.flags.extended = 1;
    msg.len = len;
    memcpy(msg.buf, data, len);
    
    return Can0.write(msg) == 1;
  }
  
  return false;
}

void FrameTransferLayer::queueFrame(uint32_t header, uint8_t data[], uint8_t len) {
  queueOperation(0, header, data, len);
  if (!queueTimerRunning) {
    queueTimer.begin(proxySendQueue, 2000); // Every 2 ms we will fill TX queue
    queueTimerRunning = true;
  }
}

void FrameTransferLayer::sendQueuedFrames() {
  bool result;
  
  do {
    result = false;
    queueNode *queuedFrame = queueOperation(1, 0, NULL, 0);
    if (queuedFrame != NULL) {  // Message in queue, we will try to send to CAN TX queue
      result = sendFrame(queuedFrame->header, queuedFrame->data, queuedFrame->len);
      if (result) queueOperation(2, 0, NULL, 0);  // Queued in CAN TX queue, delete from our queue
    }
    else {
      queueTimer.end();  // No more messages in our queue to send. Stop timer
      queueTimerRunning = false;
    }
  } while (result);
}

/* -------------------------------------------------------------
 *  OpType can be:
 *
 *  0 - Push operation
 *  1 - Read front element
 *  2 - Delete front element
 *
 *  Because we will delete it from queue only and only if we have send it
 */
queueNode* FrameTransferLayer::queueOperation(uint8_t opType, uint32_t header, uint8_t data[], uint8_t len) {
  // Only when we "pop" from the queue
  queueNode* temp = NULL;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // PUSH operation
    if (opType == 0) {
      temp = queue.push();
      
      temp->header = header;
      memcpy(temp->data, data, len);
      temp->len = len;
    }
    // READ operation first element in queue
    else if (opType == 1) {
      temp = queue.readFront();
    }
    // DELETE front element
    else if (opType == 2) {
      queue.deleteFront();
    }
  }
  
  return temp;
}

void FrameTransferLayer::printFrame(CAN_message_t &frame, int mailbox) {
  String result = "CANBus message -> ID: ";
  result.concat(String(frame.id, HEX));
  result.concat(" X:");
  result.concat(frame.flags.extended);
  result.concat(" R:");
  result.concat(frame.flags.remote);
  result.concat(" OvR:");
  result.concat(frame.flags.overrun);
  result.concat(" Len:");
  result.concat(frame.len);
  result.concat(" Data: ");
  for (int c = 0; c < frame.len; c++) {
    result.concat(String(frame.buf[c], HEX));
    result.concat(" ");
  }
  Serial.println(result);
}

bool FrameTransferLayer::frameHandler(CAN_message_t &frame, int mailbox, uint8_t controller) {
  // First we have to check source NodeID alias in case of collisions
  uint16_t incoming_sourceNodeID = frame.id & 0xFFF;
  if (incoming_sourceNodeID == sourceNodeID) {
    uint16_t cid_header = frame.id >> 24;
    bool isCheckID_frame = cid_header == 0x17 || cid_header == 0x16 ||
                           cid_header == 0x15 || cid_header == 0x14;
    
    // We have received a checkID with the same source NodeID alias we have
    // Answer with a reserveID
    if (isCheckID_frame) {
      sendFrame(RID, NULL, 0);
    }
    // We have received a frame with the same source NodeID alias we have
    // We have to transition to inhibited state
    else if (permitted) {
      sendFrame(AMR, UID_array, 6);
      checkID();
    }
    // Collision during checkID phase. Flag it. The timer will restart the process
    else {
      collisionNodeID = true;
    }
  }
  // No collisions, we will process rest of incoming messages if we are in permitted state
  else if (permitted) {
    // Only in permitted state we will process Alias Map Enquiry frames
    if ((frame.id & 0xFFFFF000) == AME) {
      if (frame.len == 0) {
          queueFrame(AMD, UID_array, 6);
      }
      else {
        if (frame.len == 6 &&
            frame.buf[0] == UID_array[0] && frame.buf[1] == UID_array[1] &&
            frame.buf[2] == UID_array[2] && frame.buf[3] == UID_array[3] &&
            frame.buf[4] == UID_array[4] && frame.buf[5] == UID_array[5])
          queueFrame(AMD, UID_array, 6);
      }  
    }
    // Message for the above layer (Network layer)
    else {
      // ONLY if it is a LCC Message (top 2 bits equal to 1)
      if ((frame.id & LCC_MSG) == LCC_MSG) {
        if (netListener != NULL) {
          uint8_t frameType = (frame.id & 0x07000000) >> 24; 
          uint16_t mti_or_dst = (frame.id & 0x00FFF000) >> 12;
          uint16_t srcAlias = (frame.id & 0x00000FFF);
          netListener->processLCCMessage(frameType, mti_or_dst, srcAlias, frame.buf, frame.len);
        }
      }
    }
  }
    
  return true;
}
