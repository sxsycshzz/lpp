#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MQTTClient.h"
#include "cJSON.h"
#include "parsekv.h"
#include "cmd.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CLIENTID        "cloud_platform_ctn2018000000001_pub"
#define TOPIC           "certusnet/up"
#define QOS             1
#define TIMEOUT         10000L
#define NET_CONF_FILE   "/etc/certusnet_conf/wan_way"

typedef struct gateway_name{                          
	char first_half[20];
	char second_half[20];
}gateway_name;

typedef struct gateway_status{                          
	char payload_info[30];
	gateway_name  name;
	char gateway_ip[20];
	char gateway_sn[20];
	char gateway_networking_way[10];
	char kernel[10];
	char cpu_utilization[20];
	char ram_use[20];
	char ram_all[20];
	char ram_utilization[20];
	char disk_use[20];
	char disk_all[20];
	char disk_utilization[20];
	char currnet_time[25];
	char uptime[20];
}gateway_status;
static char cloud_platform_Address[30] = {'\0'};

void package_gateway_info(char **json_pack_out, gateway_status *reported_data)
{
	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON *Item = NULL;	
	
	Root = cJSON_CreateObject();
	Object = cJSON_CreateObject();
	
	Item = cJSON_CreateString(reported_data->payload_info);
	cJSON_AddItemToObject(Root, "payload_info", Item);
	
	Item = cJSON_CreateString(reported_data->name.first_half);
	cJSON_AddItemToObject(Object, "first_half", Item);		
	Item = cJSON_CreateString(reported_data->name.second_half);
	cJSON_AddItemToObject(Object, "second_half", Item);	
	cJSON_AddItemToObject(Root, "gateway_name", Object);
    Item = cJSON_CreateString(reported_data->gateway_ip);
    cJSON_AddItemToObject(Root, "gateway_ip", Item);	
	Item = cJSON_CreateString(reported_data->gateway_sn);
	cJSON_AddItemToObject(Root, "gateway_sn", Item);
	Item = cJSON_CreateString(reported_data->gateway_networking_way);
	cJSON_AddItemToObject(Root, "gateway_networking_way", Item);	

    Item = cJSON_CreateString(reported_data->kernel);
    cJSON_AddItemToObject(Root, "kernel", Item);
	
    Item = cJSON_CreateString(reported_data->cpu_utilization);
    cJSON_AddItemToObject(Root, "cpu_utilization", Item);
	
    Item = cJSON_CreateString(reported_data->ram_use);            
    cJSON_AddItemToObject(Root, "ram_use", Item);
    Item = cJSON_CreateString(reported_data->ram_all);            
    cJSON_AddItemToObject(Root, "ram_all", Item);		
    Item = cJSON_CreateString(reported_data->ram_utilization);            
    cJSON_AddItemToObject(Root, "ram_utilization", Item);
	
	Item = cJSON_CreateString(reported_data->disk_use);
	cJSON_AddItemToObject(Root, "disk_use", Item);	
	Item = cJSON_CreateString(reported_data->disk_all);
	cJSON_AddItemToObject(Root, "disk_all", Item);	
    Item = cJSON_CreateString(reported_data->disk_utilization);
    cJSON_AddItemToObject(Root, "disk_utilization", Item);
	
	Item = cJSON_CreateString(reported_data->currnet_time);
	cJSON_AddItemToObject(Root, "current_time", Item);	
	Item = cJSON_CreateString(reported_data->uptime);
	cJSON_AddItemToObject(Root, "uptime", Item);
		
	*json_pack_out = cJSON_Print(Root);	                
	cJSON_Delete(Root);		
}



