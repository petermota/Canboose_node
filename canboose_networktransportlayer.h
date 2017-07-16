#ifndef __CANBOOSE_NETWORKTRANSPORTLAYER_H__
#define __CANBOOSE_NETWORKTRANSPORTLAYER_H__

#include "Arduino.h"
#include "canboose_linkedlist.h"
#include "canboose_frametransferlayer.h"

class ApplicationListener {
  public:
    virtual void processApplicationMessage(uint16_t mti, uint16_t srcAlias, uint8_t data[], uint8_t len);
    virtual void processApplicationDatagram(uint16_t srcAlias, uint8_t data[], uint8_t len);
};

/* -------------------------------------------------------------
 *  Message Network Layer. Global and addressed messages
 */

#define INIT_COMPLETE_FULL          0x100
#define INIT_COMPLETE_SIMPLE        0x101
#define VERIFY_NODE_ID_ADDRESSED    0x488
#define VERIFY_NODE_ID_GLOBAL       0x490
#define VERIFIED_NODE_ID_FULL       0x170
#define VERIFIED_NODE_ID_SIMPLE     0x171
#define OPTIONAL_INTERACTION_REJ    0x068
#define TERMINATE_DUE_TO_ERROR      0x0A8
#define PROTOCOL_SUPPORT_INQUIRY    0x828
#define PROTOCOL_SUPPORT_REPLY      0x668

/* -------------------------------------------------------------
 *  Protocol support reply
 */
#define SIMPLE_PROTOCOL_SUBSET                  0x800000
#define DATAGRAM_PROTOCOL                       0x400000
#define STREAM_PROTOCOL                         0x200000
#define MEMORY_CONFIGURATION_PROTOCOL           0x100000
#define RESERVATION_PROTOCOL                    0x080000
#define PRODUCER_CONSUMER_PROTOCOL              0x040000
#define IDENTIFICATION_PROTOCOL                 0x020000
#define TEACHING_LEARNING_PROTOCOL              0x010000
#define REMOTE_BUTTON_PROTOCOL                  0x008000
#define ABREVIATED_DEFAULT_CDI_PROTOCOL         0x004000
#define DISPLAY_PROTOCOL                        0x002000
#define SIMPLE_NODE_INFORMATION_PROTOCOL        0x001000
#define CONFIGURATION_DESCRIPTION_INFORMATION   0x000800
#define TRACTION_CONTROL_PROTOCOL               0x000400
#define FUNCTION_DESCRIPTION_INFORMATION        0x000200
#define DCC_COMMAND_STATION_PROTOCOL            0x000100
#define SIMPLE_TRAIN_NODE_INFORMATION           0x000080
#define FUNCTION_CONFIGURATION                  0x000040
#define FIRMWARE_UPGRADE_PROTOCOL               0x000020
#define FIRMWARE_UPGRADE_ACTIVE                 0x000010

/* -------------------------------------------------------------
 *  Event Transport Layer
 */
#define PRODUCER_CONSUMER_EVENT_REPORT    0x5B4
#define IDENTIFY_CONSUMER                 0x8F4
#define CONSUMER_IDENTIFIED_VALID         0x4C4
#define CONSUMER_IDENTIFIED_INVALID       0x4C5
#define CONSUMER_IDENTIFIED_UNKNOWN       0x4C7
#define CONSUMER_RANGE_IDENTIFIED         0x4A4
#define IDENTIFY_PRODUCER                 0x914
#define PRODUCER_IDENTIFIED_VALID         0x544
#define PRODUCER_IDENTIFIED_INVALID       0x545
#define PRODUCER_IDENTIFIED_UNKNOWN       0x547
#define PRODUCER_RANGE_IDENTIFIED         0x524
#define IDENTIFY_EVENTS_GLOBAL            0x970
#define IDENTIFY_EVENTS_ADDRESSED         0x968
#define LEARN_EVENT                       0x594

/* -------------------------------------------------------------
 * Datagram Layer
 */

#define DATAGRAM_RECEIVED_OK      0xA28
#define DATAGRAM_REJECTED         0xA48

class NetworkTransportLayer : public NetworkTransportListener {
  public:
    void init(ApplicationListener *listener);
    void initializationComplete();
    void processLCCMessage(uint8_t frameType, uint16_t mti_or_dst, uint16_t srcAlias, uint8_t data[], uint8_t len);
    void processGlobalAndAddressedMessage(uint16_t mti_or_dst, uint16_t srcAlias, uint8_t data[], uint8_t len);
    void sendMessage(uint16_t type, uint8_t data[], uint8_t len);
    void sendDatagram(uint16_t dstAlias, uint8_t data[], uint8_t len, bool ackPreviousDatagram);
    void sendDatagramOK(uint16_t dstAlias);
    void sendDatagramRejected(uint16_t dstAlias, uint16_t errorCode);

    FrameTransferLayer frameTransferLayer;
    
  private:
    bool isMessageForUs(uint8_t data[], uint8_t len);
    bool isDatagramForUs(uint16_t dstAlias);
    linkedListNode* linkedListOperation(uint8_t opType, LinkedListClass *list, uint16_t srcAlias, uint8_t data[], uint8_t len);
    void fragmentDatagramAndSend(uint16_t dstAlias, uint8_t data[], uint8_t len);
    
    ApplicationListener *appListener;
    LinkedListClass outgoingDatagrams;
    LinkedListClass incomingDatagrams;
};

#endif
