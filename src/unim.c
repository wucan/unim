#include <gtk/gtk.h>

#include "unim.h"
#include "unim_oauth.h"


static GtkWidget *btn, *api_call_btn;
static GtkWidget *token_entry, *token_secret_entry;
static GtkWidget *access_token_entry, *access_token_secret_entry;
static GtkWidget *api_call_uri_entry, *result_view;
static GtkWidget *verifier_entry;
static GtkComboBoxText *provider_cbox, *url_cbox;

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
	PROVIDER_WEIBO,

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

	{
		"weibo",
		{NULL,
		 "https://api.weibo.com/oauth2/authorize",
		 "https://api.weibo.com/oauth2/access_token",
		 "https://api.weibo.com/2/statuses/home_timeline.json",
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

	{
	 &providers[PROVIDER_WEIBO],
	 {"2085678772", "ade7b75e363da0399f991cefa84655c7"},
	},
};

static struct unim_account *account = &accounts[PROVIDER_WEIBO];

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

static void weibo_login()
{
	int rc;

	if (strcmp(gtk_button_get_label(GTK_BUTTON(btn)), "Verify") == 0) {
		gtk_button_set_label(GTK_BUTTON(btn), "Login");
		login_info.verifier = gtk_entry_get_text(GTK_ENTRY(verifier_entry));
		if (!strlen(login_info.verifier)) {
			return;
		}
		goto do_access_token;
	}

	clean_login_info();

	login_info.authorize_uri = account->provider->url[URL_AUTHORIZE];
	login_info.access_token_uri = account->provider->url[URL_ACCESS];
	login_info.consumer_key = account->consumer.key;
	login_info.consumer_secret = account->consumer.secret;

	/*
	 * authorize
	 */
	if (login_info.authorize_uri) {
		rc = fork();
		if (rc == 0) {
			char cmd[256];
			sprintf(cmd, "xdg-open \"%s?client_id=%s&redirect_uri=%s\"",
				login_info.authorize_uri, login_info.consumer_key,
				"http://unim.canbaby.org/response");
			system(cmd);
			exit(0);
		}
		gtk_button_set_label(GTK_BUTTON(btn), "Verify");
		return;
	}

do_access_token:
	login_info.request_added_argc = 5;
	char client_id_buf[128], client_secret_buf[128];
	sprintf(client_id_buf, "client_id=%s", login_info.consumer_key);
	sprintf(client_secret_buf, "client_secret=%s", login_info.consumer_secret);
	login_info.request_added_argv[0] = client_id_buf;
	login_info.request_added_argv[1] = client_secret_buf;
#if 0
	/* use grant_type password need authorization from sina? */
	login_info.request_added_argv[2] = "grant_type=password";
	login_info.request_added_argv[3] = "username=wu.canus@gmail.com";
	login_info.request_added_argv[4] = "password=xxxxxx";
#else
	char code_buf[128], redirect_uri_buf[128];
	login_info.verifier = gtk_entry_get_text(GTK_ENTRY(verifier_entry));
	if (!strlen(login_info.verifier)) {
		return;
	}
	sprintf(code_buf, "code=%s", login_info.verifier);
	sprintf(redirect_uri_buf, "redirect_uri=http://unim.canbaby.org/response");
	login_info.request_added_argv[2] = "grant_type=authorization_code";
	login_info.request_added_argv[3] = code_buf;
	login_info.request_added_argv[4] = redirect_uri_buf;
#endif
	rc = weibo_oauth_access(&login_info);
	if (rc) {
		g_print("access token failed!\n");
		return;
	}

	gtk_entry_set_text(GTK_ENTRY(access_token_entry), login_info.access_token_key);

	login_info.login = 1;
}

static void login_button_press(GtkWidget *widget,
			GdkEventButton *event, gpointer *data)
{
	int rc;

	if (strcmp(account->provider->name, "weibo") == 0) {
		weibo_login();
		return;
	}

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
	if (strcmp(account->provider->name, "weibo") == 0) {
		static char access_token_buf[128], source_buf[128];
		sprintf(access_token_buf, "access_token=%s", login_info.access_token_key);
		sprintf(source_buf, "source=%s", login_info.consumer_key);
		login_info.request_added_argc = 2;
		login_info.request_added_argv[0] = access_token_buf;
		login_info.request_added_argv[1] = source_buf;
		rc = weibo_oauth_api_call(&login_info, &api_call_info);
	} else {
		rc = unim_oauth_api_call(&login_info, &api_call_info);
	}
	if (rc) {
		char sbuf[256];
		sprintf(sbuf, "Exec %s Failed!", api_call_info.uri);
		gtk_text_buffer_set_text(text_buf, sbuf, -1);
		return;
	}
	gtk_text_buffer_set_text(text_buf, api_call_info.result, -1);
	free(api_call_info.result);
}

static void reset_text_widgets()
{
	gtk_entry_set_text(GTK_ENTRY(token_entry), "");
	gtk_entry_set_text(GTK_ENTRY(token_secret_entry), "");
	gtk_entry_set_text(GTK_ENTRY(access_token_entry), "");
	gtk_entry_set_text(GTK_ENTRY(access_token_secret_entry), "");
	gtk_entry_set_text(GTK_ENTRY(verifier_entry), "");

	GtkTextBuffer *text_buf;
	text_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(result_view));
	gtk_text_buffer_set_text(text_buf, "", -1);
}

