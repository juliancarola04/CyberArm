// LES DEJO ESTE CÓDIGO DE PRUEBA PARA QUE TESTEEN LOS DATOS QUE LLEGAN. SI QUIEREN VEAN EL ENVIAR LOS DATOS A LOS SERVOS.
// VEAN BIEN AMBOS CÓDIGOS, EN ESPECIAL EL OTRO.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "credenciales.h" // Hagan un archivo en la misma carpeta llamado credenciales.h y pongan el SSID y la pass de la red. Les dejo el archivo igual por las dudas.
#include <ESP32Servo.h>

const uint16_t PUERTO_UDP = 4210;

WiFiUDP udp;

char bufferPaquete[128];

unsigned long ultimo_paquete_recibido = 0;
int limite_intervalo = 5000;

Servo servoBase;
Servo servoCodo;
Servo servoHombro;
Servo servoPinza;

int anguloBaseDefault = 90;
int anguloCodoDefault = 90;
int anguloHombroDefault = 90;
int anguloPinzaDefault = 60;

int anguloBase = 0;
int anguloCodo = 0;
int anguloHombro = 0;
int anguloPinza = 0;

bool conectadoPreviamente = false;

bool recibiendoPaquetes = false;

void conectarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    if (conectadoPreviamente) {
        Serial.print("Reconectándose al WiFi");
    }
    else {
        Serial.print("Conectándose al WiFi");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    if (conectadoPreviamente) {
        Serial.println("WiFi reconectado con éxito");
    }
    else {
        Serial.println("WiFi conectado con éxito");
        conectadoPreviamente = true;       
    }

    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());
}

void PonerValoresDefaultServos() {
    servoBase.write(anguloBaseDefault);
    servoCodo.write(anguloCodoDefault);
    servoHombro.write(anguloHombroDefault);
    servoPinza.write(anguloPinzaDefault);    
}

void setup() {
    Serial.begin(115200);

    conectarWiFi();

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    servoBase.setPeriodHertz(50);
    servoCodo.setPeriodHertz(50);
    servoHombro.setPeriodHertz(50);
    servoPinza.setPeriodHertz(50);

    servoBase.attach(13, 500, 2400);
    servoCodo.attach(14), 500, 2400;
    servoHombro.attach(15), 500, 1450;
    servoPinza.attach(16, 500, 1133);

    PonerValoresDefaultServos();

    if (udp.begin(PUERTO_UDP)) {
        Serial.print("Escuchando paquetes UDP en el puerto ");
        Serial.println(PUERTO_UDP);
    } else {
        Serial.println("No se pudo iniciar UDP");
    }
}

void loop() {

    if ((millis() - ultimo_paquete_recibido >= limite_intervalo) && recibiendoPaquetes){
        Serial.println("Hace 5s que no se recibe un paquete de actualización de la computadora. Volviendo a los valores de fábrica.");
        PonerValoresDefaultServos();     
        recibiendoPaquetes = false;
    }

    if (WiFi.status() != WL_CONNECTED){
        conectarWiFi();
    }

    int tamanioPaquete = udp.parsePacket();

    if (tamanioPaquete <= 0) {
        return;
    }

    int cantidadLeida = udp.read(
        bufferPaquete,
        sizeof(bufferPaquete) - 1
    );

    if (cantidadLeida <= 0) {
        return;
    }

    // Caracter nulo que se le agrega así desp lo podemos leer bien con sscanf.
    bufferPaquete[cantidadLeida] = '\0';

    int cantidadValores = sscanf(
        bufferPaquete,
        "%d,%d,%d,%d",
        &anguloBase,
        &anguloCodo,
        &anguloHombro,
        &anguloPinza
    );

    if (cantidadValores == 4) {
        Serial.print("Base: ");
        Serial.print(anguloBase);
        servoBase.write(anguloBase);        

        Serial.print(" | Codo: ");
        Serial.print(anguloCodo);
        servoCodo.write(anguloCodo);        

        Serial.print(" | Hombro: ");
        Serial.print(anguloHombro);
        servoHombro.write(anguloHombro);        

        Serial.print(" | Pinza: ");
        Serial.println(anguloPinza);
        servoPinza.write(anguloPinza);        
        
        ultimo_paquete_recibido = millis();
        recibiendoPaquetes = true;
    } else {
        Serial.print("Por alguna razón el paquete recibido llegó mal: ");
        Serial.println(bufferPaquete);
    }
}