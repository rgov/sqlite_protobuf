find_package(Protobuf REQUIRED)
find_package(SQLite3 REQUIRED)


add_library(sqlite_protobuf SHARED
    extension_main.cpp
    protobuf_enum.cpp
    protobuf_extract.cpp
    protobuf_load.cpp
    utilities.cpp
)
set_property(TARGET sqlite_protobuf PROPERTY CXX_STANDARD 11)
target_include_directories(sqlite_protobuf
    PUBLIC
    ${PROTOBUF_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)
target_link_libraries(sqlite_protobuf
    PUBLIC
    dl
    ${PROTOBUF_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)
