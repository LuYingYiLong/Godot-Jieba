#include "jieba_utf8.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

void JiebaUTF8::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_source_path", "source_path"), &JiebaUTF8::set_source_path);
	ClassDB::bind_method(D_METHOD("get_source_path"), &JiebaUTF8::get_source_path);
	ClassDB::bind_method(D_METHOD("set_source_encoding", "source_encoding"), &JiebaUTF8::set_source_encoding);
	ClassDB::bind_method(D_METHOD("get_source_encoding"), &JiebaUTF8::get_source_encoding);
	ClassDB::bind_method(D_METHOD("set_utf8_text", "utf8_text"), &JiebaUTF8::set_utf8_text);
	ClassDB::bind_method(D_METHOD("get_utf8_text"), &JiebaUTF8::get_utf8_text);
	ClassDB::bind_method(D_METHOD("set_source_data", "source_data"), &JiebaUTF8::set_source_data);
	ClassDB::bind_method(D_METHOD("get_source_data"), &JiebaUTF8::get_source_data);
	ClassDB::bind_method(D_METHOD("get_utf8_data"), &JiebaUTF8::get_utf8_data);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_source_path", "get_source_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_encoding", PROPERTY_HINT_ENUM, "UTF-8,GBK", PROPERTY_USAGE_DEFAULT), "set_source_encoding", "get_source_encoding");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "utf8_text", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_utf8_text", "get_utf8_text");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "source_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_source_data", "get_source_data");
}

JiebaUTF8::JiebaUTF8() = default;

JiebaUTF8::~JiebaUTF8() = default;

void JiebaUTF8::set_source_path(const String& p_source_path) {
	source_path = p_source_path;
}

String JiebaUTF8::get_source_path() const {
	return source_path;
}

void JiebaUTF8::set_source_encoding(const String& p_source_encoding) {
	source_encoding = p_source_encoding;
}

String JiebaUTF8::get_source_encoding() const {
	return source_encoding;
}

void JiebaUTF8::set_utf8_text(const String& p_utf8_text) {
	utf8_text = p_utf8_text;
}

String JiebaUTF8::get_utf8_text() const {
	return utf8_text;
}

void JiebaUTF8::set_source_data(const PackedByteArray& p_source_data) {
	source_data = p_source_data;
}

PackedByteArray JiebaUTF8::get_source_data() const {
	return source_data;
}

PackedByteArray JiebaUTF8::get_utf8_data() const {
	return utf8_text.to_utf8_buffer();
}

} // namespace godot
