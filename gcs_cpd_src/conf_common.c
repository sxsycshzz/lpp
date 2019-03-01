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

extern char *p_map;


typedef  struct conf_gateway_passwd{
	char payload_info[20];
	char current_passwd[50];
	char new_passwd[50];
}conf_gateway_passwd;								   								   								

typedef struct gateway_name{                          
	char first_half[20];
	char second_half[20];
}gateway_name;

typedef struct conf_gateway_info{                          
	char payload_info[30];
	gateway_name  name;
	char gateway_key[50];           
	char gateway_sn[20];
	char current_time[25];
}conf_gateway_info;


typedef struct conf_platform{                          
	char payload_info[20];
	char ga_man_plat_ip[20];
	int ga_man_plat_port;
	char ga_man_plat_name[20];
	char ga_man_plat_passwd[30];
}conf_platform;

// uart
#define UART_FILE_PORT   "/dev/ttyS1"         //  a serial port name
typedef struct uart_interface{                          
	int baud_rate;
	int data_bit;
	char parity[10];
	float stop_bit;
}uart_interface;

typedef struct uart_interface_conf{                          
	char payload_info[30];
	uart_interface  uart_info;
}uart_interface_conf;


// usb1
typedef struct usb1_device_info{                          
	char type[20];
	char manufacturer[20];
	char model_num[20];
}usb1_device_info;

typedef struct usb1_correlation_sensor{                          
	int number;
	usb1_device_info  device_info[10];            
}usb1_correlation_sensor;

typedef struct usb1_interface{                          
	char usb1_device_type[10];
	usb1_correlation_sensor  dev;            
}usb1_interface;

typedef struct usb1_interface_conf{                          
	char payload_info[20];
	usb1_interface dev_usb1;
}usb1_interface_conf;

//usb2
typedef struct usb2_device_info{                          
	char type[20];
	char manufacturer[20];
	char model_num[20];
}usb2_device_info;

typedef struct usb2_correlation_sensor{                          
	int number;
	usb2_device_info  device_info[10];           
}usb2_correlation_sensor;

typedef struct usb2_interface{                          
	char usb2_device_type[10];
	usb2_correlation_sensor  dev;            
}usb2_interface;

typedef struct usb2_interface_conf{                          
	char payload_info[20];
	usb2_interface dev_usb2;
}usb2_interface_conf;

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
			
	memset(p_map, 0, 1024);                                    
	memcpy(p_map, json_pack_out, strlen(json_pack_out));        //update p_map
	//printf("\n%s\n",p_map);
			
	free(json_pack_out);                  
	cJSON_Delete(Root);		
}

int update_gw_passwd_profile(char *new_passwd)
{
    if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "current_passwd", new_passwd) != 0){
        printf("file_update_value \"current_passwd\" fail\n");
        return -7;
    }
	
	return 0;
}

int update_gw_info_profile(char *first_half, char *second_half, char *gateway_key, char *gateway_sn)
{	
    if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "name.first_half", first_half) != 0){
        printf("file_update_value \"name.first_half\" fail\n");
        return -7;
    }
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "name.second_half", second_half) != 0){
        printf("file_update_value \"name.second_half\" fail\n");
        return -7;
    }
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "gateway_key", gateway_key) != 0){
        printf("file_update_value \"gateway_key\" fail\n");                                                      
        return -7;
    }
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "gateway_sn", gateway_sn) != 0){
        printf("file_update_value \"gateway_sn\" fail\n");
        return -7;
    }
	
	return 0;
}

int update_cloud_platform_profile(char *ga_man_plat_ip, int ga_man_plat_port, char *ga_man_plat_name, char *ga_man_plat_passwd)
{	
	char ga_man_plat_port_buff[10] = {'\0'};
	
    if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_ip", ga_man_plat_ip) != 0){
        printf("file_update_value \"ga_man_plat_ip\" fail\n");
        return -7;
    }
	
	sprintf(ga_man_plat_port_buff, "%d", ga_man_plat_port);	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_port", ga_man_plat_port_buff) != 0){
        printf("file_update_value \"ga_man_plat_port\" fail\n");
        return -7;
    }
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_name", ga_man_plat_name) != 0){
        printf("file_update_value \"ga_man_plat_name\" fail\n");
        return -7;
    } 
	
 	if(file_update_value("/etc/certusnet_conf/gateway_conf_info", "ga_man_plat_passwd", ga_man_plat_passwd) != 0){
        printf("file_update_value \"ga_man_plat_passwd\" fail\n");
        return -7;
    } 
 	
	return 0;
}

