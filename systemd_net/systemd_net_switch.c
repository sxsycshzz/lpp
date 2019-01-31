#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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
#define NET_DNS_POST_CMD	"ls -al /etc/resolv.conf | grep systmed >/dev/null || " \
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
					"CF_BASE=\`basename \$CONFIG\`\n" \
					"PIDFILE=\"/var/run/\$CF_BASE-pppoe.pid\"\n" \
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

int MAX_CHECK_COUNT = 10;	

static int get_prefix_length(char *netmask)
{
	unsigned int a=0, aa=0, aaa=0, aaaa=0;
	int i = 0;

	sscanf(netmask, "%d.%d.%d.%d", &a, &aa, &aaa, &aaaa);
	a = (a<<24) + (aa<<16) + (aaa<<8) + aaaa;

	for(i=0; i<31 && ((0x80000000>>i)&a); i++);

	return i;
}

static int write_file(char *file, char *buf, int size)
{
	if((NULL == file) || (NULL == buf)){
		return -1;
	}
	
	int fd = 0;
	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 00644);	
	if(!fd){
		perror("open");
		return -1;
	}

	write(fd, buf, size);
	//syncfs(fd);
	sync();
	close(fd);

	return 0;
}

int set_wan_eth(char *ethx, char *way, char *address, char *netmask, char * gateway, char* dns1, char *dns2)
{
	char cmd[CMD_SIZE]={0};	
	char cmd_tmp[CMD_SIZE]={0};	
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
		return -1;
	}

	strcpy(cmd, PPPOE_STOP_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

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
	char cmd[CMD_SIZE]={0};	
	int ret = 0;

	snprintf(cmd, sizeof(cmd), PPPOE_CONF, ethx, pppoe_user);
	if(write_file(PPPOE_CONF_FILE, cmd, strlen(cmd))){
		return -1;
	}

	snprintf(cmd, sizeof(cmd), PPPOE_CHAP_PAP, pppoe_user, pppoe_passwd);
	if(write_file(PPPOE_CONF_CHAP_FILE, cmd, strlen(cmd))){
		return -1;
	}
	if(write_file(PPPOE_CONF_PAP_FILE, cmd, strlen(cmd))){
		return -1;
	}

	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

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
			return -10;
		}
	}

	sleep(5);
	strcpy(cmd, PPPOE_ROUTE_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -10;
		}
	}

	return 0;
}

