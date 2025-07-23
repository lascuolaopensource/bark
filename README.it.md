# Bark

> ⚠️ **Questa repository è in fase di sviluppo.** Alcuni dettagli tecnici relativi all'infrastruttura live potrebbero essere intenzionalmente omessi od offuscati per motivi di sicurezza.

**Bark** è un sistema per monitorare e controllare l'irrigazione dei parchi pubblici della città di Bari ed è stato ideato e sviluppato durante il progetto **[Bari Città Aperta](https://baricittaperta.xyz/).

Questa repository contiene il codice del prototipo installato presso il **Parco Mauggeri**, che ha fatto da parco pilota.

Attualmente il progetto include il firmware per il controller di irrigazione e la logica di comunicazione LoRaWAN. Il backend e il frontend per la gestione centralizzata e remota dei parchi non sono ancora stati sviluppati. Al momento è possibile inviare comandi tramite l’interfaccia di **ChirpStack**.

## Panoramica del Sistema

Bark si basa su un’infrastruttura leggera e a basso consumo che utilizza **LoRaWAN** e **ChirpStack**, con un gateway locale che funge da ponte tra i dispositivi e il network server.

### Hardware

L’unità principale è una **Heltec WiFi LoRa 32 (V3)**, collegata a una scheda a 8 relè che controlla elettrovalvole a 24V AC già presenti nel parco. Ogni valvola può essere attivata individualmente secondo programmazioni memorizzate nella memoria non volatile del dispositivo.

Le elettrovalvole sono alimentate tramite un **trasformatore 220V–24V AC**, mentre una **alimentazione da parete 5V** alimenta la Heltec e la logica dei relè.

Il microcontrollore è dotato di un piccolo display OLED per il debug in loco, e può sincronizzare l’orologio interno via NTP se una rete Wi-Fi è disponibile. In alternativa, l’orario può essere impostato manualmente.

Le programmazioni sono definite in termini di giorni della settimana, orario di inizio, durata e valvole da attivare. È possibile aggiornarle da remoto via LoRa.

### Gateway e Rete

La comunicazione tra i dispositivi sul campo e il backend avviene tramite **LoRaWAN**. Ogni scheda opera come dispositivo **Classe C**, rimanendo sempre in ascolto per ricevere istruzioni.

È stato installato un **gateway LoRaWAN** (attualmente un **modello MikroTik**, ma qualsiasi marca compatibile va bene) entro il raggio d’azione. Il gateway inoltra i pacchetti al server di rete **ChirpStack**, che gestisce l’attivazione dei dispositivi (OTAA), l’instradamento dei messaggi e la comunicazione bidirezionale.

### ChirpStack

**ChirpStack** è il server di rete che gestisce le sessioni dei dispositivi, i downlink e le integrazioni con applicazioni. In questa fase, funge anche da interfaccia per inviare comandi e aggiornare le configurazioni dei dispositivi.

## Distinta dei Materiali (BOM)

| Componente               | Descrizione                                                                       |
|--------------------------|-----------------------------------------------------------------------------------|
| Heltec WiFi LoRa 32 (V3) | Microcontrollore con LoRa e display OLED                                          |
| Scheda 8 relè            | Controlla le elettrovalvole di irrigazione                                        |
| Elettrovalvole 24V AC    | Valvole elettromeccaniche per l’irrigazione                                       |
| Trasformatore 220V–24V   | Alimenta le valvole                                                               |
| Alimentatore 5V          | Alimenta la Heltec e la scheda relè                                               |
| Gateway LoRaWAN MikroTik | Inoltra i pacchetti al server ChirpStack                                          |
| Contenitore              | Box per componenti elettronici (Impermeabile se direttamente esposto all'esterno) |
| Cavi e connettori vari   | Cablaggio elettrico                                                               |
| Server ChirpStack        | Può essere in cloud o su server locale                                            |

## Istruzioni di Installazione

1. **Installazione Gateway**\
   Se non c’è un gateway LoRaWAN nelle vicinanze, installane uno e configurarlo per comunicare con ChirpStack. Registralo nel pannello di ChirpStack.

2. **Configurazione ChirpStack**\
   Accedi a ChirpStack e crea tenant, applicazione e profilo dispositivo se non esistenti. Registra il dispositivo usando il suo DevEUI.

3. **Configurazione Firmware**\
   Apri lo sketch Arduino e modifica:

    - Pin collegati ai relè (solo GPIO compatibili)
    - Chiavi OTAA (DevEUI, AppEUI, AppKey)
    - Credenziali Wi-Fi (se `useWiFiTime = true`) o imposta l’orario manualmente

4. **Cablaggio e Alimentazione**\
   Collega i relè alla Heltec e alle valvole. Alimenta i relè tramite il trasformatore 24V e la Heltec con l’alimentatore 5V. Verifica i collegamenti prima dell’accensione.

5. **Caricamento Firmware**\
   Collega la scheda via USB e carica lo sketch. Non alimentare la scheda via 5V durante questa fase.

6. **Connessione Wi-Fi**\
   Se stai usando la sincronizzazione oraria NTP, assicurati che il Wi-Fi sia disponibile. È richiesto solo all’avvio.

7. **Accensione e Test**\
   Accendi la scheda e il trasformatore. Controlla i messaggi di avvio sul display OLED. Il sistema sincronizzerà l’orario, caricherà le programmazioni e tenterà la connessione alla rete LoRaWAN.

8. **Verifica Connessione**\
   Su ChirpStack, verifica che il dispositivo sia attivo e che stia inviando messaggi. Puoi inviare downlink di test (es. attivazione valvola, aggiornamento programmazione).

## Stato Attuale

- Il firmware è installato e gestisce alcune valvole del Parco Mauggeri
- La comunicazione via LoRaWAN è debole ma stabile. È probabile che dipenda dalla mancanza di antenna esterna sul gateway o dalla posizione della scheda Heltec (all’interno di un cabinato).
- I comandi possono essere inviati via ChirpStack
- Le programmazioni non possono sovrapporsi
- Lo sviluppo del backend e del frontend è ancora in corso

## Autori

Tutto questo non sarebbe stato possibile senza il supporto di questi straordinari esseri umani:

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

## Licenza

Questo progetto è rilasciato sotto licenza **GNU Affero General Public License v3.0**.

Puoi usare, modificare e ridistribuire il codice liberamente, a condizione che:
- Ogni modifica venga rilasciata sotto la stessa licenza.
- Se il software viene eseguito su un server o reso accessibile in rete (es. dashboard web), il codice sorgente deve essere reso disponibile agli utenti.

Il testo completo della licenza si trova nel file `LICENSE`.
