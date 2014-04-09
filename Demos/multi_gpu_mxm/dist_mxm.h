//////////////////////////////////////////////////////////////////////////////
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: dist_mxm.h
// 
// Interface function declaration
//----------------------------------------------------------------------------

#include <amp.h>
#include <vector>
using namespace concurrency;

void mxm_amp_distribute(std::vector<accelerator_view> acc_views, const unsigned int stream_width, const float*  A, const float* B, float*  C, const unsigned int M, const unsigned int N, const unsigned int W);
