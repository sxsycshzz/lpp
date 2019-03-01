#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parsekv.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

extern char *p_map;
	
#define  SEARCH_BLU_DEV_SUCCESS	 	 	"success"
#define  SEARCH_BLU_DEV_FAIL      		"failure"
#define  GET_CONNECTED_BLUDEV_SUCCESS   "success"
#define  GET_CONNECTED_BLUDEV_FAIL      "failure"
#define  BLUETOOTH_MAC_LENGTH            17

//bluetooth model interface conf
typedef struct interface_conf{                          
	char blue_switch[10];                           //substitute blue_swith for swith
	char devicename[30];
	char visible[10];
	char pin_code[20];
}interface_conf;

typedef struct bluetooth_interface_conf{                          
	char payload_info[30];
	interface_conf  interface_info;
}bluetooth_interface_conf;



//search blue devices
typedef struct blue_device_info{                          
	char name[100];
	char address[20];
	char type[50];
}blue_device_info;

typedef struct blue_search{                          
	int num;
	blue_device_info  device_info[30];              //Maximum number of discovery devices 30
}blue_search;

typedef struct bluetooth_search{                          
	char payload_info[30];
	blue_search blue_sea;
}bluetooth_search;


//connect blue device
/* typedef struct blue_device_info{                 //Maximum bluetooth device name length 99                      
	char name[100];
	char address[20];
	char type[50];
}blue_device_info; */

typedef struct bluetooth_dev_connect{                          
	char payload_info[30];
	blue_device_info  dev;
}bluetooth_dev_connect;



//Connected blue device                        
/* typedef struct blue_device_info{                          
	char name[100];
	char address[20];
	char type[50];
}blue_device_info; */

typedef struct dev_connected{                          
	int num;
	blue_device_info  device_info[10];              //Maximum number of connections 10
}dev_connected;

typedef struct bluetooth_dev_connected{                          
	char payload_info[30];
	dev_connected dev;
}bluetooth_dev_connected;


int update_blue_inter_profile(char *blue_switch, char *devicename, char *visible, char *pin_code)
{
	if(blue_switch != NULL){
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_switch", blue_switch) != 0){
			printf("file_update_value \"conf_blue_interface_switch\" fail\n");
			return -7;
		} 
	}
	if(devicename != NULL){
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_devicename", devicename) != 0){
			printf("file_update_value \"conf_blue_interface_devicename\" fail\n");
			return -7;
		} 
	}
	if(visible != NULL){
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_visible", visible) != 0){
			printf("file_update_value \"conf_blue_interface_visible\" fail\n");
			return -7;
		}
	}
	if(pin_code != NULL){
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_pin_code", pin_code) != 0){
			printf("file_update_value \"conf_blue_interface_pin_code\" fail\n");
			return -7;
		} 
	}
	
	return 0;
}

int update_con_bludev_profile(char *name, char *address, char *type)
{
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conned_blue_dev_name", name) != 0){
		printf("file_update_value \"conned_blue_dev_name\" fail\n");
		return -7;
	} 		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conned_blue_dev_address", address) != 0){
		printf("file_update_value \"conned_blue_dev_address\" fail\n");
		return -7;
	} 
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conned_blue_dev_type", type) != 0){
		printf("file_update_value \"conned_blue_dev_type\" fail\n");
		return -7;
	} 
	
	return 0;
}

void gw_response_str(char *cmdtype, char *result)
{
	cJSON *Root = NULL;
	cJSON *Item = NULL;
	char *json_pack_out = NULL;
	
	Root = cJSON_CreateObject();	
	Item = cJSON_CreateString(cmdtype);
	cJSON_AddItemToObject(Root,"payload_info", Item);
	Item = cJSON_CreateString(result);
	cJSON_AddItemToObject(Root, "conf_success", Item);

	json_pack_out = cJSON_Print(Root);
    //printf("\n%s\n",json_pack_out);
			
	memset(p_map, 0, 10240);                                    
	memcpy(p_map, json_pack_out, strlen(json_pack_out));        //update p_map
	//printf("\n%s\n",p_map);
			
	free(json_pack_out);                  
	cJSON_Delete(Root);		
}

