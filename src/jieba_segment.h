#ifndef JIEBA_SEGMENT_H
#define JIEBA_SEGMENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>

namespace cppjieba {
	class Jieba;
}

namespace godot {
	class JiebaSegment : public RefCounted {
		GDCLASS(JiebaSegment, RefCounted)

	private:
		cppjieba::Jieba* jieba = nullptr;
		String dict_dir_path;
		bool initialized = false;

	protected:
		static void _bind_methods();

	public:
		JiebaSegment();
		~JiebaSegment();

		bool initialize(const String& p_dict_path = "");
		bool is_initialized() const;
		String get_dict_path() const;

		PackedStringArray cut(const String& p_text, bool p_use_hmm = true);
		PackedStringArray cut_all(const String& p_text);
		PackedStringArray cut_for_search(const String& p_text, bool p_use_hmm = true);
		PackedStringArray cut_hmm(const String& p_text);

		Dictionary tag(const String& p_text);
		PackedStringArray tag_pairs(const String& p_text);
		String lookup_tag(const String& p_word);
		Array extract_keywords(const String& p_text, int p_top_n = 5);

		bool add_word(const String& p_word, const String& p_tag = "");
		bool add_word_with_freq(const String& p_word, int p_freq, const String& p_tag = "");
		bool remove_word(const String& p_word, const String& p_tag = "");
		bool find_word(const String& p_word);

		void load_user_dict(const String& p_path);
	};
}

#endif // JIEBA_SEGMENT_H
