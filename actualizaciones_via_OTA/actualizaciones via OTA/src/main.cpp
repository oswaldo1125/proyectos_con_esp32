//===================================================//
//                  librerias                        //
//===================================================//
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ESPmDNS.h>
//#include <NetworkUdp.h>
#include <BluetoothSerial.h>

//==================================================//
//           definiciones para la pantalla          //
//==================================================//

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//===================================================//
//            declaracino de constanes               //
//===================================================//
//configuraciones para el modo OTA-------------------//
const char* SSDI = ".......";           //nombre de la red 
const char* PASSWORD = "......";       // contrase;a de la red 
const char* HOST_NAME = "Moli_1";         // nombre del dispositivo OTG
uint32_t last_ota_time = 0;

//configuraciones para el bluetooth------------------//
const char* BT_NAME = "Moli_1";         // nombre del dispositivo bluetooth

//configuracion de comandos del monitor serial-------//
const String comando_act = "OTA_ON";   // comando para activar el modo OTA
bool OTA_ACTIVE = false;               // flag para detectar si se esta en modo normal o en modo de carga firmware por OTA

//declaracion de objetos-----------------------------//
BluetoothSerial SerialBT;

//===================================================//
//         Funcion auxiliar de la pantalla           //
//===================================================//
void Mostrar_Pantalla (const char* texto,int x, int y, int j, bool limpiar){
  if (limpiar){
    display.clearDisplay();
  }
  display.setTextSize(j);
  display.setCursor(x,y);
  display.print(texto);
  display.display();
}

//deteccion de la pantalla---------------------------//
bool pantalla_conectada (){
  Wire.beginTransmission(SCREEN_ADDRESS);
  return (Wire.endTransmission() == 0);
}

//===================================================//
//          Configutaciones conexion OTA            //
//===================================================//
void Funcion_OTA (){
  ArduinoOTA.setHostname(HOST_NAME);
  Serial.println("Booting");
  Mostrar_Pantalla("conectando WIFI",19,28,1,true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSDI, PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    Mostrar_Pantalla("conexion wifi fallo", 2,5,1,true);
    Mostrar_Pantalla("Reiniciando...",20,30,1,false);
    delay(5000);
    ESP.restart();
  }

    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } 
      else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      Mostrar_Pantalla("Actualizando",30 ,0,1,true);
      Mostrar_Pantalla("Tipo: ",0 ,32, 1, false);
      display.setCursor(30,32);
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.print(type); // Imprime si es Sketch o FS (Filesystem)
      display.drawRect(0, 17, 128, 10, SSD1306_WHITE);
      display.display(); 
      
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      Mostrar_Pantalla("Actualizacion", 22, 24,1,true);
      Mostrar_Pantalla("exitosa",43, 39,1, false);
      delay(3000);
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        int porcentaje = (progress / (total / 100));
        Serial.printf("Progress: %u%%\n", porcentaje);

        //barra de porcentaje---------------------------------//
        const int BAR_W_TOTAL = 128;
        int progress_width = (BAR_W_TOTAL * porcentaje) / 100;
        display.fillRect(1, 18, BAR_W_TOTAL - 2, 8, SSD1306_BLACK);
        display.fillRect(1, 18, progress_width - 2, 8, SSD1306_WHITE);
        char buffer[6]; 
        sprintf(buffer, "%d%%", porcentaje);
        display.fillRect(50, 45, 30, 8, SSD1306_BLACK);
        
        // Mostramos el nuevo porcentaje----------------------//
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(60, 45); // Ajusta X=50 para que quede centrado
        display.print(buffer);
        display.display();
        last_ota_time = millis();
      }
    })

    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        Mostrar_Pantalla("Error de OTA",28 ,0, 1, true);
        Mostrar_Pantalla("Error de autentificacion", 0, 24, 1, false);
      } 
      else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        Mostrar_Pantalla("Error de OTA",28 ,0, 1, true);
        Mostrar_Pantalla("Error de inicio", 0, 24, 1, false);
      } 
      else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        Mostrar_Pantalla("Error de OTA",28 ,0, 1, true);
        Mostrar_Pantalla("Error de conexion", 0, 24, 1, false);
      } 
      else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        Mostrar_Pantalla("Error de OTA",28 ,0, 1, true);
        Mostrar_Pantalla("Error de recepcion", 0, 24, 1, false);
      } 
      else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        Mostrar_Pantalla("Error de OTA",28 ,0, 1, true);
        Mostrar_Pantalla("Error de finalizado", 0, 24, 1, false);
      }
    });

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//===================================================//
//            bucle de configuracion                 //
//===================================================//
void setup () {
  //inicio de la comunicacion serial-----------------//
  Serial.begin(115200);
  SerialBT.begin(BT_NAME);

  //comprobar conecxion de la pantalla---------------//
  Wire.begin();
  if(!pantalla_conectada()){
    Serial.println("Error de conecxion, pantalla no encontrada");
    while(true);
  }

  //inicio de la libreria----------------------------//
  if(!display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS)){
    Serial.println("Error de libreria de pantalla");
    while(true);
  }
  Serial.println("Pantalla iniciada correctamente");
  
  //mensaje de inicio (nombre del proyecto)----------//
  display.setTextColor(SSD1306_WHITE);
  Mostrar_Pantalla("Holi Moli",10,24,2,true);
  delay(2000);
  Funcion_OTA();
}

//==================================================//
//        envio del comando inicio OTA              //
//==================================================//
void SEND_COMMAND () {
  if (Serial.available()){
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando.equalsIgnoreCase(comando_act) && !OTA_ACTIVE){
    ArduinoOTA.begin();
    OTA_ACTIVE = true;
    }
  }

  if(SerialBT.available()){
    String comando = SerialBT.readStringUntil('\n');
    comando.trim();
    if (comando.equalsIgnoreCase(comando_act) && !OTA_ACTIVE){
      ArduinoOTA.begin();
      OTA_ACTIVE = true;
    }
  }
}


//==================================================//
//         codigo de la aplicacion principal        //
//==================================================//
void programa () {
//---------> escribe tu codigo aqui <---------------//
  Mostrar_Pantalla("iniciado 2",0,0,2,true);
  Serial.println("corriendo...");
}

//==================================================//
//                modo seleccionado                 //
//==================================================//
void SELECTED_MOD () {
  if (OTA_ACTIVE){
    Mostrar_Pantalla("OTA activada",28 ,28 ,1, true);
    ArduinoOTA.handle();
    delay(10);
  }
  else {
    programa();
    
    delay(100);
  }
}

//==================================================//
//                 bucle principal                  //
//==================================================// 
void loop(){
  SEND_COMMAND();
  SELECTED_MOD();
}

