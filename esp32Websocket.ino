#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====== DISPLAY OLED ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== CONFIGURAÇÕES DO WI-FI ======
const char* ssid = "ERRO_404";
const char* password = "84831484";

// ====== SENSOR E BUZZER ======
const int pinSensor = 32;   // Pino analógico onde o sensor MQ2 está conectado
const int pinBuzzer = 12;   // Pino digital onde o buzzer está conectado

const float voltageMin = 0.1;  // Tensão mínima medida quando há gás (em volts)
const float voltageMax = 4.0;  // Tensão máxima medida quando há gás (em volts)
const float ppmMin = 0;        // Concentração de GLP correspondente à tensão mínima medida (em PPM)
const float ppmMax = 500;      // Concentração de GLP correspondente à tensão máxima medida (em PPM)
const float ppmThreshold = 100;// Limite de concentração de GLP para acionar o buzzer (em PPM)

// ====== CONFIGURAÇÕES DO WEBSOCKET ======
const char* websocket_host = "192.168.100.8"; 
const uint16_t websocket_port = 7777;        
const char* websocket_path = "/";            

WebSocketsClient webSocket;
unsigned long lastSend = 0;

// ====== EVENTOS DO WEBSOCKET ======
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Desconectado do WebSocket");
      break;

    case WStype_CONNECTED:
      Serial.println("Conectado ao WebSocket");
      webSocket.sendTXT("ping");  // Envia ping logo após conectar (handshake)
      break;

    case WStype_TEXT:
      Serial.printf("Recebido do servidor: %s\n", payload);
      if (strcmp((char*)payload, "pong") == 0) {
        Serial.println("Recebido pong do servidor");
      }
      break;
  }
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(pinBuzzer, OUTPUT); // Configura o pino do buzzer como saída

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado.");
  Serial.println(WiFi.localIP());

  // Inicializa WebSocket
  webSocket.begin(websocket_host, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Tenta reconectar a cada 5s
}

// ====== LOOP PRINCIPAL ======
void loop() {
  int sensorValue = analogRead(pinSensor);
  
  // Convertendo o valor lido para tensão (em volts)
  float voltage = sensorValue * (5.0 / 4095.0); // A ESP32 possui 12 bits de resolução (0-4095) e tensão de referência de 5V
  
  // Calculando a concentração de GLP com base na tensão medida
  float value = map(voltage, voltageMin, voltageMax, ppmMin, ppmMax); 
  
  // Exibindo a tensão lida e a concentração de GLP no monitor serial
  Serial.print("Tensao: ");
  Serial.print(voltage);
  Serial.println(" V");
  Serial.print("Concentracao de GLP: ");
  Serial.print(value);
  Serial.println(" PPM");
  
  // Exibindo a concentração de GLP no display OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Conc. de GLP:");
  display.setCursor(0, 10);
  display.print(value);
  display.print(" PPM");
  display.display();
  
  // Verifica se a concentração de GLP ultrapassou o limite
  if (value > ppmThreshold) {
    digitalWrite(pinBuzzer, HIGH);
  } else {
    digitalWrite(pinBuzzer, LOW);
  }
  
  // Aguarda um segundo antes da próxima leitura
  //delay(1000);

  // Mantém conexão ativa
  webSocket.loop(); 

  // Envia leitura do MQ-4 a cada 1 segundo
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    
    // Envia só se conectado
    if (webSocket.isConnected()) {
      String json = "{\"mq4\":" + String(value) + "}";
      webSocket.sendTXT(json);
      Serial.print("Enviado ao servidor: ");
      Serial.println(json);
    } else {
      Serial.println("WebSocket desconectado, não enviando dados.");
    }
  }
}
