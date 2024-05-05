#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DHT11.h>
#include <FS.h> // dosya sistemi kütüphanesi
#include <ArduinoJson.h> // JSON 

#define selection_pin_A 2
#define selection_pin_B 3
#define selection_pin_C 4

// network ayarları
const char* ssid = "Kablonet Netmaster-ED18-G_EXT";
const char* password = "f7e47cd2";
const char* server = "192.168.0.102";
const int port = 8080;
const char* jsonFilePath = "/data.json"; // JSON dosyasına yazmak için gerekli path

// hava sıcaklığı ve nem için tanımlamalar
DHT11 dht11(0);
int temperature = -99;
int humidity = -99;

// solar panel ve batarya akımları için tanımlamalar
double bat_current=0.0;
double lipo_current=0.0;
int RawValue = 0;
int ACSoffset = 2500;
double Voltage=0.0;
int mVperAmp =185; // sensor datasheetinin önerdiği değer 
// multiplexer içinde kanal seçimi için gerekli kısım
int selection=0; 
const int sensorCount = 2; // Bağlı sensör sayısı
const int sensorPin = A0; // ADC pini




void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  pinMode(selection_pin_A, OUTPUT);
  pinMode(selection_pin_B, OUTPUT);
  pinMode(selection_pin_C, OUTPUT);

  delay(10);
  // Bağlantıyı başlat
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS dosya sistemi başlatılamadı!");
    return;
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi bağlandı!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  int result = dht11.readTemperatureHumidity(temperature, humidity);
  for (int i = 0; i < sensorCount; ++i) {
    // İlgili sensörü seç
    selectSensor(i);
    if(i==1){
      bat_current = measurementOfCurrent();
    }
    else{
      lipo_current=measurementOfCurrent();
    }
    // Bir sonraki sensöre geçmeden önce kısa bir bekleme yapabilirsiniz
    delay(500);
  }

  writeFileToJSON(temperature, humidity,bat_current,lipo_current);

  
  sendPOSTRequest();// Sunucuya POST isteği gönderme

  delay(60000); // Her 1 dakikada bir tekrar et

}
void selectSensor(int sensorIndex) {
  switch (sensorIndex) {
    case 0:
      digitalWrite(selection_pin_A, HIGH);
      digitalWrite(selection_pin_B, LOW);
      digitalWrite(selection_pin_C, LOW);


      break;
    case 1:
      digitalWrite(selection_pin_A, LOW);
      digitalWrite(selection_pin_B, HIGH);
      digitalWrite(selection_pin_C, LOW);
      break;
    // Ek sensörler için gerekirse case'ler ekleyebilirsiniz
  }
}
double measurementOfCurrent(){
  RawValue = analogRead(A0);//MODUL ANALOG DEĞERI OKUNUYOR

  Voltage = (RawValue / 1024.0) * 3300; // VOLT HESABI YAPILIYOR

  double current = ((Voltage - ACSoffset) / mVperAmp); // AKIM HESAPLA

  return current;
}

void writeFileToJSON(int temperatureValue, int humidityValue,double lipo_current,double bat_current ) {
  // JSON nesnesi oluştur
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["sensor"] = "DHT11";
  jsonDoc["temperature"] = temperatureValue;
  jsonDoc["humidity"] = humidityValue;
  jsonDoc["Battery Current"] = lipo_current;
  jsonDoc["Lipo Charge Current"] = bat_current;
  // JSON nesnesini dosyaya yaz
  File jsonFile = SPIFFS.open(jsonFilePath, "w");
  if (!jsonFile) {
    Serial.println("JSON dosyası açılamadı!");
    return;
  }

  serializeJson(jsonDoc, jsonFile);
  jsonFile.close();
}

void sendPOSTRequest() {
  // WiFi istemcisini oluştur
  WiFiClient client;
  if (!client.connect(server, port)) {
    Serial.println("Bağlantı hatası");
    return;
  }

  // JSON dosyasını oku
  File jsonFile = SPIFFS.open(jsonFilePath, "r");
  if (!jsonFile) {
    Serial.println("JSON dosyası açılamadı!");
    return;
  }

  String jsonPayload = jsonFile.readString();
  jsonFile.close();

  // POST isteği oluştur
  client.println("POST /your-endpoint HTTP/1.1");
  client.println("Host: " + String(server));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonPayload.length());
  client.println();
  client.println(jsonPayload);

  // Yanıtı oku
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("Bağlantı kapatılıyor");
  client.stop();
}