#
# Contains macros to handle library dependencies.
#


# DRUNTIME_LIBRARIES_THREAD
# -------------------------
# Allow specifying the thread library to link with or autodetect
# Add thread library to LIBS if necessary.
AC_DEFUN([DRUNTIME_LIBRARIES_THREAD],
[
enable_thread_lib=yes
AC_ARG_ENABLE(thread-lib,
  AC_HELP_STRING([--enable-thread-lib=<arg>],
                 [specify linker option for the system thread library (default: autodetect)]))

  AS_IF([test "x$enable_thread_lib" = "xyes"], [
    AC_SEARCH_LIBS([pthread_create], [pthread])
  ], [
    AS_IF([test "x$enable_thread_lib" = "xno"], [
      AC_MSG_CHECKING([for thread library])
      AC_MSG_RESULT([disabled])
      ], [
      AC_CHECK_LIB([$enable_thread_lib], [pthread_create], [], [
        AC_MSG_ERROR([Thread library not found])])
        ])
      ])
  ])
])


# DRUNTIME_LIBRARIES_ZLIB
# -----------------------
# Allow specifying whether to use the system zlib or
# compiling the zlib included in GCC. Define
# DRUNTIME_ZLIB_SYSTEM conditional and add zlib to
# LIBS if necessary.
AC_DEFUN([DRUNTIME_LIBRARIES_ZLIB],
[
  AC_ARG_WITH(target-system-zlib,
    AS_HELP_STRING([--with-target-system-zlib],
                   [use installed libz (default: no)]))

  system_zlib=false
  AS_IF([test "x$with_target_system_zlib" = "xyes"], [
    AC_CHECK_LIB([z], [deflate], [
      system_zlib=yes
      ], [
      AC_MSG_ERROR([System zlib not found])])
  ], [
    AC_MSG_CHECKING([for zlib])
    AC_MSG_RESULT([just compiled])
  ])

  AM_CONDITIONAL([DRUNTIME_ZLIB_SYSTEM], [test "$with_target_system_zlib" = yes])
])

# DRUNTIME_LIBRARIES_ATOMIC
# -------------------------
# Allow specifying whether to use libatomic for atomic support.
AC_DEFUN([DRUNTIME_LIBRARIES_ATOMIC],
[
  AC_ARG_WITH(libatomic,
    AS_HELP_STRING([--without-libatomic],
                   [Do not use libatomic in core.atomic (default: auto)]))

  DCFG_HAVE_LIBATOMIC=false
  LIBATOMIC=
  AS_IF([test "x$with_libatomic" != "xno"], [
    DCFG_HAVE_LIBATOMIC=true
    LIBATOMIC=../../libatomic/libatomic_convenience.la
  ], [
    AC_MSG_CHECKING([for libatomic])
    AC_MSG_RESULT([disabled])
  ])

  AC_SUBST(DCFG_HAVE_LIBATOMIC)
  AC_SUBST(LIBATOMIC)
])

# DRUNTIME_LIBRARIES_BACKTRACE
# ---------------------------
# Allow specifying whether to use libbacktrace for backtrace support.
# Adds subsitute for BACKTRACE_SUPPORTED, BACKTRACE_USES_MALLOC,
# and BACKTRACE_SUPPORTS_THREADS.
AC_DEFUN([DRUNTIME_LIBRARIES_BACKTRACE],
[
  AC_LANG_PUSH([C])
  BACKTRACE_SUPPORTED=false
  BACKTRACE_USES_MALLOC=false
  BACKTRACE_SUPPORTS_THREADS=false
  LIBBACKTRACE=""

  AC_ARG_WITH(libbacktrace,
    AS_HELP_STRING([--without-libbacktrace],
                   [Do not use libbacktrace in core.runtime (default: auto)]))

  AS_IF([test "x$with_libbacktrace" != "xno"], [
    LIBBACKTRACE=../../libbacktrace/libbacktrace.la

    gdc_save_CPPFLAGS=$CPPFLAGS
    CPPFLAGS+=" -I../libbacktrace "

    AC_CHECK_HEADER(backtrace-supported.h, have_libbacktrace_h=true,
      have_libbacktrace_h=false)

    if $have_libbacktrace_h; then
      AC_MSG_CHECKING([libbacktrace: BACKTRACE_SUPPORTED])
      AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
      [
      #include <backtrace-supported.h>
      #if BACKTRACE_SUPPORTED
        FOUND_LIBBACKTRACE_RESULT_GDC
      #endif
      ], BACKTRACE_SUPPORTED=true, BACKTRACE_SUPPORTED=false)
      AC_MSG_RESULT($BACKTRACE_SUPPORTED)

      AC_MSG_CHECKING([libbacktrace: BACKTRACE_USES_MALLOC])
      AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
      [
      #include <backtrace-supported.h>
      #if BACKTRACE_USES_MALLOC
        FOUND_LIBBACKTRACE_RESULT_GDC
      #endif
      ], BACKTRACE_USES_MALLOC=true, BACKTRACE_USES_MALLOC=false)
      AC_MSG_RESULT($BACKTRACE_USES_MALLOC)

      AC_MSG_CHECKING([libbacktrace: BACKTRACE_SUPPORTS_THREADS])
      AC_EGREP_CPP(FOUND_LIBBACKTRACE_RESULT_GDC,
      [
      #include <backtrace-supported.h>
      #if BACKTRACE_SUPPORTS_THREADS
        FOUND_LIBBACKTRACE_RESULT_GDC
      #endif
      ], BACKTRACE_SUPPORTS_THREADS=true, BACKTRACE_SUPPORTS_THREADS=false)
      AC_MSG_RESULT($BACKTRACE_SUPPORTS_THREADS)
    fi
    CPPFLAGS=$gdc_save_CPPFLAGS
  ], [
    AC_MSG_CHECKING([for libbacktrace])
    AC_MSG_RESULT([disabled])
  ])

  AC_SUBST(LIBBACKTRACE)
  AC_SUBST(BACKTRACE_SUPPORTED)
  AC_SUBST(BACKTRACE_USES_MALLOC)
  AC_SUBST(BACKTRACE_SUPPORTS_THREADS)
  AC_LANG_POP([C])
])
