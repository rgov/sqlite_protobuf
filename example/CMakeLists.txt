find_package(Protobuf REQUIRED)


# Create a shared library from AddressBook.proto
protobuf_generate_cpp(
    ADDRESSBOOK_PROTO_CPP_SRCS
    ADDRESSBOOK_PROTO_CPP_HDRS
    AddressBook.proto
)
add_library(addressbook SHARED
    ${ADDRESSBOOK_PROTO_CPP_SRCS}
    ${ADDRESSBOOK_PROTO_CPP_HDRS}
)
set_property(TARGET addressbook PROPERTY CXX_STANDARD 11)
target_include_directories(addressbook
    PUBLIC
    ${PROTOBUF_INCLUDE_DIRS}
)
target_link_libraries(addressbook
    PUBLIC
    ${PROTOBUF_LIBRARIES}
)


# Create a Python module from AddressBook.proto
protobuf_generate_python(
    ADDRESSBOOK_PROTO_PY_SRCS
    AddressBook.proto
)
add_custom_target(addressbook_py
    ALL 
    DEPENDS ${ADDRESSBOOK_PROTO_PY_SRCS}
)


# Copy some files into the build directory
foreach(FILE Pipfile create_database.py query_database.py)
    configure_file(
        "${FILE}"
        "${CMAKE_CURRENT_BINARY_DIR}/${FILE}"
        COPYONLY
    )
endforeach()
