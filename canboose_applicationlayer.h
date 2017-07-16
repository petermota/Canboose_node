#ifndef __CANBOOSE_APPLICATIONLAYER_H__
#define __CANBOOSE_APPLICATIONLAYER_H__

#include <Arduino.h>

#include "canboose_networktransportlayer.h"

/* -------------------------------------------------------------
 *  Application Layer. Implementation of application protocols
 */

/* -------------------------------------------------------------
 * Simple Node Imformation Protocol 
 */

#define SIMPLE_NODE_INFORMATION_REQUEST     0xDE8
#define SIMPLE_NODE_INFORMATION_REPLY       0xA08

class ApplicationLayer : public ApplicationListener {
  public:
    void init();
    void processApplicationMessage(uint16_t mti, uint16_t srcAlias, uint8_t data[], uint8_t len);
    void processApplicationDatagram(uint16_t srcAlias, uint8_t data[], uint8_t len);
    
    NetworkTransportLayer network;

  private:
    // Simple Node Information Protocol
    void sendSimpleNodeInformationReply(uint16_t srcAlias);
    uint8_t addStringToArray(String s, uint8_t dest[], uint8_t atPosition);

    // Memory configuration protocol
    void processMemoryConfigurationProtocol(uint16_t srcAlias, uint8_t data[], uint8_t len);
    void writeCommand(uint16_t srcAlias, uint8_t data[], uint8_t len);
    void writeUserSpace(uint16_t srcAlias, uint32_t address, uint8_t data[], uint8_t count);
    void readReply(uint16_t srcAlias, uint8_t data[], uint8_t len);
    void readCDI(uint16_t srcAlias, uint32_t address, uint8_t count);
    void readManufacturerSpace(uint16_t srcAlias, uint32_t address, uint8_t count);
    void readUserSpace(uint16_t srcAlias, uint32_t address, uint8_t count);
    uint8_t getVersionProvidedByUser();
    void setVersionProvidedByUser(uint8_t b); 
    String getNameProvidedByUser();
    void setNameProvidedByUser(uint8_t data[], uint8_t len);
    String getDescriptionProvidedByUser();
    void setDescriptionProvidedByUser(uint8_t data[], uint8_t len);
    void readConfigurationSpace(uint16_t srcAlias, uint32_t address, uint8_t count);
    void sendReply(uint16_t srcAlias, uint8_t command_type, uint32_t address, uint8_t space, uint8_t data[], uint8_t count);
    void getConfigurationOptionsReply(uint16_t srcAlias, uint8_t data[], uint8_t len);
    void getAddressSpaceInformationReply(uint16_t srcAlias, uint8_t data[], uint8_t len);

    // Manufacturer information
    uint8_t mft_version = 1;
    String  mft = "Canboose Inc.";
    String  mft_model = "Canboose 16 I/O node";
    String  mft_hw_version = "1.0";
    String  mft_sw_version = "1.0";

    // String to store the xml
    String cdi_string = "<?xml version=\"1.0\"?>\n"
                        "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                        "xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\n"
                        "<identification>\n"
                          "<manufacturer>" + mft + "</manufacturer>\n"
                          "<model>" + mft_model + "</model>\n"
                          "<hardwareVersion>" + mft_hw_version + "</hardwareVersion>\n"
                          "<softwareVersion>" + mft_sw_version + "</softwareVersion>\n"
                        "</identification>\n"
                        "<acdi/>\n"
                        "<segment space=\"251\">\n"
                          "<name>User Identification</name>\n"
                          "<description>Add your own name and description for this node</description>\n"
                          "<int size=\"1\">\n"
                            "<name>Version</name>\n"
                          "</int>\n"
                          "<string size=\"63\">\n"
                            "<name>Node Name</name>\n"
                          "</string>\n"
                          "<string size=\"64\">\n"
                            "<name>Node Description</name>\n"
                          "</string>\n"
                        "</segment>\n"
                        "<segment space=\"253\">\n"
                          "<group replication=\"16\">\n"
                            "<name>Turnouts</name>\n"
                            "<description>16 output lines to control Kato turnouts</description>\n"
                            "<repname>Turnout</repname>\n"
                            "<eventid>\n"
                              "<name>Straight EventId</name>\n"
                              "<description>This event will set turnout to straight position</description>\n"
                            "</eventid>\n"
                            "<eventid>\n"
                              "<name>Diverging EventId</name>\n"
                              "<description>This event will set turnout to diverging position</description>\n"
                            "</eventid>\n"
                          "</group>\n"
                        "</segment>\n"
                        "</cdi>";
};

#endif
