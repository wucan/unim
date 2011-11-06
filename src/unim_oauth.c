#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>

#include "unim_oauth.h"


int unim_oauth_request(struct unim_login_info *login_info)
{
	char *req_url, *reply;
	int rc = -1;

	req_url = oauth_sign_url2(login_info->request_token_uri,
				NULL, OA_HMAC, NULL,
				login_info->consumer_key, login_info->consumer_secret,
				NULL, NULL);
	reply = oauth_http_get(req_url, NULL);
	if (reply) {
		char **rv = NULL;

		printf("HTTP-reply:\n\t%s\n", reply);
		rc = oauth_split_url_parameters(reply, &rv);
		qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
		if (rc == 2 &&
			!strncmp(rv[0], "oauth_token=", 11) &&
			!strncmp(rv[1], "oauth_token_secret=", 18)) {
			login_info->res_token_key = strdup(&(rv[0][12]));
			login_info->res_token_secret = strdup(&(rv[1][19]));
			rc = 0;
		}
		if (rv)
			free(rv);
	}

	if (req_url)
		free(req_url);
	if (reply)
		free(reply);

	return rc;
}

int unim_oauth_access(struct unim_login_info *login_info)
{
	char *req_url, *reply;
	int rc = -1;

	req_url = oauth_sign_url2(login_info->access_token_uri,
				NULL, OA_HMAC, NULL,
				login_info->consumer_key, login_info->consumer_secret,
				login_info->res_token_key, login_info->res_token_secret);
	reply = oauth_http_get(req_url, NULL);
	if (reply) {
		char **rv = NULL;

		printf("HTTP-reply:\n\t%s\n", reply);
		rc = oauth_split_url_parameters(reply, &rv);
		qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
		if (rc == 2 &&
			!strncmp(rv[0], "oauth_token=", 11) &&
			!strncmp(rv[1], "oauth_token_secret=", 18)) {
			login_info->access_token_key = strdup(&(rv[0][12]));
			login_info->access_token_secret = strdup(&(rv[1][19]));
			rc = 0;
		}
		if (rv)
			free(rv);
	}

	if (req_url)
		free(req_url);
	if (reply)
		free(reply);

	return rc;
}

int unim_oauth_api_call(struct unim_login_info *login_info,
				struct unim_api_call_info *api_call_info)
{
	char *req_url, *reply;
	int rc = -1;

	req_url = oauth_sign_url2(api_call_info->uri,
				NULL, OA_HMAC, NULL,
				login_info->consumer_key, login_info->consumer_secret,
				login_info->access_token_key, login_info->access_token_secret);
	reply = oauth_http_get(req_url, NULL);
	if (reply) {
		printf("HTTP-reply:\n\t%s\n", reply);
		api_call_info->result = reply;
		rc = 0;
	}

	if (req_url)
		free(req_url);

	return rc;
}

