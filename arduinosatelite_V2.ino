/* [V2] PROTOCOLO DE APLICACION
   Todos los mensajes son "codigo" o "codigo:datos" y acaban en salto de linea.
   El codigo es un numero que dice de que tipo es el mensaje.

   Tierra -> Satelite (ordenes)
     1            Iniciar
     2 / 3        Pausar / Reanudar temperatura y humedad (T/H)
     4 / 5        Pausar / Reanudar distancia
     6:ms         Nuevo periodo de T/H, en milisegundos        (ejemplo: 6:5000)
     7:ms         Nuevo periodo de distancia, en milisegundos  (ejemplo: 7:500)
     8            Modo barrido (el servo va y viene sin parar)
     9:angulo     Apuntar a un angulo fijo, de 0 a 180         (ejemplo: 9:90)
     10:1 / 10:0  La media se calcula en el satelite / en tierra

   Satelite -> Tierra
     1:T:H        Temperatura y humedad                        (ejemplo: 1:23.50:45.00)
     2:s:angulo:d Distancia d (cm) del sensor s (1 o 2) mirando a ese angulo
                  (ejemplo: 2:1:45:23.50). El sensor 2 envia su angulo + 180
     3:media      Media de las 10 ultimas temperaturas (solo si se calcula aqui)
     4            Fallo del sensor de T/H
     5:s          Fallo del sensor de distancia s (1 o 2)
     7            Latido (el satelite sigue vivo)

   Arduino de tierra -> Python
     6            Fallo de comunicacion con el satelite
*/
#include <DHT.h>
#include <SoftwareSerial.h>
#include <Servo.h>   //[V2] libreria del servomotor
SoftwareSerial mySerial(10, 11); //RX, TX
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const int led = 7; //LED en el pin 7 (verde)
bool stateled = LOW;
bool enviarDatos = false;
long ultimoEnvio = 0;
long intervalo = 3000;   //Tiempo entre dato y dato ([V2] ya no es const: se puede cambiar desde tierra)
long inicioLed = 0;
bool iniciado = false;
const long intervalolatido = 2000;   //un latido cada 2 s (menos que el timeout de tierra)
long ultimolatido = 0;               //cuando se envio el ultimo latido

//variables para la alarma del sensor de humedad y temperatura
bool esperandoTimeout = false;  //se esta esperando el tiempo para que se arregle el error? false -> no se esta esperando
long nextTimeoutHT;  //tiempo que se le da para arreglar el error de recopilacion de datos

//[V2] variables para los dos sensores de distancia (montados de espaldas sobre el servo)
const int trigPin1 = 5;   //sensor 1
const int echoPin1 = 6;
const int trigPin2 = 3;   //sensor 2 (mira al lado contrario que el sensor 1)
const int echoPin2 = 4;
const int servoPin = 9;
Servo servo;
bool enviarDistancia = false;      //se envian datos de distancia?
long ultimaDistancia = 0;          //cuando se hizo la ultima medida
long intervaloDistancia = 200;     //tiempo entre medida y medida
int angulo = 0;                    //angulo del servo (0-180)
int paso = 5;                      //grados que avanza el servo en cada medida
bool barrido = true;               //true = el servo barre, false = apunta a un angulo fijo

//[V2] alarma de los sensores de distancia (igual que la del sensor T/H)
bool esperandoTimeoutD1 = false;
long nextTimeoutD1;
bool esperandoTimeoutD2 = false;
long nextTimeoutD2;

//[V2] cola circular con las ultimas 10 temperaturas, para calcular la media
float cola[10];
int posicion = 0;        //donde se guardara la siguiente temperatura
int cantidad = 0;        //cuantas temperaturas hay guardadas (maximo 10)
bool mediaEnSatelite = false;   //true = la media se calcula aqui, false = en tierra

//[V2] medirDistancia: mide la distancia con un sensor HC-SR04
//parametros: trig y echo -> pines a los que esta conectado el sensor
//resultado: la distancia en cm; 999 si no hay ningun objeto al alcance;
//           -1 si el sensor no contesta (desconectado o roto)
float medirDistancia(int trig, int echo){
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);      //pulso de 10 microsegundos: el sensor lanza el ultrasonido
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duracion = pulseIn(echo, HIGH, 40000);   //tiempo que tarda el eco (espera como mucho 40 ms)
  if (duracion == 0){
    //no se ha podido medir el eco. Hay dos motivos posibles:
    if (digitalRead(echo) == HIGH){
      return 999;                //el sensor ha contestado pero el eco no acaba: funciona, y no hay nada cerca
    }
    return -1;                   //el sensor no ha contestado: esta desconectado o roto
  }
  return duracion * 0.0343 / 2;  //velocidad del sonido (cm/us); se divide entre 2 porque va y vuelve
}

