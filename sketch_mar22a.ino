#include <WiFi.h>//Biblioteca que gerencia o WiFi.
#include <WiFiServer.h>//Biblioteca que gerencia o uso do TCP.
#include <TimeLib.h>
#include <WiFiUDP.h>//Biblioteca do UDP.
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 4); // RX, TX //conexao serial virtual

//Conectar o pino 2(rx)(usar o divisor de tensao) no pino tx(9) do arduino
//Conectar o pino 4(tx) no pino rx(8) do arduino
WiFiUDP Udp;//Cria um objeto "UDP".

WiFiServer servidor(80);
IPAddress ip(192,168,85,115);
IPAddress roteador(192,168,85,1);
IPAddress mascara(255,255,255,0);
IPAddress dns(8,8,8,8);

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = -3;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
String hora;//Váriavel que armazenara o horario do NTP.



 String horas, minutos, segundos ,dia, mes, ano;
  


int led = 2;

const char* ssid   = "Casa nova familia lazarini";
const char* password  = "Alineerik";
String formattedDate;

void setup() {
  // put your setup code here, to run once:
pinMode(led, OUTPUT);
digitalWrite(led, HIGH);

mySerial.begin(9600);
 Serial.begin(9600);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, roteador, mascara, dns);
  WiFi.begin(ssid, password);
  


  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
 
  

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
//  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  servidor.begin();


}



void loop() {
  // put your main code here, to run repeatedly:
String mensagem;
WiFiClient cliente = servidor.available();

 if(cliente)
 {
  Serial.println("Ha um cliente conectado");
  while(cliente.connected())
  {
    if(cliente.available())
    {
      char bytes = cliente.read();
      Serial.print(bytes);
      mensagem += bytes;

      if(mensagem == "pD")
      {
        //pedido de dados recebido
        Serial.println("Pedido de dados Recebido");
      }
      
    }
  }

  
 }



if(Serial.available())
{
 char comando = Serial.read();
 switch(comando)
 {

  case 'a':{digitalWrite(led, LOW);Serial.println("a");}break;
  case 'b':{digitalWrite(led, HIGH);Serial.println("b");}break;
  case 'c':{Serial.println(WiFi.localIP());}break;
  case 'd':
  {
    String dataCompleta = retorna_info_tempo();
    Serial.println(dataCompleta);
  }break;
  
    case 'e':
      {//envia mensagem pro arduino
       mySerial.println("enviando mensagem pro arduino");
        Serial.println("e recebido");
      }break;
 }
   


  
}

if(mySerial.available())
  {
    char recebido = mySerial.read();
    Serial.print(recebido);
    switch(recebido)
    {
      case 'p':
      {//retorna a hora
        Serial.println("pedido de hora, enviando");
        delay(100);
        String dataCompleta = retorna_info_tempo_arduino();
        Serial.print("hora pra enviar: ");
        Serial.println(dataCompleta);
        Serial.println("enviando...");  
        mySerial.println(dataCompleta);
      }break;
    }

    
  }

}


String retorna_info_tempo()
{
    horas = hour();
    minutos = minute();
    segundos = second();
    dia = day();
    mes = month();
    ano  = year();

    String dataCompleta = horas + ":" + minutos + ":" + segundos + " " + dia + "/" + mes + "/" + ano;
    return dataCompleta;
}

String retorna_info_tempo_arduino()
{
    horas = hour();
    minutos = minute();
    segundos = second();
    dia = day();
    mes = month();
    ano  = year();

    String dataCompleta = "*" + horas + "," + minutos + "," + segundos + "," + dia + "," + mes + "," + ano + "&";
    return dataCompleta;
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket("pool.ntp.br", 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
