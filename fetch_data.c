#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    char **response = (char **)userdata;

    if (*response == NULL) {
        *response = malloc(total_size + 1); // Allocate initial memory
        (*response)[0] = '\0'; // Initialize with an empty string
    } else {
        *response = realloc(*response, strlen(*response) + total_size + 1); // Reallocate
    }

    if (*response) {
        strncat(*response, ptr, total_size); // Safely concatenate data
        (*response)[strlen(*response)] = '\0'; // Null-terminate
    } else {
        fprintf(stderr, "Memory allocation failed in write_callback.\n");
        return 0; // Signal an error to libcurl
    }
    return total_size;
}

// void store_html_response(const char *response_html, const char *object_id);

// bool fetch_exam_marks(){
//     get user 'Cookie: switchmenu=; JSESSIONID=from db and then get then try to get the page from original if down then from db and return the html response
// }
// bool fetch_exam_grades(){

// }
// bool fetch_sgpa_cgpa(){

// }

// bool fetch_data(){
//     a menu that gives options to choose which page to return;
// }


bool fetch_data(const char *username, const char *password) {
    CURL *curl;
    CURLcode res;
    long response_code;
    char *login_response = NULL;
    const char *cookie_file = "cookies.txt";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Configure login request
        curl_easy_setopt(curl, CURLOPT_URL, "https://webkiosk.thapar.edu/CommonFiles/UserAction.jsp");

        char postfields[256];
        snprintf(postfields, sizeof(postfields),
                 "txtuType=Member+Type&UserType=S&txtCode=Enrollment+No&MemberCode=%s&txtPin=Password%%2FPin&Password=%s&BTNSubmit=Submit",
                 username, password);
        printf("POST Fields: %s\n", postfields);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &login_response);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file);

        // Add browser-like headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br, zstd");
        headers = curl_slist_append(headers, "Accept-Language: en-GB,en-US;q=0.9,en;q=0.8,hi;q=0.7");
        headers = curl_slist_append(headers, "Cache-Control: max-age=0");
        headers = curl_slist_append(headers, "Connection: keep-alive");
        headers = curl_slist_append(headers, "Sec-Ch-Ua: \"Google Chrome\";v=\"123\", \"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"123\"");
        headers = curl_slist_append(headers, "Sec-Ch-Ua-Mobile: ?0");
        headers = curl_slist_append(headers, "Sec-Ch-Ua-Platform: \"Linux\"");
        headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the User-Agent header
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");

        // Perform login request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            printf("\nLogin Response Code: %ld\n", response_code);

            // Check for 302 redirection success
            if (response_code == 302) {
                printf("Login successful.\n");

                // Fetch student info page
                CURL *info_curl = curl_easy_init();
                char *info_response = NULL;

                if (info_curl) {
                    curl_easy_setopt(info_curl, CURLOPT_URL,
                                     "https://webkiosk.thapar.edu/StudentFiles/PersonalFiles/StudPersonalInfo.jsp");
                    curl_easy_setopt(info_curl, CURLOPT_COOKIEFILE, cookie_file);
                    curl_easy_setopt(info_curl, CURLOPT_WRITEFUNCTION, write_callback);
                    curl_easy_setopt(info_curl, CURLOPT_WRITEDATA, &info_response);
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");



                    res = curl_easy_perform(info_curl);
                    if (res != CURLE_OK) {
                        fprintf(stderr, "Fetching student info failed: %s\n", curl_easy_strerror(res));
                    } else {
                        curl_easy_getinfo(info_curl, CURLINFO_RESPONSE_CODE, &response_code);
                        printf("\nStudent Info Page Response Code: %ld\n", response_code);

                        if (info_response) {
                            printf("Student Info Page HTML:\n%s\n", info_response);

                            // Save the response HTML to MongoDB
                            store_html_response(info_response, username);
                        } else {
                            fprintf(stderr, "No response received for student info page.\n");
                        }
                        free(info_response);
                    }

                    curl_easy_cleanup(info_curl);
                }
            } else {
                fprintf(stderr, "Login failed with response code %ld.\n", response_code);
            }
        }

        free(login_response);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        fprintf(stderr, "Failed to initialize CURL.\n");
    }

    curl_global_cleanup();
    return 1;
}