static void _gtk_combo_box_text_remove_all(GtkComboBoxText *cbox)
{
	GtkTreeModel *model = gtk_combo_box_get_model(cbox);
	int n = gtk_tree_model_iter_n_children(model, NULL);
	int i;

	for (i = n - 1; i >= 0; i--)
		gtk_combo_box_text_remove(cbox, i);
}

static void provider_cbox_changed(GtkComboBox *cbox, gpointer user_data)
{
	int provider_idx;
	int i;

	provider_idx = gtk_combo_box_get_active(cbox);
	// update account
	account = &accounts[provider_idx];
	// clear old urls
	_gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(url_cbox));
	// fillin new urls
	struct provider_info *pi = &providers[provider_idx];
	for (i = 0; i < URL_NUM; i++) {
		if (pi->url[i])
			gtk_combo_box_text_append_text(url_cbox, pi->url[i]);
		else
			gtk_combo_box_text_append_text(url_cbox, "NA");
	}
	gtk_combo_box_set_active(url_cbox, 0);
	// update api call uri
	gtk_entry_set_text(GTK_ENTRY(api_call_uri_entry), pi->url[URL_API]);

	reset_text_widgets();
}

static void build_gui()
{
	GtkWidget *msg_win;
	GtkWidget *vbox;

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

	GtkTable *table;
	GtkWidget *label;
	char *markup;
	int i;

	/*
	 * begin login and auth widgets
	 */
	label = gtk_label_new(NULL);
	markup = g_markup_printf_escaped(
		"<span style=\"italic\">%s</span>", "Login & Auth");
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	table = gtk_table_new(5, 3, FALSE);

	/*
	 * service provider
	 */
	provider_cbox = gtk_combo_box_text_new();
	for (i = 0; i < PROVIDER_NUM; i++)
		gtk_combo_box_text_append_text(provider_cbox, providers[i].name);
	gtk_combo_box_set_active(provider_cbox, PROVIDER_WEIBO);
	gtk_signal_connect(GTK_OBJECT(provider_cbox), "changed",
			GTK_SIGNAL_FUNC(provider_cbox_changed), NULL);
	gtk_table_attach_defaults(table, provider_cbox, 0, 1, 0, 1);

	/*
	 * service url
	 */
	struct provider_info *pi = &providers[PROVIDER_WEIBO];
	url_cbox = gtk_combo_box_text_new();
	for (i = 0; i < URL_NUM; i++) {
		if (pi->url[i])
			gtk_combo_box_text_append_text(url_cbox, pi->url[i]);
		else
			gtk_combo_box_text_append_text(url_cbox, "NA");
	}
	gtk_combo_box_set_active(url_cbox, 0);
	// add tearoffs
	gtk_combo_box_set_add_tearoffs(url_cbox, TRUE);
	gtk_combo_box_set_title(url_cbox, "urls");

	gtk_table_attach_defaults(table, url_cbox, 1, 2, 0, 1);

	/*
	 * login button
	 */
	btn = gtk_button_new_with_label("Login");
	gtk_signal_connect(GTK_OBJECT(btn), "button_press_event",
			GTK_SIGNAL_FUNC(login_button_press), NULL);
	gtk_table_attach_defaults(table, btn, 2, 3, 0, 1);

	/*
	 * verifier or Authorization_Code
	 */
	label = gtk_label_new("Verifier/Authorization_Code");
	gtk_table_attach_defaults(table, label, 0, 1, 1, 2);
	verifier_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(verifier_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(verifier_entry), 80);
	gtk_table_attach_defaults(table, verifier_entry, 1, 2, 1, 2);

	/*
	 * Reply oauth Token
	 */
	label = gtk_label_new("Token");
	gtk_table_attach_defaults(table, label, 0, 1, 2, 3);
	token_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(token_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(token_entry), 80);
	gtk_table_attach_defaults(table, token_entry, 1, 2, 2, 3);

	/*
	 * Reply oauth Token Secret
	 */
	label = gtk_label_new("Token Secret");
	gtk_table_attach_defaults(table, label, 0, 1, 3, 4);
	token_secret_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(token_secret_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(token_secret_entry), 80);
	gtk_table_attach_defaults(table, token_secret_entry, 1, 2, 3, 4);

	/*
	 * Reply oauth Access Token
	 */
	label = gtk_label_new("Access Token");
	gtk_table_attach_defaults(table, label, 0, 1, 4, 5);
	access_token_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(access_token_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(access_token_entry), 80);
	gtk_table_attach_defaults(table, access_token_entry, 1, 2, 4, 5);

	/*
	 * Reply oauth Access Token Secret
	 */
	label = gtk_label_new("Access Token Secret");
	gtk_table_attach_defaults(table, label, 0, 1, 5, 6);
	access_token_secret_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(access_token_secret_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(access_token_secret_entry), 80);
	gtk_table_attach_defaults(table, access_token_secret_entry, 1, 2, 5, 6);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	/*
	 * begin api call widgets
	 */
	label = gtk_label_new(NULL);
	markup = g_markup_printf_escaped(
		"<span style=\"italic\">%s</span>", "API Call");
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/*
	 * api call uri
	 */
	table = gtk_table_new(2, 3, FALSE);
	label = gtk_label_new("URI");
	gtk_table_attach_defaults(table, label, 0, 1, 0, 1);
	api_call_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(api_call_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(api_call_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(api_call_uri_entry), account->provider->url[URL_API]);
	gtk_table_attach_defaults(table, api_call_uri_entry, 1, 2, 0, 1);

	/*
	 * api call button
	 */
	api_call_btn = gtk_button_new_with_label("API Call");
	gtk_signal_connect(GTK_OBJECT(api_call_btn), "button_press_event",
			GTK_SIGNAL_FUNC(api_call_button_press), NULL);
	gtk_table_attach_defaults(table, api_call_btn, 2, 3, 0, 1);

	/*
	 * API Call Result
	 */
	label = gtk_label_new("Result");
	gtk_table_attach_defaults(table, label, 0, 1, 1, 2);
	result_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(result_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(result_view), GTK_WRAP_WORD);
	GtkScrolledWindow *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(sw, result_view);
	gtk_widget_set_size_request(sw, -1, 200);
	gtk_table_attach_defaults(table, sw, 1, 3, 1, 2);

	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	/*
	 * show gui
	 */
	gtk_widget_show_all(msg_win);
}

