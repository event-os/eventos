cmake_minimum_required(VERSION 3.0)

project(Meow)

set(CMAKE_C_STANDARD 99)

include_directories(
    inc/core
    inc/meow
    inc/toolkit
    test
    meowc
    config
    ./
)

link_directories(
    ${LIB_PATH}
)

#file(GLOB_RECURSE MODULES_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp )
#file(GLOB_RECURSE MODULES_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp )
#file(GLOB_RECURSE PMG_C_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/meow/*.c )
#file(GLOB_RECURSE EXAMPLE_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/toolkit/*.cpp )

add_executable(${PROJECT_NAME}
        main_unittest.c
        meowc/meow.c
        meowc/port_unittest.c
        meowc/unittest_meow.c
        meowc/unittest_betree.c
        mdebug/m_debug.c
        mdebug/port_mdbg.c
    )

target_link_libraries(${PROJECT_NAME}
        pthread
    )
    