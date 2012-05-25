#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include "wu_http.h"


static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;
	struct wu_http_get_request *req = (struct wu_http_get_request *)stream;

	if ((req->response_buf_size - req->response_size) < realsize) {
		req->response = realloc(req->response,
			req->response_buf_size + realsize * 8);
		if (!req->response) {
			return -1;
		}
		req->response_buf_size += realsize * 8;
	}
	memcpy(req->response + req->response_size, ptr, realsize);
	req->response_size += realsize;

	return realsize;
}

int wu_http_get(struct wu_http_get_request *req)
{
	CURL *curl;
	CURLcode res;

	req->response_size = 0;
	curl = curl_easy_init();
	if (!curl) {
		printf("failed to create curl!\n");
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_URL, req->url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10); // 10 second for connect
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60 * 10); // 10 minute enough?

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (!req->response_size)
		return -1;

	req->response[req->response_size] = 0;

	return 0;
}

void wu_http_get_request_init(struct wu_http_get_request *req, const char *url)
{
	memset(req, 0, sizeof(*req));
	req->url = url;
}

void wu_http_get_request_free(struct wu_http_get_request *req)
{
	if (req->response)
		free(req->response);

	req->response_size = 0;
	req->response_buf_size = 0;
	req->response = NULL;
}

int wu_http_post(struct wu_http_post_request *req)
{
	CURL *curl;
	CURLcode res;
	struct curl_httppost *formpost = NULL, *lastptr = NULL;
	int i;
	int off = 0, first_url_arg = 1;
	char prefix;

	curl_global_init(CURL_GLOBAL_ALL);

	off += sprintf(req->url_buf, "%s", req->url);

	for (i = 0; i < req->nr_args; i++) {
		struct wu_http_post_arg *arg = &req->args[i];
		if (!arg->in_body) {
			if (first_url_arg)
				prefix = '?';
			else
				prefix = '&';
			off += sprintf(req->url_buf + off, "%c%s=%s",
				prefix, arg->name, arg->value);
			continue;
		}
		if (arg->type == PostArgTypeContent) {
			curl_formadd(&formpost, &lastptr,
				CURLFORM_COPYNAME, arg->name,
				CURLFORM_COPYCONTENTS, arg->value,
				CURLFORM_END);
		} else if (arg->type == PostArgTypeFile) {
			curl_formadd(&formpost, &lastptr,
				CURLFORM_COPYNAME, arg->name,
				CURLFORM_FILE, arg->value,
				CURLFORM_END);
		}
	}

	curl = curl_easy_init();
	if (!curl) {
		printf("failed to create curl!\n");
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, req->url_buf);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10); // 10 second for connect
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60 * 10); // 10 minute enough?
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);

	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_formfree(formpost);

	if (!req->response_size)
		return -1;

	req->response[req->response_size] = 0;

	return 0;
}

void wu_http_post_request_init(struct wu_http_post_request *req,
		const char *url)
{
	memset(req, 0, sizeof(*req));
	req->url = url;
}

void wu_http_post_request_free(struct wu_http_post_request *req)
{
	if (req->response)
		free(req->response);
	memset(req, 0, sizeof(*req));
}

void wu_http_post_request_add_content_arg(struct wu_http_post_request *req,
		const char *name, const char *value, int in_body)
{
	req->args[req->nr_args].in_body = in_body;
	req->args[req->nr_args].name = name;
	req->args[req->nr_args].value = value;
	req->args[req->nr_args].type = PostArgTypeContent;
	req->nr_args++;
}

void wu_http_post_request_add_file_arg(struct wu_http_post_request *req,
		const char *name, const char *value)
{
	req->args[req->nr_args].in_body = 1;
	req->args[req->nr_args].name = name;
	req->args[req->nr_args].value = value;
	req->args[req->nr_args].type = PostArgTypeFile;
	req->nr_args++;
}

