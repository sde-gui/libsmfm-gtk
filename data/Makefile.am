NULL=

SUBDIRS=ui

man_MANS = libsmfm-pref-apps.1

desktopdir=$(datadir)/applications
desktop_in_files= \
	libsmfm-pref-apps.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST= \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(NULL)

