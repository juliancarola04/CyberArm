// CÓDIGO DE PRUEBA: ESP32 + PCA9685 + WIFI/UDP
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "credenciales.h" // Archivo en la misma carpeta con SSID y pass
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const uint16_t PUERTO_UDP = 4210;
WiFiUDP udp;
char bufferPaquete[128];

unsigned long ultimo_paquete_recibido = 0;
int limite_intervalo = 5000;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Canales del PCA9685
#define CANAL_BASE   0
#define CANAL_CODO   1
#define CANAL_HOMBRO 2
#define CANAL_PINZA  3

// --- LÍMITES EN MICROSEGUNDOS (PWM) ---
const int BASE_MIN = 500,   BASE_MAX = 2400;   
const int CODO_MIN = 1133,  CODO_MAX = 2083;   
const int HOMBRO_MIN = 500, HOMBRO_MAX = 1450; 
const int PINZA_MIN = 606,  PINZA_MAX = 1450;  

// --- AJUSTE DE LÍMITES FÍSICOS (GRADOS) ---
const int LIM_BASE_MIN = 0,    LIM_BASE_MAX = 180;
const int LIM_CODO_MIN = 60,   LIM_CODO_MAX = 150;
const int LIM_HOMBRO_MIN = 0,  LIM_HOMBRO_MAX = 90; 
const int LIM_PINZA_MIN = 10,  LIM_PINZA_MAX = 90;

int anguloBaseDefault = 90;
int anguloCodoDefault = 120;
int anguloHombroDefault = 45; 
int anguloPinzaDefault = 60;

int anguloBase = 0;
int anguloCodo = 0;
int anguloHombro = 0;
int anguloPinza = 0;

bool conectadoPreviamente = false;
bool recibiendoPaquetes = false;

// Función adaptada para mapear los grados a los pulsos del PCA9685
void moverServo(int canal, int angulo, int minGrados, int maxGrados, int minUs, int maxUs) {
    angulo = constrain(angulo, minGrados, maxGrados);
    int microsegundos = map(angulo, minGrados, maxGrados, minUs, maxUs);
    pwm.writeMicroseconds(canal, microsegundos);
}

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
    moverServo(CANAL_BASE, anguloBaseDefault, LIM_BASE_MIN, LIM_BASE_MAX, BASE_MIN, BASE_MAX);
    moverServo(CANAL_CODO, anguloCodoDefault, LIM_CODO_MIN, LIM_CODO_MAX, CODO_MIN, CODO_MAX);
    moverServo(CANAL_HOMBRO, anguloHombroDefault, LIM_HOMBRO_MIN, LIM_HOMBRO_MAX, HOMBRO_MIN, HOMBRO_MAX);
    moverServo(CANAL_PINZA, anguloPinzaDefault, LIM_PINZA_MIN, LIM_PINZA_MAX, PINZA_MIN, PINZA_MAX);
}

void setup() {
    Serial.begin(115200);

    conectarWiFi();

    // Configuración del módulo PCA9685
    Wire.begin();
    Wire.setClock(400000); // Fast Mode
    pwm.begin();
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(50);
    delay(10); 

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

    int cantidadLeida = udp.read(bufferPaquete, sizeof(bufferPaquete) - 1);

    if (cantidadLeida <= 0) {
        return;
    }

    bufferPaquete[cantidadLeida] = '\0';

    // Utilizamos la lectura optimizada por tokens en lugar de sscanf
    char* token = strtok(bufferPaquete, ",");
    if (token != NULL) {
        anguloBase = atoi(token);
        token = strtok(NULL, ",");
        if (token != NULL) {
            anguloCodo = atoi(token);
            token = strtok(NULL, ",");
            if (token != NULL) {
                anguloHombro = atoi(token);
                token = strtok(NULL, ",");
                if (token != NULL) {
                    anguloPinza = atoi(token);

                    Serial.print("Base: ");
                    Serial.print(anguloBase);
                    moverServo(CANAL_BASE, anguloBase, LIM_BASE_MIN, LIM_BASE_MAX, BASE_MIN, BASE_MAX);

                    Serial.print(" | Codo: ");
                    Serial.print(anguloCodo);
                    moverServo(CANAL_CODO, anguloCodo, LIM_CODO_MIN, LIM_CODO_MAX, CODO_MIN, CODO_MAX);

                    Serial.print(" | Hombro: ");
                    Serial.print(anguloHombro);
                    moverServo(CANAL_HOMBRO, anguloHombro, LIM_HOMBRO_MIN, LIM_HOMBRO_MAX, HOMBRO_MIN, HOMBRO_MAX);

                    Serial.print(" | Pinza: ");
                    Serial.println(anguloPinza);
                    moverServo(CANAL_PINZA, anguloPinza, LIM_PINZA_MIN, LIM_PINZA_MAX, PINZA_MIN, PINZA_MAX);
                    
                    ultimo_paquete_recibido = millis();
                    recibiendoPaquetes = true;
                } else {
                    Serial.print("Por alguna razón el paquete recibido llegó mal: ");
                    Serial.println(bufferPaquete);
                }
            }
        }
    }
}