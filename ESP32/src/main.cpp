// CÓDIGO PRINCIPAL: ESP32 + PCA9685 + WIFI/UDP + BARRIDO SUAVE + HOMBRO INVERTIDO
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
// (Puedes ajustar HOMBRO_MIN si notas que el servo hace tope físico)
const int BASE_MIN = 500,   BASE_MAX = 2400;   
const int CODO_MIN = 1133,  CODO_MAX = 2083;   
const int HOMBRO_MIN = 900, HOMBRO_MAX = 1850; 
const int PINZA_MIN = 606,  PINZA_MAX = 1450;  

// --- AJUSTE DE LÍMITES FÍSICOS (GRADOS) ---
const int LIM_BASE_MIN = 0,    LIM_BASE_MAX = 180;
const int LIM_CODO_MIN = 60,   LIM_CODO_MAX = 150;
const int LIM_HOMBRO_MIN = 0,  LIM_HOMBRO_MAX = 90; 
const int LIM_PINZA_MIN = 10,  LIM_PINZA_MAX = 90;

// --- CONFIGURACIÓN DE MOVIMIENTO ---
const int UMBRAL_MOVIMIENTO = 5; // Grados mínimos para ignorar temblores de cámara
const int VELOCIDAD_PASO = 15;   // Milisegundos entre cada grado (mayor = más lento)
unsigned long tiempo_ultimo_paso = 0;

int anguloBaseDefault = 90;
int anguloCodoDefault = 120;
int anguloHombroDefault = 45;
int anguloPinzaDefault = 60;

// VARIABLES OBJETIVO (A donde queremos ir)
int objBase = anguloBaseDefault;
int objCodo = anguloCodoDefault;
int objHombro = anguloHombroDefault;
int objPinza = anguloPinzaDefault;

// VARIABLES ACTUALES (Donde está físicamente el brazo ahora)
int actBase = anguloBaseDefault;
int actCodo = anguloCodoDefault;
int actHombro = anguloHombroDefault;
int actPinza = anguloPinzaDefault;

bool conectadoPreviamente = false;
bool recibiendoPaquetes = false;

// Mapeo de grados a pulsos del PCA9685
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
    } else {
        Serial.print("Conectándose al WiFi");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    if (conectadoPreviamente) {
        Serial.println("WiFi reconectado con éxito");
    } else {
        Serial.println("WiFi conectado con éxito");
        conectadoPreviamente = true;       
    }

    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());
}

void PonerValoresDefaultServos() {
    objBase = anguloBaseDefault;
    objCodo = anguloCodoDefault;
    objHombro = anguloHombroDefault;
    objPinza = anguloPinzaDefault;
}

void setup() {
    Serial.begin(115200);
    conectarWiFi();

    Wire.begin();
    Wire.setClock(400000); 
    pwm.begin();
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(50);
    delay(10); 

    PonerValoresDefaultServos();
    
    moverServo(CANAL_BASE, actBase, LIM_BASE_MIN, LIM_BASE_MAX, BASE_MIN, BASE_MAX);
    moverServo(CANAL_CODO, actCodo, LIM_CODO_MIN, LIM_CODO_MAX, CODO_MIN, CODO_MAX);
    moverServo(CANAL_HOMBRO, actHombro, LIM_HOMBRO_MIN, LIM_HOMBRO_MAX, HOMBRO_MIN, HOMBRO_MAX);
    moverServo(CANAL_PINZA, actPinza, LIM_PINZA_MIN, LIM_PINZA_MAX, PINZA_MIN, PINZA_MAX);

    if (udp.begin(PUERTO_UDP)) {
        Serial.print("Escuchando paquetes UDP en el puerto ");
        Serial.println(PUERTO_UDP);
    } else {
        Serial.println("No se pudo iniciar UDP");
    }
}

void loop() {

    if ((millis() - ultimo_paquete_recibido >= limite_intervalo) && recibiendoPaquetes){
        Serial.println("Hace 5s que no se recibe un paquete de actualización. Volviendo a los valores de fábrica.");
        PonerValoresDefaultServos();     
        recibiendoPaquetes = false;
    }

    if (WiFi.status() != WL_CONNECTED){
        conectarWiFi();
    }

    int tamanioPaquete = udp.parsePacket();

    if (tamanioPaquete > 0) {
        int cantidadLeida = udp.read(bufferPaquete, sizeof(bufferPaquete) - 1);
        if (cantidadLeida > 0) {
            bufferPaquete[cantidadLeida] = '\0';

            char* token = strtok(bufferPaquete, ",");
            if (token != NULL) {
                int inBase = atoi(token);
                token = strtok(NULL, ",");
                if (token != NULL) {
                    int inCodo = atoi(token);
                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        int inHombro = atoi(token);
                        
                        // --- INVERSIÓN DEL HOMBRO ---
                        // Si Python manda ángulos de 0 a 180, usa 180.
                        // Si Python manda ángulos ya limitados de 0 a 90, usa 90.
                        inHombro = 90 - inHombro; 
                        
                        token = strtok(NULL, ",");
                        if (token != NULL) {
                            int inPinza = atoi(token);

                            // --- FILTRO DE UMBRAL (Comparando contra obj para evitar temblequeo) ---
                            if (abs(inBase - objBase) >= UMBRAL_MOVIMIENTO) objBase = inBase;
                            if (abs(inCodo - objCodo) >= UMBRAL_MOVIMIENTO) objCodo = inCodo;
                            if (abs(inHombro - objHombro) >= UMBRAL_MOVIMIENTO) objHombro = inHombro;
                            
                            // --- MODIFICACIÓN DE LA PINZA (Movimiento instantáneo) ---
                            if (abs(inPinza - objPinza) >= UMBRAL_MOVIMIENTO) {
                                objPinza = inPinza;
                                actPinza = inPinza; // Salta directo al objetivo
                                moverServo(CANAL_PINZA, actPinza, LIM_PINZA_MIN, LIM_PINZA_MAX, PINZA_MIN, PINZA_MAX);
                            }
                            
                            ultimo_paquete_recibido = millis();
                            recibiendoPaquetes = true;
                        }
                    }
                }
            }
        }
    }

    // --- EJECUCIÓN DEL MOVIMIENTO GRADO POR GRADO ---
    if (millis() - tiempo_ultimo_paso >= VELOCIDAD_PASO) {
        tiempo_ultimo_paso = millis();

        if (actBase != objBase) {
            if (actBase < objBase) actBase++; 
            else actBase--;
            moverServo(CANAL_BASE, actBase, LIM_BASE_MIN, LIM_BASE_MAX, BASE_MIN, BASE_MAX);
        }

        if (actCodo != objCodo) {
            if (actCodo < objCodo) actCodo++; 
            else actCodo--;
            moverServo(CANAL_CODO, actCodo, LIM_CODO_MIN, LIM_CODO_MAX, CODO_MIN, CODO_MAX);
        }

        if (actHombro != objHombro) {
            if (actHombro < objHombro) actHombro++; 
            else actHombro--;
            moverServo(CANAL_HOMBRO, actHombro, LIM_HOMBRO_MIN, LIM_HOMBRO_MAX, HOMBRO_MIN, HOMBRO_MAX);
        }
        
        // ¡El bloque de la pinza que estaba aquí se eliminó para que no se mueva lento!
    }
}