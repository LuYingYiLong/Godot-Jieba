<p align="center">
  <img alt="JiebaBanner" src="https://github.com/user-attachments/assets/d4d02802-9ec4-439d-9dc5-c227a37354d2" />
</p>

<h1 align="center">Godot-Jieba</h1>

<p align="center">
  <b>Godot 中文分词器，基于 CppJieba 的 GDExtension 封装</b>
</p>

## 简介

Godot-Jieba 为 Godot 4 提供中文分词、词性标注、关键词提取和用户词典编辑能力。项目使用 C++20、godot-cpp 和 CppJieba，适合在游戏文本搜索、对白分析、中文输入辅助、内容标签生成、编辑器内词典维护等场景中使用。

这个仓库内的 CppJieba 是项目 fork 版本，额外加入了从内存加载词典的接口。这样 Godot 导出后不必依赖原始 `.utf8` 文件能被引擎直接识别，扩展可以通过 Godot 的 `FileAccess` / `ResourceLoader` 读取词典内容，再交给 CppJieba 初始化。

## 功能

- 精确分词：`cut()`
- 全模式分词：`cut_all()`
- 搜索引擎模式：`cut_for_search()`
- HMM 分词：`cut_hmm()`
- 词性标注：`tag()` / `tag_pairs()`
- 关键词提取：`extract_keywords()`
- 用户词典增删查：`add_word()`、`add_word_with_freq()`、`remove_word()`、`find_word()`
- 用户词典加载：`load_user_dict()`
- `.utf8` / `.gbk` 自定义导入器
- 编辑器底部面板：用户词典编辑、格式校验、分词预览、词性预览、关键词预览

## Godot 版本

当前项目面向 Godot 4.x GDExtension，`godot-cpp` 子模块使用 `4.5` 分支。仓库内的 `.gdextension` 配置位于：

```text
addons/godot_jieba/bin/godot_jieba.gdextension
```

Windows debug 构建产物默认输出到：

```text
addons/godot_jieba/bin/godot_jieba.windows.template_debug.x86_64.dll
```

## 快速使用

在 GDScript 中创建并初始化分词器：

```gdscript
var jieba: JiebaSegment = JiebaSegment.new()

if jieba.initialize("res://addons/godot_jieba/dict"):
	var words := jieba.cut("小明硕士毕业于中国科学院计算所，后在日本京都大学深造")
	print(words)
```

不传路径时，扩展会尝试自动查找：

```gdscript
jieba.initialize()
```

默认优先查找：

```text
res://addons/godot_jieba/dict
```

## API 示例

### 分词

```gdscript
var words := jieba.cut("我来到北京清华大学")
var search_words := jieba.cut_for_search("小明硕士毕业于中国科学院计算所")
var all_words := jieba.cut_all("我来到北京清华大学")
var hmm_words := jieba.cut_hmm("他来到了网易杭研大厦")
```

### 词性标注

```gdscript
var tags := jieba.tag_pairs("我是拖拉机学院手扶拖拉机专业的")
for item in tags:
	print(item) # word/tag
```

`tag()` 会返回 `Dictionary`，同一个词多次出现时后面的值会覆盖前面的值。需要保留顺序时请使用 `tag_pairs()`。

### 关键词提取

```gdscript
var keywords := jieba.extract_keywords("我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪。", 5)
for item in keywords:
	print(item["word"], item["weight"], item["offsets"])
```

### 动态用户词

```gdscript
jieba.add_word("云计算", "n")
jieba.add_word_with_freq("男默女泪", 100, "nz")
jieba.remove_word("云计算")
```

## 词典文件

默认词典位于：

```text
addons/godot_jieba/dict
```

需要的基础文件：

```text
jieba.dict.utf8
hmm_model.utf8
user.dict.utf8
idf.utf8
stop_words.utf8
```

同名 `.gbk` 文件也可以被识别，例如：

```text
jieba.dict.gbk
user.dict.gbk
```

扩展初始化时会优先尝试 `.utf8`，找不到时再尝试 `.gbk`。GBK 内容会在 Godot 侧转换成 UTF-8 后再交给 CppJieba。

## 用户词典格式

用户词典每行一条记录，支持 CppJieba 的三种格式：

```text
词语
词语 词性
词语 词频 词性
```

示例：

```text
云计算 n
男默女泪 100 nz
蓝翔 nz
```

注意：

- 列之间使用空格分隔。
- 不支持行内注释。
- 不支持额外列。
- 三列格式中的第二列必须是整数词频。

## 编辑器面板

启用 GDExtension 后，Godot 编辑器底部会出现 `Jieba` 面板。

面板提供：

- 字典目录初始化
- 用户词典加载
- 用户词典保存
- 用户词典格式校验
- 分词模式预览：`Mix`、`MP`、`HMM`、`Full`、`Query`
- HMM 开关
- 词性标注预览
- 关键词提取预览

保存用户词典后，面板会重新初始化 `JiebaSegment`，确保预览结果使用最新词典。

## UTF-8 / GBK 导入器

项目注册了自定义导入器：

```text
godot_jieba.dictionary_text
```

识别扩展名：

```text
.utf8
.gbk
```

导入后会生成 `JiebaUTF8` 资源，内部保存 UTF-8 文本。导入选项包括：

- `encoding`: `Auto`、`UTF-8`、`GBK`
- `store_source_data`: 是否保存原始字节

这条导入链主要用于解决 Godot 对 `.utf8` / `.gbk` 词典文件的识别和导出问题。

## 构建

初始化子模块：

```powershell
git submodule update --init --recursive
```

Windows debug 构建：

```powershell
scons platform=windows target=template_debug -j4
```

Windows release 构建：

```powershell
scons platform=windows target=template_release -j4
```

构建完成后，SCons 会把 CppJieba 词典复制到：

```text
addons/godot_jieba/dict
```

## 子模块说明

本项目包含两个主要子模块：

```text
godot-cpp
src/thirdparty/cppjieba
```

`cppjieba` 指向项目 fork：

```text
https://github.com/LuYingYiLong/cppjieba.git
```

如果修改了 `src/thirdparty/cppjieba`，需要先在子模块内部提交，再回到父仓库提交子模块指针：

```powershell
git -C src/thirdparty/cppjieba status
git -C src/thirdparty/cppjieba add .
git -C src/thirdparty/cppjieba commit -m "Describe cppjieba change"

git add src/thirdparty/cppjieba
git commit -m "Update cppjieba submodule"
```

推送时先推子模块，再推父仓库：

```powershell
git -C src/thirdparty/cppjieba push
git push
```

否则父仓库会记录一个其他人无法获取的子模块 commit。

## 项目结构

```text
addons/godot_jieba/bin              GDExtension 配置和构建产物
addons/godot_jieba/dict             默认词典
src/jieba_segment.*                 Godot 暴露的 JiebaSegment API
src/jieba_utf8.*                    导入后的 UTF-8 词典资源
src/resource_importer_jieba.*       .utf8 / .gbk 导入器
src/jieba_editor_plugin.*           编辑器插件入口
src/jieba_editor_dock.*             Jieba 编辑器面板
src/thirdparty/cppjieba             CppJieba fork 子模块
godot-cpp                           Godot C++ 绑定
```

## 许可证

本项目自身代码请以仓库许可证为准。`godot-cpp`、`cppjieba` 及其依赖遵循各自上游项目许可证。
