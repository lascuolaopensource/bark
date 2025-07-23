#include <WiFi.h>
#include <Preferences.h>
#include "LoRaWan_APP.h"
#include <time.h>
#include <stdarg.h>
#include "HT_SSD1306Wire.h"

//Replace with the actual pins used
#define MASTER_VALVE 48
#define VALVE_4 19
#define VALVE_8 26
// ...define other vales

#define RELAY_COUNT 8   // Must be 8 in this version
#define NC_RELAYS true  // If set to true, relay are active when the pin is set to LOW and viceversa.
#define MAX_SCHEDULES 10
#define MAX_SCHEDULE_DURATION 120 // In minutes. Max allowed value is 255 minutes.
#define MIN_SCHEDULE_DURATION 2 // In minutes. 0 < MIN_SCHEDULE_DURATION < MAX_SCHEDULE_DURATION
#define TIME_ZONE "CET-1CEST,M3.5.0/2,M10.5.0/3" // Italy timezone

// Replace these two values if useWiFiTime is true
#define WIFI_SSID ""
#define WIFI_PASS ""


bool useWiFiTime = true;

int currentDisplayLine = 0;
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

Preferences preferences;

uint8_t relayPins[] = { MASTER_VALVE, VALVE_4, VALVE_8, 47, 33, 5, 6, 7 };

struct Schedule {
  bool active;           // Whether the schedule should be considered or not.
  uint8_t id;            // 0 <-> (MAX_SCHEDULES -1)
  uint8_t daysOfWeek;    // Bitmask for 7 days. Monday is the LSB, sunday is (MSB - 1) and MSB is unused. (e.g. 0b00000111 = Mon-Wed).
  uint8_t hour;          // 0<->23
  uint8_t minute;        // 0<->59
  uint8_t duration;      // 1<-> MAX_SCHEDULE_DURATION in minutes
  uint8_t activeRelays;  // Bitmask for relay statues. LSB is relayPins[0] and MSB is relayPins[RELAY_COUNT]
};


Schedule schedules[MAX_SCHEDULES];

