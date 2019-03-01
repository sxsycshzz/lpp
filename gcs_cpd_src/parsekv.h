#ifndef PARSEKV_H
#define PARSEKV_H

char *kv_del_blank(char *str);
char *kv_get_key(const char *str);
char *kv_get_value(const char *str);
bool kv_is_key_equal(const char *str, char *key);
int kv_parse_kv_to_kv(const char *str, char *key, char *value);
int kv_change_value(char *str, char *value);
int file_get_value(char *file, char *key, char *value);
int file_update_value(char *file, char *key, char *value);

#endif