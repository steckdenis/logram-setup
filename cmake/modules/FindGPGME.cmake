if (GPGME_INCLUDE_DIR AND GPGME_LIBRARY)
    set(GPGME_FIND_QUIETLY TRUE)
endif(GPGME_INCLUDE_DIR AND GPGME_LIBRARY)

find_path(GPGME_INCLUDE_DIR gpgme.h)

find_library(GPGME_LIBRARY NAMES gpgme libgpgme
    PATHS
    /usr/lib
    /usr/local/lib
)

find_package_handle_standard_args(GPGME DEFAULT_MSG GPGME_INCLUDE_DIR GPGME_LIBRARY)

mark_as_advanced(GPGME_INCLUDE_DIR GPGME_LIBRARY)
