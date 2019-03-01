#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "parsekv.h"
#include "cmd.h"

extern char *p_map;

#define _GNU_SOURCE

#define CMD_SIZE	512	
#define IP_SIZE		32	
#define SMALL_SIZE	32	

#define NET_START_CMD	"systemctl restart systemd-networkd"
#define WIFI_START_CMD	"systemctl restart wpa_supplicant"
#define NET_UP_CMD		"ifconfig %s up"
#define NET_DOWN_CMD	"ifconfig %s down"
#define WIFI_UP_CMD		"ifconfig wlp1s0 up >/dev/null 2>&1"
#define WIFI_DOWN_CMD	"ifconfig wlp1s0 down >/dev/null 2>&1"
#define NET_DNS_CMD		"systemctl restart systemd-resolved"
#define NET_DNS_POST_CMD	"ls -al /etc/resolv.conf | grep systemd >/dev/null || " \
							"ln -sf /run/systemd/resolve/resolv.conf /etc/resolv.conf"	 
//#define PPPOE_PRE_CMD	"rm -rf %s"
#define PPPOE_START_CMD	"systemctl restart pppoe"
#define PPPOE_STOP_CMD	"systemctl stop pppoe"
#define PPPOE_ROUTE_CMD	"ifconfig eth0 0.0.0.0 && route del default; route add default dev ppp0"
//#define PPPOE_ROUTE_CMD	"route del default; route add default dev ppp0"

#define NET_FILE		"/etc/systemd/network/20-eth0.network"
#define	NET_DHCP_CONF	"[Match]\nName=%s\n\n[Network]\nDHCP=yes\n\n[DHCP]\nRouteMetric=100\n" 
#define NET_STATIC_CONF	"[Match]\nName=%s\n\n[Network]\nAddress=%s\nDNS=%s\nDNS=%s\n\n[Route]\nGateway=%s\nMetric=100\n"

#define WIFI_CONF_FILE	"/etc/wpa_supplicant.conf"
#define WIFI_CONF_HEAD	"ctrl_interface=/var/run/wpa_supplicant\nupdate_config=1\n\nnetwork={\n\tscan_ssid=1\n"
#define WIFI_CONF_SSID	"\tssid=\"%s\"\n"
#define WIFI_CONF_PSK	"\tpsk=\"%s\"\n"
#define WIFI_CONF_KEY_MGMT	"\tkey_mgmt=%s\n"
#define WIFI_CONF_PAIRWISE	"\tpairwise=%s\n"
#define WIFI_CONF_TAIL	"}\n"

#define PPPOE_CONF_FILE	"/etc/ppp/pppoe.conf"
#define PPPOE_CONF_CHAP_FILE	"/etc/ppp/chap-secrets"	
#define PPPOE_CONF_PAP_FILE		"/etc/ppp/pap-secrets"
#define PPPOE_CONF	"DEMAND=no\nDNSTYPE=SERVER\nPEERDNS=yes\nDEFAULTROUTE=yes\n" \
					"CONNECT_TIMEOUT=30\nCONNECT_POLL=2\n" \
					"LCP_INTERVAL=20\nLCP_FAILURE=3\nPPPOE_TIMEOUT=80\n" \
					"PING=\".\"\n" \
					"CF_BASE=`basename $CONFIG`\n" \
					"PIDFILE=\"/var/run/$CF_BASE-pppoe.pid\"\n" \
					"SYNCHRONOUS=no\nCLAMPMSS=1412\n" \
					"FIREWALL=NONE\nLINUX_PLUGIN=\n" \
					"PPPOE_EXTRA=\"\"\nPPPD_EXTRA=\"\"\n\n" \
					"ETH=%s\nUSER=%s\n"
#define PPPOE_CHAP_PAP	"\"%s\" * \"%s\"\n"


#define CHECK_STATIC_LINK_CMD	"ping %s -c 1 >/dev/null && echo OK"
#define CHECK_DHCP_LINK_CMD		"ifconfig %s | grep -w inet"
#define CHECK_WIFI_LINK_CMD		"ifconfig wlp1s0 | grep -w inet"
#define PRE_CHECK_WIFI_LINK_CMD "systemctl status wpa_supplicant | grep 'Active: active'"
#define CHECK_PPPOE_LINK_CMD	"pppoe-status | grep -w inet"

#define CONF_NET_STATUS_FILE_STATIC "{\"_id\":\"5c6a16744a910d0001bc9c81\",\"payload_info\":\"conf_networking_way_static\",\"conf_networking_way_static\":"\
							     	"{\"ip\":\"%s\",\"netmask\":\"%s\",\"gateway\":\"%s\",\"dns1\":\"%s\",\"dns2\":\"%s\"}}"
#define CONF_NET_STATUS_FILE_DHCP   "{\"_id\":\"5c6a16744a910d0001bc9c81\",\"payload_info\":\"conf_networking_way_dhcp\"}"
#define CONF_NET_STATUS_FILE_WIFI   "{\"_id\":\"5c6a16744a910d0001bc9c81\",\"payload_info\":\"conf_networking_way_wifi\",\"conf_networking_way_wifi\":"\
									"{\"ssid\":\"%s\",\"authentication_method\":\"%s\",\"encryption\":\"%s\",\"passwd\":\"%s\"}}"