int update_uart_interface_profile(int baud_rate, int data_bit, char *parity, float stop_bit)
{
	char baud_rate_buff[10] = {'\0'};
	char data_bit_buff[5] = {'\0'};
	char stop_bit_buff[5] = {'\0'};		

	sprintf(baud_rate_buff, "%d", baud_rate);
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "uart_baud_rate", baud_rate_buff) != 0){
		printf("file_update_value \"uart_baud_rate\" fail\n");
		return -7;
	}
	
	sprintf(data_bit_buff, "%d", data_bit);	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "uart_data_bit", data_bit_buff) != 0){
		printf("file_update_value \"uart_data_bit\" fail\n");
		return -7;
	}
		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "uart_parity", parity) != 0){
		printf("file_update_value \"uart_parity\" fail\n");
		return -7;
	}
	
	sprintf(stop_bit_buff, "%.1f", stop_bit);	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "uart_stop_bit", stop_bit_buff) != 0){
		printf("file_update_value \"uart_stop_bit\" fail\n");
		return -7;
	}
	
	return 0;
}

int update_usb1_interface_profile(usb1_interface_conf *usb1_inter_info)
{
	int i = 0;
	char number_buff[5] = {'\0'};
	char sensor_type[30] = {'\0'};
	char sensor_model_num[30] = {'\0'};
	char sensor_manufacturer[30] = {'\0'};
		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "usb1_device_type", usb1_inter_info->dev_usb1.usb1_device_type) != 0){
		printf("file_update_value \"usb1_device_type\" fail\n");
		return -7;
	}
	
	sprintf(number_buff, "%d", usb1_inter_info->dev_usb1.dev.number);
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "usb1_sensor_num", number_buff) != 0){
		printf("file_update_value \"usb1_sensor_num\" fail\n");
		return -7;
	}
		
  	for(i = 0; i < usb1_inter_info->dev_usb1.dev.number; i++){
		sprintf(sensor_type, "usb1_sensor%d_type", i + 1);
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_type, usb1_inter_info->dev_usb1.dev.device_info[i].type) != 0){
			printf("file_update_value \"usb1_sensor%d_type\" fail\n", i + 1);                   
			return -7;
		}
		sprintf(sensor_manufacturer, "usb1_sensor%d_manufacturer", i + 1);	
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_manufacturer, usb1_inter_info->dev_usb1.dev.device_info[i].manufacturer) != 0){
			printf("file_update_value \"usb1_sensor%d_manufacturer\" fail\n",i + 1);
			return -7;
		} 
		sprintf(sensor_model_num,"usb1_sensor%d_model_num",i + 1);	
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_model_num, usb1_inter_info->dev_usb1.dev.device_info[i].model_num) != 0){
			printf("file_update_value \"usb1_sensor%d_model_num\" fail\n",i + 1);
			return -7;
		} 				
	}  
	
	return 0;
}

