// Medidor de CO₂ con ESP32 + Sensirion SCD41 + pantalla OLED SSD1306 + LED WS2812 + zumbador opcional.
//
// Conexiones (ESP32 DevKit):
//   SCD41 y OLED comparten el bus I2C: SDA -> GPIO21, SCL -> GPIO22, 3V3 y GND.
//   LED WS2812 (un píxel): datos -> GPIO4, 5V (VIN) y GND.
//   Zumbador activo (opcional): GPIO25 y GND.
//   Botón BOOT (GPIO0): una pulsación silencia la alarma; 10 s sostenido borra el WiFi guardado.

#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <SensirionI2cScd4x.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>

#include "historial.h"
#include "pagina.h"
#include "semaforo.h"

#ifndef VERSION_FIRMWARE
#define VERSION_FIRMWARE "dev"
#endif

namespace {

constexpr uint8_t PIN_SDA = 21;
constexpr uint8_t PIN_SCL = 22;
constexpr uint8_t PIN_LED = 4;
constexpr uint8_t PIN_ZUMBADOR = 25;
constexpr uint8_t PIN_BOTON = 0;
constexpr char NOMBRE_RED[] = "medidor-co2";

SensirionI2cScd4x sensor;
Adafruit_SSD1306 pantalla(128, 64, &Wire, -1);
Adafruit_NeoPixel led(1, PIN_LED, NEO_GRB + NEO_KHZ800);
WebServer servidor(80);
WiFiManager wifi;
Semaforo semaforo;
Historial historial(300);

bool hayPantalla = false;
bool haySensor = false;
bool alarmaSilenciada = false;

struct Lectura {
  uint16_t co2 = 0;
  float temperatura = NAN;
  float humedad = NAN;
  uint32_t momento = 0;  // segundos desde el arranque
  bool valida = false;
} ultima;

uint32_t segundos() { return millis() / 1000; }

void mostrarLed(Nivel nivel) {
  const uint32_t color = colorNivel(nivel);
  // Brillo bajo: el LED es una señal, no una lámpara.
  led.setPixelColor(0, led.Color((color >> 16 & 0xFF) / 6, (color >> 8 & 0xFF) / 6, (color & 0xFF) / 6));
  led.show();
}

void pitar(uint8_t veces) {
  for (uint8_t i = 0; i < veces; i++) {
    digitalWrite(PIN_ZUMBADOR, HIGH);
    delay(120);
    digitalWrite(PIN_ZUMBADOR, LOW);
    delay(120);
  }
}

void dibujar() {
  if (!hayPantalla) return;
  pantalla.clearDisplay();
  pantalla.setTextColor(SSD1306_WHITE);
  if (!ultima.valida) {
    pantalla.setTextSize(1);
    pantalla.setCursor(0, 0);
    pantalla.println(haySensor ? "Calentando sensor..." : "No se encontro el SCD41");
    pantalla.println();
    pantalla.println(haySensor ? "Primera lectura en ~5 s" : "Revisa SDA 21 / SCL 22");
  } else {
    pantalla.setTextSize(3);
    pantalla.setCursor(0, 0);
    pantalla.print(ultima.co2);
    pantalla.setTextSize(1);
    pantalla.print(" ppm");
    pantalla.setCursor(0, 30);
    pantalla.setTextSize(2);
    pantalla.print(consejoNivel(semaforo.nivel()));
    pantalla.setTextSize(1);
    pantalla.setCursor(0, 54);
    pantalla.printf("%.1f C  %.0f%%", ultima.temperatura, ultima.humedad);
  }
  if (WiFi.isConnected()) {
    pantalla.setCursor(92, 54);
    pantalla.print("WiFi");
  }
  pantalla.display();
}

void leerSensor() {
  if (!haySensor) return;
  bool lista = false;
  if (sensor.getDataReadyStatus(lista) != 0 || !lista) return;

  uint16_t co2 = 0;
  float temperatura = 0;
  float humedad = 0;
  if (sensor.readMeasurement(co2, temperatura, humedad) != 0 || co2 == 0) return;

  ultima.co2 = co2;
  ultima.temperatura = temperatura;
  ultima.humedad = humedad;
  ultima.momento = segundos();
  ultima.valida = true;
  historial.agregar(co2, ultima.momento);
  mostrarLed(semaforo.actualizar(co2));
  if (semaforo.recienAlto() && !alarmaSilenciada) pitar(3);
  if (semaforo.nivel() != Nivel::Alto) alarmaSilenciada = false;
  dibujar();
}

// --- Panel web ---

void responderLectura() {
  char json[256];
  if (!ultima.valida) {
    snprintf(json, sizeof json, "{\"valida\":false,\"version\":\"%s\"}", VERSION_FIRMWARE);
  } else {
    snprintf(json, sizeof json,
             "{\"valida\":true,\"co2\":%u,\"temperatura\":%.1f,\"humedad\":%.0f,\"nivel\":\"%s\","
             "\"consejo\":\"%s\",\"hace\":%lu,\"version\":\"%s\"}",
             ultima.co2, ultima.temperatura, ultima.humedad, nombreNivel(semaforo.nivel()),
             consejoNivel(semaforo.nivel()), static_cast<unsigned long>(segundos() - ultima.momento),
             VERSION_FIRMWARE);
  }
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", json);
}

void responderHistorial() {
  // 288 valores de hasta 5 dígitos con coma: menos de 2 KB.
  static char json[2048];
  char valores[1800];
  if (historial.json(valores, sizeof valores) == 0) {
    servidor.send(500, "application/json", "{\"error\":\"historial demasiado grande\"}");
    return;
  }
  snprintf(json, sizeof json, "{\"minutos_por_punto\":%lu,\"valores\":%s}",
           static_cast<unsigned long>(historial.segundosPorIntervalo() / 60), valores);
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", json);
}

// Recalibración forzada: con el medidor al aire libre (o junto a una ventana abierta) al menos 3 minutos,
// se le indica que la concentración real es ~420 ppm.
void responderCalibrar() {
  if (servidor.method() != HTTP_POST) {
    servidor.send(405, "application/json", "{\"error\":\"usa POST\"}");
    return;
  }
  const long referencia = servidor.hasArg("ppm") ? servidor.arg("ppm").toInt() : 420;
  if (referencia < 400 || referencia > 2000) {
    servidor.send(400, "application/json", "{\"error\":\"ppm debe estar entre 400 y 2000\"}");
    return;
  }
  uint16_t correccion = 0;
  sensor.stopPeriodicMeasurement();
  delay(500);
  const int16_t error = sensor.performForcedRecalibration(static_cast<uint16_t>(referencia), correccion);
  sensor.startPeriodicMeasurement();
  if (error != 0 || correccion == 0xFFFF) {
    servidor.send(500, "application/json", "{\"error\":\"el sensor rechazo la calibracion\"}");
    return;
  }
  char json[96];
  snprintf(json, sizeof json, "{\"ok\":true,\"correccion\":%d}", static_cast<int>(correccion) - 0x8000);
  servidor.send(200, "application/json", json);
}

void iniciarServidor() {
  servidor.on("/", [] { servidor.send_P(200, "text/html; charset=utf-8", PAGINA); });
  servidor.on("/api/lectura", responderLectura);
  servidor.on("/api/historial", responderHistorial);
  servidor.on("/api/calibrar", responderCalibrar);
  servidor.onNotFound([] { servidor.send(404, "text/plain", "No existe"); });
  servidor.begin();
}

void revisarBoton() {
  static uint32_t presionadoDesde = 0;
  const bool presionado = digitalRead(PIN_BOTON) == LOW;
  if (presionado && presionadoDesde == 0) presionadoDesde = millis();
  if (!presionado && presionadoDesde != 0) {
    const uint32_t duracion = millis() - presionadoDesde;
    presionadoDesde = 0;
    if (duracion >= 10000) {
      wifi.resetSettings();
      ESP.restart();
    } else if (duracion >= 50) {
      alarmaSilenciada = true;
      pitar(1);
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ZUMBADOR, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  led.begin();
  led.clear();
  led.show();

  Wire.begin(PIN_SDA, PIN_SCL);
  hayPantalla = pantalla.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  sensor.begin(Wire, SCD41_I2C_ADDR_62);
  sensor.wakeUp();
  sensor.stopPeriodicMeasurement();
  sensor.reinit();
  uint64_t serie = 0;
  haySensor = sensor.getSerialNumber(serie) == 0 && sensor.startPeriodicMeasurement() == 0;
  Serial.printf("Medidor de CO2 %s, sensor %s\n", VERSION_FIRMWARE, haySensor ? "listo" : "no encontrado");
  dibujar();

  // Si no hay WiFi guardado abre la red "medidor-co2" para configurarlo desde el celular.
  // Sin WiFi sigue midiendo con pantalla y LED.
  WiFi.mode(WIFI_STA);
  wifi.setConfigPortalBlocking(false);
  wifi.setConfigPortalTimeout(300);
  wifi.setHostname(NOMBRE_RED);
  wifi.autoConnect(NOMBRE_RED);
  iniciarServidor();
}

void loop() {
  static uint32_t ultimaRevision = 0;
  wifi.process();
  servidor.handleClient();
  revisarBoton();

  // El SCD41 entrega una lectura cada 5 s; se pregunta cada segundo si ya hay una nueva.
  if (millis() - ultimaRevision >= 1000) {
    ultimaRevision = millis();
    leerSensor();
    static bool conMdns = false;
    if (!conMdns && WiFi.isConnected()) {
      conMdns = MDNS.begin(NOMBRE_RED);
      if (conMdns) MDNS.addService("http", "tcp", 80);
      dibujar();
    }
  }
}
