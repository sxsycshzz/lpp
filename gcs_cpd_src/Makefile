ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

obj =   MQTTClient_publish.o MQTTClient_net_subscribe.o MQTTClient_bluetooth_subscribe.o MQTTClient_common_subscribe.o conf_network.o conf_bluetooth.o conf_common.o init_blue_net.o
obj +=  MQTTClient.o Base64.o Clients.o Heap.o LinkedList.o Log.o MQTTPacket.o MQTTPacketOut.o MQTTPersistence.o MQTTPersistenceDefault.o MQTTProperties.o MQTTProtocolClient.o 
obj +=  MQTTProtocolOut.o MQTTReasonCodes.o Messages.o OsWrapper.o SHA1.o SSLSocket.o Socket.o SocketBuffer.o StackTrace.o Thread.o Tree.o WebSocket.o utf-8.o cJSON.o parsekv.o cmd.o

parameters = -pthread -ldl -o

CUR_DIR = $(shell pwd)
OUT_DIR = $(shell pwd)/output
PROFILE_DIR = $(shell pwd)/profile
MQTT_LIB_DIR = $(shell pwd)/MQTT_library
TEST_PROGRAM_DIR = $(shell pwd)/test_program
TEST_AUDIO_DIR = $(shell pwd)/test_audio
PRE_ROOT ?= ../deployment/
OBJ_DIR = $(PRE_ROOT)usr/local/bin/
PROFILE_OBJ_DIR = $(PRE_ROOT)etc/certusnet_conf/
MQTT_LIB_OBJ_DIR = $(PRE_ROOT)lib/

ext_file_pub = MQTT_server_cloud_client_pub
ext_file_net = MQTT_net_sub
ext_file_blu = MQTT_blu_sub
ext_file_com = MQTT_comm_sub
ext_file_init = init_blue_net

source_file_base =  MQTTClient.c Base64.c Clients.c Heap.c LinkedList.c Log.c MQTTPacket.c MQTTPacketOut.c MQTTPersistence.c MQTTPersistenceDefault.c MQTTProperties.c MQTTProtocolClient.c
source_file_base += MQTTProtocolOut.c MQTTReasonCodes.c Messages.c OsWrapper.c SHA1.c SSLSocket.c Socket.c SocketBuffer.c StackTrace.c Thread.c Tree.c WebSocket.c utf-8.c cJSON.c parsekv.c cmd.c 
source_file_pub = MQTTClient_publish.c $(source_file_base)
source_file_net = MQTTClient_net_subscribe.c conf_network.c $(source_file_base)
source_file_blu = MQTTClient_bluetooth_subscribe.c conf_bluetooth.c $(source_file_base)
source_file_com = MQTTClient_common_subscribe.c conf_common.c $(source_file_base)
source_file_init = init_blue_net.c parsekv.c 

all: $(obj)
	@$(CC) $(source_file_pub) $(parameters) $(ext_file_pub)
	@$(CC) $(source_file_net) $(parameters) $(ext_file_net)
	@$(CC) $(source_file_blu) $(parameters) $(ext_file_blu)
	@$(CC) $(source_file_com) $(parameters) $(ext_file_com)
	@$(CC) $(source_file_init) $(parameters) $(ext_file_init)
	@mv $(ext_file_pub) $(ext_file_net) $(ext_file_blu) $(ext_file_com) $(ext_file_init) $(OUT_DIR)

.PHONY: pre_install
pre_install:
	cp $(OUT_DIR)/* $(OBJ_DIR) -f
	cp $(PROFILE_DIR)/* $(PROFILE_OBJ_DIR) -f
	cp $(TEST_PROGRAM_DIR)/* $(OBJ_DIR) -f
	cp $(MQTT_LIB_DIR)/* $(MQTT_LIB_OBJ_DIR) -df
	cp $(TEST_AUDIO_DIR)/* $(PROFILE_OBJ_DIR) -f

.PHONY: clean
clean:
	rm *.o $(OUT_DIR)/*   -f