int update_usb2_interface_profile(usb2_interface_conf *usb2_inter_info)
{
	int i = 0;
	char number_buff[5] = {'\0'};
	char sensor_type[30] = {'\0'};
	char sensor_model_num[30] = {'\0'};
	char sensor_manufacturer[30] = {'\0'};
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "usb2_device_type", usb2_inter_info->dev_usb2.usb2_device_type) != 0){
		printf("file_update_value \"usb2_device_type\" fail\n");
		return -7;
	}
	
	sprintf(number_buff, "%d", usb2_inter_info->dev_usb2.dev.number);
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "usb2_sensor_num", number_buff) != 0){
		printf("file_update_value \"usb2_sensor_num\" fail\n");
		return -7;
	}
		
	for(i = 0; i < usb2_inter_info->dev_usb2.dev.number; i++){
		sprintf(sensor_type, "usb2_sensor%d_type", i + 1);
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_type, usb2_inter_info->dev_usb2.dev.device_info[i].type) != 0){
			printf("file_update_value \"usb2_sensor%d_type\" fail\n", i + 1);                   
			return -7;
		}
		sprintf(sensor_manufacturer, "usb2_sensor%d_manufacturer", i + 1);	
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_manufacturer, usb2_inter_info->dev_usb2.dev.device_info[i].manufacturer) != 0){
			printf("file_update_value \"usb2_sensor%d_manufacturer\" fail\n",i + 1);
			return -7;
		} 
		sprintf(sensor_model_num,"usb2_sensor%d_model_num",i + 1);	
		if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", sensor_model_num, usb2_inter_info->dev_usb2.dev.device_info[i].model_num) != 0){
			printf("file_update_value \"usb2_sensor%d_model_num\" fail\n",i + 1);
			return -7;
		} 				
	}
	
	return 0;
}
	
static int conf_gw_passwd(conf_gateway_passwd *gw_passwd_info)   
{	
    char cmd_buff[150] = {'\0'};         
	int ret = -1;
	
	//cryptographic check, not used
	/*char kv_value[50] = {'\0'};
    if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "current_passwd", kv_value) != 0){
        printf("get kv \"current_passwd\" fail\n");
        return -7;
    }
    if(strcmp(kv_value,gw_passwd_info->current_passwd) != 0){
        printf("The input password does not match the backup password\n");
		return -1;
    }*/

    sprintf(cmd_buff, "(echo %s; sleep 1; echo %s) | passwd user > /dev/null 2>&1 && exit 0", gw_passwd_info->new_passwd, gw_passwd_info->new_passwd);
    ret = system(cmd_buff);
    if(ret != 0){     
        printf("Failed to change gateway password\n");	
        return -2;
    }
	
    ret = update_gw_passwd_profile(gw_passwd_info->new_passwd);
	if(ret != 0){
		return ret;
	}
		
	return 0;
}

static int conf_gw_info(conf_gateway_info *gw_info)
{
	int ret = -1;     
	
	//change gateway time, not used
	/*char cmd_buff[50] = {0};	
	sprintf(cmd_buff,"date  -s  \"%s\"",gw_info->current_time);
	rt = system("timedatectl set-ntp 0");
    rt = rt || system(cmd_buff);
	rt = rt || system("hwclock -w");
	if(rt != 0){     
       printf("Failed to change gateway time\n");
       return -3;
    }*/		
	
	ret = update_gw_info_profile(gw_info->name.first_half, gw_info->name.second_half, gw_info->gateway_key, gw_info->gateway_sn);
	if(ret != 0){
		return ret;
	}
	
	return 0;
}

static int conf_cloud_platform(conf_platform *plat_info)
{	
	int ret = -1;
	
	ret = update_cloud_platform_profile(plat_info->ga_man_plat_ip, plat_info->ga_man_plat_port, plat_info->ga_man_plat_name, plat_info->ga_man_plat_passwd);
	if(ret != 0){
		return ret;
	}
	
	return 0;
}

