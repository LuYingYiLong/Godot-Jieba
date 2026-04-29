#ifndef JIEBA_EDITOR_PLUGIN_H
#define JIEBA_EDITOR_PLUGIN_H

#include "resource_importer_jieba.h"

#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot {
	class JiebaEditorPlugin : public EditorPlugin {
		GDCLASS(JiebaEditorPlugin, EditorPlugin)

	private:
		Ref<ResourceImporterJieba> importer;

	protected:
		static void _bind_methods();

	public:
		JiebaEditorPlugin();
		~JiebaEditorPlugin();

		void _enter_tree() override;
		void _exit_tree() override;
	};
}

#endif // JIEBA_EDITOR_PLUGIN_H
