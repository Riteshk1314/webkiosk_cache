#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "./includes/dotenv.h"

#define LOGIN_URL "https://webkiosk.thapar.edu/CommonFiles/UserAction.jsp"
#define STUDENT_INFO "https://webkiosk.thapar.edu/StudentFiles/PersonalFiles/StudPersonalInfo.jsp"

size_t write_callback(void *ptr, size_t size, size_t nmemb) {
    printf("%.*s", (int)(size * nmemb), (char *)ptr);
    return size * nmemb;
}

int main(void) {
    CURL *curl;
    CURLcode res;
    long response_code;
    load_env(".env");
    
    const char *username = getenv("USERNAME");
    const char *password = getenv("PASSWORD");

    if (!username || !password) {
        fprintf(stderr, "Missing USERNAME or PASSWORD in .env file\n");
        return 1;
    }
    
    char postfields[256];
    snprintf(postfields, sizeof(postfields),
             "txtuType=Member+Type&UserType=S&txtCode=Enrollment+No&MemberCode=%s&txtPin=Password%%2FPin&Password=%s&BTNSubmit=Submit",
             username, password);

    
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        
        const char *cookie_file = "cookies.txt";        
        curl_easy_setopt(curl, CURLOPT_URL, LOGIN_URL);      
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            printf("\nLogin Response Code: %ld\n", response_code);
        }

        curl_easy_cleanup(curl);        
        curl = curl_easy_init();
        if (curl) {
            
            curl_easy_setopt(curl, CURLOPT_URL, STUDENT_INFO);
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_file);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } else {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                printf("\nPersonal Info Page Response Code: %ld\n", response_code);
            }

            curl_easy_cleanup(curl);
        }
    } 
    curl_global_cleanup();
    printf("Operation Complete\n");

    return 0;
}