void gw_blue_search_response_str(bluetooth_search *disc_blue_equ, char *result)
{
 	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON *Item = NULL;
	char *json_pack_out = NULL;	
	int i = 0;
	char device_mark[10] = "device_1";       
	char Object_mark[12] = "Object2_2"; 
			
	Root = cJSON_CreateObject();
	Object = cJSON_CreateObject();	

	if(0 == strcmp(result, "success")){
		Item = cJSON_CreateString(disc_blue_equ->payload_info);
		cJSON_AddItemToObject(Root,"payload_info", Item);
			
		Item = cJSON_CreateNumber(disc_blue_equ->blue_sea.num);
		cJSON_AddItemToObject(Object, "num", Item);
		
		if(disc_blue_equ->blue_sea.num > 0){
			for(i = 0; i < disc_blue_equ->blue_sea.num; i++){					
				sprintf(Object_mark, "Object2_%d", i+1);
				sprintf(device_mark, "device_%d", i+1);
				cJSON *Object_mark = NULL;
				Object_mark = cJSON_CreateObject();
							
				Item = cJSON_CreateString(disc_blue_equ->blue_sea.device_info[i].name);
				cJSON_AddItemToObject(Object_mark, "name", Item);		
				Item = cJSON_CreateString(disc_blue_equ->blue_sea.device_info[i].address);
				cJSON_AddItemToObject(Object_mark, "address", Item);		
				Item = cJSON_CreateString(disc_blue_equ->blue_sea.device_info[i].type);
				cJSON_AddItemToObject(Object_mark, "type", Item);

				cJSON_AddItemToObject(Object, device_mark, Object_mark);
			}
		}
	} else { // failure
		Item = cJSON_CreateString("bluetooth_search");
		cJSON_AddItemToObject(Root,"payload_info", Item);
			
		Item = cJSON_CreateNumber(0);
		cJSON_AddItemToObject(Object, "num", Item);		
	}
	cJSON_AddItemToObject(Root, "bluetooth_search", Object);
	
	json_pack_out = cJSON_Print(Root);
	//printf("\n%s\n",json_pack_out);
	
	memset(p_map, 0, 10240);                                   
	memcpy(p_map, json_pack_out, strlen(json_pack_out));        //update p_map
	//printf("\n%s\n",p_map);
	
	free(json_pack_out);                   	
	cJSON_Delete(Root);		
}

void gw_connected_bludev_response_str(bluetooth_dev_connected *blu_dev_connected, char *result)
{
	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON *Item = NULL;
	char *json_pack_out = NULL;
	int i = 0;
	char device_mark[10] = "device_1";       
	char Object_mark[12] = "Object2_1";
	
	Root = cJSON_CreateObject();
	Object = cJSON_CreateObject();
	if(0 == strcmp(result, "success")){	
		Item = cJSON_CreateString(blu_dev_connected->payload_info);
		cJSON_AddItemToObject(Root, "payload_info", Item);
		
		Item = cJSON_CreateNumber(blu_dev_connected->dev.num);
		cJSON_AddItemToObject(Object, "num", Item);
		
		if(blu_dev_connected->dev.num > 0){
			for(i = 0; i < blu_dev_connected->dev.num; i++){					
				sprintf(Object_mark, "Object2_%d", i+1);
				sprintf(device_mark, "device_%d", i+1);
				cJSON *Object_mark = NULL;
				Object_mark = cJSON_CreateObject();						
				Item = cJSON_CreateString(blu_dev_connected->dev.device_info[blu_dev_connected->dev.num-1].name);
				cJSON_AddItemToObject(Object_mark, "name", Item);		
				Item = cJSON_CreateString(blu_dev_connected->dev.device_info[blu_dev_connected->dev.num-1].address);
				cJSON_AddItemToObject(Object_mark, "address", Item);		
				Item = cJSON_CreateString(blu_dev_connected->dev.device_info[blu_dev_connected->dev.num-1].type);
				cJSON_AddItemToObject(Object_mark, "type", Item);

				cJSON_AddItemToObject(Object, device_mark, Object_mark);
			}
		}
	} else { // failure
		Item = cJSON_CreateString("bluetooth_dev_connected");
		cJSON_AddItemToObject(Root, "payload_info", Item);
			
		Item = cJSON_CreateNumber(0);
		cJSON_AddItemToObject(Object, "num", Item);		
	}
	cJSON_AddItemToObject(Root, "bluetooth_dev_connected", Object);

	json_pack_out = cJSON_Print(Root);
    //printf("%s\n",json_pack_out);
	
	memset(p_map, 0, 10240);                                        
	memcpy(p_map, json_pack_out, strlen(json_pack_out));        //update p_map
	//printf("%s\n",p_map);
	
	free(json_pack_out);                  
	cJSON_Delete(Root);		
}


