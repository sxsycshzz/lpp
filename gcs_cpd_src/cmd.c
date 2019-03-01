#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CMD_BUF_SIZE	256 

typedef float (*func_trans)(char *buf, int buf_size);
static float trans_cpu_avg_rate(char *buf, int buf_size);
static float trans_cpu_now_rate(char *buf, int buf_size);
static float trans_mem_rate(char *buf, int buf_size);

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

struct st_cmd_gw{
	char *item;
	char format;
	char *cmd;
	func_trans trans;
	char *factor;
} cmd_gw[] = {
	{"ip1", 's', "ifconfig eth0 | awk '/inet /{ print $2; }' | cut -d : -f 2", NULL, NULL},
	{"wlp1s0", 's', "ifconfig wlp1s0 | awk '/inet /{ print $2; }' | cut -d : -f 2", NULL, NULL},
	{"pppoe", 's', "ifconfig ppp0 | awk '/inet /{ print $2; }' | cut -d : -f 2", NULL, NULL},	
//	{"ip1", 's', "ifconfig ens33 | awk '/inet addr/{ print $2; }' | cut -d : -f 2", NULL, NULL},	
	{"time_now", 's', "date '+%Y-%m-\%d %H:%M:\%S'", NULL, NULL},
	{"time_up", 's', "uptime | awk '{ print $3; }' | cut -d ',' -f1", NULL, NULL},
	{"cpu_avg_rate", 'f', "cat /proc/loadavg | cut -d ' ' -f 1", trans_cpu_avg_rate, "100*\%f/4"},
	{"cpu_now_rate", 'f', "top -b -n 1|grep Cpu|awk '{print $8; }' ", trans_cpu_now_rate, "100.00-\%f"},
	{"cpu_usr_now_rate", 'f', "top -b -n 1|grep Cpu|awk '{print $2; }'", NULL, NULL},
	{"disk_rate", 'f', "df -h | grep '/dev/root' | awk '{print $5}'", NULL, NULL},
	{"disk_use_f", 'f', "df -h | grep '/dev/root' | awk '{print $3}'", NULL, NULL},
	{"disk_all_f", 'f', "df -h | grep '/dev/root' | awk '{print $4}'", NULL, NULL},
	{"disk_use_s", 's', "df -h | grep '/dev/root' | awk '{print $3}'", NULL, NULL},
	{"disk_all_s", 's', "df -h | grep '/dev/root' | awk '{print $4}'", NULL, NULL},
//	{"disk_rate", 'f', "df -h | grep '/dev/sda' | awk '{print $5}'", NULL, NULL},
	{"ram_rate", 'f', "cat /proc/meminfo | awk '/MemTotal/{print $2}'", trans_mem_rate, NULL},
/*-----------------------------------below not usable-------------------------------------------*/
	{"ram_all", 'f', "cat /proc/meminfo | awk '/MemTotal/{print $2}'", NULL, NULL},
	{"ram_free", 'f', "cat /proc/meminfo | awk '/MemFree/{print $2}'", NULL, NULL},
	{"cpu_kernel_num", 's', "cat /proc/cpuinfo | grep \"processor\" | wc -l | awk '{print $1}'", NULL, NULL}
//	{"cpu_kernel_num", 's', "cat /proc/cpuinfo | grep \"processor\" | wc -l ", NULL, NULL}
};

// out: output the result of the string type
// f: output the result of the float type
int out_gw(char *out, int out_len, const char *in, float *f)
{
	if(NULL == in || NULL == out || out_len <= 0){
		return -1;
	}

	int i = 0;
	int len = strlen(in);
	int count = sizeof(cmd_gw)/sizeof(cmd_gw[0]);

	for(; i < count; i++){
		if(!strncasecmp(in, cmd_gw[i].item, len)){
			break;			
		}	
	}

	if(i >count){
		return -1;
	}

	int ret = exec_cmd(out, out_len, cmd_gw[i].cmd);
	if(ret <= 0){
		return -1;
	}
	
	*f = 0.0;
	if('f' == cmd_gw[i].format){
		if(cmd_gw[i].trans){
			*f = (cmd_gw[i].trans)(out, out_len);	
		} else {
			*f = atof(out);
		}
	}
	
	return cmd_gw[i].format;
}

static float trans_cpu_avg_rate(char *buf, int buf_size)
{
	return 100 * atof(buf) / 4;
}

static float trans_cpu_now_rate(char *buf, int buf_size)
{
	return 100.00 - atof(buf);
}

static float trans_mem_rate(char *buf, int buf_size)
{
	float f = 0.0;
	char out_free[CMD_BUF_SIZE] = {'\0'};
	int ret_free = out_gw(out_free, sizeof(out_free), "mem_free", &f);

	if('f' != (char)ret_free){
		return -1;
	}

	return 100.00 - 100.00 * atof(out_free) / atof(buf);
}


