N-Body simulation sample

-Overview:
The C++ AMP N-Body simulation sample simulates a dynamic particle system of (n) bodies where each body exerts a gravitational force on all other bodies in the simulation.

-Hardware requirement:
This sample requires DirectX 11 capable card, if none detected sample will use DirectX 11 Reference Emulator.

-Software requirement:
Install June 2010 DirectX SDK from MSDN http://www.microsoft.com/download/en/details.aspx?id=6812
Install Visual Studio 2012 from http://msdn.microsoft.com

-Running sample:
This sample contains NBodyDX project which builds a graphical sample which displays the N-Body simulation in a DirectX rendering window.  This sample uses the same DXUT framework and rendering path as the DirectX SDK sample only modifying the compute portions of the sample to use C++ AMP.

-References:
http://en.wikipedia.org/wiki/N-body_simulation
DirectX August 2009 SDK N-Body sample $(DXSDK)\Samples\C++\Direct3D11\NBodyGravityCS11

Known issue:
1. Macro redefinition warning/errors.
    This is due to conflict between DirectX SDK version and Visual Studion installed SDK. This shouldnt affect the sample.


File layout needed to run this app:

 Directory of <WorkingDir>

05/16/2011  03:20 PM           641,024 NBodyGravityCS11.exe
05/16/2011  03:20 PM         4,287,488 NBodyGravityCS11.pdb
05/11/2010  10:56 AM             2,787 ParticleDraw.hlsl

 Directory of <WorkingDir>\misc

05/13/2011  06:35 PM    <DIR>          .
05/13/2011  06:35 PM    <DIR>          ..
04/19/2010  05:56 AM           262,272 particle.dds
               1 File(s)        262,272 bytes

 Directory of <WorkingDir>\UI

05/13/2011  06:35 PM    <DIR>          .
05/13/2011  06:35 PM    <DIR>          ..
04/19/2010  05:56 AM            23,227 arrow.x
04/19/2010  05:56 AM           262,272 dxutcontrols.dds
04/19/2010  05:56 AM             2,594 DXUTShared.fx
04/19/2010  05:56 AM            76,128 Font.dds
               4 File(s)        364,221 bytes

