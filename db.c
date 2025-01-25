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
    CURL *curl = curl_easy_init();
    if (curl)
    {
        char *url = "https://webkiosk.thapar.edu/CommonFiles/UserAction.jsp";
        char post_fields[256];
        snprintf(post_fields, sizeof(post_fields),
                 "txtuType=Member+Type&UserType=S&txtCode=Enrollment+No&MemberCode=%s&txtPin=Password%%2FPin&Password=%s&BTNSubmit=Submit",
                 *username, *password);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_fields));
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // Do not follow redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose output for debugging

        // Activate cookie handling
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // Enable libcurl's cookie engine

        printf("\033[0;33mChecking credentials...\033[0m\n");
        CURLcode res = curl_easy_perform(curl);
        printf("\033[0;33mResponse ended...\033[0m\n");

        if (res != CURLE_OK)
        {
            fprintf(stderr, "\033[0;31mcurl_easy_perform() failed: %s\033[0m\n", curl_easy_strerror(res));
        }
        else
        {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 302)
            {
                printf("\033[0;34mLogin successful. HTTP response code: %ld\033[0m\n", response_code);

                // Retrieve and parse cookies
                struct curl_slist *cookies = NULL;
                struct curl_slist *nc = NULL;

                curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
                nc = cookies;
                while (nc)
                {
                    char *cookie_data = nc->data;
                    // Look for JSESSIONID
                    if (strstr(cookie_data, "JSESSIONID"))
                    {
                        // Split by tab and extract the last field (JSESSIONID value)
                        char *token = strtok(cookie_data, "\t");
                        char *jsessionid = NULL;
                        while (token)
                        {
                            jsessionid = token; // Update jsessionid to the last token
                            token = strtok(NULL, "\t");
                        }
                        printf("\033[0;32mJSESSIONID: %s\033[0m\n", jsessionid);
                    }
                    nc = nc->next;
                }

                curl_slist_free_all(cookies);
            }
            else
            {
                printf("\033[0;31mLogin failed. HTTP response code: %ld\033[0m\n", response_code);
            }
        }
        curl_easy_cleanup(curl);
        return true;
    }
    return false;
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

    mongoc_init();
    client = mongoc_client_new(getenv("MONGO_URI"));
    if (!client)
    {
        fprintf(stderr, "\033[0;31mFailed to initialize MongoDB client.\033[0m\n");
        return false;
    }
    else
    {
        printf("\033[0;32mMongo client initialized\033[0m\n");
    }
    mongoc_client_set_appname(client, "");

    collection = mongoc_client_get_collection(client, "webKiosk", "signin");

    query = bson_new();

    BSON_APPEND_UTF8(query, "username", *username);
    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    printf("Querying MongoDB for credentials for %s\n", *username);
    if (mongoc_cursor_next(cursor, &doc))
    {
        if (bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter))
        {
            char *db_password = strdup(bson_iter_utf8(&iter, NULL));
            // if password and db_password match
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
                        fprintf(stderr, "\033[0;31m%s\033[0m\n", error.message);
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
        printf("\033[0;33mUsername not found. Adding user to the database.\033[0m\n");
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
                fprintf(stderr, "\033[0;31m%s\033[0m\n", error.message);
            }
            else
            {
                printf("\033[0;32mNew user added successfully.\033[0m\n");
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
        fprintf(stderr, "\033[0;31mFailed to initialize MongoDB client.\033[0m\n");
        return;
    }

    collection = mongoc_client_get_collection(client, "webKiosk", "html_responses");
    doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", object_id);
    BSON_APPEND_UTF8(doc, "html", utf8_response);
    printf("\033[0;33mStoring HTML response...\033[0m\n");

    if (!mongoc_collection_insert_one(collection, doc, NULL, NULL, NULL))
    {
        fprintf(stderr, "\033[0;31mFailed to store HTML response.\033[0m\n");
    }
    else
    {
        printf("\033[0;32mHTML response stored successfully.\033[0m\n");
    }

    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}
