#include <gtk/gtk.h>

#include "unim.h"
#include "unim_oauth.h"


static GtkWidget *btn, *api_call_btn;
static GtkWidget *token_entry, *token_secret_entry;
static GtkWidget *access_token_entry, *access_token_secret_entry;
static GtkWidget *api_call_uri_entry, *result_view;
static GtkWidget *verifier_entry;

static struct unim_login_info login_info;

struct consumer_info {
	const char *key;
	const char *secret;
};

enum {
	URL_REQUEST = 0,
	URL_AUTHORIZE,
	URL_ACCESS,
	URL_API,

	URL_NUM
};

struct provider_info {
	const char *name;
	const char *url[URL_NUM];
};

struct unim_account {
	struct provider_info *provider;
	struct consumer_info consumer;
};

enum {
	PROVIDER_TERM = 0,
	PROVIDER_OPENT,

	PROVIDER_NUM
};

static struct provider_info providers[PROVIDER_NUM] = {
	{
		"term",
		{"http://term.ie/oauth/example/request_token.php",
		 NULL,
		 "http://term.ie/oauth/example/access_token.php",
		 "http://term.ie/oauth/example/echo_api.php?method=foo%20bar&bar=baz",
		},
	},

	{
		"opent",
		{"http://open.t.qq.com/cgi-bin/request_token",
		 "http://open.t.qq.com/cgi-bin/authorize",
		 "http://open.t.qq.com/cgi-bin/access_token",
		 "http://open.t.qq.com/api/statuses/home_timeline",
		},
	},
};

static struct unim_account accounts[] = {
	{
	 &providers[PROVIDER_TERM],
	 {"key", "secret"},
	},

	{
	 &providers[PROVIDER_OPENT],
	 {"801063896", "01291aa844d9c075d7daf302bc0330db"},
	},
};

static struct unim_account *account = &accounts[1];

static void build_gui();

static void clean_login_info()
{
	free(login_info.res_token_key);
	free(login_info.res_token_secret);
	free(login_info.access_token_key);
	free(login_info.access_token_secret);
	memset(&login_info, 0, sizeof(login_info));
}

void unim_init()
{
	g_thread_init(NULL);
	gtk_init(NULL, NULL);

	build_gui();

	gtk_main();
}

static gboolean msg_win_delete_event(GtkWidget *widget,
			GdkEvent *event, gpointer data)
{
	return FALSE;
}

static void msg_win_destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static void login_button_press(GtkWidget *widget,
			GdkEventButton *event, gpointer *data)
{
	int rc;

	if (strcmp(gtk_button_get_label(GTK_BUTTON(btn)), "Verify") == 0) {
		gtk_button_set_label(GTK_BUTTON(btn), "Login");
		login_info.verifier = gtk_entry_get_text(GTK_ENTRY(verifier_entry));
		if (!strlen(login_info.verifier)) {
			goto error_out;
		}
		goto do_access_token;
	}

	clean_login_info();

	login_info.authorize_uri = account->provider->url[URL_AUTHORIZE];
	login_info.request_token_uri = account->provider->url[URL_REQUEST];
	login_info.access_token_uri = account->provider->url[URL_ACCESS];
	login_info.consumer_key = account->consumer.key;
	login_info.consumer_secret = account->consumer.secret;
	if (strcmp(account->provider->name, "opent") == 0) {
		login_info.request_added_argc = 1;
		login_info.request_added_argv[0] = "oauth_callback=null";
	}
	rc = unim_oauth_request(&login_info);
	if (rc) {
		g_print("reqeust token failed!\n");
		gtk_entry_set_text(GTK_ENTRY(token_entry), "");
		gtk_entry_set_text(GTK_ENTRY(token_secret_entry), "");
		goto error_out;
	}

	gtk_entry_set_text(GTK_ENTRY(token_entry), login_info.res_token_key);
	gtk_entry_set_text(GTK_ENTRY(token_secret_entry), login_info.res_token_secret);

	/*
	 * authorize
	 */
	if (login_info.authorize_uri) {
		rc = fork();
		if (rc == 0) {
			char cmd[256];
			sprintf(cmd, "xdg-open %s?oauth_token=%s",
				login_info.authorize_uri, login_info.res_token_key);
			system(cmd);
			exit(0);
		}
		gtk_button_set_label(GTK_BUTTON(btn), "Verify");
		return;
	}

do_access_token:
	if (login_info.authorize_uri) {
		static char verifier_argv[128];
		sprintf(verifier_argv, "oauth_verifier=%s", login_info.verifier);
		login_info.request_added_argc = 1;
		login_info.request_added_argv[0] = verifier_argv;
	}
	rc = unim_oauth_access(&login_info);
	if (rc) {
		g_print("access token failed!\n");
		gtk_entry_set_text(GTK_ENTRY(access_token_entry), "");
		gtk_entry_set_text(GTK_ENTRY(access_token_secret_entry), "");
		goto error_out;
	}

	gtk_entry_set_text(GTK_ENTRY(access_token_entry), login_info.access_token_key);
	gtk_entry_set_text(GTK_ENTRY(access_token_secret_entry), login_info.access_token_secret);

	login_info.login = 1;
	return;

error_out:
	clean_login_info();
}

