#include "register_types.h"

#include "jieba_editor_plugin.h"
#include "jieba_segment.h"
#include "jieba_utf8.h"
#include "jieba_editor_dock.h"
#include "resource_importer_jieba.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_godot_jieba_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_CLASS(ResourceImporterJieba);
		GDREGISTER_CLASS(JiebaEditorDock);
		GDREGISTER_CLASS(JiebaEditorPlugin);
		EditorPlugins::add_by_type<JiebaEditorPlugin>();
		return;
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(JiebaUTF8);
	GDREGISTER_CLASS(JiebaSegment);
}

void uninitialize_godot_jieba_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorPlugins::remove_by_type<JiebaEditorPlugin>();
		return;
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
	// Initialization.
	GDExtensionBool GDE_EXPORT godot_jieba_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_godot_jieba_module);
		init_obj.register_terminator(uninitialize_godot_jieba_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_EDITOR);

		return init_obj.init();
	}
}
