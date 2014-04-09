//--------------------------------------------------------------------------------------
// File: julia4DAMP.cpp
//
// Copyright (c) 2012, Microsoft
//  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following 
// conditions are met:
//  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
//    disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// DirectCompute version coded by Jan Vlietinck, 11 Oct 2009, V 1.4
// http://users.skynet.be/fquake/
// Ported from http://www.cs.caltech.edu/~keenan/project_qjulia.html
// The original Cg code from Keenan Crane is slightly adapted, like intersectQJulia() which would otherwise hang in HLSL
// also the diffuse color is now continuously changed
//
//--------------------------------------------------------------------------------------

#include "julia4DAMP.h"

using namespace concurrency;
using namespace concurrency::fast_math;
using namespace concurrency::graphics;
using namespace concurrency::graphics::direct3d;

float dot(const float3& q1, const float3& q2) restrict(amp)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}

float dot(const float4& q1, const float4& q2) restrict(amp)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

float length(const float4& f) restrict(amp)
{
	return sqrtf(dot(f,f));
}

float length(const float3& f) restrict(amp)
{
    return sqrtf(dot(f,f));
}

float4 abs(const float4& f) restrict(amp)
{
	return float4(fabsf(f.x), fabsf(f.y), fabsf(f.z), fabsf(f.w));
}

float3 abs(const float3& f) restrict(amp)
{    
    return float3(fabsf(f.x), fabsf(f.y), fabsf(f.z));
}

float4 normalize(const float4& f) restrict(amp)
{
	float l = rsqrtf(dot(f,f));    
    return f * l;
}

float3 normalize(const float3& f) restrict(amp)
{
	float l = rsqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    return f * l;
}

// multiplies short vector m1 with matrix m2
float3 mul(const float3& m1, const float* m2) restrict(amp)
{
    float3 result;
 
	result.x = m1.x * m2[0] + m1.y * m2[1] + m1.z * m2[2];
    result.y = m1.x * m2[4] + m1.y * m2[4 + 1] + m1.z * m2[4 + 2];
    result.z = m1.x * m2[8] + m1.y * m2[8 + 1] + m1.z * m2[8 + 2];

    return result;
}

// Some constants used in the ray tracing process.  (These constants
// were determined through trial and error and are not by any means
// optimal.)

#define BOUNDING_RADIUS_2  3.0f		// radius of a bounding sphere for the set used to accelerate intersection

#define ESCAPE_THRESHOLD   10		// any series whose points' magnitude exceed this threshold are considered
                                    // divergent

#define DEL                1e-4     // delta is used in the finite difference approximation of the gradient
                                    // (to determine normals)
#define ITERATIONS         10


// --------- quaternion representation -----------------------------------------
//
// Each quaternion can be specified by four scalars q = A + Bi + Cj + Dk, so are
// stored as a float4.  I've tried a struct containing a separate scalar and
// 3-vector to avoid a lot of swizzling, but the float4 representation ends up
// using fewer instructions.  A matrix representation is also possible.
//


// -------- quatMult() ----------------------------------------------------------
//
// Returns the product of quaternions q1 and q2.
// Note that quaternion multiplication is NOT commutative (i.e., q1 ** q2 != q2 ** q1 ).
//
float4 quatMult(const float4& q1, const float4& q2 ) restrict(amp)
{
	float4 r;

	r.x   = q1.x*q2.x - dot( q1.yzw, q2.yzw );
	r.y = q1.x * q2.y + q1.y * q2.x + q1.z * q2.w - q1.w * q2.z;
	r.z = q1.x * q2.z - q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
	r.w = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;

	return r;
}

// ------- quatSq() --------------------------------------------------------------
//
// Returns the square of quaternion q.  This function is a special (optimized)
// case of quatMult().
//
float4 quatSq(const float4& q ) restrict(amp)
{
	float4 r;

	r.x   = q.x*q.x - dot( q.yzw, q.yzw );
	r.yzw = 2*q.x*q.yzw;
   
	return r;
}

// ----------- normEstimate() -------------------------------------------------------
//
// Create a shading normal for the current point.  We use an approximate normal of
// the isosurface of the potential function, though there are other ways to
// generate a normal (e.g., from an isosurface of the potential function).
//

float3 normEstimate(const float3& p, const float4& c) restrict(amp)
{
	float3 N;
	float4 qP = float4( p.x, p.y, p.z, 0.0f );
	float gradX, gradY, gradZ;

	float4 gx1 = qP - float4( DEL, 0, 0, 0 );
	float4 gx2 = qP + float4( DEL, 0, 0, 0 );
	float4 gy1 = qP - float4( 0, DEL, 0, 0 );
	float4 gy2 = qP + float4( 0, DEL, 0, 0 );
	float4 gz1 = qP - float4( 0, 0, DEL, 0 );
	float4 gz2 = qP + float4( 0, 0, DEL, 0 );

	for( int i=0; i<ITERATIONS; i++ )
	{
		gx1 = quatSq( gx1 ) + c;
		gx2 = quatSq( gx2 ) + c;
		gy1 = quatSq( gy1 ) + c;
		gy2 = quatSq( gy2 ) + c;
		gz1 = quatSq( gz1 ) + c;
		gz2 = quatSq( gz2 ) + c;
	}

	gradX = length(gx2) - length(gx1);
	gradY = length(gy2) - length(gy1);
	gradZ = length(gz2) - length(gz1);

	N = normalize(float3( gradX, gradY, gradZ ));

	return N;
}