#define CONF_NET_STATUS_FILE_PPPOE  "{\"_id\":\"5c6a16744a910d0001bc9c81\",\"payload_info\":\"conf_networking_way_pppoe\",\"conf_networking_way_pppoe\":"\
									"{\"user\":\"%s\",\"passwd\":\"%s\"}}"							

#define NET_CONF_FILE   "/etc/certusnet_conf/wan_way"
#define NET_STATUS_FILE   "/usr/local/gcs/wan_way_status"

int MAX_CHECK_COUNT = 10;
char gateway_ip[20] = {'\0'};	

int check_static_address(char *ip, char *netmask, char *gateway, char *dns1, char *dns2)
{
	int i = 0;
	int rt = -1;
	
	in_addr_t ip_num = 0;						  
	in_addr_t ip_num_temporary = 0;               
	int ip_host_bit_ref_val = 0;                  
	int ip_net_segment_num = 0;					  
	int ip_netmask_num = 0;                      
	
	in_addr_t netmask_num = 0;	
	in_addr_t netmask_num_temporary = 0;
	in_addr_t netmask_num_temporary_bak = 0;      
	int netmask_contain1_num = 0;	
	int netmask_net_segment_num = 0;	                 
	int gateway_netmask_num = 0;                  
		
	in_addr_t gateway_num = 0;
	in_addr_t gateway_num_temporary = 0;
	int gateway_net_segment_num = 0;
	
	in_addr_t dns1_num = 0;
	in_addr_t dns2_num = 0;
	
    rt = inet_pton(AF_INET, ip, &ip_num);
	if(rt != 1){
		printf("ip address format error\n");
		return -1;		
	}

	if(((ip_num & 0x000000FF) == 0)||((ip_num & 0x000000FF) == 127)||((ip_num & 0x000000FF) > 223)){
		printf("IP address format error\n");
		return -1;			
	}
	
    rt = inet_pton(AF_INET, netmask, &netmask_num);
	if(rt != 1){
		printf("netmask address format error\n");
		return -1;		
	}	
	
	netmask_num_temporary = netmask_num;
    while(netmask_num_temporary){
        if(netmask_num_temporary & 0x1){
            ++netmask_contain1_num;
		}
        netmask_num_temporary = (netmask_num_temporary >> 1);
    }
	
	if((netmask_contain1_num > 30) || (netmask_contain1_num < 8)){
		printf("netmask address contain \"1\" over the mark\n");
		return -1;
	}		

	netmask_num_temporary = ((netmask_num & 0x000000FF) << 24) + ((netmask_num & 0x0000FF00) << 8) + ((netmask_num & 0x00FF0000) >> 8) + ((netmask_num & 0xFF000000) >> 24);
	netmask_num_temporary = ~netmask_num_temporary;
	netmask_num_temporary_bak = netmask_num_temporary;               
	netmask_num_temporary = netmask_num_temporary + 1;	
	if((netmask_num_temporary & (netmask_num_temporary - 1)) != 0){
		printf("netmask address contains a discontinuous \"1\" or \"0\", or a \"1\" not at a high position\n");
		return -1;
	}

	ip_num_temporary = ((ip_num & 0x000000FF) << 24) + ((ip_num & 0x0000FF00) << 8) + ((ip_num & 0x00FF0000) >> 8) + ((ip_num & 0xFF000000) >> 24);

	ip_netmask_num = ip_num_temporary & netmask_num_temporary_bak;	
	for(i = 0, ip_host_bit_ref_val = 1; i < 32-netmask_contain1_num; i++){
		ip_host_bit_ref_val = ip_host_bit_ref_val * 2;		
	}
	ip_host_bit_ref_val -= 1;
	
	if((ip_netmask_num == 0) || (ip_netmask_num == ip_host_bit_ref_val)){
		printf("The IP host number is not valid\n");
		return -1;		
	}

    rt = inet_pton(AF_INET, gateway, &gateway_num);
	if(rt != 1){
		printf("gateway address format error\n");
		return -1;		
	}

	if(((gateway_num  & 0x000000FF) == 0) || ((gateway_num  & 0x000000FF) == 127) || ((gateway_num  & 0x000000FF) > 223)){
		printf("Gateway address format error\n");
		return -1;			
	}
	
	gateway_num_temporary = ((gateway_num & 0x000000FF) << 24) + ((gateway_num & 0x0000FF00) << 8) + ((gateway_num & 0x00FF0000) >> 8) + ((gateway_num & 0xFF000000) >> 24);
			
	netmask_num_temporary_bak = ~netmask_num_temporary_bak;                                                               
	ip_net_segment_num = ip_num_temporary & netmask_num_temporary_bak;                                                    
	gateway_net_segment_num = gateway_num_temporary & netmask_num_temporary_bak;                                          
	
	if(ip_net_segment_num != gateway_net_segment_num){
		printf("ip and gateway are not in the same network segment\n");
		return -1;		
	}
	
	netmask_num_temporary_bak = ~netmask_num_temporary_bak;                                                               
	gateway_netmask_num = gateway_num_temporary & netmask_num_temporary_bak;                                           
	
	if((gateway_netmask_num == ip_netmask_num) || (gateway_netmask_num == 0) || (gateway_netmask_num == ip_host_bit_ref_val)){
		printf("The Gateway host number is not valid\n");
		return -1;		
	}
	
    if((inet_pton(AF_INET, dns1, &dns1_num) != 1)||(inet_pton(AF_INET, dns2, &dns2_num) != 1)){
		printf("DNS address format error\n");
		return -1;		
	}
		
	return 0;
}

