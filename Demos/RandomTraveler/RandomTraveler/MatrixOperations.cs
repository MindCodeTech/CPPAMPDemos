//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MatrixOperations.cs
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RandomTraveler
{
    internal class MatrixOperations
    {
        [DllImport("MatrixOperationsCppAmp.dll", CallingConvention=CallingConvention.StdCall)]
        private unsafe extern static string MultiplyMatrix(float* vA, float* vB, float* vC, int M, int N, int W);

        public delegate void MultiplyAction(float[,] a, float[,] b, float[,] c);

        /// <summary>
        /// Multiply matrix on CPU - single thread
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="c"></param>
        public static void MultiplyMatrixCpu(float[,] a, float[,] b, float[,] c)
        {
            int dim1 = a.GetLength(0);
            int dim2 = b.GetLength(1);
            int dim3 = a.GetLength(1);

            for (int i = 0; i < dim1; i++)
            {
                for (int j = 0; j < dim2; j++)
                {
                    float sum = 0;
                    for (int k = 0; k < dim3; k++)
                    {
                        sum = sum + a[i,k] * b[k,j];
                    }
                    c[i, j] = sum;
                }
            };
        }

        /// <summary>
        /// Multiply matrix on CPU - multiple thread
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="c"></param>
        public unsafe static void MultiplyMatrixTpl(float[,] a, float[,] b, float[,] c)
        {
            int dim1 = a.GetLength(0);
            int dim2 = b.GetLength(1);
            int dim3 = a.GetLength(1);

            fixed (float* aPt = &a[0, 0])
            fixed (float* bPt = &b[0, 0])
            {
                IntPtr aPtI = (IntPtr)aPt, bPtI = (IntPtr)bPt;
                Parallel.For(0, dim1, i =>
                {
                    float* aPtCopy = (float*)aPtI, bPtCopy = (float*)bPtI;
                    for (int j = 0; j < dim2; j++)
                    {
                        float sum = 0;
                        for (int k = 0; k < dim3; k++)
                        {
                            sum = sum + *(aPtCopy + i * dim3 + k) * *(bPtCopy + (k * dim2 + j));
                        }
                        c[i, j] = sum;
                    }
                });
            }
        }

        /// <summary>
        /// Multiply the matric on accelerator using C++ AMP
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="c"></param>
        public unsafe static void MultiplyMatrixGpu(float[,] a, float[,] b, float[,] c)
        {
            fixed (float* aPt = &a[0, 0])
            fixed (float* bPt = &b[0, 0])
            fixed (float* cPt = &c[0, 0])
            {                
               string msg = MultiplyMatrix(aPt, bPt, cPt, c.GetLength(0), c.GetLength(1), a.GetLength(1));
                
                if (!string.IsNullOrEmpty(msg)) { throw new CppAmpException(msg); }
            }
        }

        /// <summary>
        /// Multiply the matrix given number times
        /// </summary>
        /// <param name="matrix"></param>
        /// <param name="exponent">Number of times to multiply</param>
        /// <param name="multiplyMatrix"></param>
        /// <param name="mmCount"></param>
        /// <param name="flopCount"></param>
        /// <returns></returns>
        public static float[,] MatrixExp(float[,] matrix, long exponent, MultiplyAction multiplyMatrix, ref long mmCount, ref long flopCount)
        {
            if (matrix.GetLength(0) != matrix.GetLength(1))
            {
                throw new ArgumentException("matrix");
            }

            if (exponent < 0)
            {
                throw new ArgumentException("exponent");
            }

            int dim = matrix.GetLength(0);
            float[,] helper = new float[dim, dim];

            float[,] result = new float[dim, dim];
            bool resultIsIdentity = true;

            for (int i = 0; i < dim; i++)
            {
                result[i, i] = 1;
            }

            float[,] matrixPower = new float[dim, dim];
            Array.Copy(matrix, matrixPower, matrix.Length);

            while(true)
            {
                if ((exponent & 1) == 1)
                {
                    if (resultIsIdentity)
                    {
                        resultIsIdentity = false;
                        Array.Copy(matrixPower, result, result.Length);
                    }
                    else
                    {
                        multiplyMatrix(result, matrixPower, helper);
                        mmCount++;
                        Swap(ref result, ref helper);
                    }
                }

                exponent >>= 1;
                if (exponent == 0) break;

                multiplyMatrix(matrixPower, matrixPower, helper);
                mmCount++;
                Swap(ref matrixPower, ref helper);
            }

            flopCount += mmCount * (2L * dim * dim * dim);

            return result;
        }

        private static void Swap(ref float[,] a, ref float[,] b)
        {
            var tmp = b;
            b = a;
            a = tmp;
        }
    }

    internal class CppAmpException : Exception
    {
        public CppAmpException(string msg) : base(msg) { }
    }
}
