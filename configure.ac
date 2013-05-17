#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libsmfm-gtk], [1.2.0-alpha1], [http://])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_HEADERS([config.h])

dnl version of API generation
FMLIBVER="1.0"

# automake 1.12 requires AM_PROG_AR.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC

# Checks for library functions.
AC_SEARCH_LIBS([pow], [m])

# libsmfm-core
PKG_CHECK_MODULES(LIBSMFM, "libsmfm-core")
AC_SUBST(LIBSMFM_CFLAGS)
AC_SUBST(LIBSMFM_LIBS)

# Vala
AC_ARG_ENABLE(actions,
    [AC_HELP_STRING([--disable-actions],
        [disable custom actions in context menu if you have no decent Vala compiler])],
    [enable_actions="${enableval}"],
    [enable_actions=yes]
)
if test x"$enable_actions" = xyes; then
    dnl no vala compiler required when sources are from dist tarball
    if test -f src/actions/libsmfmctions_la_vala.stamp; then
        AM_PROG_VALAC
    else
        AM_PROG_VALAC([0.13.0])
        if test x"$VALAC" = x; then
            AC_ERROR([No Vala compiler found but it is required.])
        fi
    fi
    LIBSMFMACTIONS='$(srcdir)/actions/libsmfmactions.la'
    ACTIONS_SUBDIR='actions'
    AC_DEFINE(HAVE_ACTIONS, 1, [Define to 1 to enable custom actions])
else
    LIBSMFMACTIONS=
    ACTIONS_SUBDIR=
fi
AC_SUBST(LIBSMFMACTIONS)
AC_SUBST(ACTIONS_SUBDIR)

# modules required
if test x"$enable_actions" = xyes; then
  gio_modules="gio-unix-2.0 >= 2.26.0 glib-2.0 >= 2.26.0"
else
  gio_modules="glib-2.0 >= 2.22.0 gio-unix-2.0"
fi
gio_modules="$gio_modules gthread-2.0 gobject-2.0 pango >= 1.16.0"

PKG_CHECK_MODULES(GIO, [$gio_modules])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)


# Large file support
AC_ARG_ENABLE([largefile],
    AS_HELP_STRING([--enable-largefile],
        [enable Large file support (default: yes)]),
    largefile=$enableval, largefile="yes")

if test x"$largefile" = x"yes"; then
  CPPFLAGS="$CPPFLAGS -D_LARGEFILE_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64"
  AC_DEFINE(_LARGEFILE64_SOURCE, 1, [Whether to enable large file support])
  AC_DEFINE(_FILE_OFFSET_BITS, 64, [File offset bits])
fi


# intltool
IT_PROG_INTLTOOL([0.40.0])

dnl libgtk version
AC_ARG_WITH(gtk,
    [AC_HELP_STRING([--with-gtk=VER],
        [choose Gtk+ version for libsmfm-gtk build (default=2)])
    ],
    [ac_with_gtk="${withval}"],
    [ac_with_gtk=2]
)

case "$ac_with_gtk" in
    3|3.*)
        pkg_modules="$gio_modules gtk+-3.0 cairo >= 1.8.0"
        GTK_VERSION_MAJOR="3"
        ;;
    2|2.*)
        pkg_modules="$gio_modules gtk+-2.0 >= 2.18.0 cairo >= 1.8.0"
        GTK_VERSION_MAJOR="2"
        ;;
    *)
        AC_MSG_ERROR([Invalid value of option: --with-gtk=$ac_with_gtk])
        ;;
esac
PKG_CHECK_MODULES(GTK, [$pkg_modules])

LIBSMFM_GTK_LTLIBRARIES=libsmfm-gtk${GTK_VERSION_MAJOR}.la
LIBSMFM_PREF_APPS="libsmfm-pref-apps"

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(LIBSMFM_GTK_LTLIBRARIES)
AC_SUBST(LIBSMFM_PREF_APPS)
AC_SUBST(GTK_VERSION_MAJOR)

dnl pango_modules="pango >= 1.16.0"
dnl PKG_CHECK_MODULES(PANGO, [$pango_modules])
dnl AC_SUBST(PANGO_CFLAGS)
dnl AC_SUBST(PANGO_LIBS)

