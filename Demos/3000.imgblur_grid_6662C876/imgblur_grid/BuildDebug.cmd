@REM -- imgblur_grid command-line basic Debug build, without precompiled headers

@SETLOCAL
@IF NOT "%ProgramFiles(x86)%" == "" SET ProgramFiles=%ProgramFiles(x86)%

@SET AMP="%ProgramFiles%\Microsoft C++ AMP Prerelease"

@MKDIR Debug 2> nul

%AMP%\bin\cl /D_DEBUG /I%AMP%\include /EHsc /Fo"Debug\imgblur.obj" /c imgblur.cpp
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

%AMP%\bin\link /debug /libpath:%AMP%\lib /out:Debug\imgblur_grid.exe Debug\imgblur.obj /PDB:Debug\imgblur.pdb /nodefaultlib:libcmt.lib libcmtd.lib /nodefaultlib:libcpmt.lib libcpmtd.lib
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

COPY /y %AMP%\runtime\Debug\* Debug
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%