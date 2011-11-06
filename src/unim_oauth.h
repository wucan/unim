#ifndef _UNIM_OAUTH_H_
#define _UNIM_OAUTH_H_


struct unim_login_info {
	char *request_token_uri;
	char *consumer_key;
	char *consumer_secret;

	char *res_token_key;
	char *res_token_secret;
};

int unim_oauth_request(struct unim_login_info *login_info);


#endif /* _UNIM_OAUTH_H_ */

