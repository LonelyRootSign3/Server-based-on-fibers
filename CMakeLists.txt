cmake_minimum_required(VERSION 3.14)
project(HardProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-fno-omit-frame-pointer)
add_link_options(-rdynamic)


# 1) 找到依赖库
find_package(yaml-cpp REQUIRED)   # 找到 yaml-cpp
find_package(Threads REQUIRED)   # 找到 pthread
# 2) 收集源码并生成库
file(GLOB SRC_FILES
  ${CMAKE_SOURCE_DIR}/src/*.cc
  # ${CMAKE_SOURCE_DIR}/src/Util/*.cc
)

add_library(hardproject ${SRC_FILES})
target_include_directories(hardproject
  PUBLIC ${CMAKE_SOURCE_DIR}/src      # 你的项目头文件
)
# 3) 把 yaml-cpp 作为 hardproject 的 PUBLIC 依赖（可执行程序会自动继承）
target_link_libraries(hardproject PUBLIC 
          yaml-cpp 
          Threads::Threads
          dl) #使用 Threads::Threads 是 CMake 官方推荐 的跨平台写法

# 4) 测试可执行文件

# add_executable(config_test ${CMAKE_SOURCE_DIR}/tests/test_Config.cc)
# target_link_libraries(config_test PRIVATE hardproject)

# add_executable(thread_test ${CMAKE_SOURCE_DIR}/tests/test_Thread.cc)
# target_link_libraries(thread_test PRIVATE hardproject)

# add_executable(util_test ${CMAKE_SOURCE_DIR}/tests/test_Util.cc)
# target_link_libraries(util_test PRIVATE hardproject)

# add_executable(fiber_test ${CMAKE_SOURCE_DIR}/tests/test_Fiber1.cc)
# target_link_libraries(fiber_test PRIVATE hardproject)

# add_executable(ucontext_test ${CMAKE_SOURCE_DIR}/tests/test_ucontext.cc)
# target_link_libraries(ucontext_test PRIVATE hardproject)

# add_executable(schedule_test ${CMAKE_SOURCE_DIR}/tests/test_Schedule1.cc)
# target_link_libraries(schedule_test PRIVATE hardproject)

# add_executable(iomanager_test ${CMAKE_SOURCE_DIR}/tests/test_IOManager.cc)
# target_link_libraries(iomanager_test PRIVATE hardproject)

add_executable(hook_test ${CMAKE_SOURCE_DIR}/tests/test_Hook.cc)
target_link_libraries(hook_test PRIVATE hardproject)