// ------- iterateIntersect() -----------------------------------------------------
//
// Iterates the quaternion q for the purposes of intersection.  This function also
// produces an estimate of the derivative at q, which is required for the distance
// estimate.  The quaternion c is the parameter specifying the Julia set, and the
// integer maxIterations is the maximum number of iterations used to determine
// whether a point is in the set or not.
//
// To estimate membership in the set, we recursively evaluate
//
// q = q*q + c
//
// until q has a magnitude greater than the threshold value (i.e., it probably
// diverges) or we've reached the maximum number of allowable iterations (i.e.,
// it probably converges).  More iterations reveal greater detail in the set.
// 
// To estimate the derivative at q, we recursively evaluate
//
// q' = 2*q*q'
//
// concurrently with the evaluation of q.
//

// ---------- intersectQJulia() ------------------------------------------
//
// Finds the intersection of a ray with origin rO and direction rD with the
// quaternion Julia set specified by quaternion constant c.  The intersection
// is found using iterative sphere tracing, which takes a conservative step
// along the ray at each iteration by estimating the minimum distance between
// the current ray origin and the closest point in the Julia set.  The
// parameter maxIterations is passed on to iterateIntersect() which determines
// whether the current ray origin is in (or near) the set.


float intersectQJulia( float3& rO, float3 rD, float4 c, float epsilon ) restrict(amp)
{
	// the (approximate) distance between the first point along the ray within
	// epsilon of some point in the Julia set, or the last point to be tested if
	// there was no intersection.
	float dist; 
	float rd = 0.0f;
	dist = epsilon;

	// Intersection testing finishes if we're close enough to the surface
	// (i.e., we're inside the epsilon isosurface of the distance estimator
	// function) or have left the bounding sphere.          
	float bd = BOUNDING_RADIUS_2;
	while ( dist >= epsilon && rd < bd)
	{
		// iterate on the point at the current ray origin.  We
		// want to know if this point belongs to the set.
		float4 z = float4( rO.x, rO.y, rO.z, 0.0f );         
                                          
		// start the derivative at real 1.  The derivative is
		// needed to get a lower bound on the distance to the set.
		float4 zp = float4( 1.0f, 0.0f, 0.0f, 0.0f );                                             
		float zd = 0.0f;
		uint count = 0;
      
		// iterate this point until we can guess if the sequence diverges or converges.        
		// iterateIntersect()      
		while(zd < ESCAPE_THRESHOLD && count < ITERATIONS)
		{
			zp = 2.0f * quatMult(z, zp);
			z = quatSq(z) + c;
			zd = dot(z, z);
			count++;
		}

		// find a lower bound on the distance to the Julia set and step this far along the ray.
		float normZ = length( z );
		//lower bound on distance to surface
		dist = 0.5f * normZ * logf( normZ ) / length( zp );        
		rO += rD * dist;  // (step)      
		rd = dot(rO, rO);
	}

	// return the distance for this ray
	return dist;
}

// ----------- Phong() --------------------------------------------------
//
// Computes the direct illumination for point pt with normal N due to
// a point light at light and a viewer at eye.
//
float3 Phong( float3 light, float3 eye, float3 pt, float3 N, const QJulia4DConstants& mc  ) restrict(amp)
{
	float3 diffuse = float3( 1.00f, 0.45f, 0.25f );	// base color of shading
	const float specularExponent = 10;				// shininess of shading
	const float specularity = 0.45f;					// amplitude of specular highlight

	float3 L     = normalize( light - pt );		// find the vector to the light
	float3 E     = normalize( eye   - pt );		// find the vector to the eye
	float  NdotL = dot( N, L );					// find the cosine of the angle between light and normal
	float3 R     = L - 2.0f * NdotL * N;				// find the reflected vector   

	float3 c_diffuse = mc.diffuse.xyz;

	// add some of the normal to the
	// color to make it more interesting
	diffuse = float3(c_diffuse + abs( N ) * 0.3f);  

	// compute the illumination using the Phong equation
	return diffuse * fmaxf( NdotL, 0.0f ) + specularity*powf( fmaxf(dot(E,R),0.0f), specularExponent );
}

// ---------- intersectSphere() ---------------------------------------
//
// Finds the intersection of a ray with a sphere with statically
// defined radius BOUNDING_RADIUS centered around the origin.  This
// sphere serves as a bounding volume for the Julia set.
float3 intersectSphere( float3 rO, float3 rD ) restrict(amp)
{
	float B, C, d, t0, t1, t;

	B = 2 * dot( rO, rD );
	C = dot( rO, rO ) - BOUNDING_RADIUS_2;   
	d = sqrtf( B*B - 4*C );
	t0 = ( -B + d ) * 0.5f;
	t1 = ( -B - d ) * 0.5f;
	t = fminf( t0, t1 );
	rO += t * rD;   

	return rO;
}

