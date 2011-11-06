#ifndef _UNIM_OAUTH_H_
#define _UNIM_OAUTH_H_


struct unim_login_info {
	char *request_token_uri;
	char *access_token_uri;
	char *consumer_key;
	char *consumer_secret;

	char *res_token_key;
	char *res_token_secret;
	char *access_token_key;
	char *access_token_secret;
};

struct unim_api_call_info {
	char *uri;
	char *result;
};

int unim_oauth_request(struct unim_login_info *login_info);
int unim_oauth_access(struct unim_login_info *login_info);
int unim_oauth_api_call(struct unim_login_info *login_info,
				struct unim_api_call_info *api_call_info);


#endif /* _UNIM_OAUTH_H_ */

