scons platform=windows target=template_debug -j16
scons platform=windows target=template_release -j16
scons platform=android target=template_debug arch=arm64 -j16
scons platform=android target=template_release arch=arm64 -j16