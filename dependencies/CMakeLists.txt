find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(CPU_COUNT EQUAL 0)
    set(CPU_COUNT 1)
endif()

add_custom_target(lib64link
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/installroot/lib"
    COMMAND ln -sf lib "${CMAKE_CURRENT_BINARY_DIR}/installroot/lib64")

# older CMake is complaining when include did not exist at configure time
execute_process(COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/installroot/include/")

# ==================================================================================================
# libltdl (libtool) - dependency of GPhoto2

set(libltdl_LIBRARY
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libltdl.so
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libltdl.so.7
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libltdl.so.7.3.1)

ExternalProject_Add(libltdl
    URL "${CMAKE_CURRENT_SOURCE_DIR}/libtool-2.4.6.tar.gz"
    URL_HASH SHA256=e3bd4d5d3d025a36c21dd6af7ea818a2afcd4dfc1ea5a17b39d7854bcd0c06e3
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/installroot"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix <INSTALL_DIR>
    BUILD_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT}
    INSTALL_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT} install
    TEST_COMMAND ""
    BUILD_BYPRODUCTS ${libltdl_LIBRARY}
    )

# ==================================================================================================
# libusb

set(libusb_LIBRARY
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libusb-1.0.so
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libusb-1.0.so.0
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libusb-1.0.so.0.3.0)

ExternalProject_Add(libusb
    URL "${CMAKE_CURRENT_SOURCE_DIR}/libusb-1.0.26.tar.bz2"
    URL_HASH SHA256=12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/installroot"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix <INSTALL_DIR> --disable-udev
    BUILD_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT}
    INSTALL_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT} install
    TEST_COMMAND ""
    BUILD_BYPRODUCTS ${libusb_LIBRARY}
    )

# ==================================================================================================
# GPhoto2

set(GPhoto2_LIBRARY
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2_port.so
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2_port.so.12
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2_port.so.12.0.0
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2.so
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2.so.6
    ${CMAKE_CURRENT_BINARY_DIR}/installroot/lib/libgphoto2.so.6.2.0)

ExternalProject_Add(GPhoto2
    URL "${CMAKE_CURRENT_SOURCE_DIR}/libgphoto2-2.5.29.tar.gz"
    URL_HASH SHA256=467eaf95e5385e48786ee115802e24ece00f34ae88ac02f5dfa7c836846c6294
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/installroot"
    CONFIGURE_COMMAND cmake -E env
        "PKG_CONFIG_PATH=<INSTALL_DIR>/lib/pkgconfig"
        DEFAULT_CAMLIBS=/usr/share/harbour-camera-buddy/lib/libgphoto2/2.5.29
        DEFAULT_IOLIBS=/usr/share/harbour-camera-buddy/lib/libgphoto2_port/0.12.0
        "CFLAGS=-I<INSTALL_DIR>/include"
        "CPPFLAGS=-I<INSTALL_DIR>/include"
        "LDFLAGS=-L<INSTALL_DIR>/lib -Wl,-rpath=/usr/share/harbour-camera-buddy/lib"
        <SOURCE_DIR>/configure --prefix <INSTALL_DIR>
    BUILD_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT}
    INSTALL_COMMAND ${MAKE_EXECUTABLE} -j${CPU_COUNT} install
    TEST_COMMAND ""
    BUILD_BYPRODUCTS ${GPhoto2_LIBRARY}
    DEPENDS libltdl libusb
    )

add_library(GPhoto2::GPhoto2 INTERFACE IMPORTED GLOBAL)
target_include_directories(GPhoto2::GPhoto2 INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/installroot/include)
target_link_libraries(GPhoto2::GPhoto2 INTERFACE "${GPhoto2_LIBRARY}")  # need the quotes to expand list
add_dependencies(GPhoto2::GPhoto2 GPhoto2)

# ==================================================================================================
# Export libraries for main CMakeLists.txt

set(DEP_LIBS
    ${GPhoto2_LIBRARY}
    ${libltdl_LIBRARY}
    ${libusb_LIBRARY}
    PARENT_SCOPE)
