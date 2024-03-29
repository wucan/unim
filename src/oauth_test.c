#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>

/* 
 * a example requesting and parsing a request-token from an OAuth
 * service-provider, excercising the oauth-HTTP GET function.
 * it is almost the same as \ref request_token_example_post below. 
 */
void request_token_example_get()
{
	const char *request_token_uri =
			"http://term.ie/oauth/example/request_token.php";
	const char *req_c_key = "key";
	const char *req_c_secret = "secret";
	char *res_t_key = NULL; // replied key
	char *res_t_secret = NULL; // replied secret
	char *req_url;
	char *reply;

	printf("***** request token example, use method get *****\n");
	req_url = oauth_sign_url2(request_token_uri, NULL, OA_HMAC, NULL,
					req_c_key, req_c_secret, NULL, NULL);
	printf("request url:\n\t%s -> \n\t%s\n", request_token_uri, req_url);
	reply = oauth_http_get(req_url, NULL);
	if (!reply) {
		printf("HTTP request for an oauth request-token failed!\n");
	} else {
		/*
		 * parse reply - example:
		 * "oauth_token=2a71d1c73d2771b00f13ca0acb9836a10477d3c56&oauth_token_secret=a1b5c00c1f3e23fb314a0aa22e990266"
		 */
		int rc;
		char **rv = NULL;

		printf("HTTP-reply:\n\t%s\n", reply);
		rc = oauth_split_url_parameters(reply, &rv);
		qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
		if (rc == 2 &&
			!strncmp(rv[0], "oauth_token=", 11) &&
			!strncmp(rv[1], "oauth_token_secret=", 18)) {
			res_t_key = strdup(&(rv[0][12]));
			res_t_secret = strdup(&(rv[1][19]));
			printf("key:    '%s'\nsecret: '%s'\n", res_t_key, res_t_secret);
		}
		if (rv)
			free(rv);
	}

	if (req_url)
		free(req_url);
	if (reply)
		free(reply);
	if (res_t_key)
		free(res_t_key);
	if (res_t_secret)
		free(res_t_secret);
}

/*
 * a example requesting and parsing a request-token from an OAuth
 * service-provider, using the oauth-HTTP POST function.
 */
void request_token_example_post()
{
	const char *request_token_uri =
				"http://term.ie/oauth/example/request_token.php";
	const char *req_c_key = "key";
	const char *req_c_secret = "secret";
	char *res_t_key = NULL;
	char *res_t_secret = NULL;
	char *postarg = NULL;
	char *req_url;
	char *reply;

	printf("***** request token example, use method post *****\n");
	req_url = oauth_sign_url2(request_token_uri, &postarg, OA_HMAC, NULL,
						req_c_key, req_c_secret, NULL, NULL);
	printf("request url:\n\t%s -> \n\t%s\n", request_token_uri, req_url);
	reply = oauth_http_post(req_url, postarg);
	printf("post arg:\n\t%s\n", postarg);
	if (!reply)  {
		printf("HTTP request for an oauth request-token failed!\n");
	} else {
		int rc;
		char **rv = NULL;
		printf("HTTP-reply:\n\t%s\n", reply);
		rc = oauth_split_url_parameters(reply, &rv);
		qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
		if (rc == 2 &&
			!strncmp(rv[0], "oauth_token=", 11) &&
			!strncmp(rv[1], "oauth_token_secret=", 18)) {
			res_t_key = strdup(&(rv[0][12]));
			res_t_secret = strdup(&(rv[1][19]));
			printf("key:    '%s'\nsecret: '%s'\n", res_t_key, res_t_secret);
		}
		if (rv)
			free(rv);
	}

	if (req_url)
		free(req_url);
	if (postarg)
		free(postarg);
	if (reply)
		free(reply);
	if (res_t_key)
		free(res_t_key);
	if (res_t_secret)
		free(res_t_secret);
}

void oauth_test()
{
	// the url to sign
	const char *url = "http://base.url/&just=append?post=or_get_parameters"
					"&arguments=will_be_formatted_automatically?&dont_care"
					"=about_separators";
	// consumer key
	const char *c_key = "1234567890abcdef1234567890abcdef123456789";
	// consumer secret
	const char *c_secret = "01230123012301230123012301230123";
	// token key
	const char *t_key    = "0987654321fedcba0987654321fedcba098765432";
	// token secret
	const char *t_secret = "66666666666666666666666666666666";

	/*
	 * example sign GET request and print the signed request URL
	 */
	{
		char *geturl = NULL;
		geturl = oauth_sign_url2(url, NULL, OA_HMAC, NULL,
					c_key, c_secret, t_key, t_secret);
		printf("GET URL:\n\t%s\n", geturl);
		if (geturl)
			free(geturl);
	}

	/*
	 * sign POST example 
	 */
	{
		char *postargs = NULL, *post;
		post = oauth_sign_url2(url, &postargs, OA_HMAC, NULL,
					c_key, c_secret, t_key, t_secret);
		printf("POST URL:\n\t%s\nPARAM:%s\n", post, postargs);
		if (post)
			free(post);
		if (postargs)
			free(postargs);
	}

	/*
	 * These two will perform a HTTP request, requesting an access token. 
	 * it's intended both as test (verify signature) 
	 */
	request_token_example_get();
	request_token_example_post();
}

