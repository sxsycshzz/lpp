#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include "sbuf.h"

// MQTT
#define CMD_SIZE	256
#define MQTT_IP		"127.0.0.1"
#define MQTT_PORT	"1883" 
#define MQTT_TOPIC	"mo_datatransfer_topic"
#define PERSON_CHECK_DATA "\'{\"device\":\"personcheck\",\"readings\":[\{\"name\":\"HumnExist\",\"value\":\"%d\"\}]}\'"
#define MQTT_SEND_CMD	"mosquitto_pub -h %s -p %s -t %s -m " PERSON_CHECK_DATA

// serial
#define DEFAULT_DEVFILE "/dev/ttyUSB0"
#define BUF_SIZE	64	
#define RING_BUF_SIZE	256	
#define REPORT_FLAG	0x70
#define REPORT_STATUS_LEN 0x0E
#define REPORT_POWER_LEN 0x0D
#define REPORT_POWER_EN 0

static struct termios save_termios;
static int  ttysavefd = -1;
static enum {RESET , RAW , CBREAK} ttystate = RESET;

int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300};

/**len(2bytes) SNID(4bytes) control_flag(1byte, 0xFE) control_type(1byte) crc8(1byte)**/
unsigned char allow_join[] = {0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x9F, 0x61};
/**receive state report: report(1byte, 0x70), len(1byte), addr(2bytes), Endpoint(1byte), clusterid(2byte, 0x0500), report_count(1byte) 
** attrid(2bytes, 0x0080), data_type(1byte, 0x21), data(2byte), crc8(1byte)**/
unsigned char state_report[] = {0x70, 0x0C, 0xD5, 0xB2, 0x01, 0x00, 0x05, 0x01, \
								0x80, 0x00, 0x21, 0x01, 0x00, 0xCE};
/**receive power report: report(1byte, 0x70), len(1byte), addr(2bytes), Endpoint(1byte), clusterid(2byte, 0x0001), report_count(1byte) 
** attrid(2bytes, 0x0021), data_type(1byte, 0x20), data(1byte), crc8(1byte)**/
/**power: data/2 *100% **/
unsigned char power_report[] = {0x70, 0x0B, 0xD5, 0xB2, 0x01, 0x01, 0x00, 0x01, \
								0x21, 0x00, 0x20, 0xB4, 0xD8};

unsigned char crc8(unsigned char arr[], int len)
{
	int i = 0; 
	unsigned char crc = 0;

	for(i = 0; i < len; i++){
		crc ^= arr[i];
	}

	return crc;
}

bool check_crc8(const unsigned char *data, unsigned int len)
{
	if((NULL == data) || 0 == len){
		return false;
	}
	
	unsigned char crc = crc8((unsigned char *)(data+1), len-2);

	if(crc == data[len-1]){
		return true;	
	} else {
		return false;
	}
}

void
set_speed(int fd, int speed){
        int   i;
        int   status;
        struct termios   Opt;
        tcgetattr(fd, &Opt);
        for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
                if  (speed == name_arr[i]) {
                        tcflush(fd, TCIOFLUSH);
                        cfsetispeed(&Opt, speed_arr[i]);
                        cfsetospeed(&Opt, speed_arr[i]);
                        status = tcsetattr(fd, TCSANOW, &Opt);
                        if  (status != 0) {
                                perror("tcsetattr fd");
                                return;
                        }
                        tcflush(fd,TCIOFLUSH);
                }
        }

        return ;
}

int
set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");
		return 1;
	}
	options.c_cflag &= ~CSIZE;
	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n"); return 1;
	}
	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return 1;
	}
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return 1;
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 200;
	options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return 1;
	}
	return 0;
}

int
tty_raw(int fd)
{
        int err;
        struct termios buf;

        if(ttystate != RESET){
                errno = EINVAL;
                return -1;
        }
        if(tcgetattr(fd, &buf)<0)
                return -1;
        save_termios = buf;

        buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

        buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

        buf.c_cflag &= ~(CSIZE | PARENB);

        buf.c_cflag |= CS8;

        buf.c_oflag &= ~(OPOST);
        buf.c_cc[VMIN] = 1;
        buf.c_cc[VTIME] = 0;
        if(tcsetattr(fd, TCSAFLUSH, &buf)<0)
                return -1;

        if(tcgetattr(fd, &buf)<0){
                err = errno;
                tcsetattr(fd, TCSAFLUSH, &save_termios);
                errno = err;
                return -1;
        }

        if((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) || (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON )) || (buf.c_cflag & ( CSIZE | PARENB | CS8)) != CS8 || (buf.c_oflag & OPOST) || buf.c_cc[VMIN]!=1 || buf.c_cc[VTIME]!=0){
                tcsetattr(fd, TCSAFLUSH, &save_termios);
                errno = EINVAL;
                return -1;

        }

        ttystate = RAW;
        ttysavefd = fd;
        return 0;

}