static int get_prefix_length(char *netmask)
{
	unsigned int a = 0, aa = 0, aaa = 0, aaaa = 0;
	int i = 0;

	sscanf(netmask, "%d.%d.%d.%d", &a, &aa, &aaa, &aaaa);
	a = (a << 24) + (aa << 16) + (aaa << 8) + aaaa;

	for(i = 0; i < 31 && ((0x80000000 >> i)&a); i++);

	return i;
}

static int write_file(char *file, char *buf, int size)
{
	if((NULL == file) || (NULL == buf)){
		return -2;
	}
	
	int fd = 0;
	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 00644);	
	if(!fd){
		perror("open");
		return -2;
	}

	write(fd, buf, size);
	//syncfs(fd);
	sync();
	close(fd);

	return 0;
}

int set_wan_eth(char *ethx, char *way, char *address, char *netmask, char * gateway, char* dns1, char *dns2)
{
	char cmd[CMD_SIZE] = {'\0'};	
	char cmd_tmp[CMD_SIZE] = {'\0'};	
	int ret = 0;

	if(0 == strcmp(way, "static")){
		ret = get_prefix_length(netmask);
		sprintf(cmd_tmp, "/%d", ret);
		strcat(address, cmd_tmp);
		snprintf(cmd, sizeof(cmd), NET_STATIC_CONF, ethx, address, dns1, dns2, gateway);
	} else { // DHCP
		snprintf(cmd, sizeof(cmd), NET_DHCP_CONF, ethx);
	}

	if(write_file(NET_FILE, cmd, strlen(cmd))){
		return -3;
	}

	strcpy(cmd, PPPOE_STOP_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -3;
		}
	}

	sprintf(cmd, NET_UP_CMD, ethx);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}
	sleep(1);	
	
	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -3;
		}
	}
	sleep(1);
	
	strcpy(cmd, WIFI_DOWN_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}

	strcpy(cmd, NET_DNS_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}

	strcpy(cmd, NET_DNS_POST_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}
	return 0;
}

int set_wan_pppoe(char *ethx, char *pppoe_user, char *pppoe_passwd)
{
	char cmd[CMD_SIZE] = {'\0'};	
	int ret = 0;

	snprintf(cmd, sizeof(cmd), PPPOE_CONF, ethx, pppoe_user);
	if(write_file(PPPOE_CONF_FILE, cmd, strlen(cmd))){
		return -4;
	}

	snprintf(cmd, sizeof(cmd), PPPOE_CHAP_PAP, pppoe_user, pppoe_passwd);
	if(write_file(PPPOE_CONF_CHAP_FILE, cmd, strlen(cmd))){
		return -4;
	}
	if(write_file(PPPOE_CONF_PAP_FILE, cmd, strlen(cmd))){
		return -4;
	}

	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -4;
		}
	}
	
	sleep(1);
	
	strcpy(cmd, WIFI_DOWN_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}

	strcpy(cmd, PPPOE_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -4;
		}
	}

	sleep(5);
	strcpy(cmd, PPPOE_ROUTE_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -4;
		}
	}

	return 0;
}

int set_wan_wifi(char *ethx, char *wifi_ssid, char *wifi_psk, char *wifi_key_mgmt, char* wifi_pairwise)
{
	char cmd[CMD_SIZE]={'\0'};	
	int fd = 0, ret = 0;

	char wifi_conf[CMD_SIZE*2] = {'\0'};
	char wifi_conf_tmp[CMD_SIZE] = {'\0'};

	// configure wifi
	strcpy(wifi_conf, WIFI_CONF_HEAD);
	sprintf(wifi_conf_tmp, WIFI_CONF_SSID, wifi_ssid);
	strcat(wifi_conf, wifi_conf_tmp);
	sprintf(wifi_conf_tmp, WIFI_CONF_PSK, wifi_psk);
	strcat(wifi_conf, wifi_conf_tmp);
	if(!strcmp("wep", wifi_key_mgmt)){
		sprintf(wifi_conf_tmp, WIFI_CONF_KEY_MGMT, "WPA-EAP");
		strcat(wifi_conf, wifi_conf_tmp);
	} else if((!strcmp("wpa2_psk", wifi_key_mgmt))||(!strcmp("wpa1_psk", wifi_key_mgmt))){
		sprintf(wifi_conf_tmp, WIFI_CONF_KEY_MGMT, "WPA-PSK");
		strcat(wifi_conf, wifi_conf_tmp);
	} else {
		// auto : none
	}
	if(!strcmp("aes", wifi_pairwise)){
		sprintf(wifi_conf_tmp, WIFI_CONF_PAIRWISE, "CCMP");
		strcat(wifi_conf, wifi_conf_tmp);
	} else if(!strcmp("tkip", wifi_pairwise)){
		sprintf(wifi_conf_tmp, WIFI_CONF_PAIRWISE, "TKIP");
		strcat(wifi_conf, wifi_conf_tmp);
	} else {
		// auto : none
	}
	strcat(wifi_conf, WIFI_CONF_TAIL);

	fd = open(WIFI_CONF_FILE, O_WRONLY | O_CREAT | O_TRUNC, 00644);	
	if(!fd){
		perror("open");
		return -5;
	}
	write(fd, wifi_conf, strlen(wifi_conf));
	//syncfs(fd);
	sync();
	close(fd);
	
	// start wifi
	strcpy(cmd, PPPOE_STOP_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -5;
		}
	}

	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -5;
		}
	}

	sleep(1);
	
	strcpy(cmd, WIFI_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -5;
		}
	}

	sleep(1);
	
	sprintf(cmd, NET_DOWN_CMD, ethx);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}
	
	strcpy(cmd, NET_DNS_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}

	strcpy(cmd, NET_DNS_POST_CMD);
	ret = system(cmd);
	if(ret != 0){
		system(cmd);
	}

	return 0;
}

