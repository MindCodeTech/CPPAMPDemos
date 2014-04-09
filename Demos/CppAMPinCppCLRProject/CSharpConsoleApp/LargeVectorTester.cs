//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: LargeVectorTester.cs
// 
// Contains tests for verifying the correctness of the LargeVector class in
// the SampleCLRLibrary.
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using SampleLibrary;

namespace CSharpConsoleApp
{
    /// <summary>
    /// A class that performs tests on the LargeVector API.
    /// </summary>
    static class LargeVectorTester
    {

        private static void GenerateRandomInputs(out int[] vecA, out int[] vecB, int numElements)
        {
            Random rng = new Random();

            vecA = new int[numElements];
            vecB = new int[numElements];

            for (int i = 0; i < numElements; i++)
            {
                vecA[i] = rng.Next();
                vecB[i] = rng.Next();
            }
        }

        private static void GenerateRandomInputs(out float[] vecA, out float[] vecB, int numElements)
        {
            Random rng = new Random();

            vecA = new float[numElements];
            vecB = new float[numElements];

            for (int i = 0; i < numElements; i++)
            {
                vecA[i] = (float)rng.NextDouble();
                vecB[i] = (float)rng.NextDouble();
            }
        }

        private static int ReportItemsNotEqual<T>(T[] expectedResult, T[] actualResult)
            where T: struct, IEquatable<T>
        {
            int numElements = expectedResult.Length;
            System.Diagnostics.Debug.Assert(actualResult.Length == numElements);

            const int maxResultsToLog = 10;
            int wrongResultCnt = 0;
            for (int i = 0; i < numElements; i++)
            {
                if (!actualResult[i].Equals(expectedResult[i]))
                {
                    wrongResultCnt++;
                    if (wrongResultCnt <= maxResultsToLog)
                    {
                        Console.WriteLine("wrong result for vector component {0}. expected = {1}, actual = {2}", i, expectedResult[i], actualResult[i]);
                    }
                }
            }

            return wrongResultCnt;
        }

        public static bool TestAdd_int(int numElements)
        {
            Console.WriteLine("Testing LargeVector.Add with ints...");

            // Create the inputs
            Console.WriteLine("Generating inputs...");
            int[] vecA, vecB;
            GenerateRandomInputs(out vecA, out vecB, numElements);

            // Create array to store the results from our library and set to a sentinal value
            Console.WriteLine("Computing using our sample library using C++ AMP...");
            int[] actualResult = LargeVector.Add(vecA, vecB);

            // Compute the expected result on the CPU component by component
            Console.WriteLine("Computing on the CPU...");
            int[] expectedResult = new int[numElements];
            for (int i = 0; i < numElements; i++)
            {
                expectedResult[i] = vecA[i] + vecB[i];
            }

            // And check our results.
            Console.WriteLine("Verifying result...");
            int wrongResultCnt = ReportItemsNotEqual(expectedResult, actualResult);
            if (wrongResultCnt == 0)
            {
                Console.WriteLine("All results were correct.\n");
            }
            else
            {
                Console.WriteLine("A total of {0} elements produced the wrong result.\n", wrongResultCnt);
            }

            return wrongResultCnt == 0;
        }

        public static bool TestAdd_float(int numElements)
        {
            Console.WriteLine("Testing LargeVector.Add with floats...");

            // Create the inputs
            Console.WriteLine("Generating inputs...");
            float[] vecA, vecB;
            GenerateRandomInputs(out vecA, out vecB, numElements);

            // Create array to store the results from our library and set to a sentinal value
            Console.WriteLine("Computing using our sample library using C++ AMP...");
            float[] actualResult = LargeVector.Add(vecA, vecB);

            // Compute the expected result on the CPU component by component
            Console.WriteLine("Computing on the CPU...");
            float[] expectedResult = new float[numElements];
            for (int i = 0; i < numElements; i++)
            {
                expectedResult[i] = vecA[i] + vecB[i];
            }

            // And check our results.
            Console.WriteLine("Verifying result...");
            int wrongResultCnt = ReportItemsNotEqual(expectedResult, actualResult);
            if (wrongResultCnt == 0)
            {
                Console.WriteLine("All results were correct.\n");
            }
            else
            {
                Console.WriteLine("A total of {0} elements produced the wrong result.\n", wrongResultCnt);
            }

            return wrongResultCnt == 0;
        }

    }
}
