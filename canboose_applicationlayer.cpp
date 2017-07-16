#include <EEPROM.h>
#include "canboose_applicationlayer.h"

void ApplicationLayer::init() {
  network.init(this);
}

void ApplicationLayer::processApplicationMessage(uint16_t mti, uint16_t srcAlias, uint8_t data[], uint8_t len) {
  switch (mti) {
    case SIMPLE_NODE_INFORMATION_REQUEST:
      sendSimpleNodeInformationReply(srcAlias);
      break;
  }
}

void ApplicationLayer::sendSimpleNodeInformationReply(uint16_t srcAlias) {
  // An array to put everything
  uint8_t data2send[253] = { 0x04 };
  uint8_t index = 1;
  
  // Put everything in array
  index = addStringToArray(mft, data2send, index);
  index = addStringToArray(mft_model, data2send, index);
  index = addStringToArray(mft_hw_version, data2send, index);
  index = addStringToArray(mft_sw_version, data2send, index);
  data2send[index] = 0x02;
  index++;
  index = addStringToArray(getNameProvidedByUser(), data2send, index);
  index = addStringToArray(getDescriptionProvidedByUser(), data2send, index);

  // How many frames we have to send?
  int num_blocks = index / 6;
  int last_block_size = index % 6;
  if (last_block_size > 0) num_blocks++;
  if (last_block_size == 0) last_block_size = 6;

  // loops, loops everywhere. OK send 
  uint8_t dest[2];
  dest[0] = (srcAlias & 0x0F00) >> 8;
  dest[1] = srcAlias & 0xFF;
  for (int block = 0; block < num_blocks; block++) {
    uint8_t block2send[8];
    block2send[0] = dest[0];
    block2send[1] = dest[1];
    
    // Only block, first, middle or last block ?
    // Only one block
    if (num_blocks == 1) {  
      memcpy(&block2send[2], data2send, index);
      network.sendMessage(SIMPLE_NODE_INFORMATION_REPLY, block2send, 2 + index);
    }
    // First block
    else if (block == 0) { 
      block2send[0] += 0x10;
      memcpy(&block2send[2], &data2send[block * 6], 6);
      network.sendMessage(SIMPLE_NODE_INFORMATION_REPLY, block2send, 8);
    }
    // Last block
    else if (block + 1 == num_blocks) {
      block2send[0] += 0x20;
      memcpy(&block2send[2], &data2send[block * 6], last_block_size);
      network.sendMessage(SIMPLE_NODE_INFORMATION_REPLY, block2send, 2 + last_block_size);
    }
    // Middle block
    else {
      block2send[0] += 0x30;
      memcpy(&block2send[2], &data2send[block * 6], 6);
      network.sendMessage(SIMPLE_NODE_INFORMATION_REPLY, block2send, 8);
    }
  }
}

uint8_t ApplicationLayer::addStringToArray(String s, uint8_t dest[], uint8_t atPosition) {
  uint8_t size = s.length();
  memcpy(&dest[atPosition], &s[0], size);
  dest[atPosition + size] = 0; // Null terminated strings

  return size + atPosition + 1;
}

void ApplicationLayer::processApplicationDatagram(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  if (len > 0) {
    switch (data[0]) {
      case 0x20:
        processMemoryConfigurationProtocol(srcAlias, data, len);
        break;

      default:
        network.sendDatagramRejected(srcAlias, 0x1042);  // Datagram type Uknown
        break;
    }
  }
  else {
    network.sendDatagramRejected(srcAlias, 0x1042);  // Datagram type Uknown
  }
}

void ApplicationLayer::processMemoryConfigurationProtocol(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  if (len >= 2) {
    switch (data[1]) {
      // Write Command
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
        writeCommand(srcAlias, data, len);
        break;
      
      // Read Command
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
        readReply(srcAlias, data, len);
        break;

      // Get configuration options command
      case 0x80:
        getConfigurationOptionsReply(srcAlias, data, len);
        break;

      // Get address space information command
      case 0x84:
        getAddressSpaceInformationReply(srcAlias, data, len);
        break;

      default:
        network.sendDatagramRejected(srcAlias, 0x1042);  // Datagram type Uknown
        break;
    }
  }
  else {
    network.sendDatagramRejected(srcAlias, 0x1042);  // Datagram type Uknown
  }
}

