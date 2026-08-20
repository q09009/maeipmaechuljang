cmake_minimum_required(VERSION 3.16)

project(maeipmaechuljang VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Concurrent)
find_package(Qt6 REQUIRED COMPONENTS Sql)
find_package(Qt6 REQUIRED COMPONENTS Network)

include(CTest)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appmaeipmaechuljang
    main.cpp
)

add_subdirectory(QXlsx)

qt_add_qml_module(appmaeipmaechuljang
    URI maeipmaechuljang
    QML_FILES
        Main.qml
        QML_FILES qml/MyCalendar.qml
        SOURCES excelDatahandler.h
        SOURCES excelDatahandler.cpp
        QML_FILES qml/MonthTotal.qml
        SOURCES sqlDatahandler.h
        SOURCES sqlDatahandler.cpp
        SOURCES isqlstorage.h
        SOURCES sqlitestorage.h
        SOURCES sqlitestorage.cpp
        SOURCES apistorage.h
        SOURCES apistorage.cpp
        SOURCES datatransfer.h
        SOURCES datatransfer.cpp
        SOURCES transfersnapshot.h
        SOURCES transfersnapshot.cpp
        SOURCES syncManager.h
        SOURCES syncManager.cpp
        SOURCES
        QML_FILES
        QML_FILES qml/DocumentExportView.qml
        SOURCES updaterHelper.h
        QML_FILES qml/Configuration.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appmaeipmaechuljang PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appmaeipmaechuljang
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appmaeipmaechuljang
    PRIVATE
        Qt6::Quick
        QXlsx::QXlsx
        Qt6::Concurrent
        Qt6::Sql
        Qt6::Network
)

include(GNUInstallDirs)
install(TARGETS appmaeipmaechuljang
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(data_transfer_tests
        tests/data_transfer_tests.cpp
        isqlstorage.h
        sqlitestorage.h
        sqlitestorage.cpp
        transfersnapshot.h
        transfersnapshot.cpp
    )
    target_include_directories(data_transfer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(data_transfer_tests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
    add_test(NAME data_transfer_tests COMMAND data_transfer_tests)
    if(WIN32)
        get_target_property(_qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
        get_filename_component(_compiler_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set_tests_properties(data_transfer_tests PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${_compiler_bin_dir};PATH=path_list_prepend:${_qt_bin_dir}")
    endif()
endif()
