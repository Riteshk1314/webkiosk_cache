#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "db.h"
#include "fetch_data.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Not enough arguements\n");
    }
    char *username = (char *)malloc(strlen(argv[1]) + 1);
    char *password = (char *)malloc(strlen(argv[2]) + 1);

    if (!username || !password)
    {
        fprintf(stderr, "memory allocation failed\n");
    }

    strcpy(username, argv[1]);
    strcpy(password, argv[2]);
    //    printf("%s\n", username);
    //     printf("%s\n", password);

    if (!get_mongo_credentials(&username, &password))
    {
        fprintf(stderr, "Failed to retrieve username and password from MongoDB.\n");
        return 1;
    }

    if (username && password)
    {
        printf("Username: %s\n", username);
        printf("Password: %s\n", password);

        if (fetch_data(username, password))
        {
            printf("\033[0;32mData fetched and stored successfully.\033[0m\n");
        }
        else
        {
            fprintf(stderr, "Failed to fetch or store data.\n");
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Missing username or password.\n");
        return 1;
    }
    free(username);
    free(password);
    return 0;
}
