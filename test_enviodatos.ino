// Definiciones necesarias
#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
void setup() {
   Serial.begin(9600);
   // Inicialización del sensor
   dht.begin();
}
void loop() {
   delay(2000);
   float h = dht.readHumidity();
   float t = dht.readTemperature();
   if (isnan(h) || isnan(t))
      mySerial.println("Error al leer el sensor DHT11");
   else {
      mySerial.print("Humedad: ");
      mySerial.print(h);
      mySerial.print("%\t");
      mySerial.print("Temperatura: ");
      mySerial.print(t);
      mySerial.println("°C");
   }
}
