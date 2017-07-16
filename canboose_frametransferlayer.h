#ifndef __CANBOOSE_FRAME_TRANSFER_LAYER_H__
#define __CANBOOSE_FRAME_TRANSFER_LAYER_H__

#include <FlexCAN.h>
#include <util/atomic.h>
#include "Arduino.h"
#include "canboose_queue.h"

// This is the Unique Identifier given to us by openLCB organization
#define UID 0x050101012D00
extern uint8_t UID_array[6];

/* -----------------------------------------------------------------------------------------------------------
 * Dummy functions, will be implemented later, because we can not call a instance method
 * from a IntervalTimer
 * They are just proxies to call the right method in the singleton instance
 */
extern void proxyCheckID();
extern void proxyReserveID();
extern void proxySendQueue();

/* -----------------------------------------------------------------------------------------------------------
 *  Frame Transfer Layer. Low level communications at CAN bus
 *  level
 */

// Can Control Messages. CheckID phase
#define CID_FIRST     0x17000000  // Check ID's
#define CID_SECOND    0x16000000
#define CID_THIRD     0x15000000
#define CID_FOURTH    0x14000000
#define RID           0x10700000  // Reservation ID
#define AMD           0x10701000  // Alias Map Definition
#define AME           0x10702000  // Alias Map Enquiry
#define AMR           0x10703000  // Alias Map Reset
#define LCC_MSG       0X18000000  // Mask to detect LCC Messages

class NetworkTransportListener {
public:
  virtual void initializationComplete();
  virtual void processLCCMessage(uint8_t frameType, uint16_t mti_or_dst, uint16_t srcAlias, uint8_t data[], uint8_t len);
};

class FrameTransferLayer : public CANListener {
  public:
    void init(NetworkTransportListener *listener);
    void queueFrame(uint32_t header, uint8_t data[], uint8_t len);
    void sendQueuedFrames();
    bool frameHandler(CAN_message_t &frame, int mailbox, uint8_t controller);
    void checkID();
    void reserveID();
    
    uint16_t sourceNodeID;
    
  private:
    bool sendFrame(uint32_t header, uint8_t data[], uint8_t len);
    queueNode* queueOperation(uint8_t opType, uint32_t header, uint8_t data[], uint8_t len);
    void printFrame(CAN_message_t &frame, int mailbox);
    
    bool permitted;
    bool collisionNodeID;
    IntervalTimer checkID_timer;
    IntervalTimer queueTimer;
    bool queueTimerRunning;
    NetworkTransportListener *netListener;
    QueueClass queue;
};

#endif
