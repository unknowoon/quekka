cmake_minimum_required(VERSION 3.15)
project(quekka VERSION 1.0 LANGUAGES C CXX)

# C/C++ 표준 설정
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 빌드 타입 설정 (기본값: Debug)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 컴파일 옵션
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# pthread 라이브러리 찾기
find_package(Threads REQUIRED)

# =============================================================================
# 공통 Include 경로
# =============================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)

# =============================================================================
# Common Library (공통 라이브러리 - 로깅)
# =============================================================================
add_library(quekka_common STATIC
    libsrc/quekka/Quekka_log.c
)

target_include_directories(quekka_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(quekka_common PUBLIC
    Threads::Threads
)

# =============================================================================
# Quekka Config Library (설정 라이브러리)
# =============================================================================
add_library(quekka_config STATIC
        libsrc/quekka/Quekka_config.c
)

target_include_directories(quekka_config PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(quekka_config PUBLIC
    quekka_common
)

# =============================================================================
# Quekka Error Code Library (에러 코드 라이브러리)
# =============================================================================
add_library(quekka_error STATIC
    libsrc/quekka/Quekka_quecode.c
)

target_include_directories(quekka_error PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libsrc/quekka
)

# =============================================================================
# Quekka Publisher Library (퍼블리셔 클라이언트 라이브러리)
# =============================================================================
add_library(quekka_pub STATIC
    libsrc/quekka/Quekka_producer.c
    libsrc/internal/libsocket.c
)

target_include_directories(quekka_pub PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libsrc/quekka
)

target_link_libraries(quekka_pub PUBLIC
    quekka_common
    quekka_config
    quekka_error
)

# =============================================================================
# Quekka Subscriber Library (서브스크라이버 클라이언트 라이브러리)
# =============================================================================
add_library(quekka_sub STATIC
    libsrc/quekka/Quekka_sub.c
)

target_include_directories(quekka_sub PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(quekka_sub PUBLIC
    quekka_common
    quekka_config
)

# =============================================================================
# Server Library (서버 라이브러리 - Linux 전용, epoll 사용)
# =============================================================================
if(UNIX AND NOT APPLE)
    add_library(quekka_server_lib STATIC
        libsrc/internal/libsocket.c
    )

    target_include_directories(quekka_server_lib PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(quekka_server_lib PUBLIC
        quekka_common
    )
endif()

# =============================================================================
# Main Executable (quekka 서버 - Linux 전용, epoll 사용)
# =============================================================================
if(UNIX AND NOT APPLE)
    add_executable(quekka
        src/server/quekka.cpp
        libsrc/internal/libfile.c
        libsrc/internal/libsocket.c
        include/internal/socket_server.h
        include/internal/libsocket.h
        include/internal/epoll_handler.h
        libsrc/internal/libsocket.c
        include/quekka/Quekka_quecode.h
        libsrc/quekka/Quekka_quecode.c
        libsrc/quekka/Quekka_config.c
    )

    target_link_libraries(quekka PRIVATE
        quekka_common
        quekka_config
        quekka_pub
        quekka_sub
    )

    # =============================================================================
    # CLI Executable (quekka-cli)
    # =============================================================================
    add_executable(quekka-cli
        src/cli/quekka-cli.cpp
        libsrc/internal/libsocket.c
    )

    target_link_libraries(quekka-cli PRIVATE
        quekka_common
    )
endif()

# =============================================================================
# 설치 규칙
# =============================================================================
install(TARGETS quekka_common quekka_config quekka_error quekka_pub quekka_sub
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(UNIX AND NOT APPLE)
    install(TARGETS quekka quekka-cli
        RUNTIME DESTINATION bin
    )

    install(TARGETS quekka_server_lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

install(DIRECTORY include/quekka DESTINATION include)

# =============================================================================
# Unit Tests
# =============================================================================
add_subdirectory(src/unittest)

# =============================================================================
# 빌드 정보 출력
# =============================================================================
message(STATUS "")
message(STATUS "====================================")
message(STATUS "  Quekka Build Configuration")
message(STATUS "====================================")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler:        ${CMAKE_C_COMPILER}")
message(STATUS "  CXX compiler:      ${CMAKE_CXX_COMPILER}")
message(STATUS "  Output directory:  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(UNIX AND NOT APPLE)
    message(STATUS "  Server build:      Enabled (Linux)")
else()
    message(STATUS "  Server build:      Disabled (non-Linux)")
endif()
message(STATUS "====================================")
message(STATUS "")
