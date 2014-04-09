/**********************************************************************
Copyright ©2013 Advanced Micro Devices, Inc. All rights reserved.

C++ AMP kernels within this source tree are derivatives of kernels
from the SHOC project. Source or binary distribution of this project must
disclose derivation and include the SHOC license:

SHOC 1.1.2  license Copyright ©2011, UT-Battelle, LLC. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
•	Neither the name of Oak Ridge National Laboratory, nor UT-Battelle, LLC, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

/******************************************************************************
* @file <Stencil2D.h>
* @Define a class named Stencil2D, and implements the Stencil2D arithmetic
******************************************************************************/
#ifndef  STENCIL2D_H
#define  STENCIL2D_H

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define  DEFAULITMATRIXEDGE 4096 //default matrixEdge 4096

#define SAMPLE_VERSION "AMD-APP-SDK-v2.9.233.1"

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <amp.h>
#include <iostream>
#include <string.h>

#include "AMPUtil.hpp"

#include <random>

/******************************************************************************
* namespace std Concurrency                                                   *
******************************************************************************/
using namespace concurrency;
using namespace appsdk;



/**
****************************************************************************
* Stencil2D
* Class implements C++ AMP Stencil2D sample
***************************************************************************/
template <class T>
class Stencil2D 
{
private:
    int              matrixEdge;    /**< the length of each dimension for inMatrix and outMatrix */
    int        paddedMatrixEdge;    /**< pad the edge of the matrix*/
    int            nMatrixElems;    /**< pad the matrix except edge*/
    int               iteration;    /**< kernel execute times */
    T              centerWeight;    /**< center weight of the matrix*/
    T            cardinalWeight;    /**< cardinal weight of the matrix*/
    T            diagonalWeight;    /**< diagonal weight of the matrix*/

    std::vector<T>        inVec;    /**< input vector*/
    std::vector<T>       outVec;    /**< output vector */


    double      totalKernelTime;    /**< total kernel execute time */
    double    averageKernelTime;    /**< average kernel execute time */
    double       cpToDeviceTime;    /**< The transfer time between host and gpu */
    double         cpToHostTime;    /**< The transfer time between gpu and host */
    bool              arrayview;    /**< array/array_view flag */

    SDKTimer *sampleTimer;                     /**< Timer object */
public:
    AMPCommandArgs *sampleArgs;               /**< Command line helper object */

     /** 
     ****************************************************************************
     * Constructor 
     * Initialize member variables
     ***************************************************************************/
    Stencil2D()
    {
        matrixEdge = DEFAULITMATRIXEDGE;
        paddedMatrixEdge = matrixEdge +2;
        nMatrixElems = paddedMatrixEdge * paddedMatrixEdge;

        cpToDeviceTime = 0;
        cpToHostTime = 0;
        averageKernelTime = 0;
        totalKernelTime = 0;

        iteration = 10;
        arrayview = false;
		sampleTimer =  new SDKTimer();
        sampleArgs = new AMPCommandArgs();
        sampleArgs->sampleVerStr = SAMPLE_VERSION;
    }

     /**
     *****************************************************************************
     * @brief initialize  
     * Override from SDKSample. Initialize 
     * command line parser, add custom options
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *****************************************************************************/
    int initialize();

     /** 
     ******************************************************************************
     * @brief setup
     * Override from SDKSample
     * Allocate and initialize the input vector with random values and 
     * set zero to the output vectors
     * @return SDK_SUCCESS on success and SDK_FAILURE on failed
     ******************************************************************************/
    int setup();

     /**
     *******************************************************************************
     * @brief the compute kernel with array mode. stencil
     * Function running on amp
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int stencil(array<T,2>& inMatrix, array<T,2>& outMatrix, T cW,T carW,T dW);

     /**
     *******************************************************************************
     * @brief the compute kernel with array_view mode. stencil
     *    overload Function running on amp
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int stencil(array_view<T,2>& inMatrix, array_view<T,2>& outMatrix, T cW,T carW,T dW);

     /**
     *******************************************************************************
     * @brief calls runArray() or runArrayView()
     * Override from SDKSample
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int run();

     /**
     *******************************************************************************
     * @brief run array mode, not zerocopy
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int runArray();

     /**
     *******************************************************************************
     * @brief run array_view mode,  zerocopy
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int runArrayView();

     /**
     *******************************************************************************
     * @brief override from SDKSample
     * Verify against reference implementation
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int verifyResults();

     /**
     *******************************************************************************
     * @brief Print the results from the test
     * Print the results on the screen according to commands.
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     *******************************************************************************/
    int printStatus();

    /**
    ********************************************************************************
    * @brief Print the array of input and output
    ********************************************************************************/
    void printArray(std::string header, const std::vector<T>& data, const int width, const int height, const int offset);
};
#endif