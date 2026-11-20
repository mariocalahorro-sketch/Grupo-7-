/* TEST UNITARIO - SOLO EL SERVO
   Prueba el servo sin nada mas: sin sensores, sin comunicacion y sin Python.
   Sirve para saber si el fallo esta en el servo y sus cables, o en el programa del satelite.

   Lo que tiene que pasar: el servo va a 0, 90 y 180 grados, un segundo en cada sitio,
   y lo repite sin parar. En el Monitor Serie (9600) se escribe a donde va.

   Cables del servo:  marron -> GND     rojo -> 5V     naranja -> pin 9 */
#include <Servo.h>

const int servoPin = 9;
Servo servo;

//setup: prepara el Monitor Serie y conecta el servo al pin 9
//parametros: ninguno.  resultado: ninguno
void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
  Serial.println("Test del servo: tiene que ir a 0, 90 y 180 grados");
}

//loop: manda el servo a 0, 90 y 180 grados, esperando un segundo en cada posicion
//parametros: ninguno.  resultado: ninguno
void loop() {
  Serial.println("Voy a 0 grados");
  servo.write(0);
  delay(1000);

  Serial.println("Voy a 90 grados");
  servo.write(90);
  delay(1000);

  Serial.println("Voy a 180 grados");
  servo.write(180);
  delay(1000);
}
