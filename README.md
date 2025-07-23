# Bark
> ⚠️ **This repository is a work in progress.** Some technical details about the infrastructure or deployment may be intentionally omitted or obfuscated for security reasons.

**Bark** is a system to monitor and control the irrigation of public parks in the city of Bari.  
It was conceived and developed during the project **[Bari Città Aperta](https://baricittaperta.xyz/)**.

This repository contains the code used for the prototype installed at **Parco Mauggeri**, which served as the pilot park.

At this stage, the system includes the firmware for the irrigation controller and its LoRaWAN communication logic. The backend and frontend for centralized park management are not yet implemented. Currently, commands can be issued manually through the ChirpStack interface.

## System Overview

Bark relies on a lightweight, low-power infrastructure based on **LoRaWAN** and **ChirpStack**, with a local gateway acting as the bridge between devices and the network server.

### Hardware

The core unit is based on a **Heltec WiFi LoRa 32 (V3)** board, connected to a bank of 8 relays controlling 24V solenoid valves already available in the park. Each valve can be individually activated according to predefined irrigation schedules stored in the device’s non-volatile memory.

The solenoid valves are powered via a **220V to 24V AC transformer**. A separate **5V wall-mounted power supply** is used to power the Heltec board and relay logic.

The board includes a small OLED screen for on-site debugging and can connect to Wi-Fi to synchronize its internal clock via NTP when available. Otherwise, time can be manually set.

Schedules are defined in terms of days of the week, start time, duration, and which valves should be activated. These can be updated remotely over LoRa.

### Gateway and Networking

Communication between the field devices and the backend is handled using **LoRaWAN**. Each board operates as a **Class C device**, which remains in constant receive mode and listens for downlink instructions.

A **LoRaWAN gateway** (currently a **MikroTik model**, but any brand supporting the standard protocol can be used) is installed within range of the park. The gateway forwards messages between the devices and a **ChirpStack** network server instance, which handles device activation (OTAA), message routing, and uplink/downlink communication.

### ChirpStack

**ChirpStack** is used as the network server and is responsible for handling device sessions, downlink scheduling, and integration with applications. At this stage, it also serves as the interface for issuing commands and updating device configurations.

## Bill of Materials (BOM)

| Component                | Description                                                 |
| ------------------------ | ----------------------------------------------------------- |
| Heltec WiFi LoRa 32 (V3) | Microcontroller with LoRa and OLED display                  |
| 8-Channel Relay Module   | Controls the irrigation solenoid valves                     |
| Solenoid Valves (24V AC) | Electromechanical valves used for irrigation                |
| 220V to 24V Transformer  | Powers the solenoid valves                                  |
| 5V  Power Supply         | Powers the Heltec board and relay logic                     |
| MikroTik LoRaWAN Gateway | Acts as the LoRaWAN packet forwarder to ChirpStack          |
| Enclosure                | Case for electronics (Weatherproof is preferred for safety) |
| Misc. wires, connectors  | For all electrical connections                              |
| ChirpStack Server        | Can be hosted on a cloud VM or local server                 |

## Deployment Instructions

1. **Gateway Setup**  
   If no LoRaWAN gateway is available within range, install one and configure it to forward packets to your ChirpStack instance. Register the gateway within ChirpStack.

2. **ChirpStack Configuration**  
   Access ChirpStack and create the necessary tenant, application, and device profile (if not already existing). Register your device using its DevEUI.

3. **Arduino Configuration**  
   Open the firmware sketch and update the following parameters:
    - Relay pin definitions (ensure only usable GPIO pins are assigned)
    - OTAA credentials (DevEUI, AppEUI, AppKey)
    - Wi-Fi credentials (if `useWiFiTime = true`) or set time manually

4. **Wiring and Power**  
   Wire the relays to the Heltec board and to the valves. Connect the relay power line to the 24V transformer and the Heltec board to a 5V power adapter. Double-check all connections before powering up.

5. **Firmware Upload**  
   Connect the board via USB and upload the sketch using the Arduino IDE. Do not power the board through the 5V adapter during upload.

6. **Network Considerations**  
   Ensure Wi-Fi is available if using it for NTP time sync. Wi-Fi is only needed during startup and not required for normal LoRaWAN operation unless the board reboots.

7. **Power On and Test**  
   Plug in the board and transformer. Observe the OLED screen for startup messages. The system will sync time, load saved schedules, and attempt to join the LoRaWAN network.

8. **Verify Connection**  
   On ChirpStack, check that the device has successfully joined and is sending uplinks. You can now issue test downlink commands (e.g. trigger a valve or update a schedule).

## Current Status

- The irrigation firmware is deployed and controlling some sections (valves) of Parco Mauggeri
- Communication via LoRaWAN is a bit weak but reliable and tested through the MikroTik gateway. It’s probably because the gateway doesn’t have an external antenna, or because the Heltec and its antenna are inside a cabinet.
- Commands can be sent via ChirpStack (e.g., to update schedules or trigger relays).
- Cannot have concurrent schedules
- Backend and frontend development is pending.

## Contributors

This project would not be possible without the help of these amazing human beings:  

- Francesco de Carlo
- Giacomo Fabbri
- Silvia Fincato
- Federica Andresini
- Simona Logreco
- Claudia Ciulla
- Antonio Torchiani
- Elena Iosca
- Cristina Fanelli
- Elena Maria Sibilani
- Fabio Iaquinta
- Matteo Falcone
- Federico Ambra
- Nicoletta Lorusso
- Luca Santarelli
- Michele Scarnera
- Francesco Bagnolini
- Piergiorgio Favia
- Giorgio De Lazzari Pinnelli
- Royal Sinatti
- Francesco Zingaro
- Behnaz Hedayatpour
- Ilenia Fasanella
- Jonni Bongallino
- Alessandro Balena
- Micol Salomone
- Mohamed Fadiga
- Anna Baldassarre  

## License

This project is licensed under the **GNU Affero General Public License v3.0**.  
This means you are free to use, modify, and distribute the code, provided that:
- Any changes you make are also distributed under the same license.
- If you run a modified version on a server or over a network (e.g., web dashboard, cloud API), you must make the source code available to users.

You can find the full text of the license in the [LICENSE](./LICENSE) file.
