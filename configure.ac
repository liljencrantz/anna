
#
# This file is the main build configuration file for anna. It is used
# to determine your systems capabilities, and tries to adapt anna to
# take maximum advantage of the services your system offers.
#
# Process this file using the 'autoconf' command to produce a working
# configure script, which should in turn be executed in order to
# configure the build process.
#

AC_INIT(anna,1.0,axel@liljencrantz.se)


#
# List of output variables produced by this configure script
#
AC_SUBST(LDFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CFLAGS_WARN)
AC_SUBST(LIBS)



#
# If needed, run autoconf to regenerate the configure file
#
# This makes sure that after running autoconf once to create the first
# version of configure, we never again need to worry about manually
# running autoconf to handle an updates configure.ac.
#

AC_MSG_CHECKING([if autoconf needs to be run])
if test configure -ot configure.ac; then
	AC_MSG_RESULT([yes])
	if which autoconf >/dev/null; then
		# No need to provide any error messages if autoconf fails, the
		# shell and autconf should take care of that themselves
		AC_MSG_NOTICE([running autoconf])
		if autoconf; then
			./configure "$@"
			exit
		fi
		exit 1
	else
		AC_MSG_ERROR( 
			[cannot find the autoconf program in your path.
This program needs to be run whenever the configure.ac file is modified.
Please install it and try again.]
		)
	fi
else
	AC_MSG_RESULT([no])
fi


#
# If needed, run autoheader to regenerate config.h.in
#
# This makes sure we never ever have to run autoheader manually. It
# will be run whenever needed automatically.
#

AC_MSG_CHECKING([if autoheader needs to be run])
if test ! -f ./include/anna/config.h.in -o include/anna/config.h.in -ot configure.ac; then
	AC_MSG_RESULT([yes])
	if which autoheader >/dev/null; then
		AC_MSG_NOTICE([running autoheader])
		autoheader || exit 1
	else
		AC_MSG_ERROR(
			[cannot find the autoheader program in your path.
This program needs to be run whenever the configure.ac file is modified.
Please install it and try again.]
		)
	fi
else
	AC_MSG_RESULT([no])
fi


#
# Detect directories which may contain additional headers, libraries
# and commands. This needs to be done early - before Autoconf starts
# to mess with CFLAGS and all the other environemnt variables.
#
# This mostly helps OS X users, since fink usually installs out of
# tree and doesn't update CFLAGS.
#

for i in /usr/pkg /sw /opt /opt/local; do

	AC_MSG_CHECKING([for $i/include include directory])
	if test -d $i/include; then
		AC_MSG_RESULT(yes)
		CPPFLAGS="$CPPFLAGS -I$i/include/"
		CFLAGS="$CFLAGS -I$i/include/"
	else
	AC_MSG_RESULT(no)
	fi

	AC_MSG_CHECKING([for $i/lib library directory])
	if test -d $i/lib; then
		AC_MSG_RESULT(yes)
		LDFLAGS="$LDFLAGS -L$i/lib/ -R$i/lib/"
	else
		AC_MSG_RESULT(no)
	fi

	AC_MSG_CHECKING([for $i/bin command directory])
	if test -d $i/bin; then
		AC_MSG_RESULT(yes)
		optbindirs="$optbindirs $i/bin"
	else
		AC_MSG_RESULT(no)
	fi

done


#
# Tell autoconf to create config.h header 
#
AC_CONFIG_HEADERS(include/anna/config.h)


#
# Set up various programs needed for install
#

# Here we look for c99 before cc as Sun Studio compiler supports c99
# through the c99 binary.

AC_PROG_CC([gcc c99 cc])
AC_PROG_CPP
AC_PROG_INSTALL


#
# Test if the compiler accepts the -std=c99 flag. If so, using it
# increases the odds of correct compilation, since we want to use the
# *wprintf functions, which where defined in C99.
#
# NOTE: Never versions of autoconf has AC_CHECK_PROG_CC_C99
#

if test "$CC" != "c99"; then
  XCFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -std=c99"
	XCPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -std=c99"
	AC_MSG_CHECKING(if -std=c99 works)

	AC_CACHE_VAL(
  	  local_cv_has__std_c99,
    	[
      	  AC_TRY_RUN(
        	    [
          	      #include <stdlib.h>
            	    #include <stdio.h>

              	  int main()
                	{
                  	  return 0;
	                }
	            ],
	            local_cv_has__std_c99=yes,
	            local_cv_has__std_c99=no,
  	      )
    	]
	)

	AC_MSG_RESULT($local_cv_has__std_c99)
	case x$local_cv_has__std_c99 in
	xno)
  	  CFLAGS="$XCFLAGS"
    	CPPFLAGS="$XCPPFLAGS" ;;
	esac