// ------------ main kernel -------------------------------------------------
//
//  Each fragment performs the intersection of a single ray with
//  the quaternion Julia set.  In the current implementation
//  the ray's origin and direction are passed in on texture
//  coordinates, but could also be looked up in a texture for a
//  more general set of rays.
//
//  The overall procedure for intersection performed in main() is:
//
//  -move the ray origin forward onto a bounding sphere surrounding the Julia set
//  -test the new ray for the nearest intersection with the Julia set
//  -if the ray does include a point in the set:
//      -estimate the gradient of the potential function to get a "normal"
//      -use the normal and other information to perform Phong shading
//      -cast a shadow ray from the point of intersection to the light
//      -if the shadow ray hits something, modify the Phong shaded color to represent shadow
//  -return the shaded color if there was a hit and the background color otherwise
//
float4 QJulia(float3 rO ,                // ray origin
              float3 rD ,                // ray direction (unit length)
              float4 mu,                    // quaternion constant specifying the particular set
              float epsilon,                // specifies precision of intersection       			  
              float3 light,                 // location of a single point light
              bool renderShadows,           // flag for turning self-shadowing on/off			                
			  const QJulia4DConstants& mc)
			  restrict(amp)
{

	const float4 backgroundColor = float4( 0.3f, 0.3f, 0.3f, 0.0f );  //define the background color of the image
	float4 color;  // This color is the final output of our program.

   // Initially set the output color to the background color.  It will stay
   // this way unless we find an intersection with the Julia set.   
   color = backgroundColor;
   
   // First, intersect the original ray with a sphere bounding the set, and
   // move the origin to the point of intersection.  This prevents an
   // unnecessarily large number of steps from being taken when looking for
   // intersection with the Julia set.
   rD = normalize( rD );  //the ray direction is interpolated and may need to be normalized
   rO = intersectSphere( rO, rD );

   // Next, try to find a point along the ray which intersects the Julia set.
   // (More details are given in the routine itself.)   
   float dist = intersectQJulia(rO, rD, mu, epsilon );   
   
   // We say that we found an intersection if our estimate of the distance to
   // the set is smaller than some small value epsilon.  In this case we want
   // to do some shading / coloring.   
   if( dist > 0 && dist < epsilon )
   {
		// Determine a "surface normal" which we'll use for lighting calculations.
		float3 N = normEstimate( rO, mu);

		// Compute the Phong illumination at the point of intersection.
		float3 color_rgb = Phong( light, rD, rO, N, mc );
		color = float4(color_rgb.x, color_rgb.y, color_rgb.z, 1); // set color.a = 1 to make this fragment opaque)

		// If the shadow flag is on, determine if this point is in shadow
		if( renderShadows == true )
		{
			// The shadow ray will start at the intersection point and go
			// towards the point light.  We initially move the ray origin
			// a little bit along this direction so that we don't mistakenly
			// find an intersection with the same point again.

			float3 L = normalize( light - rO );
			rO = float3(rO + N * epsilon * 2.0f);
			dist = intersectQJulia( rO, L, mu, epsilon );

			// Again, if our estimate of the distance to the set is small, we say
			// that there was a hit.  In this case it means that the point is in
			// shadow and should be given darker shading.
			if( dist < epsilon )
			{
				color *= 0.4f;  // (darkening the shaded value is not really correct, but looks good)			
			}
		}
	}
   
	// Return the final color which is still the background color if we didn't hit anything.
	return color;
}

// Launch kernel using C++ AMP
void ampCompute(const writeonly_texture_view<unorm4, 2>& tv, const QJulia4DConstants& mc)
{
    parallel_for_each(tv.accelerator_view, tv.extent.tile<64, 4>().pad(), [&,tv,mc](tiled_index<64,4> ti) restrict(amp)		  
    {
        float4 coord = float4((float)ti.global[1], (float)ti.global[0], 0.0f, 0.0f);

        float2 size     = float2((float)mc.c_width, (float)mc.c_height);				
        float scale     = fminf(size.x, size.y);
        float2 half     = float2(0.5f, 0.5f);
        float2 position = (coord.xy - half * size) / scale *BOUNDING_RADIUS_2 *mc.zoom;

        float3 light = float3(1.5f, 0.5f, 4.0f);
        float3 eye   = float3(0.0f, 0.0f, 4.0f);
        float3 ray   = float3(position.x, position.y, 0.0f);

        // rotate fractal		
        light = mul(light, mc.orientation);		
        eye   = mul( eye, mc.orientation);
        ray   = mul(  ray, mc.orientation);

        // ray start and ray direction
        float3 rO =  eye;
        float3 rD =  ray - rO;
    
        bool selfshadow = mc.selfShadow == 0  ? false : true;
        float4 color = QJulia(rO, rD, mc.mu, mc.epsilon, light, selfshadow, mc);	
        unorm_4 u_color(unorm(color.x), unorm(color.y), unorm(color.z), unorm(color.w));
        tv.set(ti.global,u_color);
    });
}