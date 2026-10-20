#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); //RX, TX
const int led1 = 7; //LED verde
const int led2 = 4; //LED rojo (alarma sensor)
const int led3 = 2; //LED amarillo (alarma comunicacion)

bool vigilando = false;                  //vigilamos si el satelite deberia estar enviando
bool alarmaCom = false;                  //alarma de fallo en comunicaciones
unsigned long ultimoMensaje = 0;
const unsigned long timeoutCom = 8000;  //8 segundos sin mensajes = fallo
long ultimoAvisoCom = 0;   //aviso de fallos de comunicacion
const long intervaloAvisoCom = 5000;   //la alarma sonara cada 5 segundos

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
}

void loop() {
  // comunicación SATELITE -> PYTHON
  if (mySerial.available()){
    String datos = mySerial.readString();
    Serial.print(datos);

    ultimoMensaje = millis();            //reinicia el timeout
    alarmaCom = false;
    digitalWrite(led3, LOW);             //la comunicacion se ha recuperado

    if (datos != "Fallo\r\n"){ //se pone \r\n pq el mensaje Fallo se envia con salto de linea
      digitalWrite(led2, LOW);
      digitalWrite(led1, HIGH);
      delay(200);
      digitalWrite(led1, LOW);
    }else{
      digitalWrite(led2, HIGH);
    }
  }

  // comunicacion PYTHON -> SATELITE
  if (Serial.available()){
    String orden = Serial.readString();
    mySerial.print(orden);

    if (orden == "Iniciar" || orden == "Reanudar"){ //si la orden es iniciar o reanudar, deberia estar recibiendo datos
      vigilando = true;
      ultimoMensaje = millis();
    }else if (orden == "Pausar"){ //en pausar no debe recibir datos, vigilando = false
      vigilando = false;
      alarmaCom = false;
      digitalWrite(led3, LOW);
    }
  }

  // ALARMA DE COMUNICACION
  if (vigilando && millis() - ultimoMensaje >= timeoutCom){ //deberian llegar datos y han pasado mas de 5 segundos
    if (!alarmaCom){
      alarmaCom = true;
      digitalWrite(led3, HIGH);
      ultimoAvisoCom = millis() - intervaloAvisoCom; //se envia el primer aviso
    }
  }
  if (alarmaCom && millis() - ultimoAvisoCom >= intervaloAvisoCom){ //se repite la alarma cada 5 segundos
    ultimoAvisoCom = millis();
    Serial.println("Comunicacion"); //avisa a python del error
  }
}