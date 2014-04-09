//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corp. 
//
// File: ps.hlsl
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this 
// file except in compliance with the License. You may obtain a copy of the License at 
// http://www.apache.org/licenses/LICENSE-2.0  
//  
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR 
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//  
// See the Apache Version 2.0 License for specific language governing permissions and 
// limitations under the License.
//--------------------------------------------------------------------------------------
#include "inc.hlsli"
Texture2D<unorm float4> txDiffuse : register( t0 );
SamplerState samp : register( s0 );
float4 main( PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample( samp, input.Tex );
}
