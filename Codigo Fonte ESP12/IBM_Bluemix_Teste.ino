#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DEBUG true
#define VERSAO_FIMEWARE 1
#define PROXIMA_COLETA_SEGUNDOS 900 //15 minutos
const String CHIP_ID = String(ESP.getChipId());

//-------- WIFI
const char* SSID_CLIENT     = "REDE WIFI";
const char* PASSWORD_CLIENT = "SENHA WIFI";
#define  PROXIMA_TENTATIVA_WIFI_SEGUNDOS 60 //1 minnuto

//-------- MQTT e Backend
//-------- example: <organizacao>.messaging.internetofthings.ibmcloud.com
char  HOST_MQTT[] = "URL PLATAFORMA IOT BLUEMIX";
int PORT_MQTT = 1883;

//-------- d: <organizacao>:<tipo_dispositivo>:<id_dispositivo
char ID_DEVICE_MQTT[] = "ID DO DISPOSITIVO COMPOSTO POR 3 INFORMACOES " ;
char USER_MQTT[] = "use-token-auth";
char PASSWORD_MQTT[] = "TOKEN DO DISPOSITIVO";
String topicPublisher = "iot-2/evt/status/fmt/json";

WiFiClient wclient;
PubSubClient mqttClient(wclient);

//-------- Sensores DHT 22
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN_SUPERIOR 14

DHT dht_superior(DHTPIN_SUPERIOR, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  printDebugLN(" ----- SOFTWARE V " + String(VERSAO_FIMEWARE) + " -----");

  printDebugLN("CHIP ID: " + CHIP_ID);
  printDebugLN("Inicializa as configuracoes do sensor ");
  dht_superior.begin();

}



void loop() {
  // put your main code here, to run repeatedly:
  if (conexaoWIFI()) {
    configuraClientMQTT();
    
    String messsagePublish = "{\"d\":{\"device_id\": \"" + CHIP_ID + "\", \"Data\": " + coleta(dht_superior) + " }}";

    printDebugLN("Mensagem enviada");
    printDebugLN(messsagePublish);

    sendBackend(messsagePublish); 

    printDebugLN("DeepSleep" + String(PROXIMA_COLETA_SEGUNDOS * 1000000));
    // deepSleep time definido em microsegundos
    ESP.deepSleep(PROXIMA_COLETA_SEGUNDOS * 1000000);
  }else{
    printDebugLN("DeepSleep de problema WIFI" + String(PROXIMA_TENTATIVA_WIFI_SEGUNDOS * 1000000));
    // deepSleep time definido em microsegundos
    ESP.deepSleep(PROXIMA_COLETA_SEGUNDOS * 1000000);
  }
}


//------------------------------------------------------------------ Logica Wifi

boolean conexaoWIFI() {
  printDebugLN("Inicia processo de conexao com Wifi: " + String(SSID_CLIENT) + " - Senha: " + String(PASSWORD_CLIENT));

  WiFi.begin(SSID_CLIENT, PASSWORD_CLIENT);

  int cont_espera = 0;
  printDebug("Aguarde ");
  while (WiFi.status() != WL_CONNECTED && cont_espera < 100) {
    delay(500);
    Serial.print(".");
    cont_espera++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    printDebugLN("Conectado com a rede Wifi");
    printDebugLN("Endereco IP: " + WiFi.localIP().toString());
    return true;
  } else {
    printDebugLN("Erro ao se conectar com rede Wifi");
    delay(1000);
    return false;
  }
}

//------------------------------------------------------------------ Logica CONFIGURA MQTT

void configuraClientMQTT() {
  mqttClient.setServer(HOST_MQTT, PORT_MQTT);
}

//------------------------------------------------------------------ Envia coletas backend

void sendBackend(String messsagePublish){
  printDebugLN("MQTT INICIO");
  
  if (!mqttClient.connected()) {
    mqttClient.loop();

    
    printDebugLN("Abre conexao");
    if (mqttClient.connect(ID_DEVICE_MQTT, USER_MQTT, PASSWORD_MQTT)) {
      printDebugLN("Envia mensagem");
      mqttClient.publish((const char*)topicPublisher.c_str(), messsagePublish.c_str());
      delay(500);
    }else {
      printDebug("failed, rc=");
      printDebug(String(mqttClient.state()));
      printDebugLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

  } 

    printDebugLN("MQTT FIM");
}

//------------------------------------------------------------------ Logica DHT 22

String coleta(DHT sensor) {
  delay(2000); //tempo recomendado para pela lib para efetuar a proxima medicao

  float coletaTemperatura = sensor.readTemperature();
  float coletaHumidade = sensor.readHumidity();

  if (isnan(coletaTemperatura) || isnan(coletaHumidade)) {
    coletaTemperatura = -10000;
    coletaHumidade = -10000;

    printDebugLN("Falha ao efetuar a leitura do sensor DHT");
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["t"] = coletaTemperatura;
  root["h"] = coletaHumidade;
  
  String valoresColeta = String(coletaTemperatura) + "," + String(coletaHumidade);

  printDebugLN("Coleta " +  valoresColeta);

  String retorno;
  root.printTo(retorno);

  return retorno;
}

//------------------------------------------------------------------ print Debug

void printDebug(String message) {
  if (DEBUG) {
    Serial.print(message);
  }
}

void printDebugLN(String message) {
  if (DEBUG) {
    Serial.println(message);
  }
}