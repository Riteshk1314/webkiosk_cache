#ifndef DB_H
#define DB_H

#include <stdbool.h>

bool get_mongo_credentials(char **username, char **password);

void store_html_response(const char *response_html, const char *object_id);

#endif 
