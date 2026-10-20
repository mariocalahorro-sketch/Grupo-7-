#include <DHT.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); //RX, TX
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const int led = 7; //LED en el pin 7 (verde)
bool stateled = LOW;
bool enviarDatos = false;
long ultimoEnvio = 0;
const long intervalo = 3000;   //Tiempo entre dato y dato
long inicioLed = 0;
bool iniciado = false;

//variables para la alarma
bool esperandoTimeout = false;  //se esta esperando el tiempo para que se arregle el error? false -> no se esta esperando
long nextTimeoutHT;  //tiempo que se le da para arreglar el error de recopilacion de datos

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  dht.begin();
  pinMode(led, OUTPUT);
}

void loop() {
  //Recibie ordenes de tierra
  if (mySerial.available()){
    String orden = mySerial.readString();
    if (orden == "Iniciar" && !iniciado){
      iniciado = true;
      enviarDatos = true;
      ultimoEnvio = millis() - intervalo;
    }else if (orden == "Pausar" && iniciado){
      enviarDatos = false;
    }else if (orden == "Reanudar" && iniciado){
      enviarDatos = true;
      ultimoEnvio = millis() - intervalo;   //envia un dato enseguida
    }
  }
  // Enviar datos cada 3 segundos si enviarDatos = True
  if (iniciado && millis() - ultimoEnvio >= intervalo){
    ultimoEnvio = millis();
    // se lee el sensor
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)){
      // Si la lectura no es correcta, mostrar mensaje de error
      if (!esperandoTimeout){
      esperandoTimeout = true;
      nextTimeoutHT = millis() + 5000; //5 segundos para arreglar el error
      }
    }else{
      // Se envian los datos al arduino tierra
      esperandoTimeout = false;
      if (enviarDatos){
        mySerial.print("T:");
        mySerial.print(t);
        mySerial.print(":H:");
        mySerial.println(h);

        // Se encinede el led cuando se envia info a tierra
        digitalWrite(led, HIGH);
        stateled = HIGH;
        inicioLed = millis();
      }
    }
    //apagar el led despues de 200 ms
  }
  //apagamos el led de envio de datos
  if (stateled && millis() - inicioLed >= 200){
    digitalWrite(led, LOW);
    stateled = LOW;
  }

  // si pasan 5 segundos sin lectura valida avisar a tierra
  if (esperandoTimeout && (millis() >= nextTimeoutHT)){  
    mySerial.println ("Fallo");
    nextTimeoutHT = millis() + 4000;

  }
  
}