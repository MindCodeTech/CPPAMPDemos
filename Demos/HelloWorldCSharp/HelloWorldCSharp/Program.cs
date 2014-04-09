using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HelloWorldCSharp
{
    class Program
    {
        /// <summary>
        /// Function defined in HelloWorldLib.dll to square an array using C++ AMP
        /// </summary>
        [DllImport("HelloWorldLib", CallingConvention = CallingConvention.StdCall)]
        extern unsafe static void square_array(float* array, int length);

        static unsafe void Main()
        {
            // Allocate an array
            float[] arr = new[] { 1.0f, 2.0f, 3.0f, 4.0f };

            // Square the array elements using C++ AMP
            fixed (float* arrPt = &arr[0])
            {
                square_array(arrPt, arr.Length);
            }

            // Enumerate the results
            foreach (var x in arr)
            {
                Console.WriteLine(x);
            }
        }
    }
}
