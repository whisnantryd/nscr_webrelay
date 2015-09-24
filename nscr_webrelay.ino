#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include <Adafruit_GPS.h>


// GPS settings++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
SoftwareSerial mySerial(3, 2);
Adafruit_GPS GPS(&mySerial);
boolean usingInterrupt = false;

#define GPSECHO  true
// --------------------------------------------------------------------------------------------


// Pin relay settings++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const int pins[] = { 5, 6, 7, 8, 9 };
const int pincount = 4;

bool pinstates[10]; // initialize all states to off
unsigned long ontimes[10]; // keep track of the last time each pin was turned on
int durations[10]; // the on duration last set for each pin
// --------------------------------------------------------------------------------------------


// Ethernet settings ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MAC address and IP address for the controller below
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x18, 0x85 };
char macstr[] = "90-A2-DA-0D-18-85";

IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Initialize the Ethernet server library (port 80 is default for HTTP)
EthernetServer server(80);

// Connect to the stream from the flag box
EthernetClient streamclient;
IPAddress host(10, 220, 10, 13);
char ipstr[] = "";

int port = 50002;
bool isconnected = 0;
// --------------------------------------------------------------------------------------------

void setupGPS() {
  // initialize gps
  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);

  // set the verbosity
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);

  // set the refresh rtate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_100_MILLIHERTZ);

  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);
}

void setupGpio() {
  // initialize pins for output
  for(int pin = 0; pin < pincount; pin++)
  {
    pinMode(pins[pin], OUTPUT);
  }
}

void setupEthernet() {
  // start the Ethernet connection and the server
  // try to use DHCP
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // initialize the ethernet device not using DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }
  
  // start the server
  server.begin();
  ip = Ethernet.localIP();
  Serial.print("server is at ");
  Serial.println(ip);
}

void setupFlagBoxConnection() {
  // connect to stream
  if(streamclient.connect(host, port)) {
    isconnected = 1;
    Serial.println("flag box connected");
  }
  else {
    Serial.println("flag box connection failed");
  }
}

void setup() {
  // Open serial communications and wait for port to open
  Serial.begin(115200);
  while (!Serial) ;

  Serial.print("Arduino value: ");
  Serial.println(ARDUINO);

  setupGPS();
  setupGpio();
  //setupEthernet();
  //setupFlagBoxConnection();// don't connect for now... enable for production  
}

bool isvalidpin(unsigned int num) {
  for(int pin = 0; pin < pincount; pin++)
  {
    if(pins[pin] == num) return 1;
  }

  return 0;
}

bool setPinState(unsigned int num, bool on, unsigned int dur) {
  if(!isvalidpin(num)) return 0;
  
  // turn the specified pin number on for 
  // the duration or off
  if(on)
  {
    digitalWrite(num, HIGH);
    pinstates[num] = (bool)1;
    ontimes[num] = millis();
    durations[num] = dur;
  }
  else
  {
    digitalWrite(num, LOW);
    pinstates[num] = (bool)0;
  }
}

bool parseUrl(String url) {
  if(url.startsWith("/relay?"))
  {
    url.remove(0, 7);
    int len = url.length();
    
    char mod[len];

    url.toCharArray(mod, len);
    
    char* spl = strtok(mod, "&");

    int num = 1;
    int on = 1;
    int dur = 10000;

    while(spl)
    {
      String params = String(spl);
      int eq = params.indexOf('=');

      String key = params.substring(0, eq);
      String val = params.substring(eq + 1);

      int c_val = val.toInt();
      
      if(key == "num") { num = c_val; }
      if(key == "on") { on = (bool)c_val; }
      if(key == "dur") { dur = (c_val * 1000); }

      spl = strtok(0, "&");
    }

    setPinState(num, on, dur);

    return 1;
  }

  return 0;
}

void acceptClient() {
  // Listen for incoming clients
  EthernetClient client = server.available();
  
  if (client && client.connected())
  {
    char curr;
    
    String request = "";
    String verb;
    String url;
    
    int spc = 0;

    // read all data from client request
    while (client.available())
    {
      curr = client.read();

      // parse only the verb ('GET') and
      // resource string ('/some/resource')
      if(spc < 3) {
        request.concat(curr);

        if(curr == ' ')
        {          
          if(++spc == 1)
          {
            verb = request;
            request = "";
          }
          else if(spc == 2)
          {
            url = request;
          }
        }
      }
    }

    //Serial.println(verb + " -- " + url);

    if(parseUrl(url))
    {
      // valid request, send ok
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.print("<html><div id='name'>NASCAR WebRelay</div>");
      client.print("<div id='version'>0.0</div>");
      client.print("<div id='ip'>");
      client.print(ip);
      client.print("</div>");
      client.print("<div id='mac'>");
      client.print(macstr);
      client.print("</div>");
      client.print("</html>");
    }
    else
    {
      // invalid request, send 404
      client.println("HTTP/1.1 404 Not Found");
      client.println("Connection: close");
      client.println();
    }
    
    // close the connection:
    client.stop();
  }
}

void readSocket() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (streamclient.available())
  {
    char c = streamclient.read();
    Serial.print(c);
  }

  // if the server's disconnected, stop the client:
  if (isconnected && !streamclient.connected())
  {
    isconnected = 0;
    Serial.println();
    Serial.println("disconnecting.");
    streamclient.stop();
  }
}

void checkStaleRelays() {
  for(int i = 0; i < pincount; i++)
  {
    int num = pins[i];
    
    if(pinstates[num])
    {
      unsigned long currms = millis();
      unsigned long ontime = ontimes[num];
      unsigned long et = (currms - ontime);

      int dur = durations[num];
      
      if(et >= dur)
      {
        setPinState(num, 0, 0);
        Serial.println("pin " + String(num) + " timed-out");
      }
    }
  }
}

void readGps() {
  GPS.read();
  
  if (GPS.newNMEAreceived())
  {
    char* last = GPS.lastNMEA();

    Serial.println(last);
    
    if (GPS.parse(last))
    {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4);
      Serial.print(GPS.lat);
      Serial.print(", "); 
      Serial.print(GPS.longitude, 4);
      Serial.println(GPS.lon);
      Serial.print("Altitude: ");
      Serial.println(GPS.altitude);
    }
  }
}

// main loop
void loop()
{
  readGps();
  return;
  
  // neither of these functions block
  // so we continue to loop while listening
  // for clients and reading data from stream
  
  acceptClient();
  readSocket();
  checkStaleRelays();
}

