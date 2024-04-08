#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Inicializa el sensor de huellas dactilares
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

// Configuración de WiFi
const char* ssid = "ZELIO_CG13"; 
const char* password = "ZELIO_155473CG"; 

// Estructura para almacenar los datos del profesor
struct Profesor {
  String nombre;
  String app;
  String apm;
  String hora;
  String mensajeHuella;
};


// Prototipos de funciones
Profesor obtenerDatosProfesor(int idHuella);
String crearJSON(Profesor datosProfesor);
void enviarDatos(String jsonDatos);

void setup() {
  Serial.begin(115200); // Inicia la comunicación serial
  finger.begin(57600); // Inicia el sensor de huellas dactilares
  
  if (finger.verifyPassword()) {
    Serial.println("Sensor de huellas dactilares encontrado!");
  } else {
    Serial.println("No se encontró el sensor de huellas dactilares :(");
    while (1) { delay(1); }
  }

  WiFi.begin(ssid, password); // Conéctate a la red WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("¡Conectado a Internet!");
}

void loop() {
  int id = leerHuella(); // Lee una huella
  
  if (id != -1) { // Si se encontró una huella
    Profesor datosProfesor = obtenerDatosProfesor(id); // Obtén los datos del profesor
    String jsonDatos = crearJSON(datosProfesor); // Crea un JSON con los datos y el mensaje
    Serial.println(jsonDatos);
    enviarDatos(jsonDatos); // Envía los datos a la API de Laravel
  }

  delay(5000); // Espera 5 segundos antes de la próxima lectura
}

int leerHuella() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    Serial.println("Error al leer la huella");
    return -1;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("Error al convertir la imagen");
    return -1;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("Huella no encontrada");
    return -1;
  }

  // Si la huella se encuentra, devuelve el ID de la huella
  Serial.print("Huella encontrada con ID: ");
  Serial.println(finger.fingerID);
  return finger.fingerID; // El ID de la huella encontrada
}

Profesor obtenerDatosProfesor(int idHuella) {
    Profesor profesor;
  HTTPClient http;
  String url = "http://192.168.1.94:3001/enrolarhuella/" + String(idHuella); // Reemplaza con la URL de tu API
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) { // Verifica que el código de estado HTTP sea 200 OK
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    // Asume que la respuesta de la API es un objeto JSON con los datos del profesor y un mensaje
    String message = doc["message"].as<String>(); // Maneja el mensaje de estado
    Serial.println(message); // Imprime el mensaje de estado
    // Asume que la respuesta de la API es un objeto JSON con los datos del profesor
    JsonObject profesorO = doc["profesor"].as<JsonObject>();
    profesor.nombre = profesorO["nombre"].as<String>();
    profesor.app = profesorO["app"].as<String>();
    profesor.apm = profesorO["apm"].as<String>();
    profesor.hora = profesorO["hora"].as<String>();
    profesor.mensajeHuella = doc["message"].as<String>();
  } else {
    Serial.print("Error en la solicitud HTTP: ");
    Serial.println(httpCode);
    // Manejar el error o devolver un profesor con valores predeterminados
  }

  http.end();
  return profesor;
}

String crearJSON(Profesor datosProfesor) {

  // Crea un JSON con los datos del profesor y el mensaje específico para la huella
  String jsonDatos = "{\"nombre\": \"" + datosProfesor.nombre + "\", \"app\": \"" + datosProfesor.app + "\", \"apm\": \"" + datosProfesor.apm + "\", \"hora\": \"" + datosProfesor.hora + "\", \"mensaje\": \"" + datosProfesor.mensajeHuella + "\"}";
  return jsonDatos;
}

void enviarDatos(String jsonDatos) {
  String url = "http://192.168.1.94:3001/recibirdatos"; // Reemplaza con la URL de tu API
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonDatos);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Respuesta del servidor: " + payload);
  } else {
    Serial.print("Error en la solicitud HTTP: ");
    Serial.println(httpCode);
  }
  http.end();
}
