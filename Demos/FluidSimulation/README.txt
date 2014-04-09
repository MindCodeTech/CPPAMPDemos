Fluid simulation sample

-Overview:
The C++ AMP fluid simulation sample simulates fluid dynamics in real time.

-Hardware requirements:
This sample requires a DirectX 11 capable card. If there is not a DirectX 11 capable card in the system, the sample will use DirectX 11 reference emulator with very low performance.

-Software requirements:
Install June 2010 DirectX SDK from MSDN http://www.microsoft.com/download/en/details.aspx?id=6812
Install Visual Studio 2012 from http://msdn.microsoft.com

-Running the sample:
This sample contains the FluidSimulation project which builds a graphical sample displaying the fluid dynamic in a DirectX rendering window.  This sample uses the same DXUT framework and rendering path as the DirectX SDK sample only modifying the compute portions of the sample to use C++ AMP.

-References:
DirectX June 2010 SDK Fluid simulation sample $(DXSDK)\Samples\C++\Direct3D11\FluidCS11

-Known issues:
1. Macro redefinition warning/errors.
    This is due to conflict between DirectX SDK version and Visual Studion installed SDK. This shouldn't affect the sample.