static int exec_cmd(char *buf, int buf_size, char *cmd)
{
	if(NULL == cmd || NULL == buf || buf_size <= 0){
		return -6;
	}

	FILE *f = NULL;
	int len = 0;

	f = popen(cmd, "r");
	if(NULL == f){
		return -6;
	}

    //memset(buf, '\0', buf_size);
	if(NULL == fgets(buf, buf_size, f)){
		pclose(f);
		return -6;
	}

	pclose(f);

	len = strlen(buf);
	if(len > 0 && (buf[len-1] == '\n')){
		buf[len-1] = '\0';
		len --;
	}

	return len;
}

int pre_check_wifi_link(void)
{
	int ret = 0;
	char cmd[CMD_SIZE] = {'\0'};	
	char buf[CMD_SIZE] = {'\0'};

	sprintf(cmd, PRE_CHECK_WIFI_LINK_CMD);
	ret = exec_cmd(buf, sizeof(buf), cmd);
	if((ret < 0) || !strcmp("", buf)){
		return -8;
	}
	
	return 0;	
}

int check_wan_link(char *ethx, char *way, char *gateway)
{
	if((NULL == way) || (NULL == ethx)){
		return -9;	
	}
	
	int ret = 0;
	char cmd[CMD_SIZE] = {'\0'};	
	char buf[CMD_SIZE] = {'\0'};

	if(!strcmp("static", way) && (NULL != gateway)){
		sprintf(cmd, CHECK_STATIC_LINK_CMD, gateway);
	} else if(!strcmp("dhcp", way)){
		sprintf(cmd, CHECK_DHCP_LINK_CMD, ethx);
	} else if(!strcmp("wifi", way)){
		sprintf(cmd, CHECK_WIFI_LINK_CMD);
	} else if(!strcmp("pppoe", way)){
		sprintf(cmd, CHECK_PPPOE_LINK_CMD);
	} else {
		return -9;
	}	
	ret = exec_cmd(buf, sizeof(buf), cmd);
	if((ret<0) || !strcmp("", buf)){
		return -9;
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
	cJSON_AddItemToObject(Root, "payload_info", Item);
	Item = cJSON_CreateString(result);
	cJSON_AddItemToObject(Root, "conf_success", Item);

	json_pack_out = cJSON_Print(Root);
    //printf("\n%s\n",json_pack_out);
			
	memset(p_map, 0, 1024);                                  
	memcpy(p_map, json_pack_out, strlen(json_pack_out));       //update p_map
	//printf("\n%s\n",p_map);
			
	free(json_pack_out);                  
	cJSON_Delete(Root);		
}

void gw_response_str_withIP(char *cmdtype, char *result, char *gateway_ip)
{
	cJSON *Root = NULL;
	cJSON *Item = NULL;
	char *json_pack_out = NULL;
	
	Root = cJSON_CreateObject();	
	Item = cJSON_CreateString(cmdtype);
	cJSON_AddItemToObject(Root, "payload_info", Item);
	Item = cJSON_CreateString(result);
	cJSON_AddItemToObject(Root, "conf_success", Item);
	Item = cJSON_CreateString(gateway_ip);
	cJSON_AddItemToObject(Root, "ip", Item);

	json_pack_out = cJSON_Print(Root);
    //printf("\n%s\n",json_pack_out);
			
	memset(p_map, 0, 1024);                                  
	memcpy(p_map, json_pack_out, strlen(json_pack_out));       //update p_map
	//printf("\n%s\n",p_map);
			
	free(json_pack_out);                  
	cJSON_Delete(Root);		
}

int set_net_status_file(void)
{
	int len = -1, fd = -1;
	char gateway_networking_way[10] = {'\0'};
	
	char address[IP_SIZE] = {'\0'};
	char netmask[IP_SIZE] = {'\0'};
	char gateway[IP_SIZE] = {'\0'};
	char nameserver1[IP_SIZE] = {'\0'};
	char nameserver2[IP_SIZE] = {'\0'};

	char wifi_ssid[SMALL_SIZE] = {'\0'};
	char wifi_psk[CMD_SIZE] = {'\0'};
	char wifi_key_mgmt[SMALL_SIZE] = {'\0'};
	char wifi_pairwise[SMALL_SIZE] = {'\0'};
	
	char pppoe_user[SMALL_SIZE] = {'\0'};
	char pppoe_passwd[SMALL_SIZE] = {'\0'};
	
	char cmd[CMD_SIZE] = {'\0'};	
		
	//get networking way
	fd = open(NET_CONF_FILE, O_RDONLY);
	if(fd < 0){
		perror("open");
		return -12;
	}
	if(read(fd, gateway_networking_way, sizeof(gateway_networking_way) - 1) < 0){
		perror("read");
		return -13;		
	}
	close(fd);

	len = strlen(gateway_networking_way);
	if(len > 0 && (gateway_networking_way[len-1] == '\n')){
		gateway_networking_way[len-1] = '\0';
	}	
	
	if(0 == strcmp(gateway_networking_way, "static")){
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_ip", address) != 0){
			printf("get kv \"networking_way_static_ip\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_netmask", netmask) != 0){
			printf("get kv \"networking_way_static_netmask\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_gateway", gateway) != 0){
			printf("get kv \"networking_way_static_gateway\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns1", nameserver1) != 0){
			printf("get kv \"networking_way_static_dns1\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns2", nameserver2) != 0){
			printf("get kv \"networking_way_static_dns2\" fail\n");
			return -7;
		}

		snprintf(cmd, sizeof(cmd), CONF_NET_STATUS_FILE_STATIC, address, netmask, gateway, nameserver1, nameserver2);
		if(write_file(NET_STATUS_FILE, cmd, strlen(cmd))){
			return -18;
		}				
	} else if(0 == (strcmp(gateway_networking_way, "dhcp"))){
		// No argument	
		snprintf(cmd, sizeof(cmd), CONF_NET_STATUS_FILE_DHCP);
		if(write_file(NET_STATUS_FILE, cmd, strlen(cmd))){
			return -18;
		}
	} else if(0 == (strcmp(gateway_networking_way, "wifi"))){
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_ssid", wifi_ssid) != 0){
			printf("get kv \"networking_way_wifi_ssid\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_pawd", wifi_psk) != 0){
			printf("get kv \"networking_way_wifi_pawd\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_auth", wifi_key_mgmt) != 0){
			printf("get kv \"networking_way_wifi_auth\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_encr", wifi_pairwise) != 0){
			printf("get kv \"networking_way_wifi_encr\" fail\n");
			return -7;
		}

		snprintf(cmd, sizeof(cmd), CONF_NET_STATUS_FILE_WIFI, wifi_ssid, wifi_key_mgmt, wifi_pairwise, wifi_psk);
		if(write_file(NET_STATUS_FILE, cmd, strlen(cmd))){
			return -18;
		}		
	} else if(0 == (strcmp(gateway_networking_way, "pppoe"))){
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_user", pppoe_user) != 0){
			printf("get kv \"networking_way_pppoe_user\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_pawd", pppoe_passwd) != 0){
			printf("get kv \"networking_way_pppoe_pawd\" fail\n");
			return -7;
		}

		snprintf(cmd, sizeof(cmd), CONF_NET_STATUS_FILE_PPPOE, pppoe_user, pppoe_passwd);
		if(write_file(NET_STATUS_FILE, cmd, strlen(cmd))){
			return -18;
		}		
	}
	
	return 0;
}

