@echo off
rmdir /s /q ipch 2> nul
rmdir /s /q Debug 2> nul
rmdir /s /q Release 2> nul
attrib -h -s -r *.suo > nul
del *.suo 2> nul
del *.sdf 2> nul
del *.vcxproj.user 2> nul
