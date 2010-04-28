if (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBRARY)
    set(LibArchive_FIND_QUIETLY TRUE)
endif(LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBRARY)

find_path(LIBARCHIVE_INCLUDE_DIR archive.h)

find_library(LIBARCHIVE_LIBRARY NAMES archive libarchive
    PATHS
    /usr/lib
    /usr/local/lib
)

find_package_handle_standard_args(LibArchive DEFAULT_MSG LIBARCHIVE_INCLUDE_DIR LIBARCHIVE_LIBRARY)

mark_as_advanced(LIBARCHIVE_INCLUDE_DIR LIBARCHIVE_LIBRARY)
