#include <SPI.h>
#include <Ethernet.h>

int led = 13;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Connect to the stream from the flag box
EthernetClient streamclient;
IPAddress host(10, 220, 10, 13);
int port = 50002;
bool isconnected = 0;

void setup() {
  // Open serial communications and wait for port to open
  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  pinMode(led, OUTPUT);

  // start the Ethernet connection and the server
  // try to use DHCP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    
    // initialize the ethernet device not using DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  return; // don't connect for now... enable for production
  // connect to stream
  if(streamclient.connect(host, port)) {
    isconnected = 1;
    Serial.println("Stream connected");
  }
  else {
    Serial.println("Stream connection failed");
  }
}

// split a string by a delimiter
// currently not used
char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // Count how many elements will be extracted
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    // Add space for trailing token
    count += last_comma < (a_str + strlen(a_str) - 1);

    // Add space for terminating null string so caller
    // knows where the list of returned strings ends
    count++;

    result = (char**)malloc(sizeof(char) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        
        *(result + idx) = 0;
    }

    return result;
}

void parseUrl(String url)
{
  String key = "";
  
  for(int i = 0; i < url.length(); i++)
  {
    char curr = url[i];

    switch(curr)
    {
      
    }
  }
}

void acceptClient()
{
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

    parseUrl(url);
    
    Serial.println(verb + " -- " + url);

    // send a standard http response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print("<html><b>NASCAR WebRelay v0.0</b></html>");

    // close the connection:
    client.stop();
  }
}

void readSocket()
{
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

// main loop
void loop()
{
  // neither of these functions block
  // so we continue to loop while listening
  // for clients and reading data from stream
  
  acceptClient();
  readSocket();
}