int set_serial(int fd, int nSpeed, int nBits, char nEvent, float nStop)         
{ 
    struct termios newttys1, oldttys1;

    if(tcgetattr(fd,&oldttys1) != 0){
        perror("Setupserial 1");
        return -6;
    }
    bzero(&newttys1,sizeof(newttys1));
    newttys1.c_cflag |= (CLOCAL | CREAD);
    newttys1.c_cflag &= ~CSIZE;
  
    switch(nBits){
		case 5:
            newttys1.c_cflag |= CS5;
            break;
        case 6:
            newttys1.c_cflag |= CS6;
            break;
        case 7:
            newttys1.c_cflag |= CS7;
            break;
        case 8:
            newttys1.c_cflag |= CS8;
            break;
    }

    switch(nEvent){
        case 'O':  
            newttys1.c_cflag |= PARENB;
            newttys1.c_iflag |= (INPCK | ISTRIP);
            newttys1.c_cflag |= PARODD;
            break;
        case 'E':
            newttys1.c_cflag |= PARENB; 
            newttys1.c_iflag |= (INPCK | ISTRIP);
            newttys1.c_cflag &= ~PARODD;
            break;
        case 'N': 
            newttys1.c_cflag &= ~PARENB;
            break;
    }
 
    switch(nSpeed){
		case 1200:
            cfsetispeed(&newttys1, B1200);
            cfsetospeed(&newttys1, B1200);
            break;
        case 2400:
            cfsetispeed(&newttys1, B2400);
            cfsetospeed(&newttys1, B2400);
            break;
        case 4800:
            cfsetispeed(&newttys1, B4800);
            cfsetospeed(&newttys1, B4800);
            break;
        case 9600:
            cfsetispeed(&newttys1, B9600);
            cfsetospeed(&newttys1, B9600);
            break;
		case 19200:
            cfsetispeed(&newttys1, B19200);
            cfsetospeed(&newttys1, B19200);
            break;
        case 115200:
            cfsetispeed(&newttys1, B115200);
            cfsetospeed(&newttys1, B115200);
            break;
        default:                                  
            cfsetispeed(&newttys1, B9600);
            cfsetospeed(&newttys1, B9600);
            break;
    }

    if(nStop == 1.0){
        newttys1.c_cflag &= ~CSTOPB;
    } else if(nStop == 2.0){
        newttys1.c_cflag |= CSTOPB;
    } else {
		newttys1.c_cflag |= CSTOPB;     
	}
	
	fcntl(fd, F_SETFL, 0); 
    newttys1.c_cc[VTIME] = 3;
    newttys1.c_cc[VMIN]  = 9; 
    tcflush(fd, TCIFLUSH);
	
    if((tcsetattr(fd, TCSANOW, &newttys1)) != 0){
        perror("com set error");
        return -8;
    }

    return 0;
}

static int conf_uart_interface(uart_interface_conf *uart_inter_info)
{
	int ret = -1, fd = -1;                    
	char uart_parity_char = '\0';
		
	if(!strcmp(uart_inter_info->uart_info.parity, "none")){
		uart_parity_char = 'N';
	}
	else if(!strcmp(uart_inter_info->uart_info.parity, "odd")){
		uart_parity_char = 'O';
	}
	else if(!strcmp(uart_inter_info->uart_info.parity, "even")){
		uart_parity_char = 'E';
	} else {
		return -4;
	}
	
	fd = open(UART_FILE_PORT, O_RDWR|O_NOCTTY|O_NDELAY);   
	if(fd < 0){
		perror("open");
		return -5;
	}
	
    ret = set_serial(fd, uart_inter_info->uart_info.baud_rate, uart_inter_info->uart_info.data_bit, uart_parity_char, uart_inter_info->uart_info.stop_bit);        
	if(ret != 0){		
		return ret;
	}
	close(fd); 
 	
	ret = update_uart_interface_profile(uart_inter_info->uart_info.baud_rate, uart_inter_info->uart_info.data_bit, uart_inter_info->uart_info.parity, uart_inter_info->uart_info.stop_bit);
	if(ret != 0){		
		return ret;
	}	
	
	return 0;	
}

static int conf_usb1_interface(usb1_interface_conf *usb1_inter_info)
{
	int ret = -1;
	ret = update_usb1_interface_profile(usb1_inter_info);	
	if(ret != 0){
		return ret;
	}
		
	return 0;
}
 
static int conf_usb2_interface(usb2_interface_conf *usb2_inter_info)
{
	int ret = -1;
	ret = update_usb2_interface_profile(usb2_inter_info);	
	if(ret != 0){
		return ret;
	}
		
	return 0;
} 