fi

#
# Try to enable large file support. This will make sure that on systems
# where off_t can be either 32 or 64 bit, the latter size is used. On
# other systems, this should do nothing. (Hopefully)
#

CFLAGS="$CFLAGS -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64"

#
# Nice to have GCC-specific CFLAGS
#

if test "$GCC" = yes; then
	#
	# -Wall is there to keep me on my toes
	#

	CFLAGS="$CFLAGS"
	CFLAGS_WARN="-Wall -Werror=implicit-function-declaration -Wmissing-braces -Wmissing-prototypes"
fi


#
# If we are compiling against glibc, set some flags to work around
# some rather stupid attempts to hide prototypes for *wprintf
# functions, as well as prototypes of various gnu extensions.
#

AC_MSG_CHECKING([if we are compiling against glibc])
AC_RUN_IFELSE(
	[
		AC_LANG_PROGRAM(
			[
				#include <stdlib.h>
				#ifdef __GLIBC__
				#define STATUS 0
				#else
				#define STATUS 1
				#endif
			],
			[
				return STATUS;
			]
		)
	], 
	[glibc=yes], 
	[glibc=no] 
)

if test "$glibc" = yes; then
	AC_MSG_RESULT(yes)

	#
	# This gives us access to prototypes for gnu extensions and C99
	# functions if we are compiling agains glibc. All GNU extensions
	# that are used must have a fallback implementation available in
	# fallback.h, in order to keep anna working on non-gnu platforms.
	#

	CFLAGS="$CFLAGS -D_GNU_SOURCE=1 -D_ISO99_SOURCE=1"
else
	AC_MSG_RESULT(no)
fi

# Check for os dependant libraries for all binaries.
AC_SEARCH_LIBS( __gmpz_init, [gmp], , [AC_MSG_ERROR([Could not find a gmp implementation.])] )
AC_SEARCH_LIBS( setupterm, [ncurses], , [AC_MSG_ERROR([Could not find a curses implementation.])] )
AC_SEARCH_LIBS( nan, [m], , [AC_MSG_ERROR([Could not find a math library implementation.])] )
AC_SEARCH_LIBS( pthread_create, [pthread], , [AC_MSG_ERROR([Could not find a posix thread library implementation.])] )
AC_SEARCH_LIBS( dlopen, [dl], , [AC_MSG_ERROR([Could not find a dl library implementation.])] )
AC_SEARCH_LIBS( readline, [readline], , [AC_MSG_ERROR([Could not find a readline library implementation.])] )
AC_SEARCH_LIBS( yywrap, [l], , [AC_MSG_ERROR([Could not find a l library implementation.])] )

#
# Check presense of various header files
#

AC_CHECK_HEADERS([getopt.h termio.h sys/resource.h term.h ncurses/term.h ncurses.h curses.h stropts.h siginfo.h sys/select.h sys/ioctl.h sys/termios.h libintl.h execinfo.h sys/prctl.h sys/signalfd.h])

#
# On some platforms (Solaris 10) adding -std=c99 in turn requires that
# _POSIX_C_SOURCE be defined to 200112L otherwise several
# POSIX-specific, non-ISO-C99 types/prototypes are made unavailable
# e.g. siginfo_t. Defining _XOPEN_SOURCE to 600 is compatible with
# the _POSIX_C_SOURCE value and provides a little assurance that
# extension functions' prototypes are available, e.g. killpg().
#
# Some other platforms (OS X), will remove types/prototypes/macros
# e.g. SIGWINCH if either _POSIX_C_SOURCE or _XOPEN_SOURCE is defined.
#
# This test adds these macros only if they enable a program that uses
# both Posix and non-standard features to compile, and that program
# does not compile without these macros.
#
# We try to make everyone happy.
#
# The ordering of the various autoconf tests is very critical as well:
#
# * This test needs to be run _after_ header detection tests, so that
# the proper headers are included.
#
# * This test needs to be run _before_ testing for the presense of any
# prototypes or other language functinality.
#
# * This test should be (but does not need to be) run after the
# conditional definition of __EXTENSIONS__, to avoid redundant tests.
#