int get_gateway_info(char **json_pack_out)
{
	int fd = -1, len = -1, ret = -1;
	float f = 0.0, ram_use = 0.0, ram_all = 0.0; 
	char buf[100] = {'\0'};
	
	char cloud_platform_Ip[20] = {'\0'};
	char cloud_platform_Port[20] = {'\0'};
	char cloud_platform_User[20] = {'\0'};
	char cloud_platform_Passwd[50] = {'\0'};
	
	float ram_utilization_temporary = 0;
	float disk_use_temporary = 0;
	float disk_all_temporary = 0;
	float disk_utilization_temporary = 0;
			
	gateway_status reported_data = {0};

	if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "payload_info", reported_data.payload_info) != 0){
		printf("get kv \"payload_info\" fail\n");
		return -7;
	}
	
    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "name.first_half", reported_data.name.first_half) != 0){
        printf("get kv \"name.first_half\" fail\n");
        return -7;
    }

    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "name.second_half", reported_data.name.second_half) != 0){
        printf("get kv \"name.second_half\" fail\n");
        return -7;
    }

    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "gateway_sn", reported_data.gateway_sn) != 0){
        printf("get kv \"gateway_sn\" fail\n");
        return -7;
    } 
		
    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_ip", cloud_platform_Ip) != 0){
        printf("get kv \"ga_man_plat_ip\" fail\n");
        return -7;
    }

    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_port", cloud_platform_Port) != 0){
        printf("get kv \"ga_man_plat_port\" fail\n");
        return -7;
    }

    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_name", cloud_platform_User) != 0){
        printf("get kv \"ga_man_plat_name\" fail\n");
        return -7;
    }

    if(file_get_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_passwd", cloud_platform_Passwd) != 0){
        printf("get kv \"ga_man_plat_passwd\" fail\n");
        return -7;
    }
	sprintf(cloud_platform_Address,"%s:%s", cloud_platform_Ip, cloud_platform_Port);

	//get networking way
	fd = open(NET_CONF_FILE, O_RDONLY);
	if(fd < 0){
		perror("open");
		return -2;
	}
	if(read(fd, reported_data.gateway_networking_way, sizeof(reported_data.gateway_networking_way) - 1) < 0){
		perror("read");
		return -3;		
	}
	close(fd);

	len = strlen(reported_data.gateway_networking_way);
	if(len > 0 && (reported_data.gateway_networking_way[len-1] == '\n')){
		reported_data.gateway_networking_way[len-1] = '\0';
	}	
	
	if(0 == strcmp(reported_data.gateway_networking_way, "wifi")){
		if(out_gw(buf, sizeof(buf), "wlp1s0", &f) > 0){
			memcpy(reported_data.gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"wlp1s0\" IP address\n");
			return -4;
		}	 				
	} else if((0 == (strcmp(reported_data.gateway_networking_way, "static"))) || (0 == strcmp(reported_data.gateway_networking_way, "dhcp"))){
		if(out_gw(buf, sizeof(buf), "ip1", &f) > 0){
			memcpy(reported_data.gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"eth0\"(%s) IP address\n",reported_data.gateway_networking_way);
			return -5;
		}					
	} else if(0 == (strcmp(reported_data.gateway_networking_way, "pppoe"))){
//		printf("pppoe is not currently supported\n");
		if(out_gw(buf, sizeof(buf), "pppoe", &f) > 0){
			memcpy(reported_data.gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"pppoe\" IP address\n");
			return -6;
		}			
	} else {
		return -8;
	}
			
    if(out_gw(buf, sizeof(buf), "cpu_kernel_num", &f) > 0){
        memcpy(reported_data.kernel, buf, strlen(buf));   
    } else {
        printf("Failed to get the number of CPU cores\n");
        return -9;
    }
	
    if(out_gw(buf, sizeof(buf), "cpu_avg_rate", &f) > 0){
		sprintf(reported_data.cpu_utilization, "%.2f", f);   	
    } else {
        printf("Failed to get CPU utilization\n");
        return -10;
    }
	
    if(out_gw(buf, sizeof(buf), "ram_all", &f) > 0){
        ram_all = f;
		strcat(buf, "KB");
		memcpy(reported_data.ram_all, buf, strlen(buf));	
    } else {
        printf("Failed to get the total ram size\n");
        return -11;
    }

    if(out_gw(buf, sizeof(buf), "ram_free", &f) > 0){
        ram_use = ram_all-f;
		sprintf(reported_data.ram_use, "%dKB", (int)ram_use);		
    } else {
        printf("Failed to get the unused ram size\n");
        return -12;
    }	
	
	ram_utilization_temporary = (ram_use / ram_all * 10000 + 0.5) / 100;
	sprintf(reported_data.ram_utilization, "%.2f", ram_utilization_temporary);

	
    if(out_gw(buf, sizeof(buf), "disk_use_s", &f) > 0){
        memcpy(reported_data.disk_use, buf, strlen(buf)); 
    } else {
        printf("Failed to get the used disk size\n");
        return -13;
    }

    if(out_gw(buf, sizeof(buf), "disk_all_s", &f) > 0){
        memcpy(reported_data.disk_all, buf, strlen(buf));  
    } else {
        printf("Failed to get total disk size\n");
        return -14;
    }	
		
    if(out_gw(buf, sizeof(buf), "disk_use_f", &f) > 0){
        disk_use_temporary = f; 
    } else {
        printf("Failed to get the used disk size\n");
        return -15;
    }

    if(out_gw(buf, sizeof(buf), "disk_all_f", &f) > 0){
        disk_all_temporary = f;  
    } else {
        printf("Failed to get total disk size\n");
        return -16;
    }			
	//printf("disk_use_temporary = %f, disk_all_temporary = %f\n",disk_use_temporary,disk_all_temporary);
	
	disk_utilization_temporary = (disk_use_temporary / disk_all_temporary) * 100 + 0.5;
	sprintf(reported_data.disk_utilization, "%.2f", disk_utilization_temporary);
	
	if(out_gw(buf, sizeof(buf), "time_now", &f) > 0){
		memcpy(reported_data.currnet_time, buf, strlen(buf));
	} else {
        printf("Failed to get gateway current time\n");
        return -17;		
	}

    if(out_gw(buf, sizeof(buf), "time_up", &f) > 0){       
        memcpy(reported_data.uptime, buf, strlen(buf));
    } else {
        printf("Failed to get gateway boot time\n");
        return -18;
    }
	
    package_gateway_info(json_pack_out, &reported_data);
	
	return 0;
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    char *PAYLOAD = NULL;
	
	rc = get_gateway_info(&PAYLOAD);
	if(rc != 0){
		printf("Failed to get gateway information, return code %d\n", rc);
		return rc;
	}
	
	MQTTClient_create(&client, cloud_platform_Address, CLIENTID,
	MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.connectTimeout = 10;
	conn_opts.cleansession = 1;

	if((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = (int)strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	 
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	printf("Waiting for up to %d seconds for publication of %s\n"
				"on topic %s for client with ClientID: %s\n",
				(int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	printf("Message with delivery token %d delivered\n", token);

	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	free(PAYLOAD);

    return rc;
}
