cmake_minimum_required(VERSION 3.10)
project(tank)

set(CMAKE_CXX_STANDARD 11)

if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    include_directories(/usr/local/opt/openssl/include)

    set(CMAKE_LIBRARY_PATH /usr/local/lib)
    include_directories(/usr/local/include)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_INC_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_LIB_DIR "${OPENSSL_ROOT_DIR}/lib")
endif(APPLE)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "gcc")
set(CMAKE_CXX_FLAGS "-std=c++11 -O0")
set(CMAKE_EXE_LINKER_FLAGS "-L${OPENSSL_LIB_DIR}")

include_directories(
        ${OPENSSL_INC_DIR}
)

find_package(Boost COMPONENTS regex system filesystem REQUIRED)
find_package(OpenSSL)
if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
endif()


#add_executable(tank main.cpp)
add_executable(tank test.cpp)

target_link_libraries(tank
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${OpenSSL_LIBRARY}
        ssl
        crypto
        stdc++
        )

