This sample shows a template Visual Studio solution for adding native C++ AMP code to a
C++ CLR Class Library project. It's intent is to simply show how to setup the projects,
files and references and not neccessarily to show the efficient ways to copy data between
the managed and native memory spaces.


Project SampleCLRLibrary:
This project is a mixed (native and managed) assembly project which exposes a simple
example API for adding large vectors. The implementation of the API makes use of C++ AMP.


Project CSharpConsoleApp:
This is a C# console application that references SampleCLRLibrary. This program executes
a test method that verifies the correctness of the SampleCLRLibrary.

