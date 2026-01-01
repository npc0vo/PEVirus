# CMakeList.txt: PEVirus 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)
project (PEVirus)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# ==========================================
# 强制使用静态运行时库（与 LIEF 保持一致）
# ==========================================
if(MSVC)
    # 替换默认的 /MD 为 /MT（Release）
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    
    # 替换默认的 /MDd 为 /MTd（Debug）
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    
    # 移除默认的 /MD 和 /MDd
    string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Vcpkg target triplet")
FILE(GLOB CLIENT_SRC "client/*.cpp")
FILE(GLOB UTILS_SRC "Utils/*.cpp")


#enable LIEF
find_package(LIEF REQUIRED CONFIG)

# 将源代码添加到此项目的可执行文件。
add_executable (PEVirus "PEVirus.cpp" ${UTILS_SRC} ${CLIENT_SRC})
add_executable (Victim "Victim.cpp")
add_library(hacker SHARED hacker/hacker.cpp  ${CLIENT_SRC})
add_executable(injector hacker/injector.cpp)

target_include_directories(hacker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hacker ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PEVirus  LIEF::LIEF)
target_link_libraries(hacker PRIVATE ws2_32 gdiplus)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PEVirus PROPERTY CXX_STANDARD 20)
  set_property(TARGET Victim PROPERTY CXX_STANDARD 20)
  set_property(TARGET hacker PROPERTY CXX_STANDARD 20)
  set_property(TARGET injector PROPERTY CXX_STANDARD 20)
endif()

