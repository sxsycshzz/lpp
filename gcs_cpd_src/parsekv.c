#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define	K_SIZE		128
#define	V_SIZE		128
#define	KV_SIZE		2*128
#define	BUF_SIZE	1024 

char *kv_del_blank(char *str)
{
	if(NULL == str){
		return str;
	}					

	int len = strlen(str);
	int i = 0, j = 0;
	
	for(j = len - 1; (j > 0) && isspace(str[j]); j--){
		str[j] = '\0';	
	}

	for(i = 0; (i < j) && isspace(str[i]); i++);
	if(i != 0){
//		strncpy(str, (char *)(str + i), j+1-i);
		strncpy(str, (char *)(str + i), len);
	}
//	str[j-i+1] = '\0';	

	return str;	
}

#if 1
/*
** str : the parsing string, one line of the config file
** ret : the key pointer which points to the offset of value in str
*/
char *kv_get_key(const char *str)
{
	if(NULL == str){
		return NULL;
	}

	int i = 0;

	while(isspace(str[i]) && (str[i] != '\0')){
		i++;
	}

	return (char *)((char*) str + i);
}

/*
** str : the parsing string, one line of the config file
** ret : the value pointer which points to the offset of value in str
*/
char *kv_get_value(const char *str)
{
	char *res = NULL;
	res = strchr(str, '=');
	if(NULL == res){
		//printf("%s: No key-value.\n", str);
		return NULL;
	}

	return (char *)((char*) res + 1);
}
#endif

/*
** str : the parsing string, one line of the config file
** key : the comparing key 
*/
bool kv_is_key_equal(const char *str, char *key)
{
	if(NULL == str || NULL == key){
		return false;
	}					

	int len = strlen(key);
	char *pstr = kv_get_key(str);

//	if(!strncmp(str, key, len) && (isspace(str[len]) || '=' == str[len])){
	if(!strncmp(pstr, key, len) && ((isspace(pstr[len]) && strchr(pstr, '=')) || '=' == pstr[len])){
		return true;
	}
	
	return false;
}

/*
** str : the parsing string, one line of the config file
** key : the value pointer which points to the offset of value in str
** value : the value pointer which points to the offset of value in str
*/
int kv_parse_kv_to_kv(const char *str, char *key, char *value)
{
	char *res = NULL;
	res = strchr(str, '=');
	if(NULL == res){
		//printf("%s: No key--value.\n", str);
		return -1;
	}
	strncpy(key, str, res-str);
	kv_del_blank(key);
	strncpy(value, res+1, strlen(res+1));
	kv_del_blank(value);
	return 0;
}

/*
** str : the parsing string, one line of the config file
** value : the new value 
*/
int kv_change_value(char *str, char *value)
{
	char *p = kv_get_value(str); 

	if(NULL == value || NULL == p){
		return -1;
	} else {
		strcpy(p, value);		
	}	

	return 0;
}

/*
** file : input, the config file
** key  : input, the key 
** value : output, the value which matches the input key 
*/
int file_get_value(char *file, char *key, char *value)
{
	if(NULL == file || key == NULL || value == NULL){
		return -1;	
	}
	
	char linebuf[KV_SIZE] = {0};
	char key_flag = 0;
	char *res = NULL;

	FILE *fp = fopen(file, "r+");
	if(NULL == fp){
		// file not exist
		return -2;
	}

	while(!feof(fp)){
		fgets(linebuf, sizeof(linebuf), fp);  // read one line
		if(kv_is_key_equal(linebuf, key)){
			key_flag = 1;
			break;
		}
	}

	if(key_flag == 0){ // key not exist
		//printf("No key [%s]\n", key);	
		return -3;
	} else {  // key exist
		res = kv_get_value(linebuf);	
		strncpy(value, res, strlen(res));
		kv_del_blank(value);
	}

	fclose(fp);
	return 0;
}

/*
** file : input, the config file
** key  : input, the key 
** value : input, the updating value which matches the key 
*/
int file_update_value(char *file, char *key, char *value)
{
	if(NULL == file || key == NULL || value == NULL){
		return -1;	
	}
	
	int filesize = 0;
	char *filebuf = NULL;
	char linebuf[KV_SIZE] = {0};
	char key_flag = 0;
	int len = 0;

	FILE *fp = fopen(file, "r+");
	if(NULL == fp){
		// file not exist
		fp = fopen(file, "w+");	
		if(NULL == fp)
			return -2;
	}

	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	filesize += KV_SIZE;

	filebuf = (char *)malloc(filesize);
	if(filebuf == NULL){
		perror("malloc");
		return -3;
	}
        memset(filebuf, 0, filesize);	
	while(!feof(fp)){
		memset(linebuf, 0, sizeof(linebuf));
		fgets(linebuf, sizeof(linebuf), fp);  // read one line
		if(kv_is_key_equal(linebuf, key)){
			key_flag = 1;
			kv_change_value(linebuf, value);
			len = strlen(linebuf);
			if(linebuf[len-1] != '\n'){
				linebuf[len] = '\n';
				linebuf[len+1] = '\0';
			}
		}
		strcat(filebuf, linebuf);
	}

	if(key_flag == 0){ // key not exist
		fprintf(fp, "%s=%s\n", key, value);
		
	} else {  // key exist
		if(fp != NULL){
			fclose(fp);
		}	

		fp = fopen(file, "w+");
		if(NULL == fp){
			perror("2 fopen");	
			free(filebuf);
			return -4;
		}
		
		fputs(filebuf, fp);
	}

	fclose(fp);
	free(filebuf);
	return 0;
}


