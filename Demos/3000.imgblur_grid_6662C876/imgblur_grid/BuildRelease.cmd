@REM -- imgblur_grid command-line basic Release build, without precompiled headers

@SETLOCAL
@IF NOT "%ProgramFiles(x86)%" == "" SET ProgramFiles=%ProgramFiles(x86)%

@SET AMP="%ProgramFiles%\Microsoft C++ AMP Prerelease"

@MKDIR Release 2> nul

%AMP%\bin\cl /I%AMP%\include /EHsc /Fo"Release\imgblur.obj" /c imgblur.cpp
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

%AMP%\bin\link /libpath:%AMP%\lib /out:Release\imgblur_grid.exe Release\imgblur.obj
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

COPY /y %AMP%\runtime\Release\* Release
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%