static void api_call_button_press(GtkWidget *widget,
			GdkEventButton *event, gpointer *data)
{
	struct unim_api_call_info api_call_info = {0};
	int rc;
	GtkTextBuffer *text_buf;

	if (!login_info.login)
		return;

	text_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(result_view));
	api_call_info.uri = gtk_entry_get_text(GTK_ENTRY(api_call_uri_entry));
	rc = unim_oauth_api_call(&login_info, &api_call_info);
	if (rc) {
		char sbuf[256];
		sprintf(sbuf, "Exec %s Failed!", api_call_info.uri);
		gtk_text_buffer_set_text(text_buf, sbuf, -1);
		return;
	}
	gtk_text_buffer_set_text(text_buf, api_call_info.result, -1);
	free(api_call_info.result);
}

static void build_gui()
{
	GtkWidget *msg_win;
	GtkWidget *vbox, *hbox;

	msg_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(msg_win), 600, 400);
	gtk_container_set_border_width(GTK_CONTAINER(msg_win), 0);
	gtk_window_set_keep_above(GTK_WINDOW(msg_win), TRUE);
	if (gtk_widget_is_composited(msg_win)) {
		gdouble opacity = 0.5;
		g_print("set opacity to %f\n", opacity);
		gtk_window_set_opacity(GTK_WINDOW(msg_win), opacity);
	}
	g_signal_connect(G_OBJECT(msg_win), "delete_event",
				G_CALLBACK(msg_win_delete_event), NULL);
	g_signal_connect(G_OBJECT(msg_win), "destroy",
				G_CALLBACK(msg_win_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(msg_win), vbox);

	GtkWidget *label;

	/*
	 * api call uri
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("API Call URI");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	api_call_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(api_call_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(api_call_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(api_call_uri_entry), account->provider->url[URL_API]);
	gtk_box_pack_start(GTK_BOX(hbox), api_call_uri_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * login button
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	btn = gtk_button_new_with_label("Login");
	gtk_signal_connect(GTK_OBJECT(btn), "button_press_event",
			GTK_SIGNAL_FUNC(login_button_press), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * api call button
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	api_call_btn = gtk_button_new_with_label("API Call");
	gtk_signal_connect(GTK_OBJECT(api_call_btn), "button_press_event",
			GTK_SIGNAL_FUNC(api_call_button_press), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), api_call_btn, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Reply oauth Token
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Token");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	token_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(token_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(token_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), token_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Reply oauth Token Secret
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Token Secret");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	token_secret_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(token_secret_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(token_secret_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), token_secret_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Reply oauth Access Token
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Access Token");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	access_token_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(access_token_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(access_token_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), access_token_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Reply oauth Access Token Secret
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Access Token Secret");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	access_token_secret_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(access_token_secret_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(access_token_secret_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), access_token_secret_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * API Call Result
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Result");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	result_view = gtk_text_view_new();
	gtk_widget_set_size_request(result_view, 600, 200);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(result_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(result_view), GTK_WRAP_WORD);
	gtk_box_pack_start(GTK_BOX(hbox), result_view, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * verifier
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Verifier");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	verifier_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(verifier_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(verifier_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), verifier_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * show gui
	 */
	gtk_widget_show_all(msg_win);
}

