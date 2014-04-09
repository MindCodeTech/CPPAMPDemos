/**********************************************************************
Copyright �2013 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

�   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
�   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
/**
*******************************************************************************
* @file <Tile_staticStorageBandWidth.hpp>
*
* @Define a class named TileStaticBandwidth,and we will implement the bandwidth
******************************************************************************/

#ifndef TILESTATIC_BANDWIDTH_H_
#define TILESTATIC_BANDWIDTH_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/

#include "AMPUtil.hpp"



/******************************************************************************
* namespace concurrency                                                       *
* namespace std                                                               *
******************************************************************************/
using namespace Concurrency;
using namespace std;
using namespace appsdk;

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define TILE_SIZE 256
#define NUM_ACCESS 256
#define LENGTH 1024 * 1024

#define SAMPLE_VERSION "AMD-APP-SDK-v2.9.233.1"

/******************************************************************************
* TileStaticBandwidth                                                         *
* class implements various resources required by the test                     *
* Publically Inherited from AmpSample                                         *
******************************************************************************/
template<class T>
class TileStaticBandwidth
{
        vector<T> output;                    /**< the output vector */
        unsigned int outputLength;                /**< the length of output vector */
        double kernelRunTime;                   /**< Kernel run time */
        unsigned int
        iter;                        /**< Number of iterations for kernel execution */
        bool isPassed;                            /**< the verify result */
        SDKTimer *sampleTimer;                    /**< Timer object */
    public:
        AMPCommandArgs *sampleArgs;               /**< Command line helper object */
        /**
        **************************************************************************
        * Constructor
        * Initialize member variables
        *************************************************************************/
        TileStaticBandwidth()
        {
            outputLength = LENGTH;
            kernelRunTime = 0;
            iter = 100;
            isPassed = true;
            sampleTimer =  new SDKTimer();
            sampleArgs = new AMPCommandArgs();
            sampleArgs->sampleVerStr = SAMPLE_VERSION;
        }

        /**
        **************************************************************************
        * @brief command line parser, add custom options
        * @return SDK_SUCCESS on success and SDK_FAILURE on failure
        *************************************************************************/
        int initialize();

        /**
        **************************************************************************
        * @breif Allocate and initialize the input vector with random values and
        *  set zero to the output vectors
        * @return SDK_SUCCESS on success and SDK_FAILURE on failed
        *************************************************************************/
        int setup();

        /**
        **************************************************************************
        * @return SDK_SUCCESS on success and SDK_FAILURE on failure
        *************************************************************************/
        int run();

        /**
        **************************************************************************
        * @return SDK_SUCCESS on success and SDK_FAILURE on failure
        *************************************************************************/
        int verifyResults();
    private:

        /**
        **************************************************************************
        * @brief calculate bandwidth function
        * @return SDK_SUCCESS on success and SDK_FAILURE on failure
        *************************************************************************/
        double calBandwidth(double);

        /**
        **************************************************************************
        * @fn readSingle
        * @brief mode: Read single test
        *************************************************************************/
        void readSingle(array<T> &);
        void measureReadSingle(array<T> &);

        /**
        **************************************************************************
        * @fn readLinear
        * @brief mode: Read linear test
        *************************************************************************/
        void readLinear(array<T> &);
        void measureReadLinear(array<T> &);

        /**
        **************************************************************************
        * @fn writeLinear
        * @brief mode: Write linear test
        *************************************************************************/
        void writeLinear(array<T> &);
        void measureWriteLinear(array<T> &);
};


#endif