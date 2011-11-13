#include <gtk/gtk.h>

#include "unim.h"
#include "unim_oauth.h"


static GtkWidget *btn, *api_call_btn;
static GtkWidget *reqtk_uri_entry, *ckey_entry, *csecret_entry;
static GtkWidget *token_entry, *token_secret_entry;
static GtkWidget *acctk_uri_entry;
static GtkWidget *access_token_entry, *access_token_secret_entry;
static GtkWidget *api_call_uri_entry, *result_entry;
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

static const char *term_url[URL_NUM] = {
	"http://term.ie/oauth/example/request_token.php",
	NULL,
	"http://term.ie/oauth/example/access_token.php",
	"http://term.ie/oauth/example/echo_api.php?method=foo%20bar&bar=baz",
};

static const char *opent_url[URL_NUM] = {
	"https://open.t.qq.com/cgi-bin/request_token",
	"https://open.t.qq.com/cgi-bin/authorize",
	"https://open.t.qq.com/cgi-bin/access_token",
	"https://open.t.qq.com/api/statuses/home_timeline",
};

static struct consumer_info term_consumer = {"key", "secret"};
static struct consumer_info opent_consumer = {"801063896", "01291aa844d9c075d7daf302bc0330db"};

static const char **url = opent_url;
static struct consumer_info *consumer = &opent_consumer;

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

	login_info.authorize_uri = url[URL_AUTHORIZE];
	login_info.request_token_uri = gtk_entry_get_text(GTK_ENTRY(reqtk_uri_entry));
	login_info.access_token_uri = gtk_entry_get_text(GTK_ENTRY(acctk_uri_entry));
	login_info.consumer_key = gtk_entry_get_text(GTK_ENTRY(ckey_entry));
	login_info.consumer_secret = gtk_entry_get_text(GTK_ENTRY(csecret_entry));
	if (url == opent_url) {
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

	if (!login_info.login)
		return;

	api_call_info.uri = gtk_entry_get_text(GTK_ENTRY(api_call_uri_entry));
	rc = unim_oauth_api_call(&login_info, &api_call_info);
	if (rc) {
		gtk_entry_set_text(GTK_ENTRY(result_entry), "Failed!");
		return;
	}
	gtk_entry_set_text(GTK_ENTRY(result_entry), api_call_info.result);
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
	 * request token uri
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Request Token URI");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	reqtk_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(reqtk_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(reqtk_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(reqtk_uri_entry), url[URL_REQUEST]);
	gtk_box_pack_start(GTK_BOX(hbox), reqtk_uri_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * access token uri
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Access Token URI");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	acctk_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(acctk_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(acctk_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(acctk_uri_entry), url[URL_ACCESS]);
	gtk_box_pack_start(GTK_BOX(hbox), acctk_uri_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * api call uri
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("API Call URI");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	api_call_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(api_call_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(api_call_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(api_call_uri_entry), url[URL_API]);
	gtk_box_pack_start(GTK_BOX(hbox), api_call_uri_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Consumer Key
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Consumer Key");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	ckey_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(ckey_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(ckey_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(ckey_entry), consumer->key);
	gtk_box_pack_start(GTK_BOX(hbox), ckey_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * Consumer Secret
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Consumer Secret");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	csecret_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(csecret_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(csecret_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(csecret_entry), consumer->secret);
	gtk_box_pack_start(GTK_BOX(hbox), csecret_entry, FALSE, FALSE, 0);

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
	result_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(result_entry), FALSE);
	gtk_entry_set_width_chars(GTK_ENTRY(result_entry), 80);
	gtk_box_pack_start(GTK_BOX(hbox), result_entry, FALSE, FALSE, 0);

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

