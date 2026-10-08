# 📡 DTMF Communicatie Systeem – PSoC6

Dit project implementeert een volledig **DTMF (Dual-Tone Multi-Frequency) communicatie systeem** op de PSoC6 met behulp van een audio shield.

Het systeem bestaat uit een **zender** en een **ontvanger** die met elkaar communiceren via geluidsgolven.

---

## 🧠 Overzicht

### 🔵 Zender

* Genereert DTMF-tonen (combinatie van 2 sinussen)
* Stuurt deze via **I2S audio output**
* Activatie via knop
* Optioneel: LED of TFT feedback

### 🟢 Ontvanger

* Leest audio via **PDM microfoon**
* Detecteert DTMF met **Goertzel-algoritme**
* Toont ontvangen toets op **TFT display**
* Simuleert een antwoord (TX)

---

## 🎥 Video Demonstratie

In de onderstaande video zie je het systeem in actie. De demonstratie laat de volledige flow zien: van het versturen van het signaal tot de succesvolle verwerking op de ontvanger.

[![Bekijk de video](https://img.youtube.com/vi/ylxAOBVmmIw/0.jpg)](https://youtube.com/shorts/ylxAOBVmmIw)
### Wat is er te zien in de video?
* **Transmissie:** Je ziet hoe een toets wordt ingedrukt op het board, waarna de PSoC6 het bijbehorende DTMF-signaal genereert.
* **Ontvangst:** De ontvanger vangt het geluid op via de PDM-microfoon (zichtbaar naast de hoofdtelefoon).
* **Verwerking:** Het scherm verspringt van "WACHTEN" naar de gedetecteerde waarde (bijv. `RX: 4`, `RX: 3` of `RX: #`).
* **Feedback:** De statusbalk onderaan het scherm geeft aan: `DTMF correct gedetecteerd`.

---

## 🔁 Systeem Flow

### Zender:

1. Knop ingedrukt
2. Genereer 2 sinussen (DTMF)
3. Stuur audio via I2S

### Ontvanger:

1. Luistert naar inkomend signaal
2. Filtert frequenties (Goertzel)
3. Detecteert toets (bijv. '5')
4. Toont op TFT:

   * `RX: 5`
   * `TX: 6` (simulatie antwoord)

---

## 🎵 DTMF Frequenties

| Toets | Frequenties (Hz) |
| ----- | ---------------- |
| 1     | 697 + 1209       |
| 2     | 697 + 1336       |
| 3     | 697 + 1477       |
| 4     | 770 + 1209       |
| 5     | 770 + 1336       |
| 6     | 770 + 1477       |
| 7     | 852 + 1209       |
| 8     | 852 + 1336       |
| 9     | 852 + 1477       |
| 0     | 941 + 1336       |
| *     | 941 + 1209       |
| #     | 941 + 1477       |

---

## 🧩 Gebruikte Technologieën

* PSoC6 (CY8CKIT-062)
* ModusToolbox
* I2S (audio output)
* PDM/PCM (microfoon input)
* Goertzel algoritme (frequentie detectie)
* emWin GUI (TFT display)
* Programmeertaal gelijkaardig aan C

---

## ⚠️ Belangrijke Opmerking

Door hardwarebeperkingen kunnen **PDM (microfoon)** en **I2S (audio output)** niet stabiel tegelijk gebruikt worden op hetzelfde board.

👉 Daarom wordt het systeem **sequentieel** geïmplementeerd:

* Eerst detectie (RX)
* Daarna simulatie van antwoord (TX via TFT)

---

## ⚠️ Technische Problemen & Beperkingen

Tijdens de ontwikkeling zijn meerdere communicatieproblemen vastgesteld tussen verschillende peripherals.

### 🔌 UART en I2C Conflicten

Bij gelijktijdig gebruik van **UART** en **I2C**:

* Kan er **geen data meer ontvangen worden via UART**
* Wordt I2C **instabiel of stopt volledig**

**Mogelijke oorzaken:**

* SCB resource conflicten binnen de PSoC6
* Interrupt conflicten
* Foutieve pin- of clockconfiguratie

**Gevolg:**
UART debugging is niet betrouwbaar wanneer I2C actief is.

**Oplossing / workaround:**

* UART en I2C niet gelijktijdig gebruiken
* Gebruik **emWin (TFT display)** voor debugging

---

### 🎤 Microfoon (PDM) en I2C Probleem

Na gebruik van de **PDM microfoon**:

* Geeft de **I2C bus geen response meer (no ACK)**
* I2C devices reageren niet meer

**Mogelijke oorzaken:**

* Clock/timing interferentie
* Hardware resource conflict
* Driver blokkering

**Gevolg:**
I2C communicatie valt volledig weg na microfoongebruik.

**Oplossing / workaround:**

* I2C opnieuw initialiseren na gebruik
* Microfoon en I2C niet gelijktijdig gebruiken
* Sequentiële aanpak:

  1. Microfoon gebruiken (RX)
  2. Microfoon uitschakelen
  3. I2C opnieuw initialiseren

---

### 🖥️ Debugging via emWin

Door bovenstaande problemen:

* Wordt **UART niet gebruikt voor debugging**
* Wordt **emWin GUI (TFT)** gebruikt voor feedback

**Voordeel:**

* Stabiel
* Onafhankelijk van communicatieconflicten

---

## 📁 Project Structuur

DTMF_Zender/
├── main.c
├── dtmf.c
└── dtmf.h

DTMF_Ontvanger/
├── main.c
├── goertzel.c
└── goertzel.h

mtb_shared/
README.md

---

## 🚀 Gebruik

### Zender

* Druk op knop
* DTMF toon wordt afgespeeld

### Ontvanger

* Houd microfoon bij zender
* TFT toont ontvangen toets

---

## 👨‍💻 Auteur

* Jonas Vanmarsenille
* Johnny Lin
* PXL Digital

---

## 🎯 Doel van het Project

Dit project demonstreert:

* Digitale signaalverwerking (DSP)
* Frequentie-analyse
* Embedded audio communicatie (DTMF)
* Hardware/software integratie

---

## 💡 Mogelijke uitbreidingen

* Real-time bidirectionele communicatie
* FFT implementatie i.p.v. Goertzel
* Geluid filtering (noise reduction)
* GUI uitbreiden

---
