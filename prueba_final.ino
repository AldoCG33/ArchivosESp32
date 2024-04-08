#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h> 
#include <HTTPClient.h> 

HardwareSerial MySerial(2);

Adafruit_Fingerprint MyFingerprint = Adafruit_Fingerprint(&MySerial);

const char* ssid = "ZELIO_CG13";     
const char* password = "ZELIO_155473CG"; 
const char* apiUrl = "http://192.168.1.94:3001/huellas";

void setup() {
    Serial.begin(57600);
    Serial.println("Sensor de Huella");

    // Inicializar el sensor de huella digital
    while (!Serial);
    delay(100);
    MyFingerprint.begin(57600);

    if (MyFingerprint.verifyPassword()) {
        Serial.println("Sensor de Huella Encontrado :)  :) ");
    } else {
        Serial.println("No fue posible encontrar al sensor de Huella  :(  :( ");
        while (1);
    }

    // Conéctate a la red WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a la red WiFi...");
    }
    Serial.println("Conectado a la red WiFi");
}

void loop() {
    mostrarHuellasRegistradas();
    Serial.println("Ingrese el Numero del ID de la huella a guardar (1 -127): ");
    int my_id = 0;
  
    while (true) {
        while (!Serial.available());

        char c = Serial.read();
    
        if (!isdigit(c)) break;
        my_id *= 10;
        my_id += c - '0';

        Serial.print("c: "); Serial.println(c);
        Serial.print("my_id: "); Serial.println(c);
    }

    // Verificar si el ID ya está en uso
    uint8_t p = MyFingerprint.loadModel(my_id);
    if (p == FINGERPRINT_OK) {
      Serial.println("Ese ID ya está en uso. Por favor, selecciona otro.");
      return;
    }

    Serial.print("Enrolando ID #");
    Serial.println(my_id);
    delay(6000);
    if (capturarHuella(my_id)) {
        Serial.println("Huella digital capturada correctamente.");
        enviarDatosAPI(my_id);
    } else {
        Serial.println("Error al capturar la huella digital.");
    }

    delay(5000); 
}

bool capturarHuella(int id) {
    Serial.println("Esperando dedo...");

    uint8_t p = MyFingerprint.getImage();
    if (p != FINGERPRINT_OK) {
        return false;
    }

    p = MyFingerprint.image2Tz();
    if (p != FINGERPRINT_OK) {
        return false;
    }

    p = MyFingerprint.storeModel(id);
    if (p != FINGERPRINT_OK) {
        return false;
    }

    return true;
}

void enviarDatosAPI(int id) {
    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/json");
    String requestBody = "{\"id\": " + String(id) + "}";
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
        Serial.print("Respuesta de la API: ");
        Serial.println(httpResponseCode);
    } else {
        Serial.print("Error al enviar los datos a la API. Código de error: ");
        Serial.println(httpResponseCode);
        Serial.print("Mensaje de error: ");
        Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
}

void mostrarHuellasRegistradas() {
  for (int id = 1; id <= 127; id++) {
    uint8_t p = MyFingerprint.loadModel(id);
    if (p == FINGERPRINT_OK) {
      Serial.print("Huella registrada con ID #");
      Serial.println(id);
    }
  }
}
