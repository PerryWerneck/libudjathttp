
dnl Process this file with autoconf to produce a configure script.

dnl The minimum version of autoconf required.
AC_PREREQ(2.59)

dnl Initialise automake with the package name, version and
dnl bug-reporting address.
AC_INIT([udjat-module-http], [1.2.1], [perry.werneck@gmail.com],[udjat-module-http],[https://github.com/PerryWerneck/udjat-module-http])

dnl Place auxilliary scripts here.
AC_CONFIG_AUX_DIR([scripts])

dnl Compute the canonical host-system type
AC_CANONICAL_HOST

dnl Put macro definitions here (though they aren't used).
AC_CONFIG_HEADER([src/include/config.h])

dnl Initialise automake stuff.
AM_INIT_AUTOMAKE

dnl Check for iconv
AM_ICONV

dnl Set gettext version
AM_GNU_GETTEXT_VERSION([0.14])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_SED
AC_PROG_LN_S

AC_LANG([C++])

PKG_CHECK_EXISTS

dnl ---------------------------------------------------------------------------
dnl Version info
dnl ---------------------------------------------------------------------------

app_vrs_major=$(echo $VERSION | cut -d. -f1)
app_vrs_minor=$(echo $VERSION | cut -d. -f2)
app_vrs_release=$(echo $VERSION | cut -d. -f3)

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_VERSION, $app_vrs_major, [The module major version])
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_VERSION, $app_vrs_minor, [The module minor version])
AC_DEFINE_UNQUOTED(PACKAGE_RELEASE, $app_vrs_release, [The module release version])

AC_SUBST(PACKAGE_MAJOR_VERSION,$app_vrs_major)
AC_SUBST(PACKAGE_MINOR_VERSION,$app_vrs_minor)
AC_SUBST(PACKAGE_RELEASE,$app_vrs_release)

AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"udjathttp-$app_vrs_major.$app_vrs_minor",[The gettext package name.])
AC_SUBST(GETTEXT_PACKAGE,"udjathttp-$app_vrs_major.$app_vrs_minor")

dnl ---------------------------------------------------------------------------
dnl Check for OS specifics
dnl ---------------------------------------------------------------------------

CPPFLAGS="$CPPFLAGS -I$ac_pwd/src/include"

case "$host" in
	*-mingw32|*-pc-msys)
		CPPFLAGS="$CPPFLAGS -pthread"
		LDFLAGS="$LDFLAGS -pthread"
		DLL_LDFLAGS="-shared -Wl,--output-def,\$(@D)/udjathttp.def"
		LIBS="$LIBS -lws2_32 -lwinhttp"
		AC_SUBST(LIBEXT,.dll)
		AC_SUBST(EXEEXT,.exe)
		AC_SUBST(OSNAME,windows)
		AC_SUBST(SONAME,udjathttp.dll)
		AC_CONFIG_FILES(sdk/windows/dynamic.pc)
		AC_CONFIG_FILES(sdk/windows/static.pc)
		AC_CONFIG_FILES(src/testprogram/windows/resources.rc)
		AC_CONFIG_FILES(src/testprogram/windows/app-manifest.xml)
		
		AC_DEFINE(HAVE_WINHTTP,[],[Do we have winhttp?])
		AC_SUBST(ENGINE,winhttp)

		;;

 	*)
		CPPFLAGS="$CPPFLAGS -pthread -ffat-lto-objects -DLIBDIR=\"\$(libdir)\" -DDATADIR=\"\$(datarootdir)\""
		LDFLAGS="$LDFLAGS -pthread"
		DLL_LDFLAGS="-shared"
		AC_SUBST(LIBEXT,.so)
		AC_SUBST(EXEEXT)
		AC_SUBST(OSNAME,linux)
		AC_SUBST(SONAME,libudjathttp.so.$app_vrs_major.$app_vrs_minor)
		AC_CONFIG_FILES(sdk/linux/dynamic.pc)
		AC_CONFIG_FILES(sdk/linux/static.pc)
		
		AC_SUBST(ENGINE,curl)
		PKG_CHECK_MODULES( [CURL], [libcurl], AC_DEFINE(HAVE_CURL,[],[Do we have libcurl?]), AC_MSG_ERROR([libcurl not present.]) )

esac

