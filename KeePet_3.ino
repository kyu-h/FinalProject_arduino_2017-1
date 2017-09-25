#include <ESP8266WiFi.h>
#include <Wire.h>
#define myPeriodic 15 //in sec | Thingspeak pub is 15sec
#define TMP102_ADDR 72 //슬레이브주소
#define TMP102_REG_TEMP 0 //레지스터주소 : 데이터가위치하는곳의 주소, 저장된 레지스터주소가 0
#define myPeriodic 15 //in sec | Thingspeak pub is 15sec
#define addr 0x1E //I2C Address for The HMC5883

IPAddress hostIp(192, 168, 219, 177);
const char* server = "api.thingspeak.com";
String apiKey = "U6F92U13BJ8WQ9HC";
const char* ssid = "U+NetDB63";
const char* password = "4000017904";

int sent = 0;
int p = 4;
int ledPin = D6; // GPIO12 D6
WiFiServer port(81);
WiFiClient client;
int status = WL_IDLE_STATUS;

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 5000L;
//입력변수//
char trans_str;
String rcvbuf;
boolean getIsConnected = false;
String transdata = "";

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(p, OUTPUT);
  digitalWrite(p, HIGH);
  connectWifi();

}

void loop() {
  digitalWrite(p, LOW);
 // delay(100);
  digitalWrite(p, HIGH);
  
  // Check if a client has connected
  WiFiClient client = port.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println("-----------------");
  Serial.println(request);
  Serial.println("-----------------");
  client.flush();

  // Match the request
  int value = LOW;
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
//    delay(3000);
    digitalWrite(ledPin, LOW);
    value = LOW;
  }
  if (request.indexOf("/LED=OFF") != -1) {
    digitalWrite(ledPin, LOW);
    value = LOW;
  }

  // Set ledPin according to the request
  //digitalWrite(ledPin, value);

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.print("GET /led.php?LED=");
  client.println("Content-Type: text/html");
  client.println(""); // do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Led pin is now: ");

  if (value == HIGH) {
    client.print("On");
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("<a href=\"/LED=ON\"><button>ON</button></a>&nbsp;<a href=\"/LED=OFF\"><button>OFF</button></a></p>");
  client.println("</html>");

  delay(1000);
  Serial.println("Client disonnected");
  Serial.println("");

  get_compass();
  float temp = getTemperature();
  Serial.print("Celsius: ");
  Serial.println(temp);

  sendTemp(temp);
  int count = myPeriodic;
  while (count--)
    delay(1000);
}

float getTemperature() {
  Wire.requestFrom(TMP102_ADDR , 2);

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4;

  float celsius = TemperatureSum * 0.0625;
  return celsius;
}

void connectWifi()
{
  Serial.print("Connecting to " + *ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected");
  Serial.println("");
  // Start the server
  port.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}//end connect


void sendTemp(float temp)
{

  if (client.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
    Serial.println("WiFi Client connected ");

    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temp);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);

  }//end if
  sent++;
  client.stop();
}//end send

void httpRequest_compass(int x, int y, int z) {
  Serial.println();

  // close any connection before send a new request
  // this will free the socket on the WiFi shield
  //client.stop();

 // delay(1000);

  // if there's a successful connection
  if (client.connect(hostIp, 80)) {
    Serial.println("Connecting...");


    // We now create a URI for the request
    String url = "/";
    url += "pet_compass.php";
    url += "?x="; url += x;
    url += "&y=";  url += y;
    url += "&z=";  url += z;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + hostIp + "\r\n" +
                 "Connection: close\r\n\r\n");

    int timeout = millis() + 5000;
    while (client.available() == 0) {
      if (timeout - millis() < 0) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    // note the time that the connection was made
    lastConnectionTime = millis();
    getIsConnected = true;
  } else {
    // if you couldn't make a connection
    Serial.println("Connection failed");
    getIsConnected = false;
  }

  //서버로 부터 값을 받는다.//
  //No Socket available문제 해결//
  while (client.available() && status == WL_CONNECTED) {
    char c = client.read();
    if ( c != NULL ) {
      if (rcvbuf.length() > 20)
        rcvbuf = "";
      rcvbuf += c;
      Serial.write(c);

      rcvbuf = "";

      client.flush();
      client.stop();
    }
  }
}
void get_compass()
{
  int x, y, z; //triple axis data
  //Tell the HMC what regist to begin writing data into
  Wire.beginTransmission(addr);
  Wire.write(0x03); //start with register 3.
  Wire.endTransmission();

  //Read the data.. 2 bytes for each axis.. 6 total bytes
  Wire.requestFrom(addr, 6);
  if (6 <= Wire.available()) {
    x = Wire.read() << 8; //MSB  x
    x |= Wire.read(); //LSB  x
    z = Wire.read() << 8; //MSB  z
    z |= Wire.read(); //LSB z
    y = Wire.read() << 8; //MSB y
    y |= Wire.read(); //LSB y
  }

  httpRequest_compass(x, y, z);

//  delay(10000);
}