void ApplicationLayer::writeCommand(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  // Check length of command is OK
  uint8_t min_length_should_be = 7;
  if (data[1] == 0x00) min_length_should_be = 8;

  // OK to continue?
  if (len >= min_length_should_be) {
    // Memory address to write
    uint32_t address = 0;
    address = (data[2] << 24) + (data[3] << 16) + (data[4] << 8) + data[5];

    // Number of bytes to write
    uint8_t count = 0;
    if (data[1] == 0x00) {
      count = len - 7;
    }
    else {
      count = len - 6;
    }

    // To what space we have to write?
    uint8_t space = 0;
    switch (data[1]) {
      case 00:
        space = data[6];
        // User provided information space?
        if (space == 0xFB) {
          writeUserSpace(srcAlias, address, &data[min_length_should_be - 1], count);
        }
        else {
          // TODO - 0xFC is a read only space. Manufacturer info space
          
        }
        break;

      case 0x01:
        // TODO - Device configuration space
        break;

      case 0x02:
        // TODO - All memory space
        break;
        
      default:
        // TODO - Space unknown or read-only, error
        break;
    }
  }
  else {
    // TODO - Send error code, not a well formed command
    
  }
}

void ApplicationLayer::writeUserSpace(uint16_t srcAlias, uint32_t address, uint8_t data[], uint8_t count) {
  if (address == 0) {
    setVersionProvidedByUser(data[0]);
  }
  else if (address == 1) {
    setNameProvidedByUser(data, count);
  }
  else if (address == 64) {
    setDescriptionProvidedByUser(data, count);
  }
  else {
    // TODO - Address out of bounds

    return;
  }

  network.sendDatagramOK(srcAlias);
}

void ApplicationLayer::readReply(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  // Check length of command is OK
  uint8_t length_should_be = 7;
  if (data[1] == 0x40) length_should_be = 8;

  // OK to continue?
  if (len == length_should_be) {
    // Memory address to read
    uint32_t address = 0;
    address = (data[2] << 24) + (data[3] << 16) + (data[4] << 8) + data[5];

    // Number of bytes to read
    uint8_t count = 0;
    if (data[1] == 0x40) {
      count = data[7] & 0x7F;  // Upper bit reserved and ignored (normative)
    }
    else {
      count = data[6] & 0x7F;  // Upper bit reserved and ignored (normative)
    }

    // From what space we have to read?
    uint8_t space = 0;
    switch (data[1]) {
      case 0x40:
        // What space?
        space = data[6];
        if (space == 0xFC) {
          readManufacturerSpace(srcAlias, address, count);
        }
        else if (space == 0xFB) {
          readUserSpace(srcAlias, address, count);
        }
        break;
      case 0x41:
        // Space = 0xFD;
        readConfigurationSpace(srcAlias, address, count);
        break;
      case 0x42:
        // Space = 0xFE;
        // TODO
        break;
      case 0x43:
        // Space = 0xFF;
        readCDI(srcAlias, address, count);
        break;
      default:
        // TODO - Space unknown, error
        break;
    }
  }
  else {
    // TODO - Send error code, not a well formed command
    
  }
}

void ApplicationLayer::readCDI(uint16_t srcAlias, uint32_t address, uint8_t count) {
  // Some configuration tool is asking for the xml
  uint16_t size = count;
  uint16_t xml_size = cdi_string.length() + 1;
  if (address < xml_size && size <= 64) {
    // Create array and send it
    if (address + size > xml_size) size = xml_size - address;
    uint8_t data[size];
    memcpy(data, &cdi_string[address], size);
    sendReply(srcAlias, 0x53, address, 0, data, size);
  }
  else {
    // Out of bounds, sorry
    
  }
}

void ApplicationLayer::readManufacturerSpace(uint16_t srcAlias, uint32_t address, uint8_t count) {
  // Five addresses can be read from manufacturer configuration: 0, 1, 42, 83 and 104
  uint8_t data[41] = { 0 };
  uint8_t size;
  String text = "";
  if (address == 0) {
    size = 1;
    data[0] = mft_version;
  }
  else if (address == 1) {
    size = 41;
    text = mft;
  }
  else if (address == 42) {
    size = 41;
    text = mft_model;
  }
  else if (address == 83) {
    size = 21;
    text = mft_hw_version;
  }
  else if (address == 104) {
    size = 21;
    text = mft_sw_version;
  }
  else {
    // TODO- Address unknown

    return;
  }

  if (address > 0) memcpy(data, &text[0], text.length() + 1);  
  sendReply(srcAlias, 0x40, address, 0xFC, data, size);  
}

void ApplicationLayer::readUserSpace(uint16_t srcAlias, uint32_t address, uint8_t count) {
  // Three addresses can be read from user configuration: 0, 1 and 64
  uint8_t data[64] = { 0 };
  uint8_t size;
  if (address == 0) {  // Version (byte)
    data[0] = getVersionProvidedByUser();
    size = 1;
    sendReply(srcAlias, 0x50, address, 0xFB, data, size);
  }
  else if (address == 1) {
    String text = getNameProvidedByUser();
    uint8_t text_size = text.length();
    size = 63;
    memcpy(data, &text[0], text_size + 1);
    sendReply(srcAlias, 0x50, address, 0xFB, data, size);
  }
  else if (address == 64) {
    String text = getDescriptionProvidedByUser();
    uint8_t text_size = text.length();
    size = 64;
    memcpy(data, &text[0], text_size + 1);  
    sendReply(srcAlias, 0x50, address, 0xFB, data, size);  
  }
  else {
    // TODO - Address not valid
    
  }
}