int gcs_sends_data_unpack(char *json_pack_in)
{
	int i = 0, ret = 0;
	
	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON *Object_child = NULL;
	cJSON  *Item = NULL;
	char device_mark[30] = {'\0'};       
	char Object_mark[30] = {'\0'};
	char cmdtype[50] = {'\0'};

	conf_gateway_passwd  gw_passwd_info = {0};
	conf_gateway_info gw_info = {0};
	conf_platform plat_info = {0};
	uart_interface_conf uart_inter_info = {0};
	usb1_interface_conf usb1_inter_info = {0};
	usb2_interface_conf usb2_inter_info = {0};	
	
	Root = cJSON_Parse(json_pack_in);
	if(NULL == Root){
		printf("Parse_content fail\n");
		return -1;
	}	
	Item = cJSON_GetObjectItem(Root, "payload_info");
	memcpy(cmdtype, Item->valuestring, strlen(Item->valuestring));	
	if(0 == strcmp(cmdtype,"conf_gateway_passwd")){		
		Item = cJSON_GetObjectItem(Root, "payload_info");
		memcpy(gw_passwd_info.payload_info, Item->valuestring, strlen(Item->valuestring));	
		
		Item = cJSON_GetObjectItem(Root, "current_passwd");
		memcpy(gw_passwd_info.current_passwd, Item->valuestring, strlen(Item->valuestring));	
		
		Item = cJSON_GetObjectItem(Root, "new_passwd");
		memcpy(gw_passwd_info.new_passwd,Item->valuestring,strlen(Item->valuestring));
		
		cJSON_Delete(Root); 					
	} else if(0 == strcmp(cmdtype, "conf_gateway_info")){
		Object = cJSON_GetObjectItem(Root, "gateway_name");
		Item = cJSON_GetObjectItem(Object, "first_half");
		memcpy(gw_info.name.first_half,Item->valuestring,strlen(Item->valuestring)); 	
		Item = cJSON_GetObjectItem(Object, "second_half");
		memcpy(gw_info.name.second_half,Item->valuestring,strlen(Item->valuestring));
		
		// not used
		Item = cJSON_GetObjectItem(Root, "gateway_key");                                 
		memcpy(gw_info.gateway_key,Item->valuestring,strlen(Item->valuestring));             

		Item = cJSON_GetObjectItem(Root, "gateway_sn");
		memcpy(gw_info.gateway_sn,Item->valuestring,strlen(Item->valuestring));
	
		// not used
		Item = cJSON_GetObjectItem(Root, "current_time");
		memcpy(gw_info.current_time,Item->valuestring,strlen(Item->valuestring));	
		
		cJSON_Delete(Root); 		
	} else if(0 == strcmp(cmdtype, "conf_platform")){
		Item = cJSON_GetObjectItem(Root, "payload_info");
		memcpy(plat_info.payload_info,Item->valuestring,strlen(Item->valuestring));	
		
		Item = cJSON_GetObjectItem(Root, "ga_man_plat_ip");
		memcpy(plat_info.ga_man_plat_ip,Item->valuestring,strlen(Item->valuestring));

		Item = cJSON_GetObjectItem(Root, "ga_man_plat_port");
		plat_info.ga_man_plat_port = Item->valueint;

		// not used
		Item = cJSON_GetObjectItem(Root, "ga_man_plat_name");
		memcpy(plat_info.ga_man_plat_name,Item->valuestring,strlen(Item->valuestring));
		// not used	
		Item = cJSON_GetObjectItem(Root, "ga_man_plat_passwd");
		memcpy(plat_info.ga_man_plat_passwd,Item->valuestring,strlen(Item->valuestring));
				
		cJSON_Delete(Root); 
	} else if(0 == strcmp(cmdtype, "uart_interface_conf")){
		Object = cJSON_GetObjectItem(Root, "uart_interface_conf");
		
		Item = cJSON_GetObjectItem(Object, "baud_rate");	
		uart_inter_info.uart_info.baud_rate = Item->valueint;
		
		Item = cJSON_GetObjectItem(Object, "data_bit");
		uart_inter_info.uart_info.data_bit = Item->valueint; 
		
		Item = cJSON_GetObjectItem(Object, "parity");
		memcpy(uart_inter_info.uart_info.parity, Item->valuestring, strlen(Item->valuestring));	
		
		Item = cJSON_GetObjectItem(Object, "stop_bit");
		uart_inter_info.uart_info.stop_bit = Item->valuedouble;

		cJSON_Delete(Root);
	} else if(0 == strcmp(cmdtype, "usb1_interface_conf")){
		Object = cJSON_GetObjectItem(Root, "usb1_interface_conf");
		Item = cJSON_GetObjectItem(Object, "usb1_device_type");
		memcpy(usb1_inter_info.dev_usb1.usb1_device_type, Item->valuestring, strlen(Item->valuestring));		
		Object_child = cJSON_GetObjectItem(Object, "usb1_correlation_sensor");
		Item = cJSON_GetObjectItem(Object_child, "number");
		usb1_inter_info.dev_usb1.dev.number = Item->valueint;			
		for(i = 0; i < usb1_inter_info.dev_usb1.dev.number; i++){			
			sprintf(Object_mark, "Object_child_child_%d", i+1);
			sprintf(device_mark, "sensor_%d", i+1);
			cJSON *Object_mark = NULL;	
			Object_mark = cJSON_GetObjectItem(Object_child, device_mark);
			Item = cJSON_GetObjectItem(Object_mark, "type");	
			memcpy(usb1_inter_info.dev_usb1.dev.device_info[i].type, Item->valuestring, strlen(Item->valuestring));	
			Item = cJSON_GetObjectItem(Object_mark, "manufacturer");	
			memcpy(usb1_inter_info.dev_usb1.dev.device_info[i].manufacturer, Item->valuestring, strlen(Item->valuestring));
			Item = cJSON_GetObjectItem(Object_mark, "model_num");
			memcpy(usb1_inter_info.dev_usb1.dev.device_info[i].model_num, Item->valuestring, strlen(Item->valuestring));			
		}			
		cJSON_Delete(Root); 		
	} else if(0 == strcmp(cmdtype, "usb2_interface_conf")){
		Object = cJSON_GetObjectItem(Root, "usb2_interface_conf");
		Item = cJSON_GetObjectItem(Object, "usb2_device_type");
		memcpy(usb2_inter_info.dev_usb2.usb2_device_type, Item->valuestring, strlen(Item->valuestring));
		
		Object_child = cJSON_GetObjectItem(Object, "usb2_correlation_sensor");
		Item = cJSON_GetObjectItem(Object_child, "number");
		usb2_inter_info.dev_usb2.dev.number = Item->valueint;
			
		for(i = 0; i < usb2_inter_info.dev_usb2.dev.number; i++){			
			sprintf(Object_mark, "Object_child_child_%d", i+1);
			sprintf(device_mark, "sensor_%d", i+1);
			cJSON *Object_mark = NULL;
				
			Object_mark = cJSON_GetObjectItem(Object_child, device_mark);
			Item = cJSON_GetObjectItem(Object_mark, "type");
			memcpy(usb2_inter_info.dev_usb2.dev.device_info[i].type, Item->valuestring, strlen(Item->valuestring));
			Item = cJSON_GetObjectItem(Object_mark, "manufacturer");
			memcpy(usb2_inter_info.dev_usb2.dev.device_info[i].manufacturer, Item->valuestring, strlen(Item->valuestring));
			Item = cJSON_GetObjectItem(Object_mark, "model_num");
			memcpy(usb2_inter_info.dev_usb2.dev.device_info[i].model_num, Item->valuestring, strlen(Item->valuestring));		
		}			
		cJSON_Delete(Root); 			
	} else {
		return 1;
	}
	if(0 == strcmp(cmdtype,"conf_gateway_passwd")){			
		ret = conf_gw_passwd(&gw_passwd_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}		
	} else if(0 == strcmp(cmdtype, "conf_gateway_info")){
		ret = conf_gw_info(&gw_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}				
	} else if(0 == strcmp(cmdtype, "conf_platform")){
		ret = conf_cloud_platform(&plat_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}
	} else if(0 == strcmp(cmdtype, "uart_interface_conf")){
		ret = conf_uart_interface(&uart_inter_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}		
	} else if(0 == strcmp(cmdtype, "usb1_interface_conf")){
		ret = conf_usb1_interface(&usb1_inter_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}		
	} else if(0 == strcmp(cmdtype, "usb2_interface_conf")){
		ret = conf_usb2_interface(&usb2_inter_info);
		if(ret != 0){
			gw_response_str(cmdtype, "no");
			return ret;
		}				
	}
	gw_response_str(cmdtype, "yes");	
	
	return 0;
}











