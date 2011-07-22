# Try to configure the Win32 client (gui-win32)

# WC_WIN32_CLIENT
# Test for Win32 and needed libraries for gui-win32

AC_DEFUN([WC_WIN32_CLIENT],
[
  if test "$client" = "win32" || test "$client" = "yes" ; then
    if test "$MINGW32" = "yes"; then
      AC_CHECK_LIB([z], [gzgets],
        [
          AC_CHECK_HEADER([zlib.h],
            [
              AC_CHECK_LIB([png], [png_read_image],
                [
                  AC_CHECK_HEADER([png.h],
                    [
                      found_client=yes
                      client=win32
                      CLIENT_LIBS="-lws2_32 -lcomctl32  -lpng -lz -mwindows"
                    ],
                    [
                      WC_NO_CLIENT([win32], [libpng-dev is needed])
                    ])
                ],
                [
                  WC_NO_CLIENT([win32], [libpng is needed])
                ], [-lz])
            ],
            [
              WC_NO_CLIENT([win32], [zlib-dev is needed])
            ])
        ],
        [
          WC_NO_CLIENT([win32], [zlib is needed])
        ])
    else
      WC_NO_CLIENT([win32], [mingw32 is needed])
    fi
  fi
])
