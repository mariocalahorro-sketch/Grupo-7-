/* [V2] PROTOCOLO DE APLICACION: el mismo que el satelite (esta completo en arduinosatelite.ino).
   Este Arduino hace de puente. Lo que le importa a el:

   Satelite -> Tierra
     1:T:H        Temperatura y humedad        -> parpadea el LED verde
     2:s:angulo:d Distancia del sensor s       -> ese sensor funciona
     3:media      Media de temperaturas
     4            Fallo del sensor de T/H      -> LED rojo
     5:s          Fallo del sensor de distancia s (1 o 2) -> LED azul
     7            Latido: se queda aqui, no se reenvia a Python

   Arduino de tierra -> Python
     6            Fallo de comunicacion con el satelite -> LED amarillo

   Python -> Satelite: las ordenes se reenvian tal cual llegan.
*/
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); //RX, TX
const int led1 = 7; //LED verde
const int led2 = 4; //LED rojo (alarma sensor)
const int led3 = 2; //LED amarillo (alarma comunicacion)
const int led4 = 8; //[V2] LED azul (alarma sensores de distancia)

bool vigilando = true;                   //con latido, se vigila siempre (tambien en pausa)
bool alarmaCom = false;                  //alarma de fallo en comunicaciones
unsigned long ultimoMensaje = 0;         //ahora: cuando llego el ultimo mensaje (dato, fallo o latido)
const unsigned long timeoutCom = 5000;  //5 segundos sin mensajes = fallo
long ultimoAvisoCom = 0;   //aviso de fallos de comunicacion
const long intervaloAvisoCom = 5000;   //la alarma sonara cada 5 segundos
bool falloDistancia1 = false;   //[V2] esta fallando el sensor de distancia 1?
bool falloDistancia2 = false;   //[V2] esta fallando el sensor de distancia 2?

//setup: se ejecuta una vez al encender. Prepara las comunicaciones y los LEDs
//parametros: ninguno.  resultado: ninguno
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);   //[V2]
}

//loop: se repite sin parar. Pasa los mensajes del satelite a Python y las ordenes
//      de Python al satelite, enciende los LEDs y vigila la comunicacion
//parametros: ninguno.  resultado: ninguno
void loop() {
  // comunicación SATELITE -> PYTHON
  if (mySerial.available()){
    String datos = mySerial.readStringUntil('\n');   //un mensaje cada vez
    datos.trim();                                    //quita el \r

    ultimoMensaje = millis();            //cualquier mensaje (latido incluido) reinicia el timeout
    alarmaCom = false;
    digitalWrite(led3, LOW);             //la comunicacion se ha recuperado

    if (datos != "7"){                   //[V2] el latido (antes "Vivo") solo indica que sigue vivo, no se reenvia
      Serial.println(datos);

      //[V2] antes solo habia datos de T/H o "Fallo". Ahora llegan mas tipos de mensaje,
      //     asi que miramos como empieza cada uno para saber cual es
      if (datos.startsWith("1:")){       //[V2] dato de T/H (antes: todo lo que no fuera "Fallo")
        digitalWrite(led2, LOW);
        digitalWrite(led1, HIGH);
        delay(200);
        digitalWrite(led1, LOW);
      }else if (datos == "4"){           //[V2] fallo del sensor T/H (antes "Fallo")
        digitalWrite(led2, HIGH);
      }else if (datos.startsWith("2:1:")){   //[V2] dato bueno del sensor de distancia 1
        falloDistancia1 = false;
      }else if (datos.startsWith("2:2:")){   //[V2] dato bueno del sensor de distancia 2
        falloDistancia2 = false;
      }else if (datos == "5:1"){             //[V2] falla el sensor de distancia 1
        falloDistancia1 = true;
      }else if (datos == "5:2"){             //[V2] falla el sensor de distancia 2
        falloDistancia2 = true;
      }

      //[V2] el LED azul se enciende si falla cualquiera de los dos sensores de distancia
      if (falloDistancia1 || falloDistancia2){
        digitalWrite(led4, HIGH);
      }else{
        digitalWrite(led4, LOW);
      }
    }
  }

  // comunicacion PYTHON -> SATELITE
  if (Serial.available()){
    String orden = Serial.readStringUntil('\n');   //[V2] una orden cada vez (antes readString, que espera 1 segundo)
    mySerial.println(orden);                       //[V2] se reenvia con su salto de linea (antes print)
  }

  // ALARMA DE COMUNICACION
  if (vigilando && millis() - ultimoMensaje >= timeoutCom){ //no ha llegado ni un latido en 5 segundos
    if (!alarmaCom){
      alarmaCom = true;
      digitalWrite(led3, HIGH);
      ultimoAvisoCom = millis() - intervaloAvisoCom; //se envia el primer aviso
    }
  }
  if (alarmaCom && millis() - ultimoAvisoCom >= intervaloAvisoCom){ //se repite la alarma cada 5 segundos
    ultimoAvisoCom = millis();
    Serial.println("6"); //[V2] avisa a python del error (antes "Comunicacion")
  }
}
