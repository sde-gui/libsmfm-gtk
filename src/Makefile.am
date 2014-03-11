# for use in libtool -version-info
ABI_VERSION=4:0:0

NULL=

# current dir needs to be built before tests
SUBDIRS= \
	$(ACTIONS_SUBDIR) \
	. \
	$(NULL)

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(srcdir)/actions \
	-I$(srcdir)/gtk \
	-I$(srcdir)/demo \
	-DPACKAGE_DATA_DIR=\""$(datadir)/libsmfm"\" \
	-DPACKAGE_UI_DIR=\""$(datadir)/libsmfm/ui"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)/libsmfm"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

glib_compat_SOURCES= \
	glib-compat.c \
	glib-compat.h \
	$(NULL)

gtk_compat_SOURCES= \
	$(glib_compat_SOURCES) \
	gtk-compat.c \
	gtk-compat.h \
	$(NULL)

exo_SOURCES = \
	gtk/exo/exo-icon-view.c \
	gtk/exo/exo-private.c \
	gtk/exo/exo-tree-view.c \
	gtk/exo/exo-icon-view.h \
	gtk/exo/exo-private.h \
	gtk/exo/exo-tree-view.h \
	gtk/exo/exo-marshal.c \
	gtk/exo/exo-string.c \
	gtk/exo/exo-marshal.h \
	gtk/exo/exo-string.h \
	$(NULL)

libsmfm_gtk2_SOURCES = \
	fm-gtk.c \
	$(gtk_compat_SOURCES) \
	$(exo_SOURCES) \
	gtk/fm-folder-menu.c \
	gtk/fm-folder-view.c \
	gtk/fm-standard-view.c \
	gtk/fm-folder-model.c \
	gtk/fm-folder-model-column.c \
	gtk/fm-places-view.c \
	gtk/fm-places-model.c \
	gtk/fm-dir-tree-view.c \
	gtk/fm-dir-tree-model.c \
	gtk/fm-side-pane.c \
	gtk/fm-app-menu-view.c \
	gtk/fm-app-chooser-dlg.c \
	gtk/fm-app-chooser-combo-box.c \
	gtk/fm-path-entry.c \
	gtk/fm-file-menu.c \
	gtk/fm-file-properties.c \
	gtk/fm-clipboard.c \
	gtk/fm-progress-dlg.c \
	gtk/fm-icon-pixbuf.c \
	gtk/fm-pixbuf-utils.c \
	gtk/fm-cell-renderer-text.c \
	gtk/fm-cell-renderer-pixbuf.c \
	gtk/fm-dnd-src.c \
	gtk/fm-dnd-dest.c \
	gtk/fm-dnd-auto-scroll.c \
	gtk/fm-gtk-utils.c \
	gtk/fm-gtk-file-launcher.c \
	gtk/fm-thumbnail.c \
	gtk/fm-tab-label.c \
	gtk/fm-path-bar.c \
	gtk/fm-dentry-properties.c \
	gtk/fm-gtk-marshal.c \
	$(NULL)


libsmfmgtkincludedir = $(includedir)/libsmfm-gtk@GTK_VERSION_MAJOR@-@FMLIBVER@/libsmfm-gtk
libsmfmgtkinclude_HEADERS = \
	fm-gtk.h \
	gtk/fm-folder-view.h \
	gtk/fm-standard-view.h \
	gtk/fm-folder-model.h \
	gtk/fm-folder-model-column.h \
	gtk/fm-places-view.h \
	gtk/fm-places-model.h \
	gtk/fm-dir-tree-view.h \
	gtk/fm-dir-tree-model.h \
	gtk/fm-side-pane.h \
	gtk/fm-app-menu-view.h \
	gtk/fm-app-chooser-dlg.h \
	gtk/fm-app-chooser-combo-box.h \
	gtk/fm-path-entry.h \
	gtk/fm-file-menu.h \
	gtk/fm-file-properties.h \
	gtk/fm-clipboard.h \
	gtk/fm-progress-dlg.h \
	gtk/fm-icon-pixbuf.h \
	gtk/fm-pixbuf-utils.h \
	gtk/fm-cell-renderer-text.h \
	gtk/fm-cell-renderer-pixbuf.h \
	gtk/fm-dnd-src.h \
	gtk/fm-dnd-dest.h \
	gtk/fm-dnd-auto-scroll.h \
	gtk/fm-gtk-utils.h \
	gtk/fm-gtk-file-launcher.h \
	gtk/fm-thumbnail.h \
	gtk/fm-tab-label.h \
	gtk/fm-path-bar.h \
	gtk/fm-sortable.h \
	gtk/fm-gtk-marshal.h \
	$(NULL)

pkginclude_HEADERS = \
	$(LIBSMFM_INCLUDES) \
	$(LIBSMFM_GTK_INCLUDES) \
	$(NULL)

EXTRA_LTLIBRARIES = libsmfm-gtk2.la libsmfm-gtk3.la

lib_LTLIBRARIES = @LIBSMFM_GTK_LTLIBRARIES@

libsmfm_gtk2_la_SOURCES = \
	$(libsmfm_gtk2_SOURCES) \
	$(NULL)