//[V2] moverServo: lleva el servo a un angulo y despues lo "suelta"
//parametros: grados -> angulo al que tiene que ir (0-180)
//            espera -> milisegundos que se le dan para llegar
//resultado: ninguno
//Por que se suelta (detach): mientras el Arduino envia datos por SoftwareSerial no puede
//atender al servo, y el servo da tirones. Si solo lo conectamos el rato que se mueve y
//luego lo soltamos, los envios de datos ya no le afectan y se queda quieto donde esta.
void moverServo(int grados, int espera){
  servo.attach(servoPin);
  servo.write(grados);
  delay(espera);
  servo.detach();
}

//setup: se ejecuta una vez al encender. Prepara las comunicaciones, los sensores y el servo
//parametros: ninguno.  resultado: ninguno
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  dht.begin();
  pinMode(led, OUTPUT);
  //[V2] pines de los sensores de distancia y servo
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  moverServo(angulo, 500);   //posicion inicial (0 grados)
}

//loop: se repite sin parar. Envia el latido, atiende las ordenes de tierra,
//      lee los sensores cuando toca y avisa si alguno falla
//parametros: ninguno.  resultado: ninguno
void loop() {
  //enviamos el latido
  if (millis() - ultimolatido >= intervalolatido){
    ultimolatido = millis();
    mySerial.println("7");   //[V2] el latido ahora es el codigo 7 (antes "Vivo")
  }

  //Recibe ordenes de tierra
  if (mySerial.available()){
    String orden = mySerial.readStringUntil('\n');   //[V2] un mensaje cada vez (antes readString, que espera 1 segundo)
    orden.trim();                                    //[V2] quita el \r

    //[V2] la orden es "codigo" o "codigo:valor". Se separan las dos partes
    int codigo = orden.toInt();              //toInt lee el numero hasta los ':' -> "9:45" da 9
    long valor = 0;
    int separador = orden.indexOf(':');      //posicion de los ':' (-1 si no hay)
    if (separador != -1){
      valor = orden.substring(separador + 1).toInt();   //lo que va detras de los ':' -> "9:45" da 45
    }

    if (codigo == 1 && !iniciado){           //[V2] Iniciar (antes "Iniciar")
      iniciado = true;
      enviarDatos = true;
      enviarDistancia = true;                //[V2]
      ultimoEnvio = millis() - intervalo;
    }else if (codigo == 2 && iniciado){      //[V2] Pausar T/H (antes "Pausar")
      enviarDatos = false;
    }else if (codigo == 3 && iniciado){      //[V2] Reanudar T/H (antes "Reanudar")
      enviarDatos = true;
      ultimoEnvio = millis() - intervalo;   //envia un dato enseguida
    }else if (codigo == 4 && iniciado){      //[V2] Pausar distancia
      enviarDistancia = false;
      esperandoTimeoutD1 = false;            //al pausar ya no se leen los sensores: se quitan sus alarmas
      esperandoTimeoutD2 = false;
    }else if (codigo == 5 && iniciado){      //[V2] Reanudar distancia
      enviarDistancia = true;
    }else if (codigo == 6 && valor >= 1000){ //[V2] periodo T/H (el DHT11 necesita 1 s como minimo)
      intervalo = valor;
    }else if (codigo == 7 && valor >= 150){  //[V2] periodo distancia (minimo 150 ms: son dos medidas)
      intervaloDistancia = valor;
    }else if (codigo == 8){                  //[V2] modo barrido
      barrido = true;
    }else if (codigo == 9 && valor >= 0 && valor <= 180){   //[V2] apuntar a un angulo fijo
      barrido = false;
      angulo = valor;
      moverServo(angulo, 500);               //medio segundo: puede tener que girar hasta 180 grados
    }else if (codigo == 10){                 //[V2] donde se calcula la media
      if (valor == 1){
        mediaEnSatelite = true;
      }else{
        mediaEnSatelite = false;
      }
    }
  }

  // Enviar datos cada "intervalo" si enviarDatos = True
  if (iniciado && millis() - ultimoEnvio >= intervalo){
    ultimoEnvio = millis();
    // se lee el sensor
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)){
      // Si la lectura no es correcta, se empieza a contar el tiempo de margen
      if (!esperandoTimeout){
        esperandoTimeout = true;
        nextTimeoutHT = millis() + 5000; //5 segundos para arreglar el error
      }
    }else{
      // Se envian los datos al arduino tierra
      esperandoTimeout = false;
      if (enviarDatos){
        mySerial.print("1:");    //[V2] antes "T:"
        mySerial.print(t);
        mySerial.print(":");     //[V2] antes ":H:"
        mySerial.println(h);

        //[V2] se guarda la temperatura en la cola circular
        cola[posicion] = t;
        posicion = posicion + 1;
        if (posicion == 10){
          posicion = 0;          //vuelve al principio y pisa la mas antigua: por eso es circular
        }
        if (cantidad < 10){
          cantidad = cantidad + 1;
        }
        //[V2] si la media se calcula en el satelite, se calcula y se envia
        if (mediaEnSatelite){
          float suma = 0;
          for (int i = 0; i < cantidad; i++){
            suma = suma + cola[i];
          }
          mySerial.print("3:");
          mySerial.println(suma / cantidad);
        }

        // Se enciende el led cuando se envia info a tierra
        digitalWrite(led, HIGH);
        stateled = HIGH;
        inicioLed = millis();
      }
    }
  }

  //apagamos el led de envio de datos despues de 200 ms
  if (stateled && millis() - inicioLed >= 200){
    digitalWrite(led, LOW);
    stateled = LOW;
  }

  // si pasan 5 segundos sin lectura valida avisar a tierra
  if (esperandoTimeout && (millis() >= nextTimeoutHT)){
    mySerial.println("4");   //[V2] el fallo del sensor ahora es el codigo 4 (antes "Fallo")
    nextTimeoutHT = millis() + 4000;
  }

  //[V2] SENSORES DE DISTANCIA: cada intervaloDistancia se mide con los dos
  if (iniciado && enviarDistancia && millis() - ultimaDistancia >= intervaloDistancia){
    ultimaDistancia = millis();

    //sensor 1: mira hacia 'angulo'
    float d1 = medirDistancia(trigPin1, echoPin1);
    if (d1 < 0){
      // no hay eco: se da 1 segundo para arreglar el error
      if (!esperandoTimeoutD1){
        esperandoTimeoutD1 = true;
        nextTimeoutD1 = millis() + 1000;
      }
    }else{
      esperandoTimeoutD1 = false;
      mySerial.print("2:1:");
      mySerial.print(angulo);
      mySerial.print(":");
      mySerial.println(d1);
    }

    //sensor 2: mira al lado contrario, asi que su angulo es angulo + 180
    float d2 = medirDistancia(trigPin2, echoPin2);
    if (d2 < 0){
      if (!esperandoTimeoutD2){
        esperandoTimeoutD2 = true;
        nextTimeoutD2 = millis() + 1000;
      }
    }else{
      esperandoTimeoutD2 = false;
      mySerial.print("2:2:");
      mySerial.print(angulo + 180);
      mySerial.print(":");
      mySerial.println(d2);
    }

    //si esta en modo barrido, el servo avanza al siguiente angulo
    if (barrido){
      angulo = angulo + paso;
      if (angulo >= 180){        //ha llegado al final: da la vuelta
        angulo = 180;
        paso = -5;
      }
      if (angulo <= 0){          //ha llegado al principio: da la vuelta
        angulo = 0;
        paso = 5;
      }
      moverServo(angulo, 150);   //150 ms conectado: tiempo de sobra para avanzar 5 grados.
                                 //mientras tanto el programa espera y no envia nada, asi el servo no da tirones
    }
  }

  //[V2] si un sensor de distancia sigue sin funcionar, avisar a tierra cada 2 segundos
  if (esperandoTimeoutD1 && (millis() >= nextTimeoutD1)){
    mySerial.println("5:1");
    nextTimeoutD1 = millis() + 2000;
  }
  if (esperandoTimeoutD2 && (millis() >= nextTimeoutD2)){
    mySerial.println("5:2");
    nextTimeoutD2 = millis() + 2000;
  }
}