/*
  OTAA parameters
  Replace them with your unique values. devEui can be generated using the unique id of the chip of the heltec device
*/
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Device EUI
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Join EUI (aka App EUI)
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/*
  ABP parameters
  Currently not used as we are doing OTAA
*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr = (uint32_t)0x00000000;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
uint8_t confirmedNbTrials = 4;
uint32_t appTxDutyCycle = 15000;
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_C;
bool overTheAirActivation = true;
bool isTxConfirmed = false;
bool loraWanAdr = true;
uint8_t appPort = 2;

void setupRelays() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], NC_RELAYS ? HIGH : LOW);
  }
}

void activateRelays(uint8_t bitmask) {
  for (int i = 0; i < RELAY_COUNT; i++) {
    digitalWrite(relayPins[i], ((bitmask & (1 << i)) ^ NC_RELAYS) ? HIGH : LOW);  //Combination of NC_RELAYS and the value of each bit simplifies to an XOR
  }
}

void syncTimeViaWiFi() {
  debugPrint("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  debugPrint("Connected");

  debugPrint("Updating time");
  configTzTime(TIME_ZONE, "pool.ntp.org");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) delay(500);
  debugPrint("Done");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  debugPrint("Wifi disabled");
}

void loadSchedules() {
  debugPrint("Loading schedules...");
  preferences.begin("schedules", true);
  for (int i = 0; i < MAX_SCHEDULES; i++) {
    char key[6];
    sprintf(key, "s%d", i);
    if (preferences.isKey(key)) {
      preferences.getBytes(key, &schedules[i], sizeof(Schedule));
      debugPrint("Loaded schedule %d: %02X %02d:%02d duration %d min relays %02X", i, schedules[i].daysOfWeek, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].activeRelays);
    } else {
      if (schedules[i].active) {
        debugPrint("Loaded default schedule %d: %02X %02d:%02d duration %d min relays %02X", i, schedules[i].daysOfWeek, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].activeRelays);
      } else debugPrint("Schedule %d is empty", i);
    }
  }
  preferences.end();
}

bool storeSchedule(Schedule* schedule) {
  if (schedule->id >= MAX_SCHEDULES || schedule->duration > MAX_SCHEDULE_DURATION || schedule->duration < MIN_SCHEDULE_DURATION) {
    debugPrint("Invalid schedule data. id: %d, duration: %d", schedule->id, schedule->duration);
    return false;
  }
  preferences.begin("schedules", false);
  char key[6];
  sprintf(key, "s%d", schedule->id);
  preferences.putBytes(key, schedule, sizeof(Schedule));
  preferences.end();
  return true;
}

void checkSchedules() {
  struct tm now;
  if (!getLocalTime(&now)) return;
  int currentDay = (now.tm_wday == 0) ? 6 : now.tm_wday - 1;
  for (int i = 0; i < MAX_SCHEDULES; i++) {
    Schedule schedule = schedules[i];
    if (!schedule.active) continue;
    if ((schedule.daysOfWeek & (1 << currentDay)) == 0) continue;
    if (now.tm_hour == schedule.hour && now.tm_min == schedule.minute) {
      debugPrint("Running schedule %d", schedule.id);
      activateRelays(schedule.activeRelays);
      // WARNING: Schedules are blocking. That means we cannot have overlapping schedules and may have delayed schedules
      delay(schedule.duration * 60000UL);
      activateRelays(0);
      debugPrint("Schedule %d completed", schedule.id);
    }
  }
}

void downLinkDataHandle(McpsIndication_t* mcpsIndication) {
  debugPrint("REC. Size %d, Port %d", mcpsIndication->BufferSize, mcpsIndication->Port);
  debugPrint("------");
  for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++) {
    debugPrint("%02X", mcpsIndication->Buffer[i]);
  }
  debugPrint("------");
  if (mcpsIndication->Port != appPort) {
    debugPrint("Invalid port");
    return;
  }

  switch (mcpsIndication->Buffer[0]) {
    case 0xA1:
      // Set current state of relays
      if (mcpsIndication->BufferSize != 2) {
        debugPrint("Invalid data size for command A1");
        return;
      }
      activateRelays(mcpsIndication->Buffer[1]);
      break;
    case 0xA2:
      // Set schedule
      if (mcpsIndication->BufferSize != sizeof(Schedule) + 1) {
        debugPrint("Invalid data size for command A2");
        return;
      }
      int id = mcpsIndication->Buffer[2];
      if (mcpsIndication->Buffer[2] >= MAX_SCHEDULES) {
        debugPrint("Invalid schedule id: %d", id);
        return;
      }
      Schedule schedule;
      memcpy(&schedule, mcpsIndication->Buffer + 1, sizeof(Schedule));

      if (storeSchedule(&schedule)) {
        appDataSize = 2;
        appData[0] = 0xB1;
        appData[1] = mcpsIndication->Buffer[7];
      }
      break;
  }
}

void debugPrint(const char* format, ...) {
  char buffer[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);

  // The display debug print has not been tested yet
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  int lineHeight = 10;
  int maxLines = 64 / lineHeight;
  String message = "-" + String(buffer);
  int lineLen = 21;  // max characters per line at font size 10

  while (message.length()) {
    String line = message.substring(0, lineLen);
    message = message.substring(lineLen);

    display.drawString(0, currentDisplayLine * lineHeight, line);
    currentDisplayLine++;

    if (currentDisplayLine >= maxLines) {
      display.clear();
      currentDisplayLine = 0;
    }
  }

  display.display();
}

void setManualTime(int year, int month, int day, int hour, int minute, int second) {
  struct tm t;
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = 0;

  time_t timeSinceEpoch = mktime(&t);
  struct timeval now = { .tv_sec = timeSinceEpoch };
  settimeofday(&now, NULL);

  debugPrint("Manual time set: %04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  setupRelays();  // We call this first to be sure all vales are closed on startup.
  Serial.begin(115200);

  VextON();
  delay(100);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  debugPrint("Start");

  if (useWiFiTime) {
    syncTimeViaWiFi();
  } else {
    // Replace with the current time. Consider using wifi or adding an RTC module, as this approach is not accurate
    // (eg. it will be off by the compile + upload time of the sketch and the startup time of the board till this line)
    setManualTime(2025, 7, 21, 18, 0, 0);
  }

  loadSchedules();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  LoRaWAN.init(loraWanClass, loraWanRegion);
  LoRaWAN.setDefaultDR(3);
  LoRaWAN.join();
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_SEND:
      {
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass);
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
  checkSchedules();
  delay(1000);
}
