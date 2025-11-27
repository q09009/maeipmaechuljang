cmake_minimum_required(VERSION 3.16)

project(QXlsx
    VERSION 1.5.0
    LANGUAGES CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

# ✅ [1] Qt 경로 지정 - Qt6 못 찾을 때 대비
if(NOT DEFINED CMAKE_PREFIX_PATH)
    # 필요시 Qt 설치 경로를 직접 넣어줍니다.
    set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/mingw_64/lib/cmake" CACHE STRING "" FORCE)
endif()

# ✅ [2] Qt 탐색
if(NOT DEFINED QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui REQUIRED)
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui REQUIRED)

# ✅ [3] Qt6.10 이상인 경우에만 GuiPrivate 추가 (6.9.2에서는 optional)
if(Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

set(EXPORT_NAME QXlsxQt${QT_VERSION_MAJOR})

# ✅ [4] C++ 표준 설정
if (QT_VERSION_MAJOR EQUAL 6)
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
else()
    set(CMAKE_CXX_STANDARD 11 CACHE STRING "")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ [5] 내부 경로 지정
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

if(NOT DEFINED QXLSX_PARENTPATH)
    set(QXLSX_PARENTPATH ${CMAKE_CURRENT_SOURCE_DIR}/../)
endif()

if(NOT DEFINED QXLSX_HEADERPATH)
    set(QXLSX_HEADERPATH ${CMAKE_CURRENT_SOURCE_DIR}/header)
endif()

if(NOT DEFINED QXLSX_SOURCEPATH)
    set(QXLSX_SOURCEPATH ${CMAKE_CURRENT_SOURCE_DIR}/source)
endif()

# ✅ [6] 빌드 설정
option(BUILD_SHARED_LIBS "Build in shared lib mode" OFF)

# ✅ [7] 소스 및 헤더 정의
file(GLOB SRC_FILES ${QXLSX_SOURCEPATH}/*.cpp)
file(GLOB QXLSX_PUBLIC_HEADERS ${QXLSX_HEADERPATH}/*.h)

add_library(QXlsx ${SRC_FILES} ${QXLSX_PUBLIC_HEADERS})
add_library(QXlsx::QXlsx ALIAS QXlsx)

# ✅ [8] 컴파일 옵션
target_compile_definitions(QXlsx PRIVATE
    QT_NO_KEYWORDS
    QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_NO_CAST_FROM_BYTEARRAY
    QT_USE_QSTRINGBUILDER
    QT_NO_SIGNALS_SLOTS_KEYWORDS
    QT_USE_FAST_OPERATOR_PLUS
    QT_DISABLE_DEPRECATED_BEFORE=0x060600
)

if (NOT WIN32)
    target_compile_definitions(QXlsx PRIVATE QT_STRICT_ITERATORS)
endif()

target_compile_features(QXlsx INTERFACE cxx_std_11)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(QXlsx PUBLIC QXlsx_SHAREDLIB)
endif()

# ✅ [9] 링크 설정
target_link_libraries(QXlsx
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

# ✅ [10] 헤더 포함 경로
target_include_directories(QXlsx
    PRIVATE
        ${QXLSX_HEADERPATH}
	${Qt6Core_PRIVATE_INCLUDE_DIRS}
	${Qt6Gui_PRIVATE_INCLUDE_DIRS}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/header>
        $<INSTALL_INTERFACE:include/QXlsxQt${QT_VERSION_MAJOR}>
)

# ✅ [11] 출력 및 설치 설정
set_target_properties(QXlsx PROPERTIES
    OUTPUT_NAME ${EXPORT_NAME}
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${QXLSX_PUBLIC_HEADERS}"
)

install(TARGETS QXlsx
    EXPORT ${EXPORT_NAME}Targets DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QXlsxQt${QT_VERSION_MAJOR} COMPONENT devel
)

install(EXPORT ${EXPORT_NAME}Targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${EXPORT_NAME}/
    FILE ${EXPORT_NAME}Targets.cmake
    NAMESPACE QXlsx::
    COMPONENT devel
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/qxlsx-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}Config.cmake
    @ONLY
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/qxlsx-config-version.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}ConfigVersion.cmake
    @ONLY
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${EXPORT_NAME}/
)
 # include(CPackConfig)

