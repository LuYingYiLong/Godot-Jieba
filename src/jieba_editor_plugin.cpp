#include "jieba_editor_plugin.h"

namespace godot {

void JiebaEditorPlugin::_bind_methods() {
}

JiebaEditorPlugin::JiebaEditorPlugin() {
}

JiebaEditorPlugin::~JiebaEditorPlugin() {
}

void JiebaEditorPlugin::_enter_tree() {
	if (importer.is_null()) {
		importer.instantiate();
	}
	add_import_plugin(importer, true);
}

void JiebaEditorPlugin::_exit_tree() {
	if (importer.is_valid()) {
		remove_import_plugin(importer);
		importer.unref();
	}
}

} // namespace godot
