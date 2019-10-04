
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <BH1750FVI.h>
// Create the Lightsensor instance
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
#define DEBUG 1
#define LED 2 


const char *ssid = "PennovationMember-2.4G";     //wifiname
const char *password = "Ideas@Work18"; //wifipsaaword
//const char ssid[]     = "iPhone";         //  -- Please use your own wifi ssid
//const char password[] = "12345678";     //   -- Please use your own wifi password
const char OneNetServer[] = "api.heclouds.com";
const char APIKEY[] = "ZXBorQQ1U8XUexVthAsRPfoF9JI=";    //  -- Please use your own API KEY
int32_t DeviceId = 527198915;                             //  -- Please use your own device ID
const char DS_light[] = "light";                        // - Stream "LIGHT"
char DataStreams[] = "LIGHT";                //  -- Stream "LIGHT"
const size_t MAX_CONTENT_SIZE = 1024;                  // -- Maximum content size
const unsigned long HTTP_TIMEOUT = 2100;                // - Timeout

int stream1;                           //-- Saving the returned value for "LIGHT"
float lux;                           //-- Saving the temperature value of LUX
WiFiClient client;
const int tcpPort = 80;
//DHT dht(DHTPIN, DHTTYPE);



struct UserData 
{ 
    int errno_val;                // - Return error code
    char error[32];               //  -- Return error information
    int recived_val;             // -- Recived data 
    char udate_at[32];            // - Last time for update
};

// -- Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() 
{
    char endOfHeaders[] = "\r\n\r\n";
    client.setTimeout(HTTP_TIMEOUT);
    bool ok = client.find(endOfHeaders);
    if (!ok) 
    {
      Serial.println("No response or invalid response!"); //未响应 -- No response
    }
    return ok;
}
//
// -- Read the body of the response from the HTTP server
void readReponseContent(char* content, size_t maxSize) 
{
    //  size_t length = client.peekBytes(content, maxSize);
    size_t length = client.readBytes(content, maxSize);
    delay(20);
    Serial.println(length);
    Serial.println("Get the data from Internet!"); // -- Get the data
    content[length] = 0;
    Serial.println(content);
    Serial.println("Read Over!");
}
//
//  -- Save data to userData struct
bool parseUserData_test(char* content, struct UserData* userData) 
{
    // -- Compute optimal size of the JSON buffer according to what we need to parse.
    //  -- This is only required if you use StaticJsonBuffer.
    const size_t BUFFER_SIZE = 1024;
    // -- Allocate a temporary memory pool on the stack
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    //  DynamicJsonBuffer jsonBuffer 
    // --If the memory pool is too big for the stack, use this instead:
    //  --DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(content);
    if (!root.success()) 
    {
      Serial.println("JSON parsing failed!");
      return false;
    }
    //  -- Here were copy the strings we're interested in
    userData->errno_val = root["errno"];
    strcpy(userData->error, root["error"]);
    //  -- Number 0 represents successful 
    if ( userData->errno_val == 0 ) 
    {
      userData->recived_val = root["data"]["datastreams"][0]["datapoints"][0]["value"];
      strcpy(userData->udate_at, root["data"]["datastreams"][0]["datapoints"][0]["at"]);
      Serial.print("Recived Value : ");
      Serial.print(userData->recived_val);
      Serial.print("\t The last update time : ");
      Serial.println(userData->udate_at);
    }
    Serial.print("errno : ");
    Serial.print(userData->errno_val);
    Serial.print("\t error : ");
    Serial.println(userData->error);
  
    return true;
}
// -- Read data
int readData(int dId, char dataStream[])
{
    // -- We now create a URI for the request
    String url = "/devices/";
    url += String(dId);
    url += "/datapoints?datastream_id=";
    url += dataStream;

    //  -- We now combine the request to the server
    String send_data = String("GET ") + url + " HTTP/1.1\r\n" +
                     "api-key:" + APIKEY + "\r\n" +
                     "Host:" + OneNetServer + "\r\n" +
                     "Connection: close\r\n\r\n";
    // -- This will send the request to server
    client.print(send_data);
    //  -- The request will be printed if we choose the DEBUG mode
    if (DEBUG)
    {
      Serial.println(send_data);
    }
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 2000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        break;
      }      
    }

    if (skipResponseHeaders())  
    { 
      char response[MAX_CONTENT_SIZE];
      //  -- We now parse the information after we recived the data
      readReponseContent(response, sizeof(response));
      UserData userData_LEDstatus;
      if (parseUserData_test(response, &userData_LEDstatus)) 
      {
        Serial.println("Data parse OK!");
        return userData_LEDstatus.recived_val;
      }
     }
 }
//
//  -- Post data
//
void postData(int dId, float val_t) 
{
    //  -- We now create a URI for the request
    String url = "/devices/";
    url += String(dId);
    url += "/datapoints?type=3";           
    String data = "{\"" + String(DS_light) + "\":" + String(val_t)  + "}";
    // -- We now combine the request to the server
    String post_data = "POST " + url + " HTTP/1.1\r\n" +
                       "api-key:" + APIKEY + "\r\n" +
                       "Host:" + OneNetServer + "\r\n" +
                       "Content-Length: " + String(data.length()) + "\r\n" +                   
                       "Connection: close\r\n\r\n" +
                       data;
  
    //  -- This will send the request to server
    client.print(post_data);
    // -- The request will be printed if we choose the DEBUG mode
    if (DEBUG)
    {
        Serial.println(post_data);  
    }
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 2000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
}

void setup() 
{
    WiFi.mode(WIFI_AP_STA);               // -- set work mode:  WIFI_AP /WIFI_STA /WIFI_AP_STA
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    delay(10);
    void smartConfig();

   // Serial.println("");
    //Serial.print("Trying to connect to ");
   // Serial.println(ssid);
    //  -- We start by connecting to a wifi network
//       WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
     delay(500);
      Serial.print(".");
   }
  
   Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
   Serial.println(WiFi.localIP());
    // 传感器打开 -- We start the DHT11
     LightSensor.begin(); 
}

void loop() 
{
    delay(2000);
    // 默认摄氏度 -- Read temperature as Celsius (the default)
   lux = LightSensor.GetLightIntensity();
    Serial.print("Light: ");
    Serial.println(lux);
     delay(250);
    // -- Check if any reads failed and exit early (to try again).

    //建立连接并判断 -- Connecting to server
    if (!client.connect(OneNetServer, tcpPort)) 
    {
      Serial.println("connection failed");
      return;
    }
    //-- post value
    postData(DeviceId, lux);
    Serial.println("closing connection");
    delay(1000);
    //-- Connecting to server
    if (!client.connect(OneNetServer, tcpPort)) 
    {
      Serial.println("connection failed");
      return;
    }
    //save at stream1 -- get data from server
    stream1=readData(DeviceId, DataStreams);
    analogWrite(LED, stream1);
    Serial.println("closing connection");
}


void smartConfig()
{
  Serial.println("\r\nWait for Smartconfig");
  delay(2000);
  // set net
  WiFi.beginSmartConfig();
 
 while (1)
  {
    Serial.print(".");
    delay(500);
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.setAutoConnect(true);  
      break;
    }
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
