#include <bson/bson.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "includes/dotenv.h"
#include <iconv.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

bool check_cred_webkiosk(char **username, char **password)
{
    printf("\033[0;33mChecking credentials...\033[0m\n");
    CURL *curl;
    CURLcode res;
    bool result = false;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0");
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.5");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Origin: https://webkiosk.thapar.edu");
        headers = curl_slist_append(headers, "Connection: keep-alive");
        headers = curl_slist_append(headers, "Referer: https://webkiosk.thapar.edu/index.jsp");
        headers = curl_slist_append(headers, "Cookie: _gcl_au=1.1.1633298723.1734614119; _ga=GA1.1.1374391389.1734614120; _ga_GJRG3ZQJED=GS1.1.1734614119.1.0.1734614121.58.0.0; _fbp=fb.1.1734614119972.292506922746728699; _ga_5K6V5S5WTM=GS1.1.1734614120.1.1.1734614121.59.0.0; JSESSIONID=533833014989CF09FE58F7AEFDC5BFA5; switchmenu=");
        headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        headers = curl_slist_append(headers, "Sec-Fetch-Dest: document");
        headers = curl_slist_append(headers, "Sec-Fetch-Mode: navigate");
        headers = curl_slist_append(headers, "Sec-Fetch-Site: same-origin");
        headers = curl_slist_append(headers, "Sec-Fetch-User: ?1");

        char postfields[256];
        snprintf(postfields, sizeof(postfields), "txtuType=Member+Type&UserType=S&txtCode=Enrollment+No&MemberCode=%s&txtPin=Password%%2FPin&Password=%s&BTNSubmit=Submit", *username, *password);

        curl_easy_setopt(curl, CURLOPT_URL, "https://webkiosk.thapar.edu/CommonFiles/UserAction.jsp");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 302)
            {
                result = true;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return result;
}

char *convert_to_utf8(const char *input)
{
    iconv_t cd = iconv_open("UTF-8", "UTF-8");
    if (cd == (iconv_t)-1)
    {
        perror("iconv_open failed");
        return NULL;
    }

    size_t in_len = strlen(input);
    size_t out_len = in_len * 4; // Increase the buffer size as needed
    char *output = (char *)malloc(out_len + 1);
    if (output == NULL)
    {
        perror("Memory allocation failed");
        iconv_close(cd);
        return NULL;
    }

    char *in_ptr = (char *)input;
    char *out_ptr = output;
    if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1)
    {
        perror("iconv failed");
        free(output);
        iconv_close(cd);
        return NULL;
    }

    *out_ptr = '\0'; // Null-terminate the output string
    iconv_close(cd);
    return output;
}

bool get_mongo_credentials(char **username, char **password)
{
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    bson_t *query;
    const bson_t *doc;
    mongoc_cursor_t *cursor;
    bson_iter_t iter;
    bson_error_t error;
    bool result = false;
    bson_t *new_doc;
    load_env(".env");

    // const char *str = getenv("MONGO_URI");
    // if (str == NULL) {
    //     fprintf(stderr, "MONGO_URI environment variable not set.\n");
    //     return false;
    // }else{
    //     printf("Mongo URI: %s\n", str);
    // }

    mongoc_init();
    client = mongoc_client_new(getenv("MONGO_URI"));
    if (!client)
    {
        fprintf(stderr, "Failed to initialize MongoDB client.\n");
        return false;
    }
    else
    {
        printf("Mongo client initialized\n");
    }
    mongoc_client_set_appname(client, "");

    collection = mongoc_client_get_collection(client, "webKiosk", "signin");

    query = bson_new();

    BSON_APPEND_UTF8(query, "username", *username);
    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    printf("Querying MongoDB for credentials for %s\n", *username);
    if (mongoc_cursor_next(cursor, &doc))
    {
        // printf("found a user with the same name");
        if (bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter))
        {
            char *db_password = strdup(bson_iter_utf8(&iter, NULL));
            if (check_cred_webkiosk(username, password))
            {
                printf("\033[0;32mCredentials are correct.\033[0m\n");
                if (strcmp(*password, db_password) == 0)
                {
                    printf("\033[0;32mPasswords match.\033[0m\n");
                    result = true;
                }
                else
                {
                    bson_t *update = BCON_NEW("$set", "{", "password", BCON_UTF8(*password), "}");
                    if (!mongoc_collection_update_one(collection, query, update, NULL, NULL, &error))
                    {
                        fprintf(stderr, "%s\n", error.message);
                    }
                    else
                    {
                        printf("\033[0;32mPassword updated successfully in DB.\033[0m\n");
                        result = true;
                    }
                    bson_destroy(update);
                }
            }
            else
            {
                printf("\033[0;31mIncorrect Password\033[0m\n");
                result = false;
            }
            free(db_password);
        }

        else
        {
            printf("\033[0;31mIncorrect Password\033[0m\n");
            result = false;
        }
    }
    else
    {
        printf("Username not found. Adding user to the database.\n");
        if (check_cred_webkiosk(username, password))
        {
            new_doc = bson_new();
            bson_oid_t oid;
            bson_oid_init(&oid, NULL);
            BSON_APPEND_OID(new_doc, "_id", &oid);
            BSON_APPEND_UTF8(new_doc, "username", *username);
            BSON_APPEND_UTF8(new_doc, "password", *password);

            if (!mongoc_collection_insert_one(collection, new_doc, NULL, NULL, &error))
            {

                fprintf(stderr, "%s\n", error.message);
            }
            else
            {
                printf("New user added successfully.\n");
                result = true;
            }
            bson_destroy(new_doc);
        }
    }
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    return result;
}
void store_html_response(const char *response_html, const char *object_id)
{
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    bson_t *doc;
    char *utf8_response = convert_to_utf8(response_html);

    client = mongoc_client_new(getenv("MONGO_URI"));
    if (!client)
    {
        fprintf(stderr, "Failed to initialize MongoDB client.\n");
        return;
    }

    collection = mongoc_client_get_collection(client, "webKiosk", "html_responses");
    doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", object_id);
    BSON_APPEND_UTF8(doc, "html", utf8_response);
    printf("Storing HTML response...\n");

    if (!mongoc_collection_insert_one(collection, doc, NULL, NULL, NULL))
    {
        fprintf(stderr, "Failed to store HTML response.\n");
    }
    printf("HTML response stored successfully.\n");

    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}
