#include "jieba_segment.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "thirdparty/cppjieba/include/cppjieba/Jieba.hpp"

#ifdef _WIN32
#include <windows.h>
#include <vector>

// 将 UTF-8 字符串转换为宽字符字符串（Windows 文件路径需要）
static std::wstring utf8_to_wstring(const std::string& utf8_str) {
    if (utf8_str.empty()) return std::wstring();
    
    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wchar_count <= 0) return std::wstring();
    
    std::vector<wchar_t> buffer(wchar_count);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, buffer.data(), wchar_count);
    
    return std::wstring(buffer.data());
}
#endif

void godot::JiebaSegment::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("initialize", "dict_path"), &godot::JiebaSegment::initialize, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("is_initialized"), &godot::JiebaSegment::is_initialized);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dict_path"), &godot::JiebaSegment::get_dict_path);

    godot::ClassDB::bind_method(godot::D_METHOD("cut", "text", "use_hmm"), &godot::JiebaSegment::cut, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("cut_all", "text"), &godot::JiebaSegment::cut_all);
    godot::ClassDB::bind_method(godot::D_METHOD("cut_for_search", "text", "use_hmm"), &godot::JiebaSegment::cut_for_search, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("cut_hmm", "text"), &godot::JiebaSegment::cut_hmm);

    godot::ClassDB::bind_method(godot::D_METHOD("tag", "text"), &godot::JiebaSegment::tag);
    godot::ClassDB::bind_method(godot::D_METHOD("lookup_tag", "word"), &godot::JiebaSegment::lookup_tag);

    godot::ClassDB::bind_method(godot::D_METHOD("add_word", "word", "tag"), &godot::JiebaSegment::add_word, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("add_word_with_freq", "word", "freq", "tag"), &godot::JiebaSegment::add_word_with_freq, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("remove_word", "word", "tag"), &godot::JiebaSegment::remove_word, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("find_word", "word"), &godot::JiebaSegment::find_word);

    godot::ClassDB::bind_method(godot::D_METHOD("load_user_dict", "path"), &godot::JiebaSegment::load_user_dict);
}

godot::JiebaSegment::JiebaSegment() = default;

godot::JiebaSegment::~JiebaSegment() {
    if (jieba != nullptr) {
        delete jieba;
        jieba = nullptr;
    }
}

// 检查字符串是否包含非 ASCII 字符
static bool contains_non_ascii(const godot::String& str) {
    for (int i = 0; i < str.length(); i++) {
        char32_t c = str[i];
        if (c > 127) {
            return true;
        }
    }
    return false;
}

