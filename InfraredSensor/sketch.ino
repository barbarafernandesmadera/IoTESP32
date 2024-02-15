#include "DHTesp.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define WIFI_SSID "Wokwi-GUEST"       // SSID da rede Wi-Fi
#define WIFI_PASSWORD ""              // Senha da rede Wi-Fi
#define WIFI_CHANNEL 6                // Canal da rede Wi-Fi

const char* mqtt_server = "test.mosquitto.org";  // Endereço do servidor MQTT
char* my_topic_SUB = "FIT/SUB";                   // Tópico MQTT para inscrição
char* my_topic_PUB = "FIT/PUB_Temperatura";       // Tópico MQTT para publicação de temperatura

WiFiClient espClient;               // Cliente Wi-Fi para comunicação
PubSubClient client(espClient);     // Cliente MQTT para comunicação

int contador = 1;                   // Variável de contador
char mensagem[30];                  // Buffer para mensagem MQTT
unsigned long lastMillis = 0;       // Último tempo em milissegundos

const int DHT_PIN = 15;             // Pino ao qual o sensor DHT22 está conectado
DHTesp dhtSensor;                   // Objeto para interação com o sensor DHT22
float lastTemp = -999.0;            // Última temperatura medida

int PIR_SENSOR_PIN = 35;            // Pino ao qual o sensor PIR está conectado
int PIR_SENSOR_STATUS = 0;          // Estado atual do sensor PIR
int previousStatus = -1;            // Estado anterior do sensor PIR
unsigned long lastChangeTime = 0;   // Última mudança de estado do sensor PIR
int led = 12;                       // Pino do LED

void setup() {
  Serial.begin(115200);             // Inicia comunicação serial
  Serial.println("Olá, ESP32!");

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);  // Inicializa o sensor DHT22

  conectawifi();                    // Conecta-se à rede Wi-Fi
  client.setServer(mqtt_server, 1883);      // Configura o servidor MQTT e porta
  client.setCallback(callback);     // Define a função de retorno de chamada MQTT
}

void loop() {
  Sensor_movimento();               // Verifica o sensor de movimento

  if (!client.connected()) {        // Verifica se está conectado ao servidor MQTT
    reconnect();                    // Reconecta-se se não estiver conectado
  }
  client.loop();                    // Mantém a conexão MQTT
  delay(100);

  if (millis() - lastMillis > 15000) {  // Envia a temperatura a cada 15 segundos
    lastMillis = millis();
    float temperature = dhtSensor.getTemperature();  // Lê a temperatura do sensor DHT22
    if (!isnan(temperature)) {      // Verifica se a leitura da temperatura é válida
      int val_int = (int)temperature;     // Parte inteira da temperatura
      float val_float = (abs(temperature) - abs(val_int)) * 100000;  // Parte decimal
      int val_fra = (int)val_float;        // Valor decimal como inteiro
      sprintf(mensagem, "%d.%d", val_int, val_fra);  // Formata a mensagem MQTT
      Serial.print("Mensagem enviada: ");
      Serial.println(mensagem);
      client.publish(my_topic_PUB, mensagem);  // Publica a temperatura no tópico MQTT
      Serial.print(contador);
      Serial.println("-Temperatura enviada com sucesso...");
      contador++;                   // Incrementa o contador
    } else {
      Serial.println("Falha ao ler temperatura do sensor");  // Mensagem de erro
    }
  }
}

void conectawifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);  // Inicia a conexão Wi-Fi
  Serial.print("Conectando ao WiFi ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {  // Aguarda a conexão ser estabelecida
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());   // Imprime o endereço IP atribuído
}

void callback(char* topic, byte* payload, unsigned int length) {
  String string;
  Serial.print("Chegou a mensagem [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    string += ((char)payload[i]);
  }
  Serial.print(string);              // Imprime a mensagem recebida
}

void Sensor_movimento() {
  PIR_SENSOR_STATUS = digitalRead(PIR_SENSOR_PIN);  // Lê o estado do sensor PIR
  if (PIR_SENSOR_STATUS != previousStatus) {  // Verifica se houve uma mudança de estado
    previousStatus = PIR_SENSOR_STATUS;
    if (PIR_SENSOR_STATUS != 0 ) {  // Verifica se o movimento foi detectado
      Serial.println("Movimento detectado");
      digitalWrite(led, HIGH);      // Liga o LED
      float temperature = dhtSensor.getTemperature();  // Lê a temperatura
      if (!isnan(temperature)) {     // Verifica se a leitura da temperatura é válida
       Serial.print("Temperatura atual: ");
       Serial.println(temperature);
      } else {
       Serial.println("Sem movimento");
       digitalWrite(led, LOW);      // Desliga o LED se não houver movimento
       return; // Retorna se não houver movimento
      }
    }else {
      Serial.println("Falha ao ler temperatura do sensor");  // Mensagem de erro
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT ...");
    if (client.connect("ESPClient")) {  // Tenta se conectar ao servidor MQTT
      Serial.println("Conectado");
      client.subscribe(my_topic_SUB);   // Inscreve-se no tópico MQTT
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tente novamente em 5 segundos");
      delay(5000);                     // Espera 5 segundos antes de tentar novamente
    }
  }
}