dnl gmodule may be used for libsmfm-chooser-hack.la
#gmodule_modules="gmodule-2.0"
#PKG_CHECK_MODULES(GMODULE, [$gmodule_modules])
#AC_SUBST(GMODULE_CFLAGS)
#AC_SUBST(GMODULE_LIBS)

menu_modules="libmenu-cache >= 0.3.2"
PKG_CHECK_MODULES(MENU_CACHE, [$menu_modules])
AC_SUBST(MENU_CACHE_CFLAGS)
AC_SUBST(MENU_CACHE_LIBS)

dnl check for menu-cache versions 0.4.x since no macro MENU_CACHE_CHECK_VERSION
dnl is available in those versions
LIBS_save="${LIBS}"
LIBS="${LIBS} ${MENU_CACHE_LIBS}"
AC_CHECK_FUNCS(menu_cache_dir_list_children)
LIBS="${LIBS_save}"

#optional features

#check for gtk-doc
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_ARG_ENABLE(demo,
    [AC_HELP_STRING([--enable-demo],
        [build libsmfm-demo file manager @<:@default=no@:>@])],
    [enable_demo="${enableval}"],
    [enable_demo=no]
)
AM_CONDITIONAL(ENABLE_DEMO, test x"$enable_demo" = x"yes")
DEMO_APPS="libsmfm-demo-gtk$GTK_VERSION_MAJOR libsmfm-file-search-cli-demo-gtk$GTK_VERSION_MAJOR"
AC_SUBST(DEMO_APPS)

GETTEXT_PACKAGE="libsmfm-gtk$GTK_VERSION_MAJOR"
AC_SUBST(GETTEXT_PACKAGE)

# Generate po/LINGUAS on the fly rather than relying on translators
# to maintain it manually. This also overcome the problem that Transifex
# cannot add a language to po/LINGUAS if a new po file is submitted.
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(FMLIBVER)

FMINCLUDEADD='-I${includedir}'
AC_SUBST(FMINCLUDEADD)

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],
        [build libsmfm with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)
if test "$enable_debug" = "yes"; then
    # turn on debug and disable optimization
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    case "$CC" in
    gcc*)
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
        ;;
    *)
        ;;
    esac
    dnl Be more strict on portability
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    # turn off glib debug checks
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

dnl Fix invalid sysconfdir when --prefix=/usr
if test `eval "echo $sysconfdir"` = /usr/etc
then
    sysconfdir=/etc
fi

dnl Make version subfields for FM_CHECK_VERSION macro
ac_version_subst_str=`echo $VERSION | awk -F '.' '/.*/ { printf "FM_VERSION_MAJOR=%d FM_VERSION_MINOR=%d FM_VERSION_MICRO=%d", $1, $2, $3 }'`
eval ${ac_version_subst_str}
AC_SUBST(FM_VERSION_MAJOR)
AC_SUBST(FM_VERSION_MINOR)
AC_SUBST(FM_VERSION_MICRO)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/actions/Makefile
    data/Makefile
    data/libsmfm-pref-apps.1
    data/ui/Makefile
    po/Makefile.in
    docs/Makefile
    docs/reference/Makefile
    docs/reference/libsmfm-gtk/Makefile
    docs/reference/libsmfm-gtk/version.xml
    libsmfm-gtk2.pc
    libsmfm-gtk3.pc
])

AC_OUTPUT

echo
echo "Libfm $VERSION Configuration Summary:"
echo
echo "prefix:                                                 $prefix"
echo "sysconfdir:                                             $sysconfdir"
echo "Enable compiler flags and other support for debugging:  $enable_debug"
echo "Build demo program src/demo/libsmfm-demo:               $enable_demo"
echo "Build with custom actions support (requires Vala):      $enable_actions"
echo "Large file support:                                     $largefile"
echo "Build libsmfm-gtk for Gtk+ version:                     $ac_with_gtk"
echo "Build API doc with gtk-doc (recommended for make dist): $enable_gtk_doc"
echo
echo
echo  "LIBSMFM_CFLAGS   : $LIBSMFM_CFLAGS"
echo  "LIBSMFM_LIBS     : $LIBSMFM_LIBS"
echo