static int conf_blue_mod_interface(bluetooth_interface_conf *blu_inter_info)     
{
	char conf_blue_inter_cmd[50] = {0};
	
	if(0 == strcmp(blu_inter_info->interface_info.blue_switch, "on")){
		if(system("hciconfig hci0 up")){
			printf("the Function system(\"hciconfig hci0 up\") execution failed\n");
			return -1;
		}	
		
		sprintf(conf_blue_inter_cmd, "hciconfig hci0 name %s", blu_inter_info->interface_info.devicename);
		if(system(conf_blue_inter_cmd)){
			printf("the Function system(\"%s\") execution failed\n", conf_blue_inter_cmd);
			return -3;			
		}
		
		if(0 == strcmp(blu_inter_info->interface_info.visible, "yes")){
			if(system("hciconfig hci0 iscan")){
				printf("the Function system(\"hciconfig hci0 iscan\") execution failed\n");
				return -4;
			}							
		} else if(0 == strcmp(blu_inter_info->interface_info.visible, "no")){
			if(system("hciconfig hci0 noscan")){
				printf("the Function system(\"hciconfig hci0 noscan\") execution failed\n");
				return -5;
		    }						
		} else {
			printf("the value \"blu_inter_info->interface_info.visible\" ormal error");
			return -6;			
		}
		
		//restart hci0
		if(system("hciconfig hci0 down")){
			printf("the Function system(\"hciconfig hci0 down\") execution failed\n");				
			return -2;
		}
		if(system("hciconfig hci0 up")){
			printf("the Function system(\"hciconfig hci0 up\") execution failed\n");		
			return -1;
		}		
		update_blue_inter_profile(blu_inter_info->interface_info.blue_switch, blu_inter_info->interface_info.devicename, blu_inter_info->interface_info.visible, blu_inter_info->interface_info.pin_code);	
	} else if(0 == strcmp(blu_inter_info->interface_info.blue_switch, "off")){
		if(system("hciconfig hci0 down")){
			printf("the Function system(\"hciconfig hci0 down\") execution failed\n");		
			return -2;
		}
		update_blue_inter_profile(blu_inter_info->interface_info.blue_switch, NULL, NULL, NULL);		
	} else {
		printf("the value \"blu_inter_info->interface_info.blue_switch\" ormal error");	
		return -6;
	}
		
	return 0;		
}

static int search_blue_devices(bluetooth_search *disc_blue_equ)    
{
    FILE *fp = NULL;
	char *p = NULL;
	int i = 0;
    int scan_bludev_num = 0;
    char dev_blue_mac[20] = {0};
    char dev_blue_name[100] = {0};
    char dev_blue_type[50] = {0};
    char scan_bludev_info_line_buffer[256]= {0};
		
    fp = popen("hcitool scan --class --length=3", "r");
    if(fp == NULL){
		printf("popen(\"hcitool scan --class\",\"r\") fail\n");
		return -1;
    }
    while(fgets(scan_bludev_info_line_buffer, sizeof(scan_bludev_info_line_buffer),fp) != NULL)
    {
		if(strlen(scan_bludev_info_line_buffer) > 1){                                              //Excluding a blank line
			if((p = strstr(scan_bludev_info_line_buffer, "Address:")) != NULL){
				p+=8;
				while(!((*p == '\n') || (*p == '\0'))){
					if(((*p > 47) && (*p < 58)) || ((*p > 64) && (*p < 91))){
						scan_bludev_num++;						
						memcpy(dev_blue_mac, p, BLUETOOTH_MAC_LENGTH);
						memcpy(disc_blue_equ->blue_sea.device_info[scan_bludev_num-1].address,dev_blue_mac,strlen(dev_blue_mac));	
						memset(dev_blue_mac,0,sizeof(dev_blue_mac));					
							
						break;                           	
					}
					p++;
				}
			} else if((p = strstr(scan_bludev_info_line_buffer, "name:")) != NULL){
				i = 0;				
				p += 5;
				while(*p == 9){		// ASCII '\t'->9 
					p++;
				}
				while(!((*p == '\n') || (*p == '\0'))){  
					dev_blue_name[i++] = *p++;						
				}
				memcpy(disc_blue_equ->blue_sea.device_info[scan_bludev_num-1].name, dev_blue_name, strlen(dev_blue_name));				
				memset(dev_blue_name, 0, sizeof(dev_blue_name));					
			} else if((p = strstr(scan_bludev_info_line_buffer, "class:")) != NULL){
				i = 0;
				p += 6;
				while(*p == 9){
					p++;
				}
				while(!((*p == '\n') || (*p == '\0') || (*p == 44))){
					dev_blue_type[i++] = *p++;						
				}
				memcpy(disc_blue_equ->blue_sea.device_info[scan_bludev_num-1].type, dev_blue_type, strlen(dev_blue_type));
				memset(dev_blue_type, 0, sizeof(dev_blue_type));					
			}						
		}
	}	
	pclose(fp);

	memcpy(disc_blue_equ->payload_info, "bluetooth_search", strlen("bluetooth_search"));		
	disc_blue_equ->blue_sea.num = scan_bludev_num;
	
	return 0;	
}

