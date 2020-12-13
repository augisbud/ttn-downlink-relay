#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

/* DEFINE THE RELAY PIN */
#define R1 21

/* THESE ARE YOUR AUTHENTICATION KEYS FOR OTAA APPEUI AND DEVEUI SHOULD BE IN MSB */
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

static const u1_t PROGMEM DEVEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

/* SIZE OF THE TRANSMIT BUFFER IS 1 */
static uint8_t dataTX[1];
static osjob_t sendjob;

/* INTERVAL BETWEEN UPLINK MESSAGES */
const unsigned TX_INTERVAL = 60; 

/* PIN MAPPINGS, ADJUST THEM TO YOUR CONFIGURATION */
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {26, 33, 32}
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        Serial.print("netid: ");
        Serial.println(netid, DEC);
        Serial.print("devaddr: ");
        Serial.println(devaddr, HEX);
        Serial.print("artKey: ");
        for (int i = 0; i < sizeof(artKey); ++i) {
          Serial.print(artKey[i], HEX);
        }
        Serial.println("");
        Serial.print("nwkKey: ");
        for (int i = 0; i < sizeof(nwkKey); ++i) {
          Serial.print(nwkKey[i], HEX);
        }
        Serial.println("");
      }
      LMIC_setLinkCheckMode(0);
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen == 4) {
        /* SIZE OF THE DOWNLINK BUFFER 1 */
        int payload[1];
        for (int i = 0; i < LMIC.dataLen; i++) {
          payload[i] = int(LMIC.frame[LMIC.dataBeg + i]);
        }
        digitalWrite(R1, (payload[0] == 1) ? HIGH : LOW);
        Serial.println((payload[0] == 1) ? "HIGH" : "LOW");
        Serial.println();
      }
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
  }
}

void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    dataTX[0] = (digitalRead(R1)) ? 1 : 0;
    Serial.println((digitalRead(R1)) ? 1 : 0);

    LMIC_setTxData2(1, dataTX, sizeof(dataTX), 0);
    Serial.println(F("Packet queued"));
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Starting"));
  
  /* INITIALIZE THE RELAY PIN */
  pinMode(R1, OUTPUT);

  os_init();
  LMIC_reset();
#define LMIC_CLOCK_ERROR_PERCENTAGE 3
  LMIC_setClockError(LMIC_CLOCK_ERROR_PERCENTAGE * (MAX_CLOCK_ERROR / 100.0));

  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
