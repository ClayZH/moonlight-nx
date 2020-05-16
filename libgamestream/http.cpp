/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "http.h"
#include "errors.h"

#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>

static CURL *curl;
static bool debug;

struct HTTP_DATA {
    char *memory;
    size_t size;
};

static size_t _write_curl(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    HTTP_DATA* mem = (HTTP_DATA *)userp;
    
    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
        return 0;
    
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int http_init(const char* key_directory, int log_level) {
    curl = curl_easy_init();
    debug = log_level >= 2;
    
    if (!curl)
        return GS_FAILED;
    
    char certificateFilePath[4096];
    sprintf(certificateFilePath, "%s/%s", key_directory, CERTIFICATE_FILE_NAME);
    
    char keyFilePath[4096];
    sprintf(&keyFilePath[0], "%s/%s", key_directory, KEY_FILE_NAME);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE,"PEM");
    curl_easy_setopt(curl, CURLOPT_SSLCERT, certificateFilePath);
    curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLKEY, keyFilePath);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_curl);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
    
    return GS_OK;
}

int http_request(char* url, Data* data) {
    if (debug)
        printf("Request %s\n", url);
    
    HTTP_DATA* http_data = (HTTP_DATA*)malloc(sizeof(HTTP_DATA));
    http_data->memory = (char*)malloc(1);
    http_data->size = 0;
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http_data);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        gs_error = curl_easy_strerror(res);
        return GS_FAILED;
    } else if (http_data->memory == NULL) {
        return GS_OUT_OF_MEMORY;
    }
    
    *data = Data(http_data->memory, http_data->size);
    
    free(http_data->memory);
    free(http_data);
    
    if (debug)
        printf("Response:\n%s\n\n", http_data->memory);
    
    return GS_OK;
}

void http_cleanup() {
    curl_easy_cleanup(curl);
}