AC_SUBST(CURL_LIBS)
AC_SUBST(CURL_CFLAGS)
AC_SUBST(DLL_LDFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for tools
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([VALGRIND], [valgrind], [no])
AC_PATH_TOOL([AR], [ar], [no])
AC_PATH_TOOL([DLLTOOL], [dlltool], [true])
AC_PATH_TOOL([WINDRES], [windres], [true])

dnl ---------------------------------------------------------------------------
dnl Check for C++ 2017 support
dnl ---------------------------------------------------------------------------
AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.59)dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_IF([test x"AS_VAR_GET(CACHEVAR)" = xyes],
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AX_CHECK_COMPILE_FLAG([-std=c++17], [CPPFLAGS="$CPPFLAGS -std=c++17"], [AC_MSG_ERROR(["C++ 17 is required"])])

dnl ---------------------------------------------------------------------------
dnl Set the ELF symbol visibility support
dnl ---------------------------------------------------------------------------
AC_DEFINE(HAVE_GNUC_VISIBILITY, 1, [supports GCC visibility attributes])

dnl ---------------------------------------------------------------------------
dnl Check for compiler visibility flag
dnl ---------------------------------------------------------------------------
AC_MSG_CHECKING(whether $CC accepts -fvisibility=hidden)
AC_CACHE_VAL(app_cv_fvisibility_ok,
[ac_save_cc="$CC"
CC="$CC -fvisibility=hidden"
AC_TRY_RUN([int main() { return 0; }],
  app_cv_fvisibility_ok=yes,
  app_cv_fvisibility_ok=no,
  app_cv_fvisibility_ok=no)
CC="$ac_save_cc"])
AC_MSG_RESULT($app_cv_fvisibility_ok)
if test $app_cv_fvisibility_ok = yes; then
  CPPFLAGS="$CPPFLAGS -fvisibility=hidden"
fi

dnl ---------------------------------------------------------------------------
dnl Check for pic
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE([pic],
	[AS_HELP_STRING([--disable-pic], [disable support for PIC libraries (required to compile on 64bits)])],
[
	app_cv_pic="$enableval"
],[

	case "$host" in
		*-mingw32)
			app_cv_pic="no"
		;;

 	s390x-*)
			app_cv_pic="yes"
		;;

	*)
		app_cv_pic="yes"
	esac
])

if test "$app_cv_pic" == "yes"; then
	CPPFLAGS="$CPPFLAGS -fPIC"
fi

dnl ---------------------------------------------------------------------------
dnl Check for libudjat
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES( [UDJAT], [libudjat], AC_DEFINE(HAVE_UDJAT,[],[Do we have libudjat?]), AC_MSG_ERROR([libudjat not present.]) )

AC_SUBST(UDJAT_LIBS)
AC_SUBST(UDJAT_CFLAGS)

app_cv_product=`$PKG_CONFIG --variable=product_name libudjat`
AC_DEFINE_UNQUOTED(PRODUCT_NAME, $app_cv_product, [The product name])
AC_SUBST(PRODUCT_NAME,$app_cv_product)

app_cv_module_path=`$PKG_CONFIG --variable=module_path libudjat`
AC_DEFINE_UNQUOTED(MODULE_PATH, $app_cv_module_path, [The path for module installation])
AC_SUBST(MODULE_PATH,$app_cv_module_path)

dnl ---------------------------------------------------------------------------
dnl Check for libintl
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([XGETTEXT], [xgettext], [true])
AC_PATH_TOOL([MSGCAT], [msgcat], [true])
AC_PATH_TOOL([MSGINIT], [msginit], [true])
AC_PATH_TOOL([MSGMERGE], [msgmerge], [true])
AC_PATH_TOOL([MSGFMT], [msgfmt], [true])

AC_CHECK_HEADER(libintl.h, [AC_DEFINE(HAVE_LIBINTL, 1, [Libintl is available])], [AC_MSG_NOTICE([Can't find libintl.h])])

dnl ---------------------------------------------------------------------------
dnl test for pugixml
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES(PUGIXML, pugixml, AC_DEFINE(HAVE_PUGIXML,[],[Do we have PUGIXML?]),AC_MSG_ERROR(pugixml is required))

AC_SUBST(PUGIXML_LIBS)
AC_SUBST(PUGIXML_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Output config
dnl ---------------------------------------------------------------------------

AC_SUBST(BASEDIR,$ac_pwd)

AC_CONFIG_FILES(locale/Makefile)
AC_CONFIG_FILES(Makefile)

dnl ---------------------------------------------------------------------------
dnl Output the generated config.status script.
dnl ---------------------------------------------------------------------------

AC_OUTPUT

