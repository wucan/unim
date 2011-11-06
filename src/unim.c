#include <gtk/gtk.h>

#include "unim.h"
#include "unim_oauth.h"


static GtkWidget *reqtk_uri_entry, *ckey_entry, *csecret_entry;
static GtkWidget *token_entry, *token_secret_entry;
static GtkWidget *acctk_uri_entry;
static GtkWidget *access_token_entry, *access_token_secret_entry;
static GtkWidget *api_call_uri_entry, *result_entry;

static void build_gui();

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
	struct unim_login_info login_info = {0};
	int rc;

	login_info.request_token_uri = gtk_entry_get_text(GTK_ENTRY(reqtk_uri_entry));
	login_info.access_token_uri = gtk_entry_get_text(GTK_ENTRY(acctk_uri_entry));
	login_info.consumer_key = gtk_entry_get_text(GTK_ENTRY(ckey_entry));
	login_info.consumer_secret = gtk_entry_get_text(GTK_ENTRY(csecret_entry));
	rc = unim_oauth_request(&login_info);
	if (rc) {
		g_print("reqeust token failed!\n");
		gtk_entry_set_text(GTK_ENTRY(token_entry), "");
		gtk_entry_set_text(GTK_ENTRY(token_secret_entry), "");
		goto error_out;
	}

	gtk_entry_set_text(GTK_ENTRY(token_entry), login_info.res_token_key);
	gtk_entry_set_text(GTK_ENTRY(token_secret_entry), login_info.res_token_secret);

	rc = unim_oauth_access(&login_info);
	if (rc) {
		g_print("access token failed!\n");
		gtk_entry_set_text(GTK_ENTRY(access_token_entry), "");
		gtk_entry_set_text(GTK_ENTRY(access_token_secret_entry), "");
		goto error_out;
	}

	gtk_entry_set_text(GTK_ENTRY(access_token_entry), login_info.access_token_key);
	gtk_entry_set_text(GTK_ENTRY(access_token_secret_entry), login_info.access_token_secret);

	/*
	 * test api call
	 */
	struct unim_api_call_info api_call_info = {0};
	api_call_info.uri = gtk_entry_get_text(GTK_ENTRY(api_call_uri_entry));
	rc = unim_oauth_api_call(&login_info, &api_call_info);
	if (rc) {
		gtk_entry_set_text(GTK_ENTRY(result_entry), "Failed!");
		goto error_out;
	}
	gtk_entry_set_text(GTK_ENTRY(result_entry), api_call_info.result);
	free(api_call_info.result);

error_out:
	free(login_info.res_token_key);
	free(login_info.res_token_secret);
	free(login_info.access_token_key);
	free(login_info.access_token_secret);
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
	GtkWidget *btn;

	/*
	 * request token uri
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	label = gtk_label_new("Request Token URI");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	reqtk_uri_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(reqtk_uri_entry), TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(reqtk_uri_entry), 80);
	gtk_entry_set_text(GTK_ENTRY(reqtk_uri_entry),
			"http://term.ie/oauth/example/request_token.php");
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
	gtk_entry_set_text(GTK_ENTRY(acctk_uri_entry),
			"http://term.ie/oauth/example/access_token.php");
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
	gtk_entry_set_text(GTK_ENTRY(api_call_uri_entry),
		"http://term.ie/oauth/example/echo_api.php?method=foo%20bar&bar=baz");
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
	gtk_entry_set_text(GTK_ENTRY(ckey_entry), "key");
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
	gtk_entry_set_text(GTK_ENTRY(csecret_entry), "secret");
	gtk_box_pack_start(GTK_BOX(hbox), csecret_entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/*
	 * button
	 */
	hbox = gtk_hbox_new(FALSE, 1);
	btn = gtk_button_new_with_label("Login");
	gtk_signal_connect(GTK_OBJECT(btn), "button_press_event",
			GTK_SIGNAL_FUNC(login_button_press), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

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
	 * show gui
	 */
	gtk_widget_show_all(msg_win);
}

