#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----- LED 1 (usa tus pines e intervalos reales) -----
const int led1 = 10;
const unsigned long interval1 = 3000;
unsigned long nextMillis1 = 0;
int stateLed1 = LOW;

// ----- LED 2 (el del pin 11, 5 segundos) -----
const int led3 = 11;
const unsigned long interval3 = 5000;
unsigned long nextMillis3 = 0;
int stateLed3 = LOW;

// ----- Mensaje cada 10 s -----
const unsigned long intervalMsg = 10000;
unsigned long nextMillisMsg = 0;

// ----- Sensor cada 5 s -----
const unsigned long intervalDHT = 5000;
unsigned long nextMillisDHT = 0;

void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led3, OUTPUT);
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  unsigned long ahora = millis();

  // Tarea 1: LED 1
  if (ahora >= nextMillis1) {
    stateLed1 = (stateLed1 == LOW) ? HIGH : LOW;
    digitalWrite(led1, stateLed1);
    nextMillis1 = ahora + interval1;
  }

  // Tarea 2: LED 2
  if (ahora >= nextMillis3) {
    stateLed3 = (stateLed3 == LOW) ? HIGH : LOW;
    digitalWrite(led3, stateLed3);
    nextMillis3 = ahora + interval3;
  }

  // Tarea 3: mensaje cada 10 s
  if (ahora >= nextMillisMsg) {
    Serial.println("Aún sigo aquí");
    nextMillisMsg = ahora + intervalMsg;
  }

  // Tarea 4: sensor cada 5 s
  if (ahora >= nextMillisDHT) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Error al leer el sensor DHT11");
    } else {
      Serial.print("Humedad: ");
      Serial.print(h);
      Serial.print("%\t");
      Serial.print("Temperatura: ");
      Serial.print(t);
      Serial.println("°C");
    }
    nextMillisDHT = ahora + intervalDHT;
  }
}
