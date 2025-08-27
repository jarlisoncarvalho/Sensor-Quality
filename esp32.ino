#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int pinSensor = 32; // Pino analógico onde o sensor MQ2 está conectado
const int pinBuzzer = 26; // Pino digital onde o buzzer está conectado

const float voltageMin = 0.1; // Tensão mínima medida quando há gás (em volts)
const float voltageMax = 4.0; // Tensão máxima medida quando há gás (em volts)
const float ppmMin = 0; // Concentração de GLP correspondente à tensão mínima medida (em PPM)
const float ppmMax = 500; // Concentração de GLP correspondente à tensão máxima medida (em PPM)
const float ppmThreshold = 100; // Limite de concentração de GLP para acionar o buzzer (em PPM)

void setup() {
  Serial.begin(115200); // Inicia a comunicação serial

  pinMode(pinBuzzer, OUTPUT); // Configura o pino do buzzer como saída

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  // Lendo o valor analógico do sensor
  int sensorValue = analogRead(pinSensor);
  
  // Convertendo o valor lido para tensão (em volts)
  float voltage = sensorValue * (5.0 / 4095.0); // A ESP32 possui 12 bits de resolução (0-4095) e tensão de referência de 5V
  
  // Calculando a concentração de GLP com base na tensão medida
  float glpConcentration = map(voltage, voltageMin, voltageMax, ppmMin, ppmMax); // Mapeia a tensão entre voltageMin e voltageMax para uma concentração de ppmMin a ppmMax
  
  // Exibindo a tensão lida e a concentração de GLP no monitor serial
  Serial.print("Tensao: ");
  Serial.print(voltage);
  Serial.println(" V");
  Serial.print("Concentracao de GLP: ");
  Serial.print(glpConcentration);
  Serial.println(" PPM");
  
  // Exibindo a concentração de GLP no display OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Conc. de GLP:");
  display.setCursor(0, 10);
  display.print(glpConcentration);
  display.print(" PPM");
  display.display();
  
  // Verifica se a concentração de GLP ultrapassou o limite
  if (glpConcentration > ppmThreshold) {
    // Se ultrapassar o limite, aciona o buzzer
    digitalWrite(pinBuzzer, HIGH);
  } else {
    // Caso contrário, desativa o buzzer
    digitalWrite(pinBuzzer, LOW);
  }
  
  // Aguarda um segundo antes da próxima leitura
  delay(1000);
}
