bin_PROGRAMS = unim

AM_CFLAGS = $(CFLAGS) \
	-I $(top_srcdir)/include \
	-I $(srcdir)/include \
	$(gtk_CFLAGS) $(oauth_CFLAGS)

unim_LDADD = \
	-lm -lpthread \
	$(gtk_LIBS) $(oauth_LIBS)


NULL_FILE =

unim_SOURCES = \
	main.c \
	oauth_test.c \
	$(NULL_FILE)

SUBDIRS =

