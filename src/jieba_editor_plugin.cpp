#include "jieba_editor_plugin.h"

#include "jieba_editor_dock.h"

#include <godot_cpp/core/memory.hpp>

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

	if (dock == nullptr) {
		dock = memnew(JiebaEditorDock);
		add_control_to_bottom_panel(dock, "Jieba");
	}
}

void JiebaEditorPlugin::_exit_tree() {
	if (dock != nullptr) {
		remove_control_from_bottom_panel(dock);
		dock->queue_free();
		dock = nullptr;
	}

	if (importer.is_valid()) {
		remove_import_plugin(importer);
		importer.unref();
	}
}

} // namespace godot
