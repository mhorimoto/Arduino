#include "MyWire.h"
#include "PN532_I2C.h"
#include "NFCCard.h"

#include "Monitor.h"

#define IRQ   (2)
#define RESET (0xff)  // Not connected by default on the NFC Shield
PN532 nfc(PN532::I2C_ADDRESS, IRQ, RESET);

Monitor mon(Serial);

const byte cardkey_b[] = {
  0xBB, 0x63, 0x45, 0x74, 0x55, 0x79, 0x4B };


long prev;
NFCCard card;

void setup() {
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  mon.begin(57600);

  Wire.begin();
  nfc.begin();

  mon << "Hi.";

  mon << "Firmware version: ";
  unsigned long r = 0;
  for (int i = 0; i < 10 ; i++) {
    if ( (r = nfc.getFirmwareVersion()) )
      break;
    delay(250);
  }
  if (! r ) {
    mon << "Couldn't find PN53x on Wire." << mon.endl;
    while (1); // halt
  } 
  else {
    mon << mon.setbase(HEX) << mon.printHexString((byte*)&r, 4) << mon.endl;
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println(r & 0xff, HEX);
  Serial.print("Firmware ver. "); 
  Serial.print(r>>8 & 0xFF, DEC); 
  Serial.print('.'); 
  Serial.println(r>>16 & 0xFF, DEC);


  Serial.println("SAMConfiguration");
  nfc.SAMConfiguration();

  delay(500);

}

void loop() {
  byte c, tmp[64];

  if ( millis() > prev + 50 ) {
    prev = millis();
    tmp[0] = nfc.Type_GenericPassiveTypeA;
    tmp[1] = nfc.Type_GenericPassive212kbFeliCa;
    tmp[2] = nfc.Type_GenericPassive424kbFeliCa;
    tmp[3] = nfc.Type_PassiveTypeB;
    if ( nfc.InAutoPoll(2,2,tmp,4) != 0 && (c = nfc.autoPoll(tmp)) != 0 ) {
      mon << mon.endl << "Num of tags = " << (int)tmp[0] << mon.endl;
      PN532::printHexString(tmp, 16);
      card.setInListPassiveTarget(tmp[1], tmp[2], tmp+3);
      mon << "type = " << card.type << mon.endl;

      if ( card.type == 0x11 ) {
        mon << "FeliCa" << mon.endl 
          << " ID: " << mon.printHexString( card.IDm(), 8) << mon.endl;
        mon << "Pad: " << mon.printHexString( card.PMm(), 8) << mon.endl;
//        mon << mon.printHexString( card.SystemCode(), 2) << mon.endl;
        int len;
        // Polling command, with system code request.
        len = nfc.felica_Polling(tmp, 0x9e80);
        mon << mon.printHexString(tmp, len) << mon.endl;
        //
        mon << "Request System Code: " << mon.endl;
        len = nfc.felica_RequestSystemCode(tmp, card.IDm());
        mon << mon.printHexString((word *)tmp, len) <<mon.endl;
        // low-byte first service code.
        // Suica, Nimoca, etc. 0x090f
        // Edy service 0x1317, system 0x00FE // 8280
        // FCF 1a8b
        mon << "Request Service: " << mon.endl;
        word scodes[] = { 0x1a8b, 0x1317, 0x090f, 0xffff };
        int snum = 4;
        c = nfc.felica_RequestService(tmp, card.IDm(), scodes, snum);
        mon << "c = " << c << mon.endl;
        mon << mon.printHexString((word*)tmp,c) << mon.endl;
        mon << "Read w/o Enc.: " << mon.endl;
        byte blks[] = {0x80, 0x00};
        c = nfc.felica_ReadWithoutEncryption(tmp, card.IDm(), 0x090f, 1, blks);
        mon << "Read: ";
        if ( c != 0 ) {
          mon << mon.printHexString(tmp, c*16) << mon.endl;
        } else {
          mon << "status flags " << mon.printHexString(tmp+9, 2) << mon.endl;
        }
      } 
      else if ( card.type == 0x10 ) {
        mon << "Mifare" << mon.endl << "  ID: ";
        mon.printHexString(card.UID(), card.IDLength());
        mon << mon.endl;
      }

      delay(1000);
    } 
  }
  delay(1000);
}
