static int connect_blue_device(bluetooth_dev_connect *blu_dev)    
{
    FILE *fp;
    char buffer[100] = {0};
    char host_blue_mac[20] = {0};
    char con_bluedev_cmd[500] = {0};
    char chek_bludev_whe_con_cmd[60] = {0};
    char discon_bluedev_cmd[100] = {0};
    int rt = 0;
    char *p = NULL;
	
	//get host mac
    fp = popen("hciconfig", "r");
    if(fp == NULL){
		printf("popen(\"hciconfig\",\"r\") fail\n");
		return -1;
    }
    rt = fread(buffer, 1, 80, fp);
    if(rt != 80){
		printf("the return value of the fuc fread(buffer, 1, 80, fp) no equal 80\n");
		return -2;
    }
    pclose(fp);

    p = strstr(buffer, "Address");
    if(p == NULL){
		printf("the fuc strstr(buffer, \"Address\")  fail\n");
		return -3;
    } 
	p += 9;
    memcpy(host_blue_mac, p, BLUETOOTH_MAC_LENGTH);

    rt = system("su -c \"pulseaudio -D\" root");
    if(rt != 0){
	system("su -c \"pulseaudio -D\" root");
    }
 
    sprintf(con_bluedev_cmd, "(echo \e \"select %s\n\"; sleep 1; echo \e \"power off\n\"; sleep 1; echo \e \"power on\n\"; sleep 1; echo \e \"agent on\n\"; sleep 1; echo \e \"default-agent\n\"; sleep 1;\
echo \e \"disconnect %s\n\"; sleep 3; echo \e \"scan on\n\"; sleep 6; echo \e \"pair %s\n\"; sleep 6; echo \e \"connect %s\n\"; sleep 6; echo \e exit) | bluetoothctl",host_blue_mac,blu_dev->dev.address,blu_dev->dev.address,blu_dev->dev.address);   
    rt  = system(con_bluedev_cmd);

    sprintf(chek_bludev_whe_con_cmd, "hcitool con | grep %s", blu_dev->dev.address);	
    rt = system(chek_bludev_whe_con_cmd);		
    if(rt != 0){
		printf("Bluetooth device connection failed, Try connecting again\n");
		system(con_bluedev_cmd);
		rt = system(chek_bludev_whe_con_cmd);
		if(rt != 0){
			printf("Bluetooth device connection failed again\n");			
			return -4;
		}
    }
	update_con_bludev_profile(blu_dev->dev.name, blu_dev->dev.address, blu_dev->dev.type);
	
    return 0;
}

static int connected_blue_device(bluetooth_dev_connected *blu_dev_connected)
{
	FILE *fp = NULL;
	char *p = NULL;
	int conned_bludev_num = 0;
	int len = 0, get_kv_rt = 0;
	char kv_value[100] = {0};                         
 	char conned_bludev_info_line_buffer[256] = {0};	

	if(system("hcitool con | grep \"<\"") != 0){
		conned_bludev_num = 0;
	} else {
		//get connected bluetooth device
		fp = popen("hcitool con", "r");
		if(fp == NULL){
			printf("popen(\"hcitool con\",\"r\") fail\n");
			return -5;
		}
		while(fgets(conned_bludev_info_line_buffer, sizeof(conned_bludev_info_line_buffer),fp) != NULL){    //get connected bluetooth device mac address
			if(strlen(conned_bludev_info_line_buffer) > 1){                                                 //Excluding a blank line
				len = strlen(conned_bludev_info_line_buffer);
				if((p = strstr(conned_bludev_info_line_buffer, "ACL")) != NULL){
					p += 3;
					
					while(!((*p == '\n') || (*p == '\0'))){																		
						if(((*p > 47) && (*p < 58)) || ((*p > 64) && (*p < 91))){	
							conned_bludev_num++;
							memcpy(blu_dev_connected->dev.device_info[conned_bludev_num-1].address, p, BLUETOOTH_MAC_LENGTH);					
								
							break;                            						
						}
						p++;
					}
				}						
			}		
		}
		pclose(fp);
		
		//get connected bluetooth device name and type
		get_kv_rt = file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conned_blue_dev_name", kv_value);
		if(get_kv_rt != 0){
			printf("get kv \"conned_blue_dev_name\" fail\n");
			return -7;
		}
		memcpy(blu_dev_connected->dev.device_info[conned_bludev_num-1].name, kv_value, strlen(kv_value));

		get_kv_rt = file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conned_blue_dev_type", kv_value);
		if(get_kv_rt != 0){
			printf("get kv \"conned_blue_dev_type\" fail\n");
			return -7;
		}
		memcpy(blu_dev_connected->dev.device_info[conned_bludev_num-1].type, kv_value, strlen(kv_value));	
	}
	
	blu_dev_connected->dev.num = conned_bludev_num;	
	memcpy(blu_dev_connected->payload_info, "bluetooth_dev_connected", strlen("bluetooth_dev_connected"));
	return 0;	
}

