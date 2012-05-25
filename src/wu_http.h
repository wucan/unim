#ifndef _WU_HTTP_H_
#define _WU_HTTP_H_


enum HTTP_METHOD {
	HTTP_METHOD_GET = 0,
	HTTP_METHOD_POST,
};

struct wu_http_get_request {
	const char *url;

	char *response;
	int response_size;
	int response_buf_size;
};

int wu_http_get(struct wu_http_get_request *req);
void wu_http_get_request_init(struct wu_http_get_request *req, const char *url);
void wu_http_get_request_free(struct wu_http_get_request *req);

enum PostArgType {
	PostArgTypeContent = 0,
	PostArgTypeFile,
};

struct wu_http_post_arg {
	int in_body;
	enum PostArgType type;
	const char *name;
	const char *value;
};

#define POST_MAX_ARGS			16

struct wu_http_post_request {
	const char *url;

	char *response;
	int response_size;
	int response_buf_size;

	char url_buf[512];

	int nr_args;
	struct wu_http_post_arg args[POST_MAX_ARGS];
};

int wu_http_post(struct wu_http_post_request *req);
void wu_http_post_request_init(struct wu_http_post_request *req,
		const char *url);
void wu_http_post_request_free(struct wu_http_post_request *req);
void wu_http_post_request_add_content_arg(struct wu_http_post_request *req,
		const char *name, const char *value, int in_body);
void wu_http_post_request_add_file_arg(struct wu_http_post_request *req,
		const char *name, const char *value);


#endif /* _WU_HTTP_H_ */

