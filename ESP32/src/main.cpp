// LES DEJO ESTE CÓDIGO DE PRUEBA PARA QUE TESTEEN LOS DATOS QUE LLEGAN. SI QUIEREN VEAN EL ENVIAR LOS DATOS A LOS SERVOS.
// VEAN BIEN AMBOS CÓDIGOS, EN ESPECIAL EL OTRO.

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiUdp.h>

// Datos de tu router.
const char* ssid = "ACÁ PONGAN EL NOMBRE DE LA RED. RESPTEN LAS MAYUS Y MINUS";
const char* password = "ACÁ LA CONTRASEÑA";

// Debe coincidir con PUERTO_UDP en Python.
const uint16_t PUERTO_UDP = 4210;

WiFiUDP udp;

char bufferPaquete[128];

int anguloBase = 90;
int anguloCodo = 90;
int anguloHombro = 90;
int anguloPinza = 60;

void conectarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Conectando al WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi conectado");

    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);

    conectarWiFi();

    if (udp.begin(PUERTO_UDP)) {
        Serial.print("Escuchando paquetes UDP en el puerto ");
        Serial.println(PUERTO_UDP);
    } else {
        Serial.println("No se pudo iniciar UDP");
    }
}

void loop() {
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

    // Terminador necesario para tratar el buffer como texto.
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
        // Evitamos valores inválidos.
        anguloBase = constrain(anguloBase, 0, 180);
        anguloCodo = constrain(anguloCodo, 0, 180);
        anguloHombro = constrain(anguloHombro, 0, 180);
        anguloPinza = constrain(anguloPinza, 0, 180);

        Serial.print("Base: ");
        Serial.print(anguloBase);

        Serial.print(" | Codo: ");
        Serial.print(anguloCodo);

        Serial.print(" | Hombro: ");
        Serial.print(anguloHombro);

        Serial.print(" | Pinza: ");
        Serial.println(anguloPinza);
    } else {
        Serial.print("Paquete inválido: ");
        Serial.println(bufferPaquete);
    }
}