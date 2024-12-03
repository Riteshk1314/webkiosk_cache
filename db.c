#include <bson/bson.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "includes/dotenv.h"
#include <iconv.h>

char *convert_to_utf8(const char *input) {
    iconv_t cd = iconv_open("UTF-8", "UTF-8");
    if (cd == (iconv_t)-1) {
        perror("iconv_open failed");
        return NULL;
    }

    size_t in_len = strlen(input);
    size_t out_len = in_len * 4; // Increase the buffer size as needed
    char *output = (char *)malloc(out_len + 1);
    if (output == NULL) {
        perror("Memory allocation failed");
        iconv_close(cd);
        return NULL;
    }

    char *in_ptr = (char *)input;
    char *out_ptr = output;
    if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1) {
        perror("iconv failed");
        free(output);
        iconv_close(cd);
        return NULL;
    }

    *out_ptr = '\0'; // Null-terminate the output string
    iconv_close(cd);
    return output;
}

bool get_mongo_credentials(const char **username, const char **password) {
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    bson_t query;
    const bson_t *doc;
    mongoc_cursor_t *cursor;
    bson_iter_t iter;
    bool result = false;

    load_env(".env");

    mongoc_init();
    client = mongoc_client_new(getenv("MONGO_URI"));
    if (!client) {
        fprintf(stderr, "Failed to initialize MongoDB client.\n");
        return false;
    }

    collection = mongoc_client_get_collection(client, "test", "signin");
    bson_init(&query);
    cursor = mongoc_collection_find_with_opts(collection, &query, NULL, NULL);

    if (mongoc_cursor_next(cursor, &doc)) {
        if (bson_iter_init(&iter, doc)) {
            if (bson_iter_find(&iter, "username")) {
                *username = bson_iter_utf8(&iter, NULL);
            }
            if (bson_iter_find(&iter, "password")) {
                *password = bson_iter_utf8(&iter, NULL);
                result = true;
            }
        }
        printf("Username: %s\n", *username);
        printf("Password: %s\n", *password);
    } else {
        fprintf(stderr, "No credentials found in MongoDB collection.\n");
    }

    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);

    return result;
}

void store_html_response(const char *response_html, const char *object_id) {
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    bson_t *doc;
    char *utf8_response = convert_to_utf8(response_html);

    client = mongoc_client_new(getenv("MONGO_URI"));
    if (!client) {
        fprintf(stderr, "Failed to initialize MongoDB client.\n");
        return;
    }

    collection = mongoc_client_get_collection(client, "test", "html_responses");
    doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", object_id);
    BSON_APPEND_UTF8(doc, "html", utf8_response);
    printf("Storing HTML response...\n");

    if (!mongoc_collection_insert_one(collection, doc, NULL, NULL, NULL)) {
        fprintf(stderr, "Failed to store HTML response.\n");
    }
    printf("HTML response stored successfully.\n");

    bson_destroy(doc);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
}
