# CMakeList.txt: NetFrame 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.20)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(NetFrame LANGUAGES CXX)

find_package(Protobuf REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS asio)

set(PROTO_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto/generated")
file(MAKE_DIRECTORY "${PROTO_GENERATED_DIR}")

# 将源代码添加到此项目的可执行文件。
add_executable(NetFrame
    "Main.cpp"
    "Server.cpp"
    "src/message/MessageManager.cpp"
    "src/session/Session.cpp"
    "src/session/SessionManager.cpp"
    "src/handle/HandleManager.cpp"
    "src/tool/Log.cpp"
    "src/handle/HeartHandle.cpp"
    "src/tool/Time.cpp"
    "src/net/IoWorker.cpp"
    "src/net/IoWorkerManager.cpp"
    "proto/message/ping.proto"
    "proto/message/pong.proto"
)

protobuf_generate(
    TARGET NetFrame
    LANGUAGE cpp
    APPEND_PATH
    PROTOC_OUT_DIR "${PROTO_GENERATED_DIR}"
)

target_include_directories(NetFrame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_features(NetFrame PRIVATE cxx_std_20)
set_target_properties(NetFrame PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if (WIN32)
    target_compile_definitions(NetFrame PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(NetFrame
    PRIVATE
    Boost::asio
    protobuf::libprotobuf
)