int set_wan_wifi(char *ethx, char *wifi_ssid, char *wifi_psk, char *wifi_key_mgmt, char* wifi_pairwise)
{
	char cmd[CMD_SIZE]={0};	
	int ret = 0;

	char wifi_conf[CMD_SIZE*2] = {0};
	char wifi_conf_tmp[CMD_SIZE] = {0};

	// configure wifi
	strcpy(wifi_conf, WIFI_CONF_HEAD);
	sprintf(wifi_conf_tmp, WIFI_CONF_SSID, wifi_ssid);
	strcat(wifi_conf, wifi_conf_tmp);
	sprintf(wifi_conf_tmp, WIFI_CONF_PSK, wifi_psk);
	strcat(wifi_conf, wifi_conf_tmp);
	if(!strcmp("wep", wifi_key_mgmt)){
		sprintf(wifi_conf_tmp, WIFI_CONF_KEY_MGMT, "WPA-EAP");
		strcat(wifi_conf, wifi_conf_tmp);
	} else if(!strcmp("wpa2_psk", wifi_key_mgmt)){
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

	if(write_file(WIFI_CONF_FILE, wifi_conf, strlen(wifi_conf))){
		return -1;
	}
	
	// start wifi
	strcpy(cmd, PPPOE_STOP_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

	strcpy(cmd, NET_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

	strcpy(cmd, WIFI_START_CMD);
	ret = system(cmd);
	if(ret != 0){
		ret = system(cmd);
		if(ret != 0){
			return -2;
		}
	}

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
		return -1;
	}

	FILE *f = NULL;
	int len = 0;

	f= popen(cmd, "r");
	if(NULL == f){
		return -1;
	}

//	memset(buf, '\0', buf_size);
	if(NULL == fgets(buf, buf_size, f)){
		pclose(f);
		return -1;
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
	char cmd[CMD_SIZE]={0};	
	char buf[CMD_SIZE]={0};

	sprintf(cmd, PRE_CHECK_WIFI_LINK_CMD);
	ret = exec_cmd(buf, sizeof(buf), cmd);
	if((ret<0) || !strcmp("", buf)){
		return -5;
	}
	
	return 0;	
}

int check_wan_link(char *ethx, char *way, char *gateway)
{
	if((NULL == way) || (NULL == ethx)){
		return -1;	
	}
	
	int ret = 0;
	char cmd[CMD_SIZE]={0};	
	char buf[CMD_SIZE]={0};

	if(!strcmp("static", way) && (NULL != gateway)){
		sprintf(cmd, CHECK_STATIC_LINK_CMD, gateway);
	} else if(!strcmp("dhcp", way)){
		sprintf(cmd, CHECK_DHCP_LINK_CMD, ethx);
	} else if(!strcmp("wifi", way)){
		sprintf(cmd, CHECK_WIFI_LINK_CMD);
	} else if(!strcmp("pppoe", way)){
		sprintf(cmd, CHECK_PPPOE_LINK_CMD);
	} else {
		return -4;
	}	
	ret = exec_cmd(buf, sizeof(buf), cmd);
	if((ret<0) || !strcmp("", buf)){
		return -4;
	}
	
	return 0;	
}

int main(int argc, char *argv[])
{
	if(argc < 3){
		printf("Usage: %s ethX dhcp|static|wifi|pppoe\n", argv[0]);		
		printf("\t %s ethX pppoe [username] [passwd]\n", argv[0]);
		printf("\t %s ethX static [addr] [netmask] [gateway] [dns1] [dns2]\n", argv[0]);		
		printf("\t %s ethX wifi [ssid] [psk] [key_mgmt] [pairwise]\n", argv[0]);		
		printf("Note: not check [addr], [ssid] and [...]!!!\n\tPlease make sure they are OK!!!\n");
		printf("-------\n");
		printf("[key_mgmt] may be: auto, wep, wpa2_psk\n");
		printf("[pairwise] may be: auto, tkip, aes\n\n");
		return -1;
	}

	char ethx[IP_SIZE] = {0};

	char address[IP_SIZE]="192.168.10.111";
//	char address[IP_SIZE]="172.17.6.124";
	char netmask[IP_SIZE]="255.255.255.0";
	char gateway[IP_SIZE]="192.168.10.1";
//	char gateway[IP_SIZE]="172.17.6.254";
	char nameserver1[IP_SIZE]="8.8.8.8";
	char nameserver2[IP_SIZE]="8.8.4.4";

	char wifi_ssid[SMALL_SIZE]="Certusnet";
	char wifi_psk[CMD_SIZE]="fv40icVY";
	char wifi_key_mgmt[SMALL_SIZE]="wpa2_psk";
	char wifi_pairwise[SMALL_SIZE]="aes";

	char pppoe_user[SMALL_SIZE]="test";
	char pppoe_passwd[SMALL_SIZE]="123456";

	int ret = 0, count = 1;
	
	strcpy(ethx, argv[1]);

	if(!strcmp(argv[2], "pppoe") && (argc==5)){
		strcpy(pppoe_user, argv[3]);
		strcpy(pppoe_passwd, argv[4]);
	}
	if(!strcmp(argv[2], "static") && (argc==8)){
		strcpy(address, argv[3]);
		strcpy(netmask, argv[4]);
		strcpy(gateway, argv[5]);
		strcpy(nameserver1, argv[6]);
		strcpy(nameserver2, argv[7]);
	}
	if(!strcmp(argv[2], "wifi") && (argc==7)){
		strcpy(wifi_ssid, argv[3]);
		strcpy(wifi_psk, argv[4]);
		strcpy(wifi_key_mgmt, argv[5]);
		strcpy(wifi_pairwise, argv[6]);
	}

	if(!strcmp(argv[2], "dhcp")){
		ret = set_wan_eth(ethx, argv[2], NULL, NULL, NULL, NULL, NULL);
	} else if(!strcmp(argv[2], "static")){
		ret = set_wan_eth(ethx, argv[2], address, netmask, gateway, nameserver1, nameserver2);
	} else if(!strcmp(argv[2], "wifi")){
		ret = set_wan_wifi(ethx, wifi_ssid, wifi_psk, wifi_key_mgmt, wifi_pairwise);
	} else if(!strcmp(argv[2], "pppoe")){
		ret = set_wan_pppoe(ethx, pppoe_user, pppoe_passwd);
	} else {
		return -3;
	}

	// check the network link
	if(!ret){
		printf("set config successfully, then check link...\n");

		// static->wifi, success after about 8s.
		// dhcp->wifi, success after about 18s.
		// others in 5s.
		if(!strcmp(argv[2], "wifi")){
			sleep(3);
			ret = pre_check_wifi_link();
			if(ret){
				printf("The wifi links failed.\n");	
				return ret;
			}
			sleep(5);
			MAX_CHECK_COUNT = 20;
		}
		for(ret = -1; ret != 0 && count < MAX_CHECK_COUNT; count++){
			sleep(1);
			ret = check_wan_link(ethx, argv[2], gateway); 
		}

		if(count == MAX_CHECK_COUNT){
			printf("The network is not connected after %d seconds.\n", count);
		} else {
			printf("The network is connected after %d seconds.\n", count);
		}
	}

	return ret;
}

