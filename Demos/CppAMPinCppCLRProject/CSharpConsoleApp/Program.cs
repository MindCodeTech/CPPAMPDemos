//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: Program.cs
// 
// Defines the entry point for this console program.
// This program just runs tests for the SampleCLRLibrary.
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CSharpConsoleApp
{
    class Program
    {
        static int Main(string[] args)
        {
            bool passed = true;
            try
            {
                passed &= LargeVectorTester.TestAdd_int(65536 / sizeof(int)); // Use 64KB per vector
                passed &= LargeVectorTester.TestAdd_float(65536 / sizeof(float)); // Use 64KB per vector
            }
            catch (System.Runtime.InteropServices.ExternalException ex)
            {
                // These exceptions are unhandled exceptions occuring in native code
                // Once in the managed runtime all C++ specific type information is lost.
                Console.Error.WriteLine("Unhandled {0} exception caught (HRESULT = 0x{2:x8}: {1}", ex.GetType().Name, ex.Message, ex.ErrorCode);
                passed = false;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("Unhandled {0} exception caught: {1}", ex.GetType().Name, ex.Message);
                passed = false;
            }

            if (!passed)
            {
                Console.Write("Press any key to continue...");
                Console.ReadKey(true);
            }

            return passed ? 0 : 1;
        }
    }
}
