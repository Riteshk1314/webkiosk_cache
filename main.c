#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "db.h"
#include "fetch_data.h"

int main(void) {
    const char *username, *password;
  
    if (!get_mongo_credentials(&username, &password)) {
        fprintf(stderr, "Failed to retrieve username and password from MongoDB.\n");
        return 1;
    }

    if (username && password) {
        printf("Username: %s\n", username); 
        printf("Password: %s\n", password); 
       
        if (fetch_data(username, password)) {
            printf("Data fetched and stored successfully.\n");
        } else {
            fprintf(stderr, "Failed to fetch or store data.\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Missing username or password.\n");
        return 1;
    }

    return 0;
}
