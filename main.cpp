#include "mbed.h"
#include "WIZnetInterface.h"
#include <MQTTClientMbedOs.h>

#define SW_USER PC_6

// ### Output for relais control ###
DigitalOut relais11(D11);//D11 LED_BLUE
DigitalOut relais12(D12);//D12 LED_GREEN
DigitalOut led(LED_RED);

// ### Input buttons ###
DigitalIn button(SW_USER,PullUp);


// ### Prototypes ###
void relais11_messageArrived(MQTT::MessageData& md);
void relais12_messageArrived(MQTT::MessageData& md);



// main() runs in its own thread in the OS
int main()
{
    relais11 = 0;
    relais12 = 0;
    
    Serial s(USBTX, USBRX);
    s.baud(115200);
    ThisThread::sleep_for(1000);

    printf("#############################################################################\n");

    // ### Connection details ###
    uint8_t mac_addr[6] = {0x00, 0x08, 0xDC, 0x1E, 0x72, 0x1F};
    uint8_t host_ip[4] = {192,168,178,42};
    const char* hostname = (char *)"192.168.178.42";
    //const char* hostname = (const char *)"NUC.fritz.box";
    int port = 1883;
    
    const char* DnsServerIP = (const char *)"192.168.178.1";

    const char* wiz_ip = (char *)"192.168.178.43";
    const char* wiz_subnet = (char *)"255.255.255.0";
    const char* wiz_gateway = (char *)"192.168.178.1";

    // ### WIZnetInterface ###
    WIZnetInterface eth;
    //int rc = eth.init(mac_addr); // with DHCP
    int rc = eth.init(mac_addr,wiz_ip,wiz_subnet,wiz_gateway); // no DHCP
    printf("[WIZNET] init returned: %d \n", rc);
    
    rc = eth.connect();
    if (rc != 0){ printf("[WIZNET] connect returned: %d\n", rc); }
    else printf("[WIZNET] Connection was successful \r\n[WIZNET] IP: %s \n",eth.get_ip_address());
    eth.set_blocking(false);
    // ### TCP Socket ###
    TCPSocket socket;  
    rc= socket.open(&eth);
    if (rc != 0){ printf("[TCP Socket] Connect returned: %d \r\n",rc); }
    else printf("[TCP Socket] Init was successful\r\n");

    rc = socket.connect(hostname, port); 
    socket.set_blocking(false); 
    if (rc != 0){ printf("[TCP Socket] Connect returned: %d \r\n",rc); }
    else printf("[TCP Socket] Connection was successful\r\n");

    // ### MQTT Client ###
    MQTTClient mqttclient(&socket);

    char MQTTClientID[30];
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    sprintf(MQTTClientID,"WIZwiki-W7500P");
    data.clientID.cstring = MQTTClientID;
    data.username.cstring = (char *)"wiznet";
    data.password.cstring = (char *)"";  
    
    if ((rc = mqttclient.connect(data)) != 0){ printf("[MQTT] Connect returned: %d \r\n",rc); }
    else printf("[MQTT] Connection was successful\r\n");

    // ### MQTT Subscribtions ###
    const char* relais11_topic = (char *)"/wiznet/relais11";
    const char* relais12_topic = (char *)"/wiznet/relais12";

    if ((rc = mqttclient.subscribe(relais11_topic, MQTT::QOS1, relais11_messageArrived)) != 0) printf("[MQTT] Subscribe returned  %d \r\n", rc);
    else printf("[MQTT] Added subscription for relais11_topic \r\n");

    if ((rc = mqttclient.subscribe(relais12_topic, MQTT::QOS1, relais12_messageArrived)) != 0) printf("[MQTT] Subscribe returned  %d \r\n", rc);
    else printf("[MQTT] Added subscription for relais12_topic \r\n");

    // ### MQTT Message ###
    MQTT::Message message;
    char buf[100];
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;

    // ### MQTT Set MEssage ###
    sprintf(buf, "Just Started");
    message.payloadlen = strlen(buf);

    // ### MQTT Publish Message ###
    rc = mqttclient.publish("/wiznet",message);

    // ### loop definitions ###
    int loopCnt = 0;
    Timer timer1, timer2;
    int timeYield=0, rc2=0;;
    timer1.start();


    while (true) {

        led = button;

        if (timer1.read_ms() >= 30000){
            timer1.reset();

            sprintf(buf, "30s Loop");
            message.payloadlen = strlen(buf);
            rc = mqttclient.publish("/wiznet",message);
            
            if ( (rc != 0) || (rc2 != 0) ){
                //printf("[MQTT] Publish failed %d ---- %d loops\n", rc,loopCnt);
                NVIC_SystemReset(); //connection lost! software reset
                return 0;
            }
           // else printf("[MQTT] Publish success %d yield() returned %d and took: %d ms \r\n", rc,rc2,timeYield); 
        }

        timer2.start();
        rc2 = mqttclient.yield(1);
        timer2.stop(); timeYield = timer2.read_ms(); timer2.reset();
    }
}

// ######################################################################################################################
// ############################################ Functions ###############################################################
// ######################################################################################################################


void relais11_messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    //printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    if( strncmp((char*)message.payload,"ON",message.payloadlen) == 0 ){
        relais11 = 1;
    } 
    else if( strncmp((char*)message.payload,"OFF",message.payloadlen) == 0 ){
        relais11 = 0;
    }
    else if( strncmp((char*)message.payload,"TOGGLE",message.payloadlen) == 0 ){
        relais11 = 1;
        ThisThread::sleep_for(100);
        relais11 = 0;
    }
}

void relais12_messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    //printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    if( strncmp((char*)message.payload,"ON",message.payloadlen) == 0 ){
        relais12 = 1;
    } 
    else if( strncmp((char*)message.payload,"OFF",message.payloadlen) == 0 ){
        relais12 = 0;
    }
}