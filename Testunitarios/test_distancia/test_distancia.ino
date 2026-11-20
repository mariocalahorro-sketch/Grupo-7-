/* TEST UNITARIO - DOS SENSORES DE DISTANCIA (HC-SR04) + SERVO
   Mueve el servo a 0, 45, 90, 135 y 180 grados y en cada posicion mide con los
   dos sensores. El sensor 2 esta de espaldas: su angulo es el del servo + 180.
   Muestra el resultado por el Monitor Serie (9600).
   Pines (los mismos que en el satelite):
     sensor 1: Trig 5, Echo 6     sensor 2: Trig 3, Echo 4     servo: 9 */
#include <Servo.h>

const int trigPin1 = 5;
const int echoPin1 = 6;
const int trigPin2 = 3;
const int echoPin2 = 4;
const int servoPin = 9;
Servo servo;

int angulo = 0;            //posicion actual del servo
long ultimo = 0;

//mide la distancia con un sensor. Devuelve los cm, o -1 si no llega eco
float medirDistancia(int trig, int echo){
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duracion = pulseIn(echo, HIGH, 40000);
  if (duracion == 0){
    return -1;
  }
  return duracion * 0.0343 / 2;
}

void setup() {
  Serial.begin(9600);
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  servo.attach(servoPin);
  servo.write(angulo);
  Serial.println("Test de los dos sensores de distancia");
}

void loop() {
  if (millis() - ultimo >= 1000){      //1 s para que el servo llegue a su sitio
    ultimo = millis();

    float d1 = medirDistancia(trigPin1, echoPin1);
    Serial.print("Sensor 1 a ");
    Serial.print(angulo);
    Serial.print(" grados -> ");
    if (d1 < 0){
      Serial.println("ERROR: sin eco");
    }else{
      Serial.print(d1);
      Serial.println(" cm");
    }

    float d2 = medirDistancia(trigPin2, echoPin2);
    Serial.print("Sensor 2 a ");
    Serial.print(angulo + 180);
    Serial.print(" grados -> ");
    if (d2 < 0){
      Serial.println("ERROR: sin eco");
    }else{
      Serial.print(d2);
      Serial.println(" cm");
    }

    //siguiente posicion: 0, 45, 90, 135, 180 y vuelta a 0
    angulo = angulo + 45;
    if (angulo > 180){
      angulo = 0;
    }
    servo.write(angulo);
  }
}
