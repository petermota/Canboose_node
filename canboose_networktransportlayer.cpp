#include "canboose_networktransportlayer.h"

void NetworkTransportLayer::init(ApplicationListener *listener) {
  appListener = listener;
  frameTransferLayer.init(this);
}

void NetworkTransportLayer::initializationComplete() {
  sendMessage(INIT_COMPLETE_FULL, UID_array, 6);
}

void NetworkTransportLayer::processLCCMessage(uint8_t frameType, uint16_t mti_or_dst, uint16_t srcAlias, uint8_t data[], uint8_t len) {
  switch (frameType) {
    // General LCC Messages
    case 1:
      processGlobalAndAddressedMessage(mti_or_dst, srcAlias, data, len);
      break;

    // Datagram. All data in one message
    case 2:
      if (mti_or_dst == frameTransferLayer.sourceNodeID) {
        appListener->processApplicationDatagram(srcAlias, data, len);
      }
      break;

    // Datagram. First message (more to come)
    case 3:
      if (mti_or_dst == frameTransferLayer.sourceNodeID) {
        linkedListNode *lln = linkedListOperation(0, &incomingDatagrams, srcAlias, data, len);
        if (lln == NULL) sendDatagramRejected(srcAlias, 0x2040);
      }
      break;

    // Datagram. Middle message (more to come)
    case 4:
      if (mti_or_dst == frameTransferLayer.sourceNodeID) {
        linkedListNode *lln = linkedListOperation(1, &incomingDatagrams, srcAlias, data, len);
        if (lln == NULL) sendDatagramRejected(srcAlias, 0x2040);
      }
      break;

    // Datagram. Last message
    case 5:
      if (mti_or_dst == frameTransferLayer.sourceNodeID) {
        linkedListNode *lln = linkedListOperation(1, &incomingDatagrams, srcAlias, data, len);  // Find node
        if (lln != NULL) {
          appListener->processApplicationDatagram(lln->alias, lln->data, lln->len);  // Process it
          linkedListOperation(2, &incomingDatagrams, lln->alias, lln->data, lln->len); // Delete it
        }
        else sendDatagramRejected(srcAlias, 0x2040);
      }
      break;

    // Stream
    case 7:
      break;
  }
}

/*
 * opType can be:
 * 
 * 0 - Insert a new node at the end
 * 1 - Find & update a node with new data
 * 2 - Return the data received (all datagrams received) and delete the node
 * 3 - Find node only
 */
linkedListNode* NetworkTransportLayer::linkedListOperation(uint8_t opType, LinkedListClass *list, uint16_t srcAlias, uint8_t data[], uint8_t len) {
  linkedListNode *lln = NULL;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    switch (opType) {
      case 0:
        // A little paranoic but maybe we are trying to insert a new node but we have one already
        // We will overwrite because it is garbage
        lln = list->findNode(srcAlias);
        if (lln == NULL) lln = list->insertNode();

        // Fill with data
        lln->alias = srcAlias;
        memcpy(lln->data, data, len);
        lln->len = len;
        break;

      case 1:
        // Update a node with newly arrived data
        lln = list->findNode(srcAlias);
        if (lln != NULL) {
          memcpy(&lln->data[lln->len], data, len);
          lln->len += len;
        }
        break;

      case 2:
        list->deleteNode(srcAlias);
        break;

      case 3:
        lln = list->findNode(srcAlias);
        break;
    }
  }

  return lln;
}

bool NetworkTransportLayer::isMessageForUs(uint8_t data[], uint8_t len) {
  if (len >= 2) {
    uint16_t aliasID = ((data[0] & 0x0F) << 8) + data[1];
    if (aliasID == frameTransferLayer.sourceNodeID) {
      return true;
    }
  }

  return false;
}