bool godot::JiebaSegment::initialize(const godot::String& p_dict_path) {
    if (initialized) {
        return true;
    }

    godot::String dict_path = p_dict_path;
    
    // 如果没有指定路径，尝试默认路径
    if (dict_path.is_empty()) {
        // 尝试多个可能的字典位置
        godot::PackedStringArray try_paths;
        
        // 项目内的 addons 目录（优先，避免中文路径问题）
        try_paths.append("res://addons/godot_jieba/dict");
        
        // 与 DLL 同目录的 dict 文件夹
        godot::OS* os = godot::OS::get_singleton();
        if (os != nullptr) {
            godot::String exe_dir = os->get_executable_path().get_base_dir();
            if (!exe_dir.is_empty()) {
                try_paths.append(exe_dir.path_join("dict"));
                try_paths.append(exe_dir.path_join("addons/godot_jieba/dict"));
            }
            
            // 用户数据目录
            godot::String user_dir = os->get_user_data_dir();
            if (!user_dir.is_empty()) {
                try_paths.append(user_dir.path_join("godot_jieba/dict"));
            }
        }

        godot::ProjectSettings* ps = godot::ProjectSettings::get_singleton();
        
        for (const godot::String& path : try_paths) {
            godot::String test_path = path;
            
            // 如果是 res:// 或 user:// 路径，尝试 globalize
            if (path.begins_with("res://") || path.begins_with("user://")) {
                if (ps != nullptr) {
                    godot::String global_path = ps->globalize_path(path);
                    if (!global_path.is_empty()) {
                        test_path = global_path;
                    }
                }
            }
            
            // 检查字典文件是否存在
            godot::String test_file = test_path.path_join("jieba.dict.utf8");
            bool file_exists = godot::FileAccess::file_exists(test_file);
            
            if (file_exists) {
                dict_path = test_path;
                break;
            }
        }
    }

    if (dict_path.is_empty()) {
        godot::UtilityFunctions::push_error("Jieba: Could not find dictionary files. Please specify dict_path or ensure dict files are in addons/godot_jieba/dict/");
        return false;
    }

    // 确保路径使用正确的分隔符
    dict_dir_path = dict_path.replace("\\", "/");
    
    // 移除末尾的斜杠
    if (dict_dir_path.ends_with("/")) {
        dict_dir_path = dict_dir_path.substr(0, dict_dir_path.length() - 1);
    }

#ifdef _WIN32
    // Windows: 检查路径是否包含中文
    if (contains_non_ascii(dict_dir_path)) {
        godot::UtilityFunctions::push_error("Jieba: Dictionary path contains non-ASCII characters (Chinese/Japanese/etc). On Windows, please use only ASCII characters in the path. Solutions:\n1. Place dict files in res://addons/godot_jieba/dict/ (recommended)\n2. Move your project to a path without Chinese characters");
        return false;
    }
#endif

    try {
        std::string dict_dir = dict_dir_path.utf8().get_data();
        
        // 构建各个字典文件的完整路径
        std::string jieba_dict = dict_dir + "/jieba.dict.utf8";
        std::string hmm_model = dict_dir + "/hmm_model.utf8";
        std::string user_dict = dict_dir + "/user.dict.utf8";
        std::string idf = dict_dir + "/idf.utf8";
        std::string stop_words = dict_dir + "/stop_words.utf8";

        jieba = new cppjieba::Jieba(
            jieba_dict,
            hmm_model,
            user_dict,
            idf,
            stop_words
        );
        
        initialized = true;
        return true;
        
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba initialization failed: ") + e.what());
        return false;
    } catch (...) {
        godot::UtilityFunctions::push_error("Jieba initialization failed: unknown error");
        return false;
    }
}

bool godot::JiebaSegment::is_initialized() const {
    return initialized;
}

godot::String godot::JiebaSegment::get_dict_path() const {
    return dict_dir_path;
}

godot::PackedStringArray godot::JiebaSegment::cut(const godot::String& p_text, bool p_use_hmm) {
    godot::PackedStringArray result;
    
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return result;
    }
    
    if (p_text.is_empty()) {
        return result;
    }

    try {
        std::string text = p_text.utf8().get_data();
        std::vector<std::string> words;
        
        jieba->Cut(text, words, p_use_hmm);
        
        result.resize(words.size());
        for (size_t i = 0; i < words.size(); ++i) {
            result[i] = godot::String::utf8(words[i].c_str());
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba cut failed: ") + e.what());
    }
    
    return result;
}

godot::PackedStringArray godot::JiebaSegment::cut_all(const godot::String& p_text) {
    godot::PackedStringArray result;
    
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return result;
    }
    
    if (p_text.is_empty()) {
        return result;
    }

    try {
        std::string text = p_text.utf8().get_data();
        std::vector<std::string> words;
        
        jieba->CutAll(text, words);
        
        result.resize(words.size());
        for (size_t i = 0; i < words.size(); ++i) {
            result[i] = godot::String::utf8(words[i].c_str());
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba cut_all failed: ") + e.what());
    }
    
    return result;
}

godot::PackedStringArray godot::JiebaSegment::cut_for_search(const godot::String& p_text, bool p_use_hmm) {
    godot::PackedStringArray result;
    
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return result;
    }
    
    if (p_text.is_empty()) {
        return result;
    }

    try {
        std::string text = p_text.utf8().get_data();
        std::vector<std::string> words;
        
        jieba->CutForSearch(text, words, p_use_hmm);
        
        result.resize(words.size());
        for (size_t i = 0; i < words.size(); ++i) {
            result[i] = godot::String::utf8(words[i].c_str());
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba cut_for_search failed: ") + e.what());
    }
    
    return result;
}

