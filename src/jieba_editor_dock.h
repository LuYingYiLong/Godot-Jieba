#ifndef JIEBA_EDITOR_DOCK_H
#define JIEBA_EDITOR_DOCK_H

#include "jieba_segment.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot {
	class CheckBox;
	class Label;
	class LineEdit;
	class OptionButton;
	class SpinBox;
	class TextEdit;

	class JiebaEditorDock : public VBoxContainer {
		GDCLASS(JiebaEditorDock, VBoxContainer)

	private:
		Ref<JiebaSegment> segment;
		String initialized_dict_path_request;
		String loaded_user_dict_path;

		LineEdit* dict_path_edit = nullptr;
		Label* status_label = nullptr;
		TextEdit* user_dict_edit = nullptr;
		TextEdit* preview_input_edit = nullptr;
		TextEdit* preview_output_edit = nullptr;
		OptionButton* cut_mode_option = nullptr;
		CheckBox* use_hmm_check = nullptr;
		SpinBox* keyword_top_n_spin = nullptr;

		void build_ui();
		bool ensure_segment();
		bool reinitialize_segment();
		String get_requested_dict_path() const;
		String get_default_user_dict_path() const;
		bool read_text_file(const String& p_path, String& r_text) const;
		bool write_text_file(const String& p_path, const String& p_text) const;
		bool validate_user_dict_text(const String& p_text, String& r_message) const;
		void set_status(const String& p_status, bool p_error = false);
		void set_output(const String& p_text);
		PackedStringArray cut_preview_text(const String& p_text);

	protected:
		static void _bind_methods();

	public:
		JiebaEditorDock();
		~JiebaEditorDock();

		void _on_initialize_pressed();
		void _on_load_user_dict_pressed();
		void _on_save_user_dict_pressed();
		void _on_validate_user_dict_pressed();
		void _on_cut_preview_pressed();
		void _on_tag_preview_pressed();
		void _on_keyword_preview_pressed();
	};
}

#endif // JIEBA_EDITOR_DOCK_H