void NetworkTransportLayer::processGlobalAndAddressedMessage(uint16_t mti_or_dst, uint16_t srcAlias, uint8_t data[], uint8_t len) {
  switch (mti_or_dst) {
    case VERIFY_NODE_ID_ADDRESSED:
      // Addressed message. First 2 bytes of data contains aliasID targeted
      if (isMessageForUs(data, len)) {
        sendMessage(VERIFIED_NODE_ID_FULL, UID_array, 6);
      }
      break;
      
    case VERIFY_NODE_ID_GLOBAL:
      // If there is no NodeID it is a message for everybody: answer it
      if (len == 0) {
        sendMessage(VERIFIED_NODE_ID_FULL, UID_array, 6);
      }
      // If there is a NodeID, check it is for us
      else if (len == 6 &&
               data[0] == UID_array[0] && data[1] == UID_array[1] && data[2] == UID_array[2] &&
               data[3] == UID_array[3] && data[4] == UID_array[4] && data[5] == UID_array[5]) {
        sendMessage(VERIFIED_NODE_ID_FULL, UID_array, 6);    
      }
      break;
      
    case PROTOCOL_SUPPORT_INQUIRY:
      if (isMessageForUs(data, len)) {
        uint32_t supported = DATAGRAM_PROTOCOL + MEMORY_CONFIGURATION_PROTOCOL + ABREVIATED_DEFAULT_CDI_PROTOCOL +
                             SIMPLE_NODE_INFORMATION_PROTOCOL + CONFIGURATION_DESCRIPTION_INFORMATION;
        uint8_t supported_data[8];
        supported_data[0] = ((srcAlias & 0x0F00) >> 8);
        supported_data[1] = srcAlias & 0xFF;
        supported_data[2] = ((supported & 0xFF0000) >> 16);
        supported_data[3] = ((supported & 0x00FF00) >> 8);
        supported_data[4] =  (supported & 0x0000FF);
        supported_data[5] = 0;
        supported_data[6] = 0;
        supported_data[7] = 0;
        sendMessage(PROTOCOL_SUPPORT_REPLY, supported_data, 8);  
      }
      break;

    // Nothing to do with other messages
    case INIT_COMPLETE_FULL:
    case INIT_COMPLETE_SIMPLE:
    case VERIFIED_NODE_ID_FULL:
    case VERIFIED_NODE_ID_SIMPLE:
    case OPTIONAL_INTERACTION_REJ:
    case TERMINATE_DUE_TO_ERROR:
    case PROTOCOL_SUPPORT_REPLY:
      break;

    case DATAGRAM_RECEIVED_OK:
      linkedListOperation(2, &outgoingDatagrams, srcAlias, data, len);
      break;

    case DATAGRAM_REJECTED:
      if (len >= 4) {
        if ((data[2] & 0xF0) == 0x20) {  // If it's a temporal error resend it
          linkedListNode *lln = linkedListOperation(3, &outgoingDatagrams, srcAlias, data, len);  // Find node
          if (lln != NULL) {
            fragmentDatagramAndSend(lln->alias, lln->data, lln->len);
          }
        }
      }
      break;

    // Maybe it is a message for the application layer. We will check if it is for us
    // There should be Destination ID bit == 1 in MTI and a 2 byte data alias 
    default:
      if ((mti_or_dst & 0x0008) > 0 && isMessageForUs(data, len)) {
        appListener->processApplicationMessage(mti_or_dst, srcAlias, data, len);
      }
      break;
  }
}

void NetworkTransportLayer::sendMessage(uint16_t type, uint8_t data[], uint8_t len) {
  uint32_t canHeader = (0x19 << 24) + (type << 12);
  frameTransferLayer.queueFrame(canHeader, data, len);
}

void NetworkTransportLayer::sendDatagram(uint16_t dstAlias, uint8_t data[], uint8_t len, bool ackPreviousDatagram) {
  // Send ACK to sender?
  if (ackPreviousDatagram) sendDatagramOK(dstAlias);

  // Store Datagram in dynamic linked list 
  linkedListOperation(0, &outgoingDatagrams, dstAlias, data, len);

  // Now fragment it and queue to send the fragments
  fragmentDatagramAndSend(dstAlias, data, len);
}

void NetworkTransportLayer::fragmentDatagramAndSend(uint16_t dstAlias, uint8_t data[], uint8_t len) {
  // How many frames will we need to send the datagram?
  if (len <= 8) { // In one frame
    uint32_t canHeader = (0x1A << 24) + (dstAlias << 12);
    frameTransferLayer.queueFrame(canHeader, data, len);
  }
  else if (len <= 72) { // Max number of frames for datagrams are 9 (72 bytes)
    // How many frames are we going to send?
    uint32_t canHeader;
    uint8_t numberFrames = len / 8;
    uint8_t lastFrameSize = len % 8;
    if (lastFrameSize > 0) numberFrames++;
    if (lastFrameSize == 0) lastFrameSize = 8;  // If it is zero it means we have all frames complete

    for (int i = 0; i < numberFrames; i++) {
      if (i == 0) {  // First frame
        canHeader = (0x1B << 24) + (dstAlias << 12);
        frameTransferLayer.queueFrame(canHeader, &data[i], 8);
      }
      else if (i + 1 == numberFrames) {  // Last frame
        canHeader = (0x1D << 24) + (dstAlias << 12);
        frameTransferLayer.queueFrame(canHeader, &data[i * 8], lastFrameSize);
      }
      else {  // Middle frame
        canHeader = (0x1C << 24) + (dstAlias << 12);
        frameTransferLayer.queueFrame(canHeader, &data[i * 8], 8);
      }
    }
  }
}

// TODO
// Add more fields for reply pending etc
void NetworkTransportLayer::sendDatagramOK(uint16_t dstAlias) {
  uint8_t data2send[3] = {(uint8_t) ((dstAlias & 0xFF00) >> 8), (uint8_t) (dstAlias & 0xFF), 0x80};
  sendMessage(DATAGRAM_RECEIVED_OK, data2send, 3);
}

void NetworkTransportLayer::sendDatagramRejected(uint16_t dstAlias, uint16_t errorCode) {
  uint8_t data2send[4] = {(uint8_t) ((dstAlias & 0xFF00) >> 8), (uint8_t) (dstAlias & 0xFF),
                          (uint8_t) ((errorCode & 0xFF00) >> 8), (uint8_t) (errorCode & 0xFF)};
  sendMessage(DATAGRAM_REJECTED, data2send, 4);
}