int gcs_sends_data_unpack(char *json_pack_in)
{
	int ret = 0;
	
	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON  *Item = NULL;
	char cmdtype[50] = {'\0'};

	bluetooth_interface_conf blu_inter_info = {0};
	bluetooth_search disc_blue_equ = {0};
	bluetooth_dev_connect blu_dev = {0};
	bluetooth_dev_connected blu_dev_connected = {0};
		
	Root = cJSON_Parse(json_pack_in);
	if(NULL == Root){
		printf("Parse_content fail\n");
		return -1;
	}	
	Item = cJSON_GetObjectItem(Root, "payload_info");
	memcpy(cmdtype, Item->valuestring, strlen(Item->valuestring));
	
	if(0 == strcmp(cmdtype,"bluetooth_interface_conf")){	
		Object = cJSON_GetObjectItem(Root, "bluetooth_interface_conf");
		Item = cJSON_GetObjectItem(Object, "switch");
		memcpy(blu_inter_info.interface_info.blue_switch, Item->valuestring, strlen(Item->valuestring));
		Item = cJSON_GetObjectItem(Object, "devicename");
		memcpy(blu_inter_info.interface_info.devicename, Item->valuestring, strlen(Item->valuestring)); 	
		Item = cJSON_GetObjectItem(Object, "visible");
		memcpy(blu_inter_info.interface_info.visible, Item->valuestring, strlen(Item->valuestring));
		Item = cJSON_GetObjectItem(Object, "pin_code");
		memcpy(blu_inter_info.interface_info.pin_code, Item->valuestring, strlen(Item->valuestring));
	
		cJSON_Delete(Root); 			
	} else if(0 == strcmp(cmdtype, "bluetooth_search")){
		// No argument
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connect")){
		Object = cJSON_GetObjectItem(Root, "bluetooth_dev_connect");
		Item = cJSON_GetObjectItem(Object, "name");
		memcpy(blu_dev.dev.name, Item->valuestring, strlen(Item->valuestring));
		Item = cJSON_GetObjectItem(Object, "address");
		memcpy(blu_dev.dev.address, Item->valuestring, strlen(Item->valuestring)); 	
		Item = cJSON_GetObjectItem(Object, "type");
		memcpy(blu_dev.dev.type, Item->valuestring, strlen(Item->valuestring));
		
		cJSON_Delete(Root); 	
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connected")){
		// No argument
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connected_test")){
		// No argument
	} else {
		return 1;
	}
	
	if(0 == strcmp(cmdtype, "bluetooth_interface_conf")){	
		ret = conf_blue_mod_interface(&blu_inter_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}
	} else if(0 == strcmp(cmdtype, "bluetooth_search")){
		ret = search_blue_devices(&disc_blue_equ);
		if(ret != 0){
			gw_blue_search_response_str(NULL, SEARCH_BLU_DEV_FAIL);
			return ret;
		}
		gw_blue_search_response_str(&disc_blue_equ, SEARCH_BLU_DEV_SUCCESS);
		return 0;
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connect")){
		ret = connect_blue_device(&blu_dev); 
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connected")){
		ret = connected_blue_device(&blu_dev_connected);
		if(ret != 0){
			gw_connected_bludev_response_str(NULL, GET_CONNECTED_BLUDEV_FAIL);
			return ret;
		}
		gw_connected_bludev_response_str(&blu_dev_connected, GET_CONNECTED_BLUDEV_SUCCESS);
		return 0;		
	} else if(0 == strcmp(cmdtype, "bluetooth_dev_connected_test")){
		ret = system("hcitool con | grep \"<\"");
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}
				
		ret = system("aplay /etc/certusnet_conf/test.wav");
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}
	}
	gw_response_str(cmdtype, "yes");	
	
	return 0;
}
