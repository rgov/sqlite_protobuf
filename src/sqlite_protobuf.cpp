#include <regex>
#include <string>

#include <dlfcn.h>

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;


/// Convenience method for constructing a std::string from sqlite3_value
static const std::string string_from_sqlite3_value(sqlite3_value *value)
{
    return std::string(reinterpret_cast<const char*>(sqlite3_value_text(value)),
                        static_cast<size_t>(sqlite3_value_bytes(value)));
}


/// Loads a shared library that presumably contains message descriptors. Returns
/// NULL on success or throws an error on failure.
///
///     SELECT protobuf_load("example/libaddressbook.dylib");
///
static void protobuf_load(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv)
{
    const std::string path = string_from_sqlite3_value(argv[0]);
    void *handle = dlopen(path.c_str(), RTLD_GLOBAL);
    if (!handle) {
        sqlite3_result_error(context, "Could not load library", -1);
        return;
    }

    sqlite3_result_null(context);
}


/// Return the element (or elements) 
///
///     SELECT protobuf_extract(data, "Person", "$.phones[0].number");
///
/// @returns a Protobuf-encoded BLOB or the appropriate SQL datatype
static void protobuf_extract(sqlite3_context *context,
                             int argc,
                             sqlite3_value **argv)
{
    const std::string message_data = string_from_sqlite3_value(argv[0]);
    const std::string message_name = string_from_sqlite3_value(argv[1]);
    const std::string path = string_from_sqlite3_value(argv[2]);
    
    // Check that the path begins with $, representing the root of the tree
    if (path.length() == 0 || path[0] != '$') {
        sqlite3_result_error(context, "Invalid path", -1);
        return;
    }
    
    // Find the message type in the descriptor pool
    const Descriptor *descriptor =
        DescriptorPool::generated_pool()->FindMessageTypeByName(message_name);
    if (!descriptor) {
        sqlite3_result_error(context, "Could not find message descriptor", -1);
        return;
    }
    
    // Deserialize the message
    DynamicMessageFactory factory;
    std::unique_ptr<Message> root_message(
        factory.GetPrototype(descriptor)->New());
    if (!root_message->ParseFromString(message_data)) {
        sqlite3_result_error(context, "Failed to parse message", -1);
        return;
    }
    
    // Special case: just return the root object
    if (path == "$") {
        sqlite3_result_blob(context, message_data.c_str(),
            message_data.length(), SQLITE_TRANSIENT);
        return;
    }
    
    // Get the Reflection interface for the message
    const Reflection *reflection = root_message->GetReflection();
    
    // As we traverse the tree, this is the "current" message we are looking at.
    // We only want the overall message to be managed by std::unique_ptr,
    // and this variable will always point into the overall structure.
    const Message *message = root_message.get();
    
    // Parse the rest
    std::regex path_element_regex("^\\.([^\\.\\[]+)(?:\\[([0-9]+)\\])?");
    std::string::const_iterator it = ++ path.cbegin();  // skip $
    while (it != path.end()) {
        std::smatch m;
        if (!std::regex_search(it, path.cend(), m, path_element_regex)) {
            sqlite3_result_error(context, "Invalid path", -1);
            return;
        }
        
        // Advance the iterator to the start of the next path component
        it += m.length();
        
        const std::string field_name = m.str(1);
        const std::string field_index_str = m.str(2);
        int field_index;  // initialized later
        
        // Get the descriptor for this field by its name
        const FieldDescriptor *field =
            descriptor->FindFieldByName(field_name);
        if (!field) {
            sqlite3_result_error(context, "Invalid field name", -1);
            return;
        }

        // If the field is optional, and it is not provided, return the default
        if (field->is_optional() && !reflection->HasField(*message, field)) {
            switch(field->cpp_type()) {
            case FieldDescriptor::CppType::CPPTYPE_INT32:
                sqlite3_result_int(context, field->default_value_int32());
                return;
            case FieldDescriptor::CppType::CPPTYPE_INT64:
                sqlite3_result_int64(context, field->default_value_int64());
                return;
            case FieldDescriptor::CppType::CPPTYPE_UINT32:
                sqlite3_log(SQLITE_WARNING,
                    "Protobuf field \"%s\" is unsigned, but SQLite does not "
                    "support unsigned types");
                sqlite3_result_int(context, field->default_value_uint32());
                return;
            case FieldDescriptor::CppType::CPPTYPE_UINT64:
                sqlite3_log(SQLITE_WARNING,
                    "Protobuf field \"%s\" is unsigned, but SQLite does not "
                    "support unsigned types");
                sqlite3_result_int64(context, field->default_value_uint64());
                return;
            case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
                sqlite3_result_double(context, field->default_value_double());
                return;
            case FieldDescriptor::CppType::CPPTYPE_FLOAT:
                sqlite3_result_double(context, field->default_value_float());
                return;
            case FieldDescriptor::CppType::CPPTYPE_BOOL:
                sqlite3_result_int(context,
                    field->default_value_bool() ? 0 : 1);
                return;
            case FieldDescriptor::CppType::CPPTYPE_ENUM:
                sqlite3_result_int(context,
                    field->default_value_enum()->number());
                return;
            case FieldDescriptor::CppType::CPPTYPE_STRING:
                sqlite3_result_text(context,
                    field->default_value_string().c_str(),
                    field->default_value_string().length(),
                    SQLITE_TRANSIENT);
                return;
            case FieldDescriptor::CppType::CPPTYPE_MESSAGE:
                sqlite3_result_null(context);
                return;
            }
        }
        
        // If the field is repeated, validate the index into it
        if (field->is_repeated()) {
            if (field_index_str.empty()) {
                sqlite3_result_error(context,
                    "Expected index into repeated field", -1);
                return;
            }
            
            // Wrap around for negative indexing
            int field_size = reflection->FieldSize(*message, field);
            field_index = std::stoi(field_index_str);
            if (field_index < 0) {
                field_index = field_size + field_index;
            }
            
            // Check that it's within range
            if (field_index >= field_size) {
                // If we error here, that means the query will stop
                sqlite3_result_null(context);
                return;
            }
        }
        
        // If the field is a submessage, descend into it
        if (field->cpp_type() == FieldDescriptor::CppType::CPPTYPE_MESSAGE) {
            // Descend into this submessage
            message = field->is_repeated()
                ? &reflection->GetRepeatedMessage(*message, field, field_index)
                : &reflection->GetMessage(*message, field);
            descriptor = message->GetDescriptor();
            reflection = message->GetReflection();
            continue;
        }
        
        // Any other type should be the end of the path
        if (it != path.cend()) {
            sqlite3_result_error(context, "Path traverses non-message elements",
                -1);
            return;
        }
        
        // Translate the field type into a SQLite type and return it
        switch(field->cpp_type()) {
            case FieldDescriptor::CppType::CPPTYPE_INT32:
            {
                int32_t value = field->is_repeated()
                    ? reflection->GetRepeatedInt32(*message, field, field_index)
                    : reflection->GetInt32(*message, field);
                sqlite3_result_int(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_INT64:
            {
                int64_t value = field->is_repeated()
                    ? reflection->GetRepeatedInt64(*message, field, field_index)
                    : reflection->GetInt64(*message, field);
                sqlite3_result_int(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_UINT32:
            {
                sqlite3_log(SQLITE_WARNING,
                    "Protobuf field \"%s\" is unsigned, but SQLite does not "
                    "support unsigned types");
                uint32_t value = field->is_repeated()
                    ? reflection->GetRepeatedUInt32(*message, field, field_index)
                    : reflection->GetUInt32(*message, field);
                sqlite3_result_int(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_UINT64:
            {
                sqlite3_log(SQLITE_WARNING,
                    "Protobuf field \"%s\" is unsigned, but SQLite does not "
                    "support unsigned types");
                uint32_t value = field->is_repeated()
                    ? reflection->GetRepeatedUInt64(*message, field, field_index)
                    : reflection->GetUInt64(*message, field);
                sqlite3_result_int(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
            {
                double value = field->is_repeated()
                    ? reflection->GetRepeatedDouble(*message, field, field_index)
                    : reflection->GetDouble(*message, field);
                sqlite3_result_double(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_FLOAT:
            {
                float value = field->is_repeated()
                    ? reflection->GetRepeatedFloat(*message, field, field_index)
                    : reflection->GetFloat(*message, field);
                sqlite3_result_double(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_BOOL:
            {
                bool value = field->is_repeated()
                    ? reflection->GetRepeatedBool(*message, field, field_index)
                    : reflection->GetBool(*message, field);
                sqlite3_result_int(context, value ? 0 : 1);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_ENUM:
            {
                int value = field->is_repeated()
                    ? reflection->GetRepeatedEnumValue(*message, field, field_index)
                    : reflection->GetEnumValue(*message, field);
                sqlite3_result_int(context, value);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_STRING:
            {
                std::string value = field->is_repeated()
                    ? reflection->GetRepeatedString(*message, field, field_index)
                    : reflection->GetString(*message, field);
                sqlite3_result_text(context, value.c_str(), value.length(),
                    SQLITE_TRANSIENT);
                return;
            }
            case FieldDescriptor::CppType::CPPTYPE_MESSAGE:
                // Covered separately above, silence the warning
                break;
        }
    }
    
    // We made it to the end of the path. This means the user selected for a
    // message, which we should return the Protobuf-encoded message we landed on
    std::string serialized;
    if (!message->SerializeToString(&serialized)) {
        sqlite3_result_error(context, "Could not serialize message", -1);
        return;
    }
    sqlite3_result_blob(context, serialized.c_str(), serialized.length(),
        SQLITE_TRANSIENT);
}


extern "C"
int sqlite3_sqliteprotobuf_init(sqlite3 *db,
                               char **pzErrMsg,
                               const sqlite3_api_routines *pApi)
{
    int err;
    
    SQLITE_EXTENSION_INIT2(pApi);
    
    err = sqlite3_create_function(db, "protobuf_load", 1,
        SQLITE_UTF8, 0, protobuf_load, 0, 0);
    if (err != SQLITE_OK) return err;
    
    err = sqlite3_create_function(db, "protobuf_extract", 3,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, protobuf_extract, 0, 0);
    if (err != SQLITE_OK) return err;
    
    return SQLITE_OK;
}
