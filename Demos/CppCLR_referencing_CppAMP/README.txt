This sample shows a template Visual Studio solution for using C++ AMP from within a C++/CLI project.
It's intent is to simply show how to setup the projects and references and not neccessarily to
show the efficient ways to copy data between the managed and native memory spaces.


Project SampleAMPLibrary:
This project is a native Win32 dll project which exports a simple example API for processing
data. The implementation of the API makes use of C++ AMP.

The APIs exposed in this library just use pointers to data rather than any smarter structures
as our intent is to show a simple example.


Project CLRConsoleApp:
This is a CLR console application that references SampleAMPLibrary. It's simple main() function
shows how to initialize data for use in our SampleAMPLibrary APIs. This example shows one possible
way to pass pinned managed array pointers down to our native library.


