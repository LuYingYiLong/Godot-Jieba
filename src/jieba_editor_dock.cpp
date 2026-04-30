#include "jieba_editor_dock.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

namespace godot {
	namespace {
		enum CutMode {
			CUT_MODE_MIX = 0,
			CUT_MODE_MP = 1,
			CUT_MODE_HMM = 2,
			CUT_MODE_FULL = 3,
			CUT_MODE_QUERY = 4,
		};

		Label* make_label(const String& p_text) {
			Label* label = memnew(Label);
			label->set_text(p_text);
			return label;
		}

		Button* make_button(const String& p_text, Object* p_target, const StringName& p_method) {
			Button* button = memnew(Button);
			button->set_text(p_text);
			button->connect("pressed", Callable(p_target, p_method));
			return button;
		}

		String normalize_dictionary_text(const String& p_text) {
			return p_text.replace("\r\n", "\n").replace("\r", "\n");
		}

		String normalize_dictionary_line(const String& p_line) {
			String line = p_line.replace("\t", " ").strip_edges();
			while (line.contains("  ")) {
				line = line.replace("  ", " ");
			}
			return line;
		}

	} // namespace

	void JiebaEditorDock::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_initialize_pressed"), &JiebaEditorDock::_on_initialize_pressed);
		ClassDB::bind_method(D_METHOD("_on_load_user_dict_pressed"), &JiebaEditorDock::_on_load_user_dict_pressed);
		ClassDB::bind_method(D_METHOD("_on_save_user_dict_pressed"), &JiebaEditorDock::_on_save_user_dict_pressed);
		ClassDB::bind_method(D_METHOD("_on_validate_user_dict_pressed"), &JiebaEditorDock::_on_validate_user_dict_pressed);
		ClassDB::bind_method(D_METHOD("_on_cut_preview_pressed"), &JiebaEditorDock::_on_cut_preview_pressed);
		ClassDB::bind_method(D_METHOD("_on_tag_preview_pressed"), &JiebaEditorDock::_on_tag_preview_pressed);
		ClassDB::bind_method(D_METHOD("_on_keyword_preview_pressed"), &JiebaEditorDock::_on_keyword_preview_pressed);
	}

	JiebaEditorDock::JiebaEditorDock() {
		build_ui();
	}

	JiebaEditorDock::~JiebaEditorDock() = default;

	void JiebaEditorDock::build_ui() {
		set_name("Jieba");
		set_custom_minimum_size(Vector2(720, 320));
		set_h_size_flags(Control::SIZE_EXPAND_FILL);
		set_v_size_flags(Control::SIZE_EXPAND_FILL);

		HBoxContainer* path_row = memnew(HBoxContainer);
		path_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		add_child(path_row);

		path_row->add_child(make_label("Dict"));

		dict_path_edit = memnew(LineEdit);
		dict_path_edit->set_text("res://addons/godot_jieba/dict");
		dict_path_edit->set_placeholder("res://addons/godot_jieba/dict");
		dict_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		path_row->add_child(dict_path_edit);

		path_row->add_child(make_button("Initialize", this, "_on_initialize_pressed"));
		path_row->add_child(make_button("Load User Dict", this, "_on_load_user_dict_pressed"));
		path_row->add_child(make_button("Save", this, "_on_save_user_dict_pressed"));
		path_row->add_child(make_button("Validate", this, "_on_validate_user_dict_pressed"));

		HSplitContainer* split = memnew(HSplitContainer);
		split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		add_child(split);

		VBoxContainer* dict_column = memnew(VBoxContainer);
		dict_column->set_custom_minimum_size(Vector2(300, 240));
		dict_column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dict_column->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		split->add_child(dict_column);

		dict_column->add_child(make_label("User Dictionary"));

		user_dict_edit = memnew(TextEdit);
		user_dict_edit->set_placeholder("word\nword tag\nword frequency tag");
		user_dict_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
		user_dict_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		user_dict_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		dict_column->add_child(user_dict_edit);

		VBoxContainer* preview_column = memnew(VBoxContainer);
		preview_column->set_custom_minimum_size(Vector2(360, 240));
		preview_column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		preview_column->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		split->add_child(preview_column);

		HBoxContainer* preview_controls = memnew(HBoxContainer);
		preview_controls->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		preview_column->add_child(preview_controls);

		cut_mode_option = memnew(OptionButton);
		cut_mode_option->add_item("Mix", CUT_MODE_MIX);
		cut_mode_option->add_item("MP", CUT_MODE_MP);
		cut_mode_option->add_item("HMM", CUT_MODE_HMM);
		cut_mode_option->add_item("Full", CUT_MODE_FULL);
		cut_mode_option->add_item("Query", CUT_MODE_QUERY);
		cut_mode_option->select(CUT_MODE_MIX);
		preview_controls->add_child(cut_mode_option);

		use_hmm_check = memnew(CheckBox);
		use_hmm_check->set_text("HMM");
		use_hmm_check->set_pressed(true);
		preview_controls->add_child(use_hmm_check);

		keyword_top_n_spin = memnew(SpinBox);
		keyword_top_n_spin->set_min(1.0);
		keyword_top_n_spin->set_max(50.0);
		keyword_top_n_spin->set_step(1.0);
		keyword_top_n_spin->set_value(5.0);
		keyword_top_n_spin->set_custom_minimum_size(Vector2(72, 0));
		preview_controls->add_child(keyword_top_n_spin);

		preview_controls->add_child(make_button("Cut", this, "_on_cut_preview_pressed"));
		preview_controls->add_child(make_button("Tag", this, "_on_tag_preview_pressed"));
		preview_controls->add_child(make_button("Keywords", this, "_on_keyword_preview_pressed"));

		preview_input_edit = memnew(TextEdit);
		preview_input_edit->set_placeholder("Enter Chinese text to preview");
		preview_input_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
		preview_input_edit->set_custom_minimum_size(Vector2(0, 92));
		preview_input_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		preview_column->add_child(preview_input_edit);

		preview_output_edit = memnew(TextEdit);
		preview_output_edit->set_editable(false);
		preview_output_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
		preview_output_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		preview_output_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		preview_column->add_child(preview_output_edit);

		status_label = make_label("Ready");
		status_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		add_child(status_label);
	}

	bool JiebaEditorDock::ensure_segment() {
		String requested_path = get_requested_dict_path();
		if (segment.is_valid() && segment->is_initialized() && requested_path == initialized_dict_path_request) {
			return true;
		}
		return reinitialize_segment();
	}

	bool JiebaEditorDock::reinitialize_segment() {
		segment.unref();
		segment.instantiate();

		String requested_path = get_requested_dict_path();
		if (!segment->initialize(requested_path)) {
			initialized_dict_path_request = "";
			set_status("Failed to initialize Jieba. Check dictionary path and required files.", true);
			return false;
		}

		initialized_dict_path_request = requested_path;
		set_status(String("Initialized: ") + segment->get_dict_path());
		return true;
	}

	String JiebaEditorDock::get_requested_dict_path() const {
		if (dict_path_edit == nullptr) {
			return "";
		}
		return dict_path_edit->get_text().strip_edges().replace("\\", "/");
	}

	String JiebaEditorDock::get_default_user_dict_path() const {
		String dict_path = get_requested_dict_path();
		if (dict_path.is_empty()) {
			dict_path = "res://addons/godot_jieba/dict";
		}

		String utf8_path = dict_path.path_join("user.dict.utf8");
		if (FileAccess::file_exists(utf8_path)) {
			return utf8_path;
		}

		String gbk_path = dict_path.path_join("user.dict.gbk");
		if (FileAccess::file_exists(gbk_path)) {
			return gbk_path;
		}

		return utf8_path;
	}

	bool JiebaEditorDock::read_text_file(const String& p_path, String& r_text) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null() || !file->is_open()) {
			return false;
		}

		uint64_t length = file->get_length();
		if (length == 0) {
			r_text = "";
			return true;
		}

		PackedByteArray source_data = file->get_buffer(static_cast<int64_t>(length));
		if (source_data.size() != static_cast<int64_t>(length)) {
			return false;
		}

		if (p_path.get_extension().to_lower() == "gbk") {
			r_text = source_data.get_string_from_multibyte_char("GBK");
		}
		else {
			r_text = source_data.get_string_from_utf8();
		}
		return true;
	}

	bool JiebaEditorDock::write_text_file(const String& p_path, const String& p_text) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
		if (file.is_null() || !file->is_open()) {
			return false;
		}

		PackedByteArray bytes;
		if (p_path.get_extension().to_lower() == "gbk") {
			bytes = p_text.to_multibyte_char_buffer("GBK");
		}
		else {
			bytes = p_text.to_utf8_buffer();
		}

		return file->store_buffer(bytes);
	}

	bool JiebaEditorDock::validate_user_dict_text(const String& p_text, String& r_message) const {
		int valid_lines = 0;
		PackedStringArray errors;
		PackedStringArray lines = normalize_dictionary_text(p_text).split("\n", true);

		for (int64_t i = 0; i < lines.size(); ++i) {
			String line = normalize_dictionary_line(lines[i]);
			if (line.is_empty()) {
				continue;
			}

			PackedStringArray columns = line.split(" ", false);
			if (columns.size() < 1 || columns.size() > 3) {
				errors.append(String("Line ") + String::num_int64(i + 1) + ": expected 1, 2, or 3 columns.");
				continue;
			}

			if (columns.size() == 3 && !columns[1].is_valid_int()) {
				errors.append(String("Line ") + String::num_int64(i + 1) + ": frequency must be an integer.");
				continue;
			}

			++valid_lines;
		}

		if (!errors.is_empty()) {
			r_message = String("\n").join(errors);
			return false;
		}

		r_message = String("Valid user dictionary lines: ") + String::num_int64(valid_lines);
		return true;
	}

	void JiebaEditorDock::set_status(const String& p_status, bool p_error) {
		if (status_label != nullptr) {
			status_label->set_text(p_error ? String("Error: ") + p_status : p_status);
		}
	}

	void JiebaEditorDock::set_output(const String& p_text) {
		if (preview_output_edit != nullptr) {
			preview_output_edit->set_text(p_text);
		}
	}

	PackedStringArray JiebaEditorDock::cut_preview_text(const String& p_text) {
		PackedStringArray words;
		if (!ensure_segment()) {
			return words;
		}

		int selected_id = cut_mode_option != nullptr ? cut_mode_option->get_selected_id() : CUT_MODE_MIX;
		bool use_hmm = use_hmm_check == nullptr || use_hmm_check->is_pressed();

		switch (selected_id) {
			case CUT_MODE_MP:
				return segment->cut(p_text, false);
			case CUT_MODE_HMM:
				return segment->cut_hmm(p_text);
			case CUT_MODE_FULL:
				return segment->cut_all(p_text);
			case CUT_MODE_QUERY:
				return segment->cut_for_search(p_text, use_hmm);
			case CUT_MODE_MIX:
			default:
				return segment->cut(p_text, use_hmm);
		}
	}

	void JiebaEditorDock::_on_initialize_pressed() {
		reinitialize_segment();
	}

	void JiebaEditorDock::_on_load_user_dict_pressed() {
		String path = get_default_user_dict_path();
		String text;
		if (!read_text_file(path, text)) {
			set_status(String("Could not read user dictionary: ") + path, true);
			return;
		}

		loaded_user_dict_path = path;
		if (user_dict_edit != nullptr) {
			user_dict_edit->set_text(normalize_dictionary_text(text));
			user_dict_edit->clear_undo_history();
			user_dict_edit->tag_saved_version();
		}
		set_status(String("Loaded user dictionary: ") + path);
	}

	void JiebaEditorDock::_on_save_user_dict_pressed() {
		if (user_dict_edit == nullptr) {
			return;
		}

		String message;
		String text = normalize_dictionary_text(user_dict_edit->get_text());
		if (!validate_user_dict_text(text, message)) {
			set_output(message);
			set_status("User dictionary has invalid rows.", true);
			return;
		}

		String path = loaded_user_dict_path.is_empty() ? get_default_user_dict_path() : loaded_user_dict_path;
		if (!write_text_file(path, text)) {
			set_status(String("Could not save user dictionary: ") + path, true);
			return;
		}

		loaded_user_dict_path = path;
		segment.unref();
		initialized_dict_path_request = "";
		ensure_segment();
		set_output(message);
		set_status(String("Saved user dictionary: ") + path);
	}

	void JiebaEditorDock::_on_validate_user_dict_pressed() {
		if (user_dict_edit == nullptr) {
			return;
		}

		String message;
		bool valid = validate_user_dict_text(user_dict_edit->get_text(), message);
		set_output(message);
		set_status(valid ? "User dictionary is valid." : "User dictionary has invalid rows.", !valid);
	}

	void JiebaEditorDock::_on_cut_preview_pressed() {
		String text = preview_input_edit != nullptr ? preview_input_edit->get_text() : String();
		PackedStringArray words = cut_preview_text(text);
		String output = String("Tokens: ") + String::num_int64(words.size()) + "\n\n" + String(" / ").join(words);
		set_output(output);
		set_status("Cut preview updated.");
	}

	void JiebaEditorDock::_on_tag_preview_pressed() {
		if (!ensure_segment()) {
			return;
		}

		String text = preview_input_edit != nullptr ? preview_input_edit->get_text() : String();
		PackedStringArray tags = segment->tag_pairs(text);
		String output = String("Tags: ") + String::num_int64(tags.size()) + "\n\n" + String("\n").join(tags);
		set_output(output);
		set_status("Tag preview updated.");
	}

	void JiebaEditorDock::_on_keyword_preview_pressed() {
		if (!ensure_segment()) {
			return;
		}

		String text = preview_input_edit != nullptr ? preview_input_edit->get_text() : String();
		int top_n = keyword_top_n_spin != nullptr ? static_cast<int>(keyword_top_n_spin->get_value()) : 5;
		Array keywords = segment->extract_keywords(text, top_n);

		PackedStringArray lines;
		for (int64_t i = 0; i < keywords.size(); ++i) {
			Dictionary keyword = keywords[i];
			PackedInt64Array offsets = keyword["offsets"];
			PackedStringArray offset_strings;
			for (int64_t j = 0; j < offsets.size(); ++j) {
				offset_strings.append(String::num_int64(offsets[j]));
			}
			lines.append(String::num_int64(i + 1) + ". " + String(keyword["word"]) + "  weight=" + String::num_real(keyword["weight"]) + "  offsets=[" + String(", ").join(offset_strings) + "]");
		}

		String output = String("Keywords: ") + String::num_int64(keywords.size()) + "\n\n" + String("\n").join(lines);
		set_output(output);
		set_status("Keyword preview updated.");
	}

} // namespace godot
