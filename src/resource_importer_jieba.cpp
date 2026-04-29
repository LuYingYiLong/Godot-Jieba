#include "resource_importer_jieba.h"

#include "jieba_utf8.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	namespace {
		const char* const ENCODING_AUTO = "Auto";
		const char* const ENCODING_UTF8 = "UTF-8";
		const char* const ENCODING_GBK = "GBK";

		String detect_encoding(const String& p_source_file, const Dictionary& p_options) {
			String encoding = p_options.get("encoding", ENCODING_AUTO);
			if (encoding != ENCODING_AUTO) {
				return encoding;
			}

			String extension = p_source_file.get_extension().to_lower();
			if (extension == "gbk") {
				return ENCODING_GBK;
			}
			return ENCODING_UTF8;
		}

		String decode_text(const PackedByteArray& p_data, const String& p_encoding) {
			if (p_encoding == ENCODING_GBK) {
				return p_data.get_string_from_multibyte_char("GBK");
			}
			return p_data.get_string_from_utf8();
		}

	} // namespace

	void ResourceImporterJieba::_bind_methods() {}

	ResourceImporterJieba::ResourceImporterJieba() {}

	ResourceImporterJieba::~ResourceImporterJieba() {}

	String ResourceImporterJieba::_get_importer_name() const {
		return "godot_jieba.dictionary_text";
	}

	String ResourceImporterJieba::_get_visible_name() const {
		return "Jieba Dictionary Text";
	}

	PackedStringArray ResourceImporterJieba::_get_recognized_extensions() const {
		PackedStringArray extensions;
		extensions.append("utf8");
		extensions.append("gbk");
		return extensions;
	}

	String ResourceImporterJieba::_get_save_extension() const {
		return "res";
	}

	String ResourceImporterJieba::_get_resource_type() const {
		return "JiebaUTF8";
	}

	float ResourceImporterJieba::_get_priority() const {
		return 1.0;
	}

	int32_t ResourceImporterJieba::_get_import_order() const {
		return IMPORT_ORDER_DEFAULT;
	}

	int32_t ResourceImporterJieba::_get_format_version() const {
		return 1;
	}

	bool ResourceImporterJieba::_can_import_threaded() const {
		return true;
	}

	int32_t ResourceImporterJieba::_get_preset_count() const {
		return 1;
	}

	String ResourceImporterJieba::_get_preset_name(int32_t p_preset_index) const {
		return p_preset_index == 0 ? "Default" : String();
	}

	TypedArray<Dictionary> ResourceImporterJieba::_get_import_options(const String& p_path, int32_t p_preset_index) const {
		TypedArray<Dictionary> options;

		Dictionary encoding_option;
		encoding_option["name"] = "encoding";
		encoding_option["default_value"] = ENCODING_AUTO;
		encoding_option["property_hint"] = PROPERTY_HINT_ENUM;
		encoding_option["hint_string"] = "Auto,UTF-8,GBK";
		options.append(encoding_option);

		Dictionary source_data_option;
		source_data_option["name"] = "store_source_data";
		source_data_option["default_value"] = false;
		options.append(source_data_option);

		return options;
	}

	bool ResourceImporterJieba::_get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const {
		return true;
	}

	Error ResourceImporterJieba::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const {
		PackedByteArray source_data = FileAccess::get_file_as_bytes(p_source_file);
		if (source_data.is_empty()) {
			ERR_PRINT(String("Jieba dictionary source is empty or unreadable: ") + p_source_file);
			return ERR_CANT_OPEN;
		}

		String encoding = detect_encoding(p_source_file, p_options);
		String utf8_text = decode_text(source_data, encoding);
		if (utf8_text.is_empty() && !source_data.is_empty()) {
			ERR_PRINT(String("Failed to decode Jieba dictionary as ") + encoding + ": " + p_source_file);
			return ERR_PARSE_ERROR;
		}

		Ref<JiebaUTF8> resource;
		resource.instantiate();
		resource->set_source_path(p_source_file);
		resource->set_source_encoding(encoding);
		resource->set_utf8_text(utf8_text);
		if (static_cast<bool>(p_options.get("store_source_data", false))) {
			resource->set_source_data(source_data);
		}

		String save_path = p_save_path + String(".") + _get_save_extension();
		Error err = ResourceSaver::get_singleton()->save(resource, save_path);
		if (err != OK) {
			ERR_PRINT(String("Failed to save imported Jieba dictionary: ") + save_path + ", error: " + String::num_int64(err));
			return err;
		}

		return OK;
	}

} // namespace godot