libsmfm_gtk3_la_SOURCES = $(libsmfm_gtk2_la_SOURCES)

libsmfm_gtk2_la_CFLAGS = \
	$(LIBSMFM_CFLAGS) \
	$(GTK_CFLAGS) \
	$(MENU_CACHE_CFLAGS) \
	-I$(srcdir) \
	-I$(srcdir)/base \
	-I$(srcdir)/job \
	-Werror-implicit-function-declaration
libsmfm_gtk3_la_CFLAGS = $(libsmfm_gtk2_la_CFLAGS)

libsmfm_gtk2_la_LIBADD = \
	$(LIBSMFM_LIBS) \
	$(GTK_LIBS) \
	$(MENU_CACHE_LIBS) \
	$(INTLLIBS) \
	$(NULL)
libsmfm_gtk3_la_LIBADD = $(libsmfm_gtk2_la_LIBADD)

libsmfm_gtk2_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^fm \
	-version-info $(ABI_VERSION) \
	-rpath $(libdir) \
	$(NULL)
libsmfm_gtk3_la_LDFLAGS = $(libsmfm_gtk2_la_LDFLAGS)

EXTRA_PROGRAMS = libsmfm-pref-apps

bin_PROGRAMS = @LIBSMFM_PREF_APPS@

libsmfm_pref_apps_SOURCES = \
	$(gtk_compat_SOURCES) \
	tools/libfm-pref-apps.c \
	$(NULL)

libsmfm_pref_apps_DEPENDENCIES = \
	@LIBSMFM_GTK_LTLIBRARIES@ \
	$(NULL)

libsmfm_pref_apps_CFLAGS = \
	$(LIBSMFM_CFLAGS) \
	$(GTK_CFLAGS) \
	$(MENU_CACHE_CFLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)

libsmfm_pref_apps_LDADD = \
	$(LIBSMFM_LIBS) \
	$(GTK_LIBS) \
	$(MENU_CACHE_LIBS) \
	$(INTLLIBS) \
	@LIBSMFM_GTK_LTLIBRARIES@ \
	$(NULL)


if ENABLE_DEMO

bin_PROGRAMS += @DEMO_APPS@

EXTRA_PROGRAMS += \
	libsmfm-file-search-cli-demo-gtk2 \
	libsmfm-file-search-cli-demo-gtk3 \
	$(NULL)

libsmfm_file_search_cli_demo_gtk2_SOURCES = \
	demo/libsmfm-file-search-cli-demo.c \
	$(NULL)
libsmfm_file_search_cli_demo_gtk3_SOURCES = $(libsmfm_file_search_cli_demo_gtk2_SOURCES)

libsmfm_file_search_cli_demo_gtk2_DEPENDENCIES = \
	@LIBSMFM_GTK_LTLIBRARIES@ \
	$(NULL)
libsmfm_file_search_cli_demo_gtk3_DEPENDENCIES = $(libsmfm_demo_gtk2_DEPENDENCIES)

libsmfm_file_search_cli_demo_gtk2_CFLAGS = \
	$(LIBSMFM_CFLAGS) \
	$(GTK_CFLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)
libsmfm_file_search_cli_demo_gtk3_CFLAGS = $(libsmfm_file_search_cli_demo_gtk2_CFLAGS)

libsmfm_file_search_cli_demo_gtk2_LDADD = \
	$(LIBSMFM_LIBS) \
	$(GTK_LIBS) \
	$(INTLLIBS) \
	@LIBSMFM_GTK_LTLIBRARIES@ \
	$(NULL)
libsmfm_file_search_cli_demo_gtk3_LDADD = $(libsmfm_file_search_cli_demo_gtk2_LDADD)

endif


# GObject marshallers
gtk/fm-gtk-marshal.h: gtk/fm-gtk-marshal.list
	glib-genmarshal --header --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

gtk/fm-gtk-marshal.c: gtk/fm-gtk-marshal.list
	glib-genmarshal --body --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

# Workarounds to force the build
BUILT_SOURCES= \
	gtk/fm-gtk-marshal.h \
	gtk/fm-gtk-marshal.c \
	$(NULL)

# Vala vapi files
VAPI_FILES = \
	vapi/libfm.vapi \
	$(NULL)

EXTRA_DIST = \
	gtk/fm-gtk-marshal.list \
	gtk/fm-file-search-ui.c \
	gtk/exo/README \
	$(VAPI_FILES) \
	$(NULL)

CLEANFILES = \
	gtk/fm-gtk-marshal.h \
	gtk/fm-gtk-marshal.c \
	$(NULL)

# A lib to be loaede with LD_PRELOADS to override GtkFileChooserWidget
#lib_LTLIBRARIES=libfm-chooser-hack.la

#libfm_chooser_hack_la_SOURCES=libfm-chooser-hack.c
#libfm_chooser_hack_la_CFLAGS=$(GTK_CFLAGS)
#libfm_chooser_hack_la_LIBADD=$(GMODULE_LIBS)
#libfm_chooser_hack_la_LDFLAGS= \
#        -no-undefined \
#        -export-symbols-regex gtk

