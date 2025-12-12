#include "HX711.h"    				// Bibliothek der Wägezelle
#include <Stepper.h>				// Stepper Bibliothek
#include <LiquidCrystal_I2C.h>	// Bibliothek für das LCD Display

// Anschlüsse der Wägezelle
#define DOUT  3
// Anschlüsse der Wägezelle
#define CLK   4


//Messgerät
HX711 kraftMesser;

// großes LC Display
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address 0x27, 20 column and 4 rows

// 0,8mm pro Umdrehung bei M5-Gewinde
const float gewindeSteigung = 0.8;        

// bei Getriebe von 20 Zaehnen auf 50 Zaehne, also pro Umdrehung des Schrittmotors nur 1/2,5 Umdrehungen der Gewindestange
const float getriebeUebersetzung = 2.5;

// Für Eichung auf null
double yAchsenAbschnitt = 0;
// Kalibrierung für Ausgabe in Einheit Newton
double kalibrierSteigung = 1;

// Pins für HMI (Human Maschine Interface)
const int TASTER = 5;
const int LED_1 = 6;
const int LED_2 = 7;

// Grenzwerte für die Erkennung, wenn der Draht reißt
const int minGrenze = 1;
const int maxGrenze = 2;

// Wert für Blink animation bei Messung
bool blinkMode = false;

// Speichern von Maximalspannung und Längenänderung zum Zeitpunkt der maximalen Spannung
long maxSigma = 0;
long maxLength = 0;

// Dauer der Pause zum Entprellen des Tasters
const int noiseDelay = 100;

// Schritte pro Umdrehung beim Schrittmotor
const int stepsPerRevolution = 2048;

// Aufeinanderfolgende Anschlüsse des Schrittmotors. Es werden die Pins 17 (A3) - 14 (A0) verwendet,
// damit die Anschlüsse der Stepperplatine auf dem Arduinoboard alle 6 nebeneinander liegen.
Stepper schrittmotor(stepsPerRevolution, 9, 11 , 8, 10);  

// Längenaenderung des Drahtes
float deltaL = 0; 


// 47.4 cm Ausgangslänge


