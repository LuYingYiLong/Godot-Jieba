#include "jieba_segment.h"

#include "jieba_utf8.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include <cstdint>
#include <cstring>
#include <string>

#include "thirdparty/cppjieba/include/cppjieba/Jieba.hpp"

namespace godot {
	namespace {
bool assign_utf8_text(String p_text, std::string& r_content) {
	CharString utf8 = p_text.utf8();
	r_content.assign(utf8.get_data(), static_cast<size_t>(utf8.length()));
	return !r_content.empty();
}

bool assign_source_bytes(const String& p_path, const PackedByteArray& p_data, std::string& r_content) {
	if (p_path.get_extension().to_lower() == "gbk") {
		return assign_utf8_text(p_data.get_string_from_multibyte_char("GBK"), r_content);
	}

	r_content.clear();
	r_content.resize(static_cast<size_t>(p_data.size()));
	if (p_data.size() > 0) {
		memcpy(&r_content[0], p_data.ptr(), static_cast<size_t>(p_data.size()));
	}
	return true;
}

bool read_file_to_string(const String& p_path, std::string& r_content) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_valid() && file->is_open()) {
		uint64_t length = file->get_length();
		if (length == 0) {
			return true;
		}

		PackedByteArray source_data = file->get_buffer(static_cast<int64_t>(length));
		if (source_data.size() != static_cast<int64_t>(length)) {
			ERR_PRINT(String("Jieba: Could not read complete dictionary file: ") + p_path);
			r_content.clear();
			return false;
		}

		return assign_source_bytes(p_path, source_data, r_content);
	}

	ResourceLoader* loader = ResourceLoader::get_singleton();
	if (loader == nullptr) {
		ERR_PRINT(String("Jieba: Could not open dictionary file: ") + p_path);
		return false;
	}

	Ref<Resource> loaded = loader->load(p_path, "JiebaUTF8", ResourceLoader::CACHE_MODE_IGNORE);
	Ref<JiebaUTF8> imported = loaded;
	if (imported.is_null()) {
		ERR_PRINT(String("Jieba: Could not open dictionary file or imported resource: ") + p_path);
		return false;
	}

	if (!assign_utf8_text(imported->get_utf8_text(), r_content)) {
		ERR_PRINT(String("Jieba: Imported dictionary resource is empty: ") + p_path);
		return false;
	}

	return true;
}

bool read_existing_dict_memory_data(const String& p_path, cppjieba::MemoryData& r_data) {
	if (!FileAccess::file_exists(p_path)) {
		ResourceLoader* loader = ResourceLoader::get_singleton();
		if (loader == nullptr || !loader->exists(p_path, "JiebaUTF8")) {
			return false;
		}
	}

	std::string content;
	if (!read_file_to_string(p_path, content)) {
		return false;
	}

	r_data = cppjieba::MemoryData(content, p_path.utf8().get_data());
	return true;
}

bool read_dict_memory_data(const String& p_dict_dir, const char* p_utf8_file_name, const char* p_gbk_file_name, cppjieba::MemoryData& r_data) {
	String utf8_path = p_dict_dir.path_join(p_utf8_file_name);
	if (read_existing_dict_memory_data(utf8_path, r_data)) {
		return true;
	}

	String gbk_path = p_dict_dir.path_join(p_gbk_file_name);
	if (read_existing_dict_memory_data(gbk_path, r_data)) {
		return true;
	}

	ERR_PRINT(String("Jieba: Could not find dictionary file: ") + utf8_path + " or " + gbk_path);
	return false;
}

