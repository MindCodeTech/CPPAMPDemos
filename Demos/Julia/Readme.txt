QJulia4D

-Overview:
The sample ports a DirectCompute implementation of the Julia 4D fractal coded by Jan Vlietinck from http://users.skynet.be/fquake/ to C++ AMP
and demonstrates how to use C++ AMP short vector types and C++ AMP/DirectX interop for textures

-Hardware requirement:
This sample requires DirectX 11 capable card, if none detected sample will use WARP if available. 
If WARP cannot be detected, then the sample will exit. It does not try to run on REF since this is very slow

-Software requirement:
Install Visual Studio 2012 from http://msdn.microsoft.com

- UI
The title of the window shows the following statistics
	- accelerator used for rendering	
	- frames per second achieved

To change the settings
 > 'mouse wheel'           : Zooms in or out of the fractal
 > 'click and drag mouse'  : Rotates the fractal

 > 'space'  : turn animation on or off
 > 's'      : switch selfshadow on or off
 > 'w'      : Increase MuC.x
 > 'x'      : Decrease MuC.x
 > 'q'      : Increase MuC.y
 > 'z'      : Decrease MuC.y
 > 'a'      : Increase MuC.z
 > 'd'      : Decrease MuC.z
 > 'e'      : Increase MuC.w
 > 'c'      : Decrease MuC.w
 > '+'      : Decrease the epsilon 
 > '-'      : Increase the epsilon 