int update_dhcp_sec_profile(void)
{					
	system("echo dhcp > /etc/certusnet_conf/wan_way");
	
	return 0;
}

int update_static_sec_profile(char *address, char *netmask, char *gateway, char *nameserver1, char *nameserver2)
{
	system("echo static > /etc/certusnet_conf/wan_way");		
	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_ip", address) != 0){
		printf("get kv \"networking_way_static_ip\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_netmask", netmask) != 0){
		printf("get kv \"networking_way_static_netmask\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_gateway", gateway) != 0){
		printf("get kv \"networking_way_static_gateway\" fail\n");
		return -7;
	}		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns1", nameserver1) != 0){
		printf("get kv \"networking_way_static_dns1\" fail\n");
		return -7;
	}		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns2", nameserver2) != 0){
		printf("get kv \"networking_way_static_dns2\" fail\n");
		return -7;
	}				

	return 0;
}

int update_wifi_sec_profile(char *wifi_ssid, char *wifi_psk, char *wifi_key_mgmt, char *wifi_pairwise)
{
	int len = 0;

	system("echo wifi > /etc/certusnet_conf/wan_way");		
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_ssid", wifi_ssid) != 0)
	{
		printf("file_update_value \"networking_way_wifi_ssid\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_pawd", wifi_psk) != 0){
		printf("get kv \"networking_way_wifi_pawd\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_auth", wifi_key_mgmt) != 0){
		printf("get kv \"networking_way_wifi_auth\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_encr", wifi_pairwise) != 0){
		printf("get kv \"networking_way_wifi_encr\" fail\n");
		return -7;
	}				

	return 0;
}