int main(int argc, char *argv[])
{
	int fd = 0;
	int i = 0, count = 0, ret = 0;
	unsigned char rx_buf[BUF_SIZE], rxp_buf[BUF_SIZE];
	char file[BUF_SIZE] = {0};
	unsigned char state = 0, power = 0, flag_join = 0;
	char cmd[CMD_SIZE] = {0};
	
	if(1 == argc){
		strcpy(file, DEFAULT_DEVFILE);	
	} else {
		sprintf(file, "%s", argv[1]);	
	}
	printf("Usage: %s [/dev/ttyUSBx] [join] [debug]\n", argv[0]);
	printf("The device file: %s\n", file);

	if((3 == argc) && !strcmp(argv[2], "join")){
		flag_join = 1;	
	}

	if(!strcmp(argv[argc-1], "debug")){
		debug = 1;	
	}

	fd = open(file, O_RDWR);
	if(fd == -1){
		perror("open ttyUSBx");
		exit(1);
	}

	tty_raw(fd);

	set_speed(fd, 57600);
	set_Parity(fd, 8, 1, 'n');

	if(flag_join){ // allow devices join the net
		write(fd, allow_join, sizeof(allow_join));
		sleep(1);
	}

	struct sbuf *psb = sbuf_init(RING_BUF_SIZE);
	if(psb == NULL){
		printf("No enough memory!\n");
		return -1;
	}

	unsigned char report_top = REPORT_FLAG;
	unsigned int outlen = 0; 
	while(1){
		memset(rx_buf, '\0', sizeof(rx_buf));
		count = read(fd, rx_buf, sizeof(rx_buf));
		sbuf_in(psb, rx_buf, count);
#if 1
		if(debug){
			printf("----Receive %d bytes:[[", count);
			for(i=0; i < count; i++){
				if(i%10 == 0)
					printf("\n");
				printf("0x%.2x ", 0xFF&rx_buf[i]);
			}
			printf("]]\n");
		}
#endif

#if 1
		while(sbuf_search_with_out_count(psb, &report_top, 1, rxp_buf, &outlen, check_crc8)){
#if 0
			printf("----Parse %d bytes:", outlen);
			for(i=0; i < outlen; i++){
				if(i%10 == 0)
					printf("\n");
				printf("0x%.2x ", 0xFF&rxp_buf[i]);
			}
			printf("\n");
#endif
			if(outlen == REPORT_STATUS_LEN)
			if((rxp_buf[5] == 0x00 && rxp_buf[6] == 0x05) \
				&& (rxp_buf[8] == 0x80 && rxp_buf[9] == 0x00) && rxp_buf[10] == 0x21){

				state = 0x1 & rxp_buf[11];
				
				// mqtt send
				sprintf(cmd, MQTT_SEND_CMD, MQTT_IP, MQTT_PORT, MQTT_TOPIC, state);
				ret = system(cmd);
				if(ret != 0){
					system(cmd);
				}

				if(debug){
					if(state){
						printf("++++++++++++++++++++++++++++++++++++++Person in\n");
					} else {
						printf("++++++++++++++++++++++++++++++++++++++Person out\n");
					}
				}
			}
#if REPORT_POWER_EN
			if(outlen == REPORT_POWER_LEN)
			if((rxp_buf[5] == 0x01 && rxp_buf[6] == 0x00) \
				&& (rxp_buf[8] == 0x21 && rxp_buf[9] == 0x00) && rxp_buf[10] == 0x20){
					power = (0xFF & rxp_buf[11]) >> 1;
					if(debug)
						printf("++++++++++++++++++++++++++++++++++++++power:%d\n", power);
			}
#endif
		}
#endif
	}

	sbuf_destroy(psb);
	close(fd);
	return 0;
}