godot::PackedStringArray godot::JiebaSegment::cut_hmm(const godot::String& p_text) {
    godot::PackedStringArray result;
    
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return result;
    }
    
    if (p_text.is_empty()) {
        return result;
    }

    try {
        std::string text = p_text.utf8().get_data();
        std::vector<std::string> words;
        
        jieba->CutHMM(text, words);
        
        result.resize(words.size());
        for (size_t i = 0; i < words.size(); ++i) {
            result[i] = godot::String::utf8(words[i].c_str());
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba cut_hmm failed: ") + e.what());
    }
    
    return result;
}

godot::Dictionary godot::JiebaSegment::tag(const godot::String& p_text) {
    godot::Dictionary result;
    
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return result;
    }
    
    if (p_text.is_empty()) {
        return result;
    }

    try {
        std::string text = p_text.utf8().get_data();
        std::vector<std::pair<std::string, std::string>> tags;
        
        jieba->Tag(text, tags);
        
        for (const auto& pair : tags) {
            godot::String word = godot::String::utf8(pair.first.c_str());
            godot::String tag_str = godot::String::utf8(pair.second.c_str());
            result[word] = tag_str;
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba tag failed: ") + e.what());
    }
    
    return result;
}

godot::String godot::JiebaSegment::lookup_tag(const godot::String& p_word) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return "";
    }
    
    if (p_word.is_empty()) {
        return "";
    }

    try {
        std::string word = p_word.utf8().get_data();
        std::string tag = jieba->LookupTag(word);
        return godot::String::utf8(tag.c_str());
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba lookup_tag failed: ") + e.what());
        return "";
    }
}

bool godot::JiebaSegment::add_word(const godot::String& p_word, const godot::String& p_tag) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return false;
    }
    
    if (p_word.is_empty()) {
        return false;
    }

    try {
        std::string word = p_word.utf8().get_data();
        std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();
        
        if (tag.empty()) {
            return jieba->InsertUserWord(word);
        } else {
            return jieba->InsertUserWord(word, tag);
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba add_word failed: ") + e.what());
        return false;
    }
}

bool godot::JiebaSegment::add_word_with_freq(const godot::String& p_word, int p_freq, const godot::String& p_tag) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return false;
    }
    
    if (p_word.is_empty() || p_freq <= 0) {
        return false;
    }

    try {
        std::string word = p_word.utf8().get_data();
        std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();
        
        if (tag.empty()) {
            return jieba->InsertUserWord(word, p_freq);
        } else {
            return jieba->InsertUserWord(word, p_freq, tag);
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba add_word_with_freq failed: ") + e.what());
        return false;
    }
}

bool godot::JiebaSegment::remove_word(const godot::String& p_word, const godot::String& p_tag) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return false;
    }
    
    if (p_word.is_empty()) {
        return false;
    }

    try {
        std::string word = p_word.utf8().get_data();
        std::string tag = p_tag.is_empty() ? "" : p_tag.utf8().get_data();
        
        if (tag.empty()) {
            return jieba->DeleteUserWord(word);
        } else {
            return jieba->DeleteUserWord(word, tag);
        }
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba remove_word failed: ") + e.what());
        return false;
    }
}

bool godot::JiebaSegment::find_word(const godot::String& p_word) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return false;
    }
    
    if (p_word.is_empty()) {
        return false;
    }

    try {
        std::string word = p_word.utf8().get_data();
        return jieba->Find(word);
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba find_word failed: ") + e.what());
        return false;
    }
}

void godot::JiebaSegment::load_user_dict(const godot::String& p_path) {
    if (!initialized || jieba == nullptr) {
        godot::UtilityFunctions::push_warning("Jieba not initialized. Call initialize() first.");
        return;
    }
    
    if (p_path.is_empty()) {
        return;
    }

    try {
        godot::String global_path = godot::ProjectSettings::get_singleton()->globalize_path(p_path);
        if (global_path.is_empty()) {
            global_path = p_path;
        }
        
        std::string path = global_path.utf8().get_data();
        jieba->LoadUserDict(path);
    } catch (const std::exception& e) {
        godot::UtilityFunctions::push_error(godot::String("Jieba load_user_dict failed: ") + e.what());
    }
}
