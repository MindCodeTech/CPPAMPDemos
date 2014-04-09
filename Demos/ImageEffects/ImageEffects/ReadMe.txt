ImageEffects

-Overview:
This samples demostrates C++ AMP/DirectX InterOp for textures. 
It also shows how to load a image file into a D3D texture, from which a C++ AMP texture can be created
It implements a few image effects using C++ AMP textures. ImageEffects.cpp/h are the major source files.

-Hardware requirement:
This sample requires DirectX 11 capable card.

-Software requirement:
Install Visual Studio 2012 from http://msdn.microsoft.com

-Note:
In the current default setting, it has "_WIN32_WINNT=0x0601" for
"Configuration Properties -> C/C++ -> Preprocessors -> Preprocessor Definition". 
So the application works for both Windows 7 and Windows 8. If this preprocessor definition is
removed, the application will only work for Windows 8.

- UI
The "Effects" manual allows user to select different effect.
User can also use keystrokes:
 > 'o': original image
 > 's': sobel
 > 'p': prewitt
 > 'c': scharr
 > 'e': embossing
 > 'g': gaussian blur
 > 'h': sharpening
 > 'm': mean removal
 > 'q': quit the app