uint8_t ApplicationLayer::getVersionProvidedByUser() {
  return EEPROM.read(0);
}

void ApplicationLayer::setVersionProvidedByUser(uint8_t b) {
  EEPROM.write(0, b);
}

String ApplicationLayer::getNameProvidedByUser() {
  uint8_t b[63];
  for (int i = 0; i < 63; i++) {
    b[i] = EEPROM.read(i + 1);
  }
  return (const char*) b;
}

void ApplicationLayer::setNameProvidedByUser(uint8_t data[], uint8_t len) {
  uint8_t max = len;
  if (max > 63) max = 63;
  for (int i = 0; i < max; i++) {
    EEPROM.write(i + 1, data[i]);
  }
}

String ApplicationLayer::getDescriptionProvidedByUser() {
  uint8_t b[64];
  for (int i = 0; i < 64; i++) {
    b[i] = EEPROM.read(i + 64);
  }
  return (const char*) b;
}

void ApplicationLayer::setDescriptionProvidedByUser(uint8_t data[], uint8_t len) {
  uint8_t max = len;
  if (max > 64) max = 64;
  for (int i = 0; i < max; i++) {
    EEPROM.write(i + 64, data[i]);
  }
}

void ApplicationLayer::readConfigurationSpace(uint16_t srcAlias, uint32_t address, uint8_t count) {
  uint8_t data[count] = { 0 };
  sendReply(srcAlias, 0x51, address, 0, data, count);
}

void ApplicationLayer::sendReply(uint16_t srcAlias, uint8_t command_type, uint32_t address, uint8_t space, uint8_t data[], uint8_t count) {
  uint8_t array_of_data[72] = {0x20, command_type, (uint8_t) ((address & 0xFF000000) >> 24), 
                                                   (uint8_t) ((address & 0x00FF0000) >> 16),
                                                   (uint8_t) ((address & 0x0000FF00) >> 8),
                                                   (uint8_t) (address & 0x000000FF) };
  uint8_t index = 6;
  if (space > 0) {
    array_of_data[6] = space;
    index++;
  }
  
  memcpy(&array_of_data[index], data, count);
  network.sendDatagram(srcAlias, array_of_data, count + index, true);
}

void ApplicationLayer::getConfigurationOptionsReply(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  // Send GetConfigurationOptionsReply
  // TODO
  uint8_t data2send[7] = {0x20, 0x82, 0x08 + 0x04 + 0x02, 0x00, 0x80 + 0x02, 0xFF, 0xFB};
  network.sendDatagram(srcAlias, data2send, 7, true);
}

void ApplicationLayer::getAddressSpaceInformationReply(uint16_t srcAlias, uint8_t data[], uint8_t len) {
  if (len == 3) {
    // Send getAddressSpaceInformationReply
    // TODO - Calculate high address of each space
    String description = "";
    uint8_t spacePresent = 0x87;  // true
    uint8_t flags = 0x00;
    uint32_t high_address = 0;

    switch (data[2]) {
      case 0xFF:
        description = "Configuration definition information";
        high_address = cdi_string.length() + 1;
        flags = 0x01;
        break;

      case 0xFE:
        description = "All device's memory";
        break;

      case 0xFD:
        description = "Device configuration";
        break;

      case 0xFC:
        description = "Manufacturer information";
        high_address = 125;
        flags = 0x01;
        break;

      case 0xFB:
        description = "User entered information";
        high_address = 128;
        break;

      default:
        spacePresent = 0x86;
        break;
    }

    // Prepare array to send as a datagram
    uint8_t descLength = description.length();
    uint8_t arrayLength = 9 + descLength;
    uint8_t data2send[arrayLength] = {0x20, spacePresent, data[2], (uint8_t) ((high_address & 0xFF000000) >> 24), 
                                                                   (uint8_t) ((high_address & 0x00FF0000) >> 16),
                                                                   (uint8_t) ((high_address & 0x0000FF00) >> 8),
                                                                   (uint8_t) (high_address & 0x000000FF), flags};
    memcpy(&data2send[8], &description[0], descLength + 1);

    // Send it
    network.sendDatagram(srcAlias, data2send, arrayLength, true);
  }
  else {
    network.sendDatagramRejected(srcAlias, 0x1042);  // Datagram type Uknown
  }
}