int update_pppoe_sec_profile(char *pppoe_user, char *pppoe_passwd)
{
	system("echo pppoe > /etc/certusnet_conf/wan_way");	
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_user", pppoe_user) != 0){
		printf("get kv \"networking_way_pppoe_user\" fail\n");
		return -7;
	}
	if(file_update_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_pawd", pppoe_passwd) != 0){
		printf("get kv \"networking_way_pppoe_pawd\" fail\n");
		return -7;
	}				

	return 0;	
}

int get_gw_ip(char *gateway_ip)
{
	int fd = -1, len = -1;
	float f = 0.0;	
	char gateway_networking_way[10] = {'\0'};
	char buf[100] = {'\0'};

	//get networking way
	fd = open(NET_CONF_FILE, O_RDONLY);
	if(fd < 0){
		perror("open");
		return -12;
	}
	if(read(fd, gateway_networking_way, sizeof(gateway_networking_way) - 1) < 0){
		perror("read");
		return -13;		
	}
	close(fd);

	len = strlen(gateway_networking_way);
	if(len > 0 && (gateway_networking_way[len-1] == '\n')){
		gateway_networking_way[len-1] = '\0';
	}	
	
	if(0 == strcmp(gateway_networking_way, "wifi")){
		if(out_gw(buf, sizeof(buf), "wlp1s0", &f) > 0){
			memcpy(gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"wlp1s0\" IP address\n");
			return -14;
		}	 				
	} else if((0 == (strcmp(gateway_networking_way, "static"))) || (0 == strcmp(gateway_networking_way, "dhcp"))){
		if(out_gw(buf, sizeof(buf), "ip1", &f) > 0){
			memcpy(gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"eth0\" IP address\n");
			return -15;
		}					
	} else if(0 == (strcmp(gateway_networking_way, "pppoe"))){
		if(out_gw(buf, sizeof(buf), "pppoe", &f) > 0){
			memcpy(gateway_ip, buf, strlen(buf));
		} else {
			printf("Failed to obtain port \"pppoe\" IP address\n");
			return -16;
		}			
	} else {
		return -17;
	}

	return 0;
}

int rollback_func(void)
{
	int fd = -1, len = 0;
	char net_type[10] = {'\0'};
	
	char ethx[IP_SIZE] = "eth0";

	char address[IP_SIZE] = {'\0'};
	char netmask[IP_SIZE] = {'\0'};
	char gateway[IP_SIZE] = {'\0'};
	char nameserver1[IP_SIZE] = {'\0'};
	char nameserver2[IP_SIZE] = {'\0'};

	char wifi_ssid[SMALL_SIZE] = {'\0'};
	char wifi_psk[CMD_SIZE] = {'\0'};
	char wifi_key_mgmt[SMALL_SIZE] = {'\0'};
	char wifi_pairwise[SMALL_SIZE] = {'\0'};
	
	char pppoe_user[SMALL_SIZE] = {'\0'};
	char pppoe_passwd[SMALL_SIZE] = {'\0'};
	
	int ret = 0, count = 1;
	
	fd = open(NET_CONF_FILE, O_RDONLY);
	if(fd < 0){
		perror("open");
		return -10;
	}
	if(read(fd, net_type, sizeof(net_type) - 1) < 0){
		perror("read");
		return -10;		
	}
	close(fd);
	
	len = strlen(net_type);
	if(len > 0 && (net_type[len-1] == '\n')){
		net_type[len-1] = '\0';
	}
	
	if(!strcmp("static", net_type)){		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_ip", address) != 0){
			printf("get kv \"networking_way_static_ip\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_netmask", netmask) != 0){
			printf("get kv \"networking_way_static_netmask\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_gateway", gateway) != 0){
			printf("get kv \"networking_way_static_gateway\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns1", nameserver1) != 0){
			printf("get kv \"networking_way_static_dns1\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_static_dns2", nameserver2) != 0){
			printf("get kv \"networking_way_static_dns2\" fail\n");
			return -7;
		}	
	} else if(!strcmp("dhcp", net_type)){
		// No argument
	} else if(!strcmp("wifi", net_type)){
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_ssid", wifi_ssid) != 0){
			printf("get kv \"networking_way_wifi_ssid\" fail\n");
			return -7;
		}		
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_pawd", wifi_psk) != 0){
			printf("get kv \"networking_way_wifi_pawd\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_auth", wifi_key_mgmt) != 0){
			printf("get kv \"networking_way_wifi_auth\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_wifi_encr", wifi_pairwise) != 0){
			printf("get kv \"networking_way_wifi_encr\" fail\n");
			return -7;
		}			
	} else if(!strcmp("pppoe", net_type)){
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_user", pppoe_user) != 0){
			printf("get kv \"networking_way_pppoe_user\" fail\n");
			return -7;
		}
		if(file_get_value("/etc/certusnet_conf/gateway_conf_file_sub_client", "networking_way_pppoe_pawd", pppoe_passwd) != 0){
			printf("get kv \"networking_way_pppoe_pawd\" fail\n");
			return -7;
		}	
	} else {
		return -10;
	}

	if(!strcmp(net_type, "dhcp")){
		ret = set_wan_eth(ethx, net_type, NULL, NULL, NULL, NULL, NULL);
	} else if(!strcmp(net_type, "static")){
		ret = set_wan_eth(ethx, net_type, address, netmask, gateway, nameserver1, nameserver2);	
	} else if(!strcmp(net_type, "wifi")){		
		ret = set_wan_wifi(ethx, wifi_ssid, wifi_psk, wifi_key_mgmt, wifi_pairwise);
	} else if(!strcmp(net_type, "pppoe")){		
		ret = set_wan_pppoe(ethx, pppoe_user, pppoe_passwd);
	}	

	// check the network link
	if(!ret){
		printf("set config successfully, then check link...\n");

		// static->wifi, success after about 8s.
		// dhcp->wifi, success after about 18s.
		// others in 5s.
		if(0 == strcmp(net_type, "static")){
			return 0;
		}		
		if(0 == strcmp(net_type, "wifi")){
			sleep(3);
			ret = pre_check_wifi_link();
			if(ret){
				printf("The wifi links failed.\n");	
				return ret;
			}
			sleep(5);
			MAX_CHECK_COUNT = 20;
		} else {
			MAX_CHECK_COUNT = 10;
		}
		for(ret = -1; ret != 0 && count < MAX_CHECK_COUNT; count++){
			sleep(1);
			ret = check_wan_link(ethx, net_type, gateway); 
		}

		if(count == MAX_CHECK_COUNT){
			printf("The network is not connected after %d seconds.\n", count);
			return ret;
		} else {
			printf("The network is connected after %d seconds.\n", count);
		}
	}
	
	return 0;
}

