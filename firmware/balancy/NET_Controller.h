#pragma once

class NET_Server;
using FnCallback =void(*)(void);

class NETBaseVariable{
public:
  std::string name;
  NET_Server *controller;
  int index;
  virtual void addTo(DynamicJsonDocument &doc)=0;
};

template<typename  T> 
class NETVariable : public NETBaseVariable
{
public:
  T value;
  void set(T v);
  T get();

  void addTo(DynamicJsonDocument &doc){

    std::vector<std::string> subStrings;
    int j=0;
    for(int i =0; i < name.length(); i++){
      if(name[i] == '.'){
        subStrings.push_back(name.substr(j,i));
        j = i+1;
      }
    }
    subStrings.push_back(name.substr(j,name.length())); //to grab the last value of the string

     if (subStrings.size() == 1)
        doc[name] =value;
    else if (subStrings.size() == 2)
        doc[subStrings[0]][subStrings[1]] =value;
    else if (subStrings.size() == 3)
        doc[subStrings[0]][subStrings[1]][subStrings[2]] =value;
  }
};

//================================================================================

class NET_Server
{
private:

  std::vector<NETBaseVariable*> varList;
  std::vector<NETBaseVariable*> constList;
  int port = 8888;  //Port number
  WiFiClient client;
  String buffer;

public:

  WiFiServer *server;

  NET_Server()
  {
    server = new WiFiServer(port);

  }

  ~NET_Server(){
    delete server;
     for(int i=0;i<varList.size();i++)
      delete varList[i];
     for(int i=0;i<constList.size();i++)
      delete constList[i];
  }
  
  void setup(void) 
  {
    // ACCESS PONT
    /*
    WiFi.mode(WIFI_AP_STA); //ESP8266 works in both AP mode and station mode
      // Starting the access point
  WiFi.softAPConfig(APlocal_IP, APgateway, APsubnet);                 // softAPConfig (local_ip, gateway, subnet)
  WiFi.softAP(ssid, password, 1, 0, MAXSC);                           // WiFi.softAP(ssid, password, channel, hidden, max_connection)
  Serial.println("WIFI < " + String(ssid) + " > ... Started");
  
  // wait a bit
  delay(50);

  // getting server IP
  IPAddress IP = WiFi.softAPIP();
    // printing the server IP address
  Serial.print("AccessPoint IP : ");
  Serial.println(IP);

*/
  WiFi.mode(WIFI_STA); 
    WiFi.begin(ssid, password); //Connect to wifi

    // Wait for connection  
    Serial.println("Connecting to Wifi");
    while (WiFi.status() != WL_CONNECTED) {   
      delay(500);
      Serial.print(".");
      delay(500);
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  

  }

  void start(void) {
   // Start server
    server->begin();
    Serial.print("Open Telnet and connect to IP:");
    Serial.print(WiFi.localIP());
    Serial.print(" on port ");
    Serial.println(port);
  }

  template<typename  T> 
  NETVariable<T>&  addVar(const std::string  &name,T startValue){
   // variables[name] = startValue;
    NETVariable<T> *var =  new NETVariable<T> ();
    var->controller=this;
    var->name = name;
    var->index =  varList.size();
    var->value = startValue;
    varList.push_back(var);
    return *var;
  }
  /*
  template<typename  T> 
  void  setVar(  NETVariable<T> &var,T val){
      set(var.name,val);
  }
  */

  template<typename  T> 
    NETVariable<T>&  setConstant(  const std::string  &name,T val){
     NETVariable<T> *var =  new NETVariable<T> ();
    var->controller=this;
    var->name = name;
    var->index =  constList.size();
    var->value = val;
    constList.push_back(var);
    return *var;
  }

  void  addListener( const std::string name , FnCallback handler){
  
  }

  void sendOK(){
  }

  void sendKO(const String &msg){
  }

// ===================

  String onProcess(const String &uri)
  {
      if (uri.startsWith("/dump"))
      {
          DynamicJsonDocument doc(1024);
          for(int i=0;i<varList.size();i++){
            varList[i]->addTo (doc);//doc[varList[i].name] = varList[i].get();
          }
         for(int i=0;i<constList.size();i++){
            constList[i]->addTo (doc);//doc[varList[i].name] = varList[i].get();
          }
          String buf;
          serializeJson(doc, buf);

          //Serial.println(buf);
          return buf;
      }
      return "";
  }

/// format: ID.message
  void onReceive(const String &cmd){
    if (cmd[0] == '/')
    {
      //client.print("ACK\n");
    //  client.flush();
        return;
    }

    int idx = cmd.indexOf(".");
    String ID = cmd.substring(0,idx);
    String msg = cmd.substring(idx+1);
    
    Serial.print("<< ");
    Serial.print(ID);
    Serial.print(" ");
    Serial.println(msg.c_str());

    String result=onProcess(msg);
    //Send(">"+ID+"."+result+"\n");

      client.print(">"+ID+"."+result+"\n");
        client.flush();
  }

  void Send(const String &msg)
  {
    int n = msg.length() / 32;
     for(int i=0;i<n;i++)
     {
        client.print(msg.substring(i*32, i+1 * 32));
        client.flush();
     }
     int r = msg.length() - n * 32;
     if (r>0)
     {
        client.print(msg.substring(n*32));
        client.flush();
     }
  }
  
  // ====================================

  void tick(float dt) 
  { 
    size_t size;

    if (!client)
      client = server->available();

    if (client)
    {
      if(client.available()>0)
      {
          while((size = client.available()) > 0)
          {
            // String request = client.readStringUntil('\n');

            uint8_t* msg = (uint8_t*)malloc(size);
            size = client.read(msg,size);
            for(int i=0;i<size;i++)
            {
              buffer+= (char)msg[i];
             // Serial.print((char)msg[i]);
              if ((char)msg[i] == '\n')
              {
                onReceive(buffer);
                buffer="";
              }
            }

          //  Serial.write(msg,size);
          //  free(msg);
           // Serial.println("");
            
          }
          //client.print("ACK\n");
      }
      // Serial.println("STOP");
    //  client.stop();
    }

    return;

    if (!client)
      client = server->available();
    if (!client)
    {
      return;
    }

   // Serial.println("tick");

    
      if(client.connected())
      {
        Serial.println("Client Connected");
      }
     
      
  if (client.available()) 
   {
    String request = client.readStringUntil('\n');
    Serial.println(request);

  /*
    if (request.indexOf("/Rolloverbindung=OFF") != -1)
    {
      Serial.println("Rolloverbindung Aus");
      rolloverbindung=false;
    }
    */

  }

  }

};
// ==========


//==========
 template<typename  T> 
 T NETVariable<T>::get(){
     return this->value;
  }

template<typename  T> 
 void NETVariable<T>::set(T v){
    value = v;
    //controller->setVar<T>(*this,v);
  }