bool dict_source_exists(const String& p_path) {
	if (FileAccess::file_exists(p_path)) {
		return true;
	}

	ResourceLoader* loader = ResourceLoader::get_singleton();
	return loader != nullptr && loader->exists(p_path, "JiebaUTF8");
}

	} // namespace

	void JiebaSegment::_bind_methods() {
		ClassDB::bind_method(D_METHOD("initialize", "dict_path"), &JiebaSegment::initialize, DEFVAL(""));
		ClassDB::bind_method(D_METHOD("is_initialized"), &JiebaSegment::is_initialized);
		ClassDB::bind_method(D_METHOD("get_dict_path"), &JiebaSegment::get_dict_path);

		ClassDB::bind_method(D_METHOD("cut", "text", "use_hmm"), &JiebaSegment::cut, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("cut_all", "text"), &JiebaSegment::cut_all);
		ClassDB::bind_method(D_METHOD("cut_for_search", "text", "use_hmm"), &JiebaSegment::cut_for_search, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("cut_hmm", "text"), &JiebaSegment::cut_hmm);

		ClassDB::bind_method(D_METHOD("tag", "text"), &JiebaSegment::tag);
		ClassDB::bind_method(D_METHOD("tag_pairs", "text"), &JiebaSegment::tag_pairs);
		ClassDB::bind_method(D_METHOD("lookup_tag", "word"), &JiebaSegment::lookup_tag);
		ClassDB::bind_method(D_METHOD("extract_keywords", "text", "top_n"), &JiebaSegment::extract_keywords, DEFVAL(5));

		ClassDB::bind_method(D_METHOD("add_word", "word", "tag"), &JiebaSegment::add_word, DEFVAL(""));
		ClassDB::bind_method(D_METHOD("add_word_with_freq", "word", "freq", "tag"), &JiebaSegment::add_word_with_freq, DEFVAL(""));
		ClassDB::bind_method(D_METHOD("remove_word", "word", "tag"), &JiebaSegment::remove_word, DEFVAL(""));
		ClassDB::bind_method(D_METHOD("find_word", "word"), &JiebaSegment::find_word);

		ClassDB::bind_method(D_METHOD("load_user_dict", "path"), &JiebaSegment::load_user_dict);
	}

	JiebaSegment::JiebaSegment() = default;

	JiebaSegment::~JiebaSegment() {
		if (jieba != nullptr) {
			delete jieba;
			jieba = nullptr;
		}
	}

	bool JiebaSegment::initialize(const String& p_dict_path) {
		if (initialized) {
			return true;
		}

		String dict_path = p_dict_path;

		// 如果没有指定路径，尝试默认路径
		if (dict_path.is_empty()) {
			// 尝试多个可能的字典位置
			PackedStringArray try_paths;
			try_paths.append("res://addons/godot_jieba/dict");

			OS* os = OS::get_singleton();
			if (os != nullptr) {
				String exe_dir = os->get_executable_path().get_base_dir();
				if (!exe_dir.is_empty()) {
					try_paths.append(exe_dir.path_join("dict"));
					try_paths.append(exe_dir.path_join("addons/godot_jieba/dict"));
				}

				// 用户数据目录
				String user_dir = os->get_user_data_dir();
				if (!user_dir.is_empty()) {
					try_paths.append(user_dir.path_join("godot_jieba/dict"));
				}
			}

		for (const String& path : try_paths) {
			// 检查字典文件是否存在
			bool file_exists = dict_source_exists(path.path_join("jieba.dict.utf8")) || dict_source_exists(path.path_join("jieba.dict.gbk"));
			
			if (file_exists) {
				dict_path = path;
					break;
				}
			}
		}

		if (dict_path.is_empty()) {
			ERR_PRINT("Jieba: Could not find dictionary files. Please specify dict_path or ensure dict files are in addons/godot_jieba/dict/");
			return false;
		}

		// 确保路径使用正确的分隔符
		dict_dir_path = dict_path.replace("\\", "/");

		// 移除末尾的斜杠
		if (dict_dir_path.ends_with("/")) {
			dict_dir_path = dict_dir_path.substr(0, dict_dir_path.length() - 1);
		}

	try {
		cppjieba::JiebaMemoryData memory_data;
		if (!read_dict_memory_data(dict_dir_path, "jieba.dict.utf8", "jieba.dict.gbk", memory_data.dict) ||
				!read_dict_memory_data(dict_dir_path, "hmm_model.utf8", "hmm_model.gbk", memory_data.hmm_model) ||
				!read_dict_memory_data(dict_dir_path, "user.dict.utf8", "user.dict.gbk", memory_data.user_dict) ||
				!read_dict_memory_data(dict_dir_path, "idf.utf8", "idf.gbk", memory_data.idf) ||
				!read_dict_memory_data(dict_dir_path, "stop_words.utf8", "stop_words.gbk", memory_data.stop_words)) {
			return false;
		}

			jieba = new cppjieba::Jieba(memory_data);

			initialized = true;
			return true;

		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba initialization failed: ") + e.what());
			return false;
		}
		catch (...) {
			ERR_PRINT("Jieba initialization failed: unknown error");
			return false;
		}
	}

	bool JiebaSegment::is_initialized() const {
		return initialized;
	}

	String JiebaSegment::get_dict_path() const {
		return dict_dir_path;
	}

	PackedStringArray JiebaSegment::cut(const String& p_text, bool p_use_hmm) {
		PackedStringArray result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::string> words;

			jieba->Cut(text, words, p_use_hmm);

			result.resize(words.size());
			for (size_t i = 0; i < words.size(); ++i) {
				result[i] = String::utf8(words[i].c_str());
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba cut failed: ") + e.what());
		}

		return result;
	}

	PackedStringArray JiebaSegment::cut_all(const String& p_text) {
		PackedStringArray result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::string> words;

			jieba->CutAll(text, words);

			result.resize(words.size());
			for (size_t i = 0; i < words.size(); ++i) {
				result[i] = String::utf8(words[i].c_str());
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba cut_all failed: ") + e.what());
		}

		return result;
	}

	PackedStringArray JiebaSegment::cut_for_search(const String& p_text, bool p_use_hmm) {
		PackedStringArray result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::string> words;

			jieba->CutForSearch(text, words, p_use_hmm);

			result.resize(words.size());
			for (size_t i = 0; i < words.size(); ++i) {
				result[i] = String::utf8(words[i].c_str());
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba cut_for_search failed: ") + e.what());
		}

		return result;
	}

	PackedStringArray JiebaSegment::cut_hmm(const String& p_text) {
		PackedStringArray result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::string> words;

			jieba->CutHMM(text, words);

			result.resize(words.size());
			for (size_t i = 0; i < words.size(); ++i) {
				result[i] = String::utf8(words[i].c_str());
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba cut_hmm failed: ") + e.what());
		}

		return result;
	}

	Dictionary JiebaSegment::tag(const String& p_text) {
		Dictionary result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::pair<std::string, std::string>> tags;

			jieba->Tag(text, tags);

			for (const auto& pair : tags) {
				String word = String::utf8(pair.first.c_str());
				String tag_str = String::utf8(pair.second.c_str());
				result[word] = tag_str;
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba tag failed: ") + e.what());
		}

		return result;
	}

	PackedStringArray JiebaSegment::tag_pairs(const String& p_text) {
		PackedStringArray result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty()) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<std::pair<std::string, std::string>> tags;

			jieba->Tag(text, tags);

			for (const auto& pair : tags) {
				String word = String::utf8(pair.first.c_str());
				String tag_str = String::utf8(pair.second.c_str());
				result.append(word + "/" + tag_str);
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba tag_pairs failed: ") + e.what());
		}

		return result;
	}

	String JiebaSegment::lookup_tag(const String& p_word) {
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, String(), "Jieba not initialized. Call initialize() first.");

		if (p_word.is_empty()) {
			return "";
		}

		try {
			std::string word = p_word.utf8().get_data();
			std::string tag = jieba->LookupTag(word);
			return String::utf8(tag.c_str());
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba lookup_tag failed: ") + e.what());
			return "";
		}
	}

	Array JiebaSegment::extract_keywords(const String& p_text, int p_top_n) {
		Array result;
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, result, "Jieba not initialized. Call initialize() first.");

		if (p_text.is_empty() || p_top_n <= 0) {
			return result;
		}

		try {
			std::string text = p_text.utf8().get_data();
			std::vector<cppjieba::KeywordExtractor::Word> keywords;

			jieba->extractor.Extract(text, keywords, static_cast<size_t>(p_top_n));

			for (const cppjieba::KeywordExtractor::Word& keyword : keywords) {
				Dictionary item;
				PackedInt64Array offsets;
				for (size_t offset : keyword.offsets) {
					offsets.append(static_cast<int64_t>(offset));
				}

				item["word"] = String::utf8(keyword.word.c_str());
				item["weight"] = keyword.weight;
				item["offsets"] = offsets;
				result.append(item);
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba extract_keywords failed: ") + e.what());
		}

		return result;
	}

	bool JiebaSegment::add_word(const String& p_word, const String& p_tag) {
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, false, "Jieba not initialized. Call initialize() first.");

		if (p_word.is_empty()) {
			return false;
		}

		try {
			std::string word = p_word.utf8().get_data();
			std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();

			if (tag.empty()) {
				return jieba->InsertUserWord(word);
			}
			else {
				return jieba->InsertUserWord(word, tag);
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba add_word failed: ") + e.what());
			return false;
		}
	}

	bool JiebaSegment::add_word_with_freq(const String& p_word, int p_freq, const String& p_tag) {
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, false, "Jieba not initialized. Call initialize() first.");

		if (p_word.is_empty() || p_freq <= 0) {
			return false;
		}

		try {
			std::string word = p_word.utf8().get_data();
			std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();

			if (tag.empty()) {
				return jieba->InsertUserWord(word, p_freq);
			}
			else {
				return jieba->InsertUserWord(word, p_freq, tag);
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba add_word_with_freq failed: ") + e.what());
			return false;
		}
	}

	bool JiebaSegment::remove_word(const String& p_word, const String& p_tag) {
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, false, "Jieba not initialized. Call initialize() first.");

		if (p_word.is_empty()) {
			return false;
		}

		try {
			std::string word = p_word.utf8().get_data();
			std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();

			if (tag.empty()) {
				return jieba->DeleteUserWord(word);
			}
			else {
				return jieba->DeleteUserWord(word, tag);
			}
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba remove_word failed: ") + e.what());
			return false;
		}
	}

	bool JiebaSegment::find_word(const String& p_word) {
		ERR_FAIL_COND_V_MSG(!initialized || jieba == nullptr, false, "Jieba not initialized. Call initialize() first.");

		if (p_word.is_empty()) {
			return false;
		}

		try {
			std::string word = p_word.utf8().get_data();
			return jieba->Find(word);
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba find_word failed: ") + e.what());
			return false;
		}
	}

	void JiebaSegment::load_user_dict(const String& p_path) {
		ERR_FAIL_COND_MSG(!initialized || jieba == nullptr, "Jieba not initialized. Call initialize() first.");

		if (p_path.is_empty()) {
			return;
		}

		try {
			std::string content;
			if (!read_file_to_string(p_path, content)) {
				return;
			}

			jieba->LoadUserDict(cppjieba::MemoryData(content, p_path.utf8().get_data()));
		}
		catch (const std::exception& e) {
			ERR_PRINT(String("Jieba load_user_dict failed: ") + e.what());
		}
	}

} // namespace godot
