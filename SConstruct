#!/usr/bin/env python
import os
import sys
import shutil

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=[
    "src/",
    "src/thirdparty/cppjieba/deps/limonp/include/"
])

# 启用 C++ 异常处理（MSVC 需要 /EHsc）
if env["platform"] == "windows":
    env.Append(CXXFLAGS=["/EHsc"])

# Collect C++ sources
sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/godot_jieba/bin/libgodot_jieba.{}.{}.framework/libgodot_jieba.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/godot_jieba/bin/libgodot_jieba.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/godot_jieba/bin/libgodot_jieba.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )

elif env["platform"] == "android":
    library = env.SharedLibrary(
        "addons/godot_jieba/bin/libgodot_jieba{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

else:
    library = env.SharedLibrary(
        "addons/godot_jieba/bin/godot_jieba{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

# 复制字典文件到输出目录
def copy_dict_files(target, source, env):
    """复制 CppJieba 字典文件到插件目录"""
    dict_source = "src/thirdparty/cppjieba/dict"
    dict_dest = "addons/godot_jieba/dict"
    
    # 需要复制的字典文件
    dict_files = [
        "jieba.dict.utf8",
        "hmm_model.utf8",
        "user.dict.utf8",
        "idf.utf8",
        "stop_words.utf8"
    ]
    
    # 确保目标目录存在
    if not os.path.exists(dict_dest):
        os.makedirs(dict_dest)
        print(f"Created directory: {dict_dest}")
    
    # 复制字典文件
    copied_count = 0
    for filename in dict_files:
        src_path = os.path.join(dict_source, filename)
        dst_path = os.path.join(dict_dest, filename)
        
        if os.path.exists(src_path):
            try:
                shutil.copy2(src_path, dst_path)
                print(f"Copied: {src_path} -> {dst_path}")
                copied_count += 1
            except Exception as e:
                print(f"Error copying {filename}: {e}")
        else:
            print(f"Warning: Dictionary file not found: {src_path}")
    
    # 复制 pos_dict 目录（如果存在）
    pos_src = os.path.join(dict_source, "pos_dict")
    pos_dst = os.path.join(dict_dest, "pos_dict")
    if os.path.exists(pos_src):
        if not os.path.exists(pos_dst):
            os.makedirs(pos_dst)
        for item in os.listdir(pos_src):
            src_item = os.path.join(pos_src, item)
            dst_item = os.path.join(pos_dst, item)
            if os.path.isfile(src_item):
                try:
                    shutil.copy2(src_item, dst_item)
                    print(f"Copied: {src_item} -> {dst_item}")
                    copied_count += 1
                except Exception as e:
                    print(f"Error copying {item}: {e}")
    
    print(f"Dictionary files copy completed. Total: {copied_count} files")
    return None

# 添加复制字典的构建步骤
dict_copy = env.Command(
    "addons/godot_jieba/dict/.dict_copied",
    Glob("src/thirdparty/cppjieba/dict/*"),
    copy_dict_files
)

# 让库构建依赖于字典复制（确保字典先复制）
env.Depends(library, dict_copy)

# 把字典复制也加入默认目标
Default(dict_copy)

# 添加别名方便单独执行字典复制
env.Alias("dict", dict_copy)
