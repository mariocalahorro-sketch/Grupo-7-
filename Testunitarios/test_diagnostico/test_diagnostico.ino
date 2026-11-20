/* TEST UNITARIO DE DIAGNOSTICO - SENSORES DE DISTANCIA (HC-SR04) + SERVO
   Sirve para saber QUE PARTE falla cuando un sensor de distancia no funciona.
   A cada sensor le hace tres pruebas, una detras de otra:
     Prueba 1: esta conectado el cable Echo?
     Prueba 2: contesta el sensor cuando le mandamos el pulso por Trig?
     Prueba 3: recibe el eco y da una distancia?
   Y las repite dos veces: con el servo quieto y justo despues de mover el servo,
   para ver si el servo deja a los sensores sin corriente.

   Como usarlo:
     1. Pon un objeto (un libro, la mano) a unos 20 cm delante de CADA sensor. No lo pegues al sensor.
     2. Carga este programa en el Arduino satelite y abre el Monitor Serie a 9600.
     3. Lee lo que pone en cada sensor: dice que prueba falla y que cables revisar.

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

//comprobarSensor: hace las tres pruebas a un sensor y escribe el resultado en el Monitor Serie
//parametros: numero -> 1 o 2, solo para escribirlo;  trig y echo -> pines del sensor
//resultado: true si el sensor mide bien, false si alguna prueba falla
bool comprobarSensor(int numero, int trig, int echo){
  Serial.print("  Sensor ");
  Serial.print(numero);
  Serial.print(": ");

  //PRUEBA 1: esta conectado el cable Echo?
  //Activamos un momento la resistencia interna del pin (INPUT_PULLUP), que lo sube a 5 V
  //si nadie lo sujeta. Un sensor conectado mantiene Echo a 0 V mientras no mide.
  //Por eso, si leemos HIGH, es que el cable no llega al sensor.
  pinMode(echo, INPUT_PULLUP);
  delay(5);
  bool echoSuelto = (digitalRead(echo) == HIGH);
  pinMode(echo, INPUT);
  if (echoSuelto){
    Serial.print("FALLA LA PRUEBA 1. El cable Echo (pin ");
    Serial.print(echo);
    Serial.println(") esta suelto, o Trig y Echo estan intercambiados.");
    return false;
  }

  //PRUEBA 2: contesta el sensor?
  //Mandamos el pulso de 10 microsegundos por Trig. Un sensor que funciona
  //pone Echo en HIGH enseguida. Esperamos como mucho 20 ms.
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long inicioEspera = micros();
  while (digitalRead(echo) == LOW && micros() - inicioEspera < 20000){
    //no hacemos nada: solo esperamos
  }
  if (digitalRead(echo) == LOW){
    Serial.print("FALLA LA PRUEBA 2. El sensor no contesta. Revisa: VCC al carril + (5V), GND al carril -, y el cable Trig (pin ");
    Serial.print(trig);
    Serial.println("). Si todo esta bien, el sensor esta roto.");
    return false;
  }

  //PRUEBA 3: recibe el eco?
  //Medimos cuanto tiempo se queda Echo en HIGH. Con un objeto a 20 cm dura
  //poco mas de 1 ms. Si dura 30 ms o mas, el sensor no ha oido ningun eco.
  unsigned long inicioEco = micros();
  while (digitalRead(echo) == HIGH && micros() - inicioEco < 300000){
    //esperamos a que acabe el eco (como mucho 300 ms)
  }
  unsigned long duracion = micros() - inicioEco;
  if (duracion >= 30000){
    Serial.println("FALLA LA PRUEBA 3. El sensor contesta pero no oye el eco. Pon un objeto a 20 cm (ni pegado ni lejos). Si ya lo hay, el sensor esta roto.");
    return false;
  }

  float distancia = duracion * 0.0343 / 2;   //velocidad del sonido (cm/us), ida y vuelta
  Serial.print("BIEN. Mide ");
  Serial.print(distancia);
  Serial.println(" cm");
  return true;
}

//setup: prepara el Monitor Serie y los pines. El servo todavia no se conecta,
//       para que la primera ronda de pruebas sea con el servo parado
//parametros: ninguno.  resultado: ninguno
void setup() {
  Serial.begin(9600);
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  Serial.println("TEST DE DIAGNOSTICO. Pon un objeto a 20 cm delante de cada sensor.");
}

//loop: cada 4 segundos prueba los dos sensores con el servo quieto, mueve el servo,
//      los vuelve a probar y escribe un resumen
//parametros: ninguno.  resultado: ninguno
void loop() {
  Serial.println();
  Serial.println("--- Con el servo quieto ---");
  bool quieto1 = comprobarSensor(1, trigPin1, echoPin1);
  bool quieto2 = comprobarSensor(2, trigPin2, echoPin2);

  //movemos el servo a la siguiente posicion: 0, 90, 180 y vuelta a 0
  angulo = angulo + 90;
  if (angulo > 180){
    angulo = 0;
  }
  servo.attach(servoPin);
  servo.write(angulo);
  delay(50);     //el servo acaba de arrancar: es cuando mas corriente gasta

  Serial.print("--- Mientras el servo va a ");
  Serial.print(angulo);
  Serial.println(" grados (mira si se mueve) ---");
  bool moviendo1 = comprobarSensor(1, trigPin1, echoPin1);
  bool moviendo2 = comprobarSensor(2, trigPin2, echoPin2);

  //resumen
  Serial.print("RESUMEN: ");
  if (quieto1 && quieto2 && moviendo1 && moviendo2){
    Serial.println("los dos sensores funcionan. Si el servo se ha movido, todo esta bien.");
  }else if (quieto1 && quieto2){
    Serial.println("los sensores solo fallan cuando se mueve el servo: el servo los deja sin corriente. Alimenta el servo con una fuente de 5 V aparte y une su GND con el del Arduino.");
  }else{
    Serial.println("hay un fallo de cableado o de sensor. Mira arriba que prueba falla en cada uno.");
  }

  delay(4000);
}
