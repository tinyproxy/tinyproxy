dnl as-compiler-flag.m4 0.1.0

dnl autostars m4 macro for detection of compiler flags

dnl David Schleef <ds@schleef.org>
dnl Tim-Philipp Müller <tim centricular net>

dnl AS_COMPILER_FLAG(CFLAGS, ACTION-IF-ACCEPTED, [ACTION-IF-NOT-ACCEPTED])
dnl Tries to compile with the given CFLAGS.
dnl Runs ACTION-IF-ACCEPTED if the compiler can compile with the flags,
dnl and ACTION-IF-NOT-ACCEPTED otherwise.

AC_DEFUN([AS_COMPILER_FLAG],
[
  AC_MSG_CHECKING([to see if compiler understands $1])

  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_TRY_COMPILE([ ], [], [flag_ok=yes], [flag_ok=no])
  CFLAGS="$save_CFLAGS"

  if test "X$flag_ok" = Xyes ; then
    $2
    true
  else
    $3
    true
  fi
  AC_MSG_RESULT([$flag_ok])
])

dnl AS_CXX_COMPILER_FLAG(CPPFLAGS, ACTION-IF-ACCEPTED, [ACTION-IF-NOT-ACCEPTED])
dnl Tries to compile with the given CPPFLAGS.
dnl Runs ACTION-IF-ACCEPTED if the compiler can compile with the flags,
dnl and ACTION-IF-NOT-ACCEPTED otherwise.

AC_DEFUN([AS_CXX_COMPILER_FLAG],
[
  AC_REQUIRE([AC_PROG_CXX])

  AC_MSG_CHECKING([to see if c++ compiler understands $1])

  save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $1"

  AC_LANG_PUSH(C++)

  AC_TRY_COMPILE([ ], [], [flag_ok=yes], [flag_ok=no])
  CPPFLAGS="$save_CPPFLAGS"

  if test "X$flag_ok" = Xyes ; then
    $2
    true
  else
    $3
    true
  fi

  AC_LANG_POP(C++)

  AC_MSG_RESULT([$flag_ok])
])

