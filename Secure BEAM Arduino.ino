#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== PIN CONFIG =====
const int LASER_PIN   = 9;
const int LDR_PIN     = A0;
const int LED_PIN     = 13;
const int BUZZER_PIN  = 3;

// ===== MORSE SETTINGS =====
const int DOT = 400;           // ms for one dot; try 300-500 if needed
const int DASH = DOT * 3;
const int THRESH = 550;        // light <550 ⇒ laser ON
const int WORD_GAP = DOT * 7;

// ===== STATE =====
bool isReceiving = false;

// ===== LCD helper =====
void lcdShow(String a, String b="") {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(a);
  lcd.setCursor(0,1); lcd.print(b);
}

// ===== Morse lookup =====
char decodeMorse(String s) {
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const char* codes[] = {
    ".-","-...","-.-.","-..",".","..-.","--.","....","..",
    ".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",
    "..-","...-",".--","-..-","-.--","--..",
    "-----",".----","..---","...--","....-",".....","-....","--...","---..","----."
  };
  for (int i=0;i<36;i++) if (s==codes[i]) return chars[i];
  return '?';
}

String encodeMorse(char c) {
  c = toupper(c);
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const char* codes[] = {
    ".-","-...","-.-.","-..",".","..-.","--.","....","..",
    ".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",
    "..-","...-",".--","-..-","-.--","--..",
    "-----",".----","..---","...--","....-",".....","-....","--...","---..","----."
  };
  for (int i=0;i<36;i++) if (c==chars[i]) return codes[i];
  return "";
}

// ===== Send a dot/dash =====
void sendPulse(int duration) {
  digitalWrite(LASER_PIN,HIGH);
  digitalWrite(LED_PIN,HIGH);
  tone(BUZZER_PIN,1000);
  delay(duration);
  digitalWrite(LASER_PIN,LOW);
  digitalWrite(LED_PIN,LOW);
  noTone(BUZZER_PIN);
  delay(DOT); // intra-element gap
}

// ===== Transmit message =====
void transmitMessage(String msg) {
  lcdShow("TX Mode","Sending...");
  Serial.println("\n[TX] Sending Morse...");
  msg.toUpperCase();

  for (int i=0;i<msg.length();i++) {
    char c = msg[i];
    if (c==' ') { delay(WORD_GAP); continue; }

    String code = encodeMorse(c);
    for (int j=0;j<code.length();j++) {
      if (code[j]=='.') sendPulse(DOT);
      else if (code[j]=='-') sendPulse(DASH);
    }
    delay(DOT*3); // gap between letters
  }

  lcdShow("TX Done");
  Serial.println("[TX] Done.\n");
}

// ===== Receive Morse =====
void receiveLoop() {
  lcdShow("RX Mode","Listening...");
  Serial.println("[RX] Listening...");
  delay(1000);

  String morseBuffer="";
  unsigned long lastChange=millis();
  bool prevLight=false;
  unsigned long now,duration;

  while(isReceiving) {
    int val=analogRead(LDR_PIN);
    bool isOn=(val<THRESH);
    now=millis();
    duration=now-lastChange;

    if(isOn!=prevLight){ // transition
      lastChange=now;

      if(isOn){ // just turned ON ⇒ OFF gap ended
        if(duration>WORD_GAP){ Serial.print(' '); lcdShow("RX:"," "); }
        else if(duration>DOT*2){
          char decoded=decodeMorse(morseBuffer);
          Serial.print(decoded);
          lcdShow("RX:",String(decoded));
          morseBuffer="";
        }
      } else { // just turned OFF ⇒ ON ended
        if(duration<DOT*2) morseBuffer+=".";
        else morseBuffer+="-";
      }
      prevLight=isOn;
    }

    if(Serial.available()){ Serial.read(); isReceiving=false; break; }
  }
  lcdShow("RX Stopped");
  Serial.println("\n[RX] Stopped.\n");
}

// ===== Setup =====
void setup(){
  pinMode(LASER_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);
  digitalWrite(LASER_PIN,LOW);
  digitalWrite(LED_PIN,LOW);
  noTone(BUZZER_PIN);

  lcd.init(); lcd.backlight();
  lcdShow("SeCure BEAM","Ready");
  Serial.begin(9600);
  Serial.println("==== LASER MORSE LINK READY ====");
  Serial.println("Commands: TX <msg> | RX");
}

// ===== Main loop =====
void loop(){
  if(Serial.available()){
    String cmd=Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd.startsWith("TX ")){
      transmitMessage(cmd.substring(3));
    } else if(cmd.equalsIgnoreCase("RX")){
      isReceiving=true;
      receiveLoop();
    } else {
      Serial.println("Use: TX <msg> or RX");
    }
  }
}
