#ifndef _CONFIG_H
#define _CONFIG_H

#define PACKAGE_NAME "@CMAKE_PROJECT_NAME@"
#define PACKAGE_VERSION "@PROJECT_VERSION@"
#define PACKAGE_STRING PACKAGE_NAME " v" PACKAGE_VERSION

#define CASTIO_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"
#define CASTIO_DATA_DIR "@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_DATAROOTDIR@/@CMAKE_PROJECT_NAME@"


#endif /* _CONFIG_H */