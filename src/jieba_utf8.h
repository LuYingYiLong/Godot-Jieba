#ifndef JIEBA_UTF8_H
#define JIEBA_UTF8_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {
	class JiebaUTF8 : public Resource {
		GDCLASS(JiebaUTF8, Resource)

	private:
		String source_path;
		String source_encoding;
		String utf8_text;
		PackedByteArray source_data;

	protected:
		static void _bind_methods();

	public:
		JiebaUTF8();
		~JiebaUTF8();

		void set_source_path(const String& p_source_path);
		String get_source_path() const;

		void set_source_encoding(const String& p_source_encoding);
		String get_source_encoding() const;

		void set_utf8_text(const String& p_utf8_text);
		String get_utf8_text() const;

		void set_source_data(const PackedByteArray& p_source_data);
		PackedByteArray get_source_data() const;

		PackedByteArray get_utf8_data() const;
	};
}

#endif // !JIEBA_UTF8_H