void setup() {
  Serial.begin(9600);

  // Konfiguration Schrittmotor
  schrittmotor.setSpeed(15);

  // Configuration HMI
  pinMode(TASTER, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  lcd.init(); //initialize the lcd
  lcd.backlight(); //open the backlight 

  // Anschluss Wägezelle
  kraftMesser.begin(DOUT, CLK);
  pinMode(2,OUTPUT); // GND
  digitalWrite(2,LOW); 
  
  pinMode(5,OUTPUT);	// VCC
  digitalWrite(5,HIGH);	// GND

  calibrate();
  messung();
}


void loop() {
  schrittmotor.step(stepsPerRevolution/4); // Drehdauer = 1s
  // Animate LEDs
  blinkNext();
  Serial.print(deltaL);   //nach jeder Umdrehung wird die Längenänderung abgelesen
  
  deltaL += gewindeSteigung / getriebeUebersetzung / 20;    //Längenänderung entspricht Steigung des Gewindes pro Übersetzungsverhaeltnis
  Serial.print("; ");
  
  // mechanische Spannung in N
  float val = (kraftMesser.get_units() - yAchsenAbschnitt) / kalibrierSteigung;
  // Betrag bilden
  if(val < 0) val *= -1;
  // Speichern der Maximalwerte
  if(val > maxSigma){
    maxSigma = val;
    maxLength = deltaL;
  }
  // Werte ausdrucken
  Serial.print(val,3);
  lcd.setCursor(8, 2);
  lcd.print(deltaL);
  lcd.setCursor(8, 3);
  lcd.print(val);

  // prüfe, ob der draht gerissen ist
  if((maxSigma - val) > maxGrenze){
    if(val > minGrenze){
      // HMI
      lcd.clear();
      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("Messung fertig!");
      lcd.setCursor(0, 1);
      lcd.print("max sigma:");
      lcd.setCursor(12, 1);
      lcd.print(maxSigma);
      lcd.setCursor(0, 2);
      lcd.print("max length:");
      lcd.setCursor(12, 2);
      lcd.print(maxLength);
      lcd.setCursor(3, 3);
      lcd.print("Neue Messung >");
      
      // Warten bis Taster gedrückt wird
      while(digitalRead(TASTER) == HIGH){delay(10);}
      delay(noiseDelay);
      // Warten bis Taster nicht mehr gedrückt wird
      while(digitalRead(TASTER) == LOW){delay(10);}
      delay(noiseDelay);

      // zurücksetzen der Werte für neue Messung
      maxSigma = 0;
      maxLength = 0;
      lcd.clear();
    }
  }
  
  Serial.println(); // Zeilenumbruch
}

void calibrate(){
  // Funktion zum Kalibrieren

  // Modus 1
  // HMI
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, LOW);
  lcd.setCursor(0, 0);
  lcd.print("[=========         ]");
  lcd.setCursor(0, 1);
  lcd.print("Kalibrierung");
  lcd.setCursor(17, 1);
  lcd.print("1/2");
  lcd.setCursor(0, 2);
  lcd.print("Last: ohne Last");
  lcd.setCursor(5, 3);
  lcd.print("Weiter >");
  // Warten bis Taster gedrückt wird
  while(digitalRead(TASTER) == HIGH){delay(10);}
  delay(noiseDelay);
  // Warten bis Taster nicht mehr gedrückt wird
  while(digitalRead(TASTER) == LOW){delay(10);}
  // Berechnung des yAchsenabschnitt
  yAchsenAbschnitt = kraftMesser.get_units();
  Serial.println(yAchsenAbschnitt);
  delay(noiseDelay);

  // Modus 2
  // HMI
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("[==================]");
  lcd.setCursor(17, 1);
  lcd.print("2");
  lcd.setCursor(0, 2);
  lcd.print("Last: 10N           ");
  lcd.setCursor(5, 3);
  lcd.print("Weiter >");
  // Warten bis Taster gedrückt wird
  while(digitalRead(TASTER) == HIGH){delay(10);}
  delay(noiseDelay);
  // Warten bis Taster nicht mehr gedrückt wird
  while(digitalRead(TASTER) == LOW){delay(10);}
  // Kalibrierung der Steigung
  kalibrierSteigung = (kraftMesser.get_units() - yAchsenAbschnitt) / -10;
  Serial.println(kalibrierSteigung);
  delay(noiseDelay);

  // Ende der Kalibrierung
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kalibrierung fertig!");
  lcd.setCursor(0, 1);
  lcd.print("y:");
  lcd.setCursor(3, 1);
  lcd.print(round(yAchsenAbschnitt));
  lcd.setCursor(0, 2);
  lcd.print("f:");
  lcd.setCursor(3, 2);
  lcd.print(kalibrierSteigung);
  lcd.setCursor(5, 3);
  lcd.print("Messung >");
  Serial.println(); // Zeilenumbruch zum trennen von Kalibrierung und csv code
  // Warten bis Taster gedrückt wird
  while(digitalRead(TASTER) == HIGH){delay(10);}
  delay(noiseDelay);
  // Warten bis Taster nicht mehr gedrückt wird
  while(digitalRead(TASTER) == LOW){delay(10);}
  delay(noiseDelay);
  // Clear HMI
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  lcd.clear();
  
}

void messung(){
  lcd.setCursor(0, 0);
  lcd.print("Messung laeuft...");
  lcd.setCursor(0, 2);
  lcd.print("sigma: ");
  lcd.setCursor(0, 3);
  lcd.print("length: ");
}

void blinkNext(){
  if(blinkMode){
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, LOW);
  } else{
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, HIGH);
  }
  blinkMode = !blinkMode;
}
