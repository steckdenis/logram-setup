if (ELF_INCLUDE_DIR)
    set(ELF_FIND_QUIETLY TRUE)
endif(ELF_INCLUDE_DIR)

find_path(ELF_INCLUDE_DIR gelf.h)

find_library(ELF_LIBRARY NAMES elf libelf
    PATHS
    /lib
    /usr/lib
    /usr/local/lib
)

find_package_handle_standard_args(ELF DEFAULT_MSG ELF_INCLUDE_DIR ELF_LIBRARY)

mark_as_advanced(ELF_INCLUDE_DIR)
