dnl $Id: argenable.m4,v 1.1 2004-08-24 18:40:21 rjkaes Exp $
dnl
dnl Define a new AC_ARG_ENABLE like macro which handles invalid inputs
dnl correctly.  The macro takes three arguments:
dnl  1) the option name (used like --enable-option)
dnl  2) the help string
dnl  3) the default value (either yes or no)
dnl
dnl This macro also defines on variable in the form "option_enabled"
dnl set to either "yes" or "no".
dnl
AC_DEFUN([TP_ARG_ENABLE],
[var=`echo $1 | tr - _`
AC_ARG_ENABLE([$1],
  AS_HELP_STRING([--enable-$1], [$2]),
  [case "${enableval}" in
    yes)  eval ${var}_enabled=yes ;;
    no)   eval ${var}_enabled=no ;;
    *)    AC_MSG_ERROR([bad value ${enableval} for --enable-$1]) ;;
   esac],[eval ${var}_enabled=$3])])
