bin_PROGRAMS = unim

AM_CFLAGS = $(CFLAGS) \
	-I $(top_srcdir)/include \
	-I $(srcdir)/include \
	$(gtk_CFLAGS) $(oauth_CFLAGS) $(json_CFLAGS)

unim_LDADD = \
	-lm -lpthread \
	-lcurl -ljansson \
	$(gtk_LIBS) $(oauth_LIBS) $(json_LIBS)


NULL_FILE =

unim_SOURCES = \
	main.c \
	oauth_test.c \
	unim.c unim_oauth.c weibo_oauth.c \
	wu_http.c \
	$(NULL_FILE)

SUBDIRS =

