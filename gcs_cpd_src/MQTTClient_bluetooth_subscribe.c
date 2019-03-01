#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "cJSON.h"
#include "conf_gateway.h"
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#define ADDRESS     "tcp://127.0.0.1:1883"
//#define ADDRESS     "172.17.6.189:1883"
//#define ADDRESS     "172.16.94.50:1883"
#define CLIENTID1    "local_platform_bluetooth_sub"
#define CLIENTID2    "local_platform_bluetooth_pub"
#define TOPIC1       "certusnet/local/down"
#define TOPIC2       "certusnet/local/up"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;
int  disconnect_tag = 0;
pid_t fpid;
char *p_map = NULL;

int  gcs_sends_data_feedback(const char *reserved)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    char PAYLOAD[10240] = {0};                                                                     //array size: 10k        

	while(1){
		raise(SIGSTOP);
		sleep(1);
					
		memset(PAYLOAD, 0, sizeof(PAYLOAD));         
		memcpy(PAYLOAD, p_map, strlen(p_map));                                                     //update PAYLOAD
		//printf(PAYLOAD = \n%s\n",PAYLOAD);					

		MQTTClient_create(&client, ADDRESS, CLIENTID2,
		MQTTCLIENT_PERSISTENCE_NONE, NULL);
		conn_opts.keepAliveInterval = 20;
		conn_opts.cleansession = 1;

		if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
			printf("Failed to connect, return code %d\n", rc);
			exit(EXIT_FAILURE);
		}
		pubmsg.payload = PAYLOAD;
		pubmsg.payloadlen = (int)strlen(PAYLOAD);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		MQTTClient_publishMessage(client, TOPIC2, &pubmsg, &token);
		printf("Waiting for up to %d seconds for publication of %s\n"
				"on topic %s for client with ClientID: %s\n",
				(int)(TIMEOUT/1000), PAYLOAD, TOPIC2, CLIENTID2);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		MQTTClient_disconnect(client, 10000);
		MQTTClient_destroy(&client);
	}
    return rc;
}


void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i,rt = -1;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++){
        putchar(*payloadptr++);
    }
    putchar('\n');

	rt = gcs_sends_data_unpack(message->payload);                                                  //handler,gcs:Gateway configuration system
	printf("rt = %d\n",rt);
	if(rt == 1){
		// Do Nothing
	} else if(rt != 0){
		printf("gcs_sends_data_unpack fail\n");
        if(kill(fpid,SIGCONT) == 0){
            printf("send \"SIGCONT\" successfully\n");
        } else {
            printf("send \"SIGCONT\" fail \n");
        } 
	} else {   
		printf("gcs_sends_data_unpack successfully\n");		
        if(kill(fpid,SIGCONT) == 0){
			printf("send \"SIGCONT\" successfully\n");
        } else {
			printf("send \"SIGCONT\" fail \n");
		}	         
	}
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
    disconnect_tag = 1;
}

int main(int argc, char* argv[])
{
	p_map = (char*)mmap(NULL, 10240, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);       //Shared memory size: 10K，type: char
	memset(p_map, 0, 10240);                                                                       //init p_map 0

    fpid = fork();
    if (fpid < 0){   
        printf("error in fork!");
	} else if(fpid == 0){
		gcs_sends_data_feedback(NULL);
    } else { 
		MQTTClient client;
		MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
		int rc;
		int ch;

		MQTTClient_create(&client, ADDRESS, CLIENTID1,
		MQTTCLIENT_PERSISTENCE_NONE, NULL);
		conn_opts.keepAliveInterval = 100;                                                         //set keepalive 100s
		conn_opts.cleansession = 1;

		MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

		if((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
			printf("Failed to connect, return code %d\n", rc);
			kill(fpid,SIGKILL);
			sleep(1);
			exit(EXIT_FAILURE);
		}
		printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
			   , TOPIC1, CLIENTID1, QOS);
		MQTTClient_subscribe(client, TOPIC1, QOS);
		
		while(1){
			sleep(3);
			if(disconnect_tag == 1){
				kill(fpid,SIGKILL);
				sleep(1);
				break;
			}		
		}
	
		MQTTClient_unsubscribe(client, TOPIC1);
		MQTTClient_disconnect(client, 10000);
		MQTTClient_destroy(&client);
		return rc;		
    }
}