XCFLAGS="$CFLAGS"

echo Checking how to use -D_XOPEN_SOURCE=600 and -D_POSIX_C_SOURCE=200112L...
local_found_posix_switch=no

for i in "" "-D_POSIX_C_SOURCE=200112L" "-D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L"; do

	AC_MSG_CHECKING( if switches \"$i\" works)
	CFLAGS="$XCFLAGS $i"

	#
	# Try to run this program, which should test various extensions
	# and Posix functionality. If this program works, then everything
	# should work. Hopefully.
	#

	AC_TRY_LINK(
		[
			#include <stdlib.h>
			#include <stdio.h>
			#include <sys/types.h>

			/* POSIX, C89 and C99: POSIX extends this header.
			 * For: kill(), killpg(), siginfo_t, sigset_t,
			 * struct sigaction, sigemptyset(), sigaction(),
			 * SIGIO and SIGWINCH.  */
			#include <signal.h>

			#ifdef HAVE_SIGINFO_H
			/* Neither POSIX, C89 nor C99: Solaris-specific (others?).
			 * For: siginfo_t (also defined by signal.h when in
			 * POSIX/extensions mode). */
			#include <siginfo.h>
			#endif

			#ifdef HAVE_SYS_TERMIOS_H
			/* Neither POSIX, C89 nor C99: a common extension.
			 * For: TIOCGWINSZ and struct winsize (under at least
			 * Solaris, NetBSD and (dual-listed) FreeBSD). */
			#include <sys/termios.h>
			#endif

			#ifdef HAVE_SYS_IOCTL_H
			/* As above (under at least Linux and FreeBSD). */
			#include <sys/ioctl.h>
			#endif
		],
		[
			/* Avert high-level optimisation, by making the program's
			 * return value depend on all tested identifiers. */
			int ret = 0;
			/* POSIX only: might be unhidden by _POSIX_C_SOURCE. */
			struct sigaction sa;
			sigset_t ss;
			siginfo_t info;
			ret += (int)(void *)&info + kill( 0, 0 ) +
				sigaction( 0, &sa, 0 ) + sigemptyset( &ss );
			/* Extended-POSIX: might be unhidden by _XOPEN_SOURCE. */
			ret += killpg( 0, 0 );
			/* Non-standard: might be hidden by the macros. */
			{ 
				struct winsize termsize;
				ret += (int)(void *)&termsize;
				ret += SIGWINCH + TIOCGWINSZ + SIGIO; 
			}
			return ret;

		],
		local_cv_use__posix_c_source=yes,
		local_cv_use__posix_c_source=no,
	)

	if test x$local_cv_use__posix_c_source = xyes; then
		AC_MSG_RESULT( yes )
		local_found_posix_switch=yes
		break;
	else
		AC_MSG_RESULT( no )
	fi

done

#
# We didn't find any combination of switches that worked - revert to
# no switches and hope that the fallbacks work.
#

if test ! x$local_found_posix_switch = xyes; then
	CFLAGS="$XCFLAGS"
fi


#
# Check for presense of various functions used by anna
#

#AC_CHECK_FUNCS(wcsdup wcslen wcscmp wcschr wcsrchr)
AC_CHECK_FUNCS(fputwc fgetwc wcstol prctl swprintf vswprintf clearenv)
AC_CHECK_DECLS([cpow], [], [], [[#include <complex.h>]])
AC_CHECK_DECLS([clog], [], [], [[#include <complex.h>]])
AC_CHECK_DECLS([clog10], [], [], [[#include <complex.h>]])
AC_CHECK_DECLS([fdatasync], [], [], [[#include <unistd.h>]])

# Tell the world what we know
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_CHECK_LIB([intl], [libintl_dcgettext], [have_libintl=yes], [have_libintl=no])
if test x$have_libintl = xyes; then
   LDFLAGS="$LDFLAGS -lintl"
fi

AC_CHECK_LIB([iconv], [iconv_open], [have_libiconv=yes], [have_libiconv=no])
if test x$have_libiconv = xyes; then
   LDFLAGS="$LDFLAGS -liconv"
fi

AC_CHECK_LIB([execinfo], [backtrace], [have_libexecinfo=yes], [have_libexecinfo=no])
if test x$have_libexecinfo = xyes; then
   LDFLAGS="$LDFLAGS -lexecinfo"
fi


echo "anna is now configured."
echo "Use 'make' and 'make install' to build and install."