int gcs_sends_data_unpack(char *json_pack_in)           
{
	cJSON *Root = NULL;
	cJSON *Object = NULL;
	cJSON *Item = NULL;	
	char cmdtype[50] = {'\0'};
	char net_type[10] = {'\0'};
	
	char ethx[IP_SIZE] = "eth0";

	char address[IP_SIZE] = {'\0'};
	char address_tmp[IP_SIZE] = {'\0'};
	char netmask[IP_SIZE] = {'\0'};
	char gateway[IP_SIZE] = {'\0'};
	char nameserver1[IP_SIZE] = {'\0'};
	char nameserver2[IP_SIZE] = {'\0'};

	char wifi_ssid[SMALL_SIZE] = {'\0'};
	char wifi_psk[CMD_SIZE] = {'\0'};
	char wifi_key_mgmt[SMALL_SIZE] = {'\0'};
	char wifi_pairwise[SMALL_SIZE] = {'\0'};
	
	char pppoe_user[SMALL_SIZE] = {'\0'};
	char pppoe_passwd[SMALL_SIZE] = {'\0'};
	
	int ret = 0, count = 1;
		
	Root = cJSON_Parse(json_pack_in);
	if(NULL == Root){
		printf("Parse_content fail\n");
		return -11;
	}	
	Item = cJSON_GetObjectItem(Root, "payload_info");
	memcpy(cmdtype, Item->valuestring, strlen(Item->valuestring));
	
	if(0 == strcmp(cmdtype,"conf_networking_way_static")){
		Object = cJSON_GetObjectItem(Root, "conf_networking_way_static");
		Item = cJSON_GetObjectItem(Object, "ip");
		memcpy(address, Item->valuestring, strlen(Item->valuestring));
		memcpy(address_tmp, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "netmask");
		memcpy(netmask, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "gateway");
		memcpy(gateway, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "dns1");
		memcpy(nameserver1, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "dns2");
		memcpy(nameserver2, Item->valuestring, strlen(Item->valuestring));			
					
		cJSON_Delete(Root);
		
		ret = check_static_address(address, netmask, gateway, nameserver1, nameserver2);
		if(ret != 0){
			if(rollback_func() != 0){
				printf("rollback_func failed");	
			}
			//gw_response_str(cmdtype, "no");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			gw_response_str(cmdtype,"no");			
			
			return ret;
		}
	} else if(0 == strcmp(cmdtype, "conf_networking_way_dhcp")){	
		// No argument
	} else if(0 == strcmp(cmdtype, "conf_networking_way_wifi")){				
		Object = cJSON_GetObjectItem(Root, "conf_networking_way_wifi");
		Item = cJSON_GetObjectItem(Object, "ssid");
		memcpy(wifi_ssid, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "passwd");					
		memcpy(wifi_psk, Item->valuestring, strlen(Item->valuestring));	
		
		Item = cJSON_GetObjectItem(Object, "authentication_method"); 	
		memcpy(wifi_key_mgmt, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "encryption");
		memcpy(wifi_pairwise, Item->valuestring, strlen(Item->valuestring));
			
		cJSON_Delete(Root);				
	}
	else if(0 == strcmp(cmdtype, "conf_networking_way_pppoe")){
		Object = cJSON_GetObjectItem(Root, "conf_networking_way_pppoe");
		Item = cJSON_GetObjectItem(Object, "user");
		memcpy(pppoe_user, Item->valuestring, strlen(Item->valuestring));
		
		Item = cJSON_GetObjectItem(Object, "passwd");					
		memcpy(pppoe_passwd, Item->valuestring, strlen(Item->valuestring));			
			
		cJSON_Delete(Root);			
	} else {
		return 1;
	}	
	
	if(!strcmp(cmdtype, "conf_networking_way_dhcp")){
		//memset(net_type,0,sizeof(net_type));
		strcpy(net_type, "dhcp");
		ret = set_wan_eth(ethx, net_type, NULL, NULL, NULL, NULL, NULL);
		if(ret != 0){
			if(rollback_func() != 0){
				printf("rollback_func failed");	
			}
			//gw_response_str(cmdtype, "no");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			gw_response_str(cmdtype,"no");
			
			return ret;
		}		
	} else if(!strcmp(cmdtype, "conf_networking_way_static")){
		//memset(net_type,0,sizeof(net_type));
		strcpy(net_type, "static");
		ret = set_wan_eth(ethx, net_type, address, netmask, gateway, nameserver1, nameserver2);
	} else if(!strcmp(cmdtype, "conf_networking_way_wifi")){
		//memset(net_type,0,sizeof(net_type));
		strcpy(net_type, "wifi");		
		ret = set_wan_wifi(ethx, wifi_ssid, wifi_psk, wifi_key_mgmt, wifi_pairwise);
		if(ret != 0){
			if(rollback_func() != 0){
				printf("rollback_func failed");	
			}
			//gw_response_str(cmdtype, "no");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			gw_response_str(cmdtype,"no");
			
			return ret;
		}		
	} else if(!strcmp(cmdtype, "conf_networking_way_pppoe")){
		//memset(net_type,0,sizeof(net_type));
		strcpy(net_type, "pppoe");		
		ret = set_wan_pppoe(ethx, pppoe_user, pppoe_passwd);
		if(ret != 0){
			if(rollback_func() != 0){
				printf("rollback_func failed");	
			}
			//gw_response_str(cmdtype, "no");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			gw_response_str(cmdtype,"no");
			
			return ret;
		}		
	}

	// check the network link
	if(!ret){
		printf("set config successfully, then check link...\n");

		// static->wifi, success after about 8s.
		// dhcp->wifi, success after about 18s.
		// others in 5s.
		if(0 == strcmp(net_type, "static")){
			if(update_static_sec_profile(address_tmp, netmask, gateway, nameserver1, nameserver2) != 0){
				printf("update_static_sec_profile failed\n");
			}
			//gw_response_str(cmdtype,"yes");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			ret = set_net_status_file();
			if(ret != 0){
				return ret;
			}
			gw_response_str(cmdtype,"yes");
			
			return 0;
		}		
		if(0 == strcmp(net_type, "wifi")){
			sleep(3);
			ret = pre_check_wifi_link();
			if(ret){
				printf("The wifi links failed.\n");	
				if(rollback_func() != 0){
					printf("rollback_func failed");	
				}
				//gw_response_str(cmdtype,"no");
				memset(gateway_ip, 0, sizeof(gateway_ip));
				ret = get_gw_ip(gateway_ip);
				if(ret != 0){	//Theories don't happen
					if((ret == -12) || (ret == -13) || (ret == -17)){
						printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
					} else {
						printf("Failed to get gateway current IP\n");
					}
				}
				gw_response_str(cmdtype,"no");
				
				return ret;
			}
			sleep(5);
			MAX_CHECK_COUNT = 20;
		} else {
			MAX_CHECK_COUNT = 10;
		}
		for(ret = -1; ret != 0 && count < MAX_CHECK_COUNT; count++){
			sleep(1);
			ret = check_wan_link(ethx, net_type, gateway); 
		}
		
		if(count == MAX_CHECK_COUNT){
			printf("The network is not connected after %d seconds.\n", count);
			if(rollback_func() != 0){
				printf("rollback_func failed");	
			}
			//gw_response_str(cmdtype,"no");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}
			gw_response_str(cmdtype,"no");
			
			return ret;
		} else {
			printf("The network is connected after %d seconds.\n", count);
			if(!strcmp(net_type, "dhcp")){
				update_dhcp_sec_profile();
			} else if(!strcmp(net_type, "wifi")){
				if(update_wifi_sec_profile(wifi_ssid, wifi_psk, wifi_key_mgmt, wifi_pairwise) != 0){
					printf("update_wifi_sec_profile failed\n");
				}
			} else if(!strcmp(net_type, "pppoe")){
				if(update_pppoe_sec_profile(pppoe_user, pppoe_passwd) != 0){
					printf("update_pppoe_sec_profile failed\n");
				}
			}
			//gw_response_str(cmdtype,"yes");
			memset(gateway_ip, 0, sizeof(gateway_ip));
			ret = get_gw_ip(gateway_ip);
			if(ret != 0){	//Theories don't happen
				if((ret == -12) || (ret == -13) || (ret == -17)){
					printf("Failed to get \"/etc/certusnet_conf/wan_way\" file contents\n");
				} else {
					printf("Failed to get gateway current IP\n");
				}
			}		
			ret = set_net_status_file();
			if(ret != 0){
				return ret;
			}
			gw_response_str(cmdtype,"yes");
		}
	}
	
	return 0; 
}
