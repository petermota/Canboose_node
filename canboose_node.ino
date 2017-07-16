/* -------------------------------------------------------------
 *  Canboose Node
 */

#include <FlexCAN.h>
#include <util/atomic.h>

uint8_t UID_array[6] = { 0x05, 0x01, 0x01, 0x01, 0x2D, 0x00 };

ApplicationLayer app;

/* -------------------------------------------------------------
 *  Init in setup. Everything else asynchronous
 */
void setup(void) {  
  Serial.begin(9600);
  Serial.println("Canboose Node v1.0"); 

  app.init();
}

void proxyCheckID() {
  app.network.frameTransferLayer.checkID();
}

void proxyReserveID() {
  app.network.frameTransferLayer.reserveID();
}

void proxySendQueue() {
  app.network.frameTransferLayer.sendQueuedFrames();
}

/* -------------------------------------------------------------
 *  Loop event
 *  Empty
 */
void loop(void) {

}
