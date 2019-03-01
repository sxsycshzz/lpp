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

static int init_blue_mod_interface(void)     
{
	char conf_blue_interface_switch[10] = {'\0'};
	char conf_blue_interface_devicename[30] = {'\0'};
	char conf_blue_interface_devicename_cmd[50] = {'\0'};
	char conf_blue_interface_visible[10] = {'\0'};
	
	//Gets the switch state, name, and visibility of the bluetooth module
	if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_switch", conf_blue_interface_switch) != 0){
		printf("get kv \"conf_blue_interface_switch\" fail\n");
		return -7;
	}
	
	if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_devicename", conf_blue_interface_devicename) != 0){
		printf("get kv \"conf_blue_interface_devicename\" fail\n");
		return -7;
	}	
	
	if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "conf_blue_interface_visible", conf_blue_interface_visible) != 0){
		printf("get kv \"conf_blue_interface_visible\" fail\n");
		return -7;
	}	
		
	if(0 == strcmp(conf_blue_interface_switch, "on")){
		if(system("hciconfig hci0 up")){
			printf("the Function system(\"hciconfig hci0 up\") execution failed\n");
			return -1;
		}	
		
		sprintf(conf_blue_interface_devicename_cmd, "hciconfig hci0 name %s", conf_blue_interface_devicename);
		if(system(conf_blue_interface_devicename_cmd)){
			printf("the Function system(\"%s\") execution failed\n", conf_blue_interface_devicename_cmd);
			return -3;			
		}
		
		if(0 == strcmp(conf_blue_interface_visible, "yes")){
			if(system("hciconfig hci0 iscan")){
				printf("the Function system(\"hciconfig hci0 iscan\") execution failed\n");
				return -4;
			}							
		} else if(0 == strcmp(conf_blue_interface_visible, "no")){
			if(system("hciconfig hci0 noscan")){
				printf("the Function system(\"hciconfig hci0 noscan\") execution failed\n");
				return -5;
		    }						
		} else {
			printf("the value \"conf_blue_interface_visible\" ormal error");
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
	} else if(0 == strcmp(conf_blue_interface_switch, "off")){
		if(system("hciconfig hci0 down")){
			printf("the Function system(\"hciconfig hci0 down\") execution failed\n");		
			return -2;
		}		
	} else {
		printf("the value \"conf_blue_interface_switch\" ormal error");	
		return -6;
	}
		
	return 0;		
}

static int init_net(void)
{
	
	return 0;
}


int main(void)
{
	int ret_net = 0, ret_blue = 0;
	
	ret_blue = init_blue_mod_interface();
	if(ret_blue != 0){
		printf("Failed to initialize bluetooth module interface, ret_blue = %d\n", ret_blue);		
	}
	
	ret_net = init_net();
	if(ret_blue != 0){
		printf("Failed to initialize net, ret_net = %d\n", ret_net);		
	}
	
	if((ret_net || ret_blue) != 0){
		return -1;
	}
	
	return 0;
}

