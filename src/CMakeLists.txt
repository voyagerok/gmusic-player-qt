
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Widgets Network Sql Multimedia REQUIRED)
find_package(OpenSSL 1.0.2 REQUIRED)

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(crypto RSA_set0_key "openssl/rsa.h" LCRYPTO_HAS_RSA_SET0_KEY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(RESOURCES
    main.qrc)

set(UI
    mainwindow.ui
    loginwidget.ui
    syncwidget.ui
    librarywidget.ui
    playertoolbar.ui)

set(SRC
    main.cpp
    utils.cpp
    utils.h
    gmapi.cpp
    gmapi.h
    model.cpp
    model.h
    user.cpp
    user.h
    database.cpp
    database.h
    proxyresult.cpp
    proxyresult.h
    tracklistmodel.cpp
    tracklistmodel.h
    mainwindow.cpp
    mainwindow.h
    loginwidget.cpp
    loginwidget.h
    settingsmodel.cpp
    settingsmodel.h
    syncwidget.cpp
    syncwidget.h
    librarywidget.cpp
    librarywidget.h
    librarymodel.cpp
    librarymodel.h
    imagestorage.cpp
    imagestorage.h
    playertoolbar.cpp
    playertoolbar.h)

add_executable(gmusic-player ${SRC} ${RESOURCES} ${UI})

target_include_directories(
    gmusic-player
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OPENSSL_INCLUDE_DIRS})

target_link_libraries(
    gmusic-player
    Qt5::Core
    Qt5::Network
    Qt5::Widgets
    Qt5::Sql
    Qt5::Multimedia
    ${OPENSSL_LIBRARIES}
    )
