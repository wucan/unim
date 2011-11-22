#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <json-glib/json-glib.h>

#include "unim_oauth.h"


int weibo_oauth_access(struct unim_login_info *login_info)
{
	char *req_param, *reply;
	int rc = -1;
	int argc = 0;
	char **argv = NULL;
	char *postargs = NULL;
	int i;

	argc = oauth_split_url_parameters(login_info->access_token_uri, &argv);
	if (login_info->request_added_argc) {
		for (i = 0; i < login_info->request_added_argc; i++) {
			oauth_add_param_to_array(&argc, &argv,
					login_info->request_added_argv[i]);
		}
	}
	req_param = oauth_serialize_url_parameters(argc, argv);
	printf("req_param: %s\n", req_param);
	reply = oauth_http_post(login_info->access_token_uri, req_param);
	if (reply) {
		JsonParser *jparser;
		GError *err;
		gboolean rb;

		printf("HTTP-reply:\n\t%s\n", reply);
		jparser = json_parser_new();
		rb = json_parser_load_from_data(jparser, reply, strlen(reply), &err);
		if (!rb) {
			printf("json parse failed! err: %s\n", err->message);
			g_object_unref(jparser);
			goto error_out;
		}
		JsonReader *jreader = json_reader_new(json_parser_get_root(jparser));
		rb = json_reader_read_member(jreader, "access_token");
		if (rb) {
			login_info->access_token_key =
				strdup(json_reader_get_string_value(jreader));
			rc = 0;
		}
		g_object_unref(jreader);
	}

error_out:
	if (req_param)
		free(req_param);
	if (reply)
		free(reply);
	if (postargs)
		free(postargs);

	return rc;
}

int weibo_oauth_api_call(struct unim_login_info *login_info,
				struct unim_api_call_info *api_call_info)
{
	char *req_param, *reply;
	int rc = -1;
	int argc = 0;
	char **argv = NULL;
	int i;

	argc = oauth_split_url_parameters(api_call_info->uri, &argv);
	if (login_info->request_added_argc) {
		for (i = 0; i < login_info->request_added_argc; i++) {
			oauth_add_param_to_array(&argc, &argv,
					login_info->request_added_argv[i]);
		}
	}
	req_param = oauth_serialize_url_parameters(argc, argv);
	printf("req_param: %s\n", req_param);
	reply = oauth_http_get(api_call_info->uri, req_param);
	if (reply) {
		printf("HTTP-reply:\n\t%s\n", reply);
		api_call_info->result = reply;
		rc = 0;
	}

	if (req_param)
		free(req_param);

	return rc;
}

