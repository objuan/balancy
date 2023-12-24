#pragma once

// #define DEBUG_NET

ESP8266WebServer server(80);
class WIFI_Controller;

class BaseVariable{
public:
  std::string name;
  WIFI_Controller *controller;
  int index;
};

template<typename  T> 
class Variable : public BaseVariable
{
public:

  T value;

  void set(T v);
  T get();
};

// =====================================================

class WIFI_Controller
{
  DynamicJsonDocument variables;

  std::vector<BaseVariable*> varList;
  JsonArray cmds;

public:
  WIFI_Controller() : variables(DynamicJsonDocument(1024))
  {
   /*  JsonObject root = variables.to<JsonObject>();
    cmds = root.createNestedArray("cmds");
    cmds.add("pp");
    */
     cmds = variables.createNestedArray("cmds");
  }

  ~WIFI_Controller()
  {
    for(int i=0;i<varList.size();i++)
      delete varList[i];
  }

  template<typename  T> 
  Variable<T>&  addVar(const std::string  &name,T startValue){
   // variables[name] = startValue;
    Variable<T> *var =  new Variable<T> ();
    var->controller=this;
    var->name = name;
    var->index =  varList.size();
    var->value = startValue;
    varList.push_back(var);

    set(name,startValue);
    return *var;
  }

  template<typename  T> 
  void  setVar(  Variable<T> &var,T val){
    //Serial.println(var.name);
    // Serial.println(val);
      set(var.name,val);
  }
  

  template<typename  T> 
  T getVar(  const std::string  &name){
    std::string s = variables[name];
    Serial.println(name.c_str());
    Serial.println(s.c_str());
      return  variables[name].as<T>();
  }


  template<typename  T> 
  void set(  const std::string  &name,T val){
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
        variables[name] =val;
    else if (subStrings.size() == 2)
        variables[subStrings[0]][subStrings[1]] =val;
    else if (subStrings.size() == 3)
        variables[subStrings[0]][subStrings[1]][subStrings[2]] =val;
  }

  using FnCallback =void(*)(void);


  void  addListener( const std::string name , FnCallback handler){
    //FnCallback handler1 = handler;
    server.on( ("/"+name).c_str(),handler);
    /*
     server.on( ("/"+name).c_str(),
      [&] {
        handler1();
        server.send ( 200, "text/json", "{\"success\":true}" );
      });
      */
    // set("commands."+name,"");
    cmds.add(name);
  }
  void  addListener( const std::string  &name,HTTPMethod method, std::function<void(void)> fn){
     server.on( ("/"+name).c_str(),method,fn);
      cmds.add(name);
  }
  /*
  void  addListener( const std::string  &name,HTTPMethod method, std::function<void(void)> fn, std::function<void(void)> ufn){
     server.on(name,method,fn,ufn);
  }
  */

  // ===============================================
  void sendResponse(DynamicJsonDocument &doc)
  {
        String buf;
        serializeJson(doc, buf);
        server.send(200, F("application/json"), buf);
  }

  void sendOK( )
 {
         server.send ( 200, "text/json", "{\"success\":true}" );
  }

  void sendKO( const std::string &msg)
 {
         server.send ( 200, "text/json", "{\"success\":false}" );
  }

  void ping() {
    Serial.print(F("ping"));
    server.send ( 200, "text/json", "{\"success\":true}" );
  }

  void dump() {
    sendResponse(variables);
  }
  void vars() {
    sendResponse(variables);
  }

   void readVar() {
    String name = server.arg("name");
    Serial.print(F("readVar"));
    DynamicJsonDocument doc(512);
    doc[name] = variables[name];

    sendResponse(doc);
  }
  
  void writeVar() {
     std::string name = server.arg("name").c_str();
     String val = server.arg("val");
     String type = server.arg("type");

     #ifdef  DEBUG_NET
     Serial.println(F("writeVar"));
     Serial.println(name.c_str());
     Serial.println(val);
     Serial.println(type);
     #endif

      int idx=-1;
      for(int i=0;i<varList.size();i++)
        if (varList[i]->name == name)
        {
          idx=i;
          break;
        }
      if(idx==-1)
      {
         Serial.println( (name+ " not found").c_str());
        sendKO(name+ " not found");
      }
      else
      {  
        #ifdef  DEBUG_NET
        Serial.print("idx ");
        Serial.println(idx);
        #endif

        if (type == "B") 
        {
          set<bool>(name.c_str(),val=="true");
          Variable<bool> &b = (Variable<bool> &)*varList[idx];

            Serial.println( b.value);

          b.value = val=="true";
          Serial.println( b.value);
        }
        else  if (type == "I") 
        {
          int v = val.toInt();
          set<int>(name.c_str(), v);
          Variable<int> &b = (Variable<int> &)*varList[idx];
          b.value = v;
        }
      else  if (type == "F  ") 
          set<float>(name.c_str(),val.toFloat());
        else
          set<std::string>(name.c_str(),val.c_str());
      }

      sendOK();
  }

  // Manage not found URL
  void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
  void drive() {
 //  Serial.print(F("setSumo.."));
   String steering = server.arg("steering");
   String speed = server.arg("speed");
   if (steering !="")
      set("driver.steering", steering.toInt());
  if (speed !="")
      set("driver.speed", speed.toInt());
   server.send ( 200, "text/json", "{\"success\":true}" );
}

  // Define routing
  void restServerRouting() {
      WIFI_Controller *self = this;
      server.on("/", HTTP_GET, []() {
          server.send(200, F("text/html"),
              F("Welcome to the SUMO REST Web Server"));
      });
      server.on(F("/ping"), HTTP_GET, [&,self]() {
        self->ping();
      });

      server.on(F("/read"), HTTP_GET, [&,self]() {
        self->readVar();
      });

      server.on(F("/write"), HTTP_GET, [&,self]() {
        self->writeVar();
      });

      server.on(F("/dump"), HTTP_GET, [&,self]() {
        self->dump();
      });
    
      server.on(F("/vars"), HTTP_GET, [&,self]() {
        self->vars();
      });
    
      server.on(F("/drive"), HTTP_GET, [&,self]() {
        self->drive();
      });
      /*
      server.on(F("/mode"), HTTP_GET, setMode);
      server.on(F("/dump"), HTTP_GET, dump);
      server.on(F("/motor/set"), HTTP_GET, setMotor);
      server.on(F("/motor/break"), HTTP_GET, setBreak);
      server.on(F("/sumo/set"), HTTP_GET, setSumo);
      */
  }
    
  void setup(void) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
  
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    // Activate mDNS this is used to be able to connect to the server
    // with local DNS hostmane esp8266.local
    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }
    // Set server routing
    restServerRouting();
     WIFI_Controller *self = this;
    // Set not found response
    server.onNotFound( [&,self]() {
        self->handleNotFound();
      });

  }
  void start(void) {
        // Start server
    server.begin();
    Serial.println("HTTP server started");
  }
  
};

//==========
 template<typename  T> 
 T Variable<T>::get(){
     return this->value;
  }

  template<typename  T> 
 void Variable<T>::set(T v){

    controller->setVar<T>(*this,v);
  }