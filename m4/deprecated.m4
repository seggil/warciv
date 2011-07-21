AC_DEFUN([WC_DEPRECATED], [
AC_ARG_ENABLE(deprecate-gtk,
[  --enable-deprecate-gtk[[=no/yes]] turn off deprecate GTK [[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_gtk=no ;;
  no)  enable_deprecate_gtk=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gtk} for --enable-deprecate-gtk) ;;
esac], [enable_deprecate_gtk=yes])

if test "x${enable_deprecate_gtk}" = "xyes" ; then
   AC_DEFINE(GTK_ENABLE_DEPRECATED, 1, [Enable deprecate GTK functions])
fi

AC_ARG_ENABLE(deprecate-gdk,
[  --enable-deprecate-gdk[[=no/yes]] turn off deprecate GDK [[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_gdk=no ;;
  no)  enable_deprecate_gdk=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gdk} for --enable-deprecate-gdk) ;;
esac], [enable_deprecate_gdk=yes])

if test "x${enable_deprecate_gdk}" = "xyes" ; then
   AC_DEFINE(GDK_ENABLE_DEPRECATED, 1, [Enable deprecate GDK functions])
fi

AC_ARG_ENABLE(deprecate-gdk-pixbuf,
[  --enable-deprecate-gdk-pixbuf[[=no/yes]] turn off deprecate GDK PIXBUF [[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_gdk_pixbuf=no ;;
  no)  enable_deprecate_gdk_pixbuf=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gdk-pixbuf} for --enable-deprecate-gdk-pixbuf) ;;
esac], [enable_deprecate_gdk_pixbuf=yes])

if test "x${enable_deprecate_gdk_pixbuf}" = "xyes" ; then
   AC_DEFINE(GDK_PIXBUF_ENABLE_DEPRECATED, 1, [Enable deprecate GdkPixbuf functions])
fi

AC_ARG_ENABLE(deprecate-glib,
[  --enable-deprecate-glib[[=no/yes]] turn off deprecate glib [[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_glib=no ;;
  no)  enable_deprecate_glib=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-glib} for --enable-deprecate-glib) ;;
esac], [enable_deprecate_glib=yes])

if test "x${enable_deprecate_glib}" = "xyes" ; then
   AC_DEFINE(G_ENABLE_DEPRECATED, 1, [Enable deprecate GLib functions])
fi

AC_ARG_ENABLE(deprecate-gtk-multihead-safe,
[  --enable-deprecate-gtk-multihead-safe[[=no/yes]] turn off deprecate GTK MULTIHEAD SAFE [[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_gtk_multihead_safe=no ;;
  no)  enable_deprecate_gtk_multihead_safe=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gtk-multihead-safe} for --enable-deprecate-gtk-multihead-safe) ;;
esac], [enable_deprecate_gtk_multihead_safe=yes])

if test "x${enable_deprecate_gtk_multihead_safe}" = "xyes" ; then
   AC_DEFINE(GTK_MULTIHEAD_SAFE, 1,[Don't use any functions which may be problematic in a multihead setting])
fi

AC_ARG_ENABLE(deprecate-gtk-enable-single-includes,
[  --enable-deprecate-gtk-enable-single-includes[[=no/yes]] turn off deprecate GTK ENABLE SINGLE INCLUDES[[default=no]]],
[case "${enableval}" in
  yes) enable_deprecate_gtk_enable_single_includes=no ;;
  no)  enable_deprecate_gtk_enable_single_includes=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gtk-enable-single-includes} for --enable-deprecate-gtk-enable-single-includes) ;;
esac], [enable_deprecate_gtk_enable_single_includes=yes])

if test "x${enable_deprecate_gtk_enable_single_includes}" = "xyes" ; then
   AC_DEFINE(GTK_ENABLE_SINGLE_INCLUDES, 1, [Only include the toplevel headers gtk.h])
fi

AC_ARG_ENABLE(deprecate-gdk-pixbuf-enable-single-includes,
[  --enable-deprecate-gdk-pixbuf-enable-single-includes[[=no/yes]]  turn off deprecate GDK PIXBUF ENABLE SINGLE INCLUDES[[default=no]]],
[case "${enableval}" in   
  yes) enable_deprecate_gdk_pixbuf_enable_single_includes=no ;;
  no)  enable_deprecate_gdk_pixbuf_enable_single_includes=yes ;;
  *)   AC_MSG_ERROR(bad value ${deprecate-gdk-pixbuf-enable-single-includes} for --enable-deprecate-gdk-pixbuf-enable-single-includes) ;;
esac], [enable_deprecate_gdk_pixbuf_enable_single_includes=yes])

if test "x${enable_deprecate_gdk_pixbuf_enable_single_includes}" = "xyes" ; then
   AC_DEFINE(GDK_PIXBUF_ENABLE_SINGLE_INCLUDES, 1, [Only include the toplevel headers gdk.h gdk-pixbuf.h])
fi
])
