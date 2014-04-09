#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : MathHelper.h
 * File Description : Simple math functions
 */
#include <DirectXMath.h>

namespace MathHelper
{
	inline float Rand()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	inline float Rand(float a, float b)
	{
		return a + Rand() * (b-a);
	}

	inline float Restrict(float val, float Min, float Max)
	{
		if (val < Min) return Min;
		if (val > Max) return Max;
		return val;
	}

	inline float Dot(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline float Length(DirectX::XMFLOAT3& a)
	{
		return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	}

	inline DirectX::XMFLOAT3 Cross(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b)
	{
		return DirectX::XMFLOAT3((a.y*b.z)-(a.z*b.y), (a.z*b.x)-(a.x*b.z), (a.x*b.y)-(a.y*b.x));
	}

	inline DirectX::XMFLOAT3 Normalize(DirectX::XMFLOAT3& v)
	{
		auto len = Length(v);
		return DirectX::XMFLOAT3(v.x / len, v.y / len, v.z / len);
	}

	inline DirectX::XMFLOAT3 VectorToFloat3(DirectX::XMVECTOR& v)
	{
		return DirectX::XMFLOAT3(DirectX::XMVectorGetX(v), DirectX::XMVectorGetY(v), DirectX::XMVectorGetZ(v));
	}

	inline DirectX::XMFLOAT4 VectorToFloat4(DirectX::XMVECTOR& v)
	{
		return DirectX::XMFLOAT4(DirectX::XMVectorGetX(v), DirectX::XMVectorGetY(v)
			, DirectX::XMVectorGetZ(v), DirectX::XMVectorGetW(v));
	}

	inline DirectX::XMFLOAT3 Transform(DirectX::XMFLOAT3& v, DirectX::XMMATRIX& m)
	{
		return VectorToFloat3(DirectX::XMVector3Transform(DirectX::XMVectorSet(v.x, v.y, v.z, 1.0f), m));
	}

	inline DirectX::XMFLOAT4 Transform(DirectX::XMFLOAT4& f, DirectX::XMMATRIX& m)
	{
		return VectorToFloat4(DirectX::XMVector3Transform(DirectX::XMVectorSet(f.x, f.y, f.z, f.w), m));
	}

	inline DirectX::XMFLOAT3 TransformNormal(DirectX::XMFLOAT3& v, DirectX::XMMATRIX& m)
	{
		return VectorToFloat3(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(v.x, v.y, v.z, 1.0f), m));
	}

	inline DirectX::XMFLOAT4 TransformNormal(DirectX::XMFLOAT4& f, DirectX::XMMATRIX& m)
	{
		return VectorToFloat4(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(f.x, f.y, f.z, f.w), m));
	}

#pragma region Matrix

	inline DirectX::XMFLOAT3 XMMatrixForward(DirectX::XMMATRIX& m)
	{
		DirectX::XMFLOAT4X4 float4x4;
		DirectX::XMStoreFloat4x4( &float4x4, m);
		return DirectX::XMFLOAT3(-float4x4._31, -float4x4._32, -float4x4._33);
	}

	inline DirectX::XMFLOAT3 XMMatrixUp(DirectX::XMMATRIX& m)
	{
		DirectX::XMFLOAT4X4 float4x4;
		DirectX::XMStoreFloat4x4( &float4x4, m);
		return DirectX::XMFLOAT3(float4x4._21, float4x4._22, float4x4._23);
	}

	inline DirectX::XMFLOAT3 XMMatrixLeft(DirectX::XMMATRIX& m)
	{
		DirectX::XMFLOAT4X4 float4x4;
		DirectX::XMStoreFloat4x4( &float4x4, m);
		return DirectX::XMFLOAT3(-float4x4._11, -float4x4._12, -float4x4._13);
	}

#pragma endregion

#pragma region float3 operators

	inline bool operator == (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	inline bool operator != (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		return a.x != b.x || a.y != b.y || a.z != b.z;
	}

	inline DirectX::XMFLOAT3 operator + (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		return DirectX::XMFLOAT3 (a.x + b.x, a.y + b.y, a.z + b.z);
	}

	inline DirectX::XMFLOAT3 operator - (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		return DirectX::XMFLOAT3 (a.x - b.x, a.y - b.y, a.z - b.z);
	}

	inline DirectX::XMFLOAT3 operator * (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		return DirectX::XMFLOAT3 (a.x * b.x, a.y * b.y, a.z * b.z);
	}

	inline DirectX::XMFLOAT3 operator * (float a, DirectX::XMFLOAT3 b)
	{
		return DirectX::XMFLOAT3 (a * b.x, a * b.y, a * b.z);
	}

	inline DirectX::XMFLOAT3 operator * (DirectX::XMFLOAT3 a, float b)
	{
		return DirectX::XMFLOAT3 (a.x * b, a.y * b, a.z * b);
	}

	inline DirectX::XMFLOAT3 operator / (DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
	{
		try
		{
			return DirectX::XMFLOAT3 (a.x / b.x, a.y / b.y, a.z / b.z);
		}
		catch(...)
		{
			throw "Logical error : Divide by zero";
		}
	}

	inline DirectX::XMFLOAT3 operator / (DirectX::XMFLOAT3 a, float b)
	{
		try
		{
			return DirectX::XMFLOAT3 (a.x / b, a.y / b, a.z / b);
		}
		catch(...)
		{
			throw "Logical error : Divide by zero";
		}
	}

#pragma endregion

#pragma region float4 operators

	inline bool operator == (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}

	inline bool operator != (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
	}

	inline DirectX::XMFLOAT4 operator + (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		return DirectX::XMFLOAT4 (a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
	}

	inline DirectX::XMFLOAT4 operator - (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		return DirectX::XMFLOAT4 (a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
	}

	inline DirectX::XMFLOAT4 operator * (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		return DirectX::XMFLOAT4 (a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	inline DirectX::XMFLOAT4 operator * (float a, DirectX::XMFLOAT4 b)
	{
		return DirectX::XMFLOAT4 (a * b.x, a * b.y, a * b.z, a * b.w);
	}

	inline DirectX::XMFLOAT4 operator * (DirectX::XMFLOAT4 a, float b)
	{
		return DirectX::XMFLOAT4 (a.x * b, a.y * b, a.z * b, a.w * b);
	}

	inline DirectX::XMFLOAT4 operator / (DirectX::XMFLOAT4 a, DirectX::XMFLOAT4 b)
	{
		try
		{
			return DirectX::XMFLOAT4 (a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
		}
		catch(...)
		{
			throw "Logical error : Divide by zero";
		}
	}

	inline DirectX::XMFLOAT4 operator / (DirectX::XMFLOAT4 a, float b)
	{
		try
		{
			return DirectX::XMFLOAT4 (a.x / b, a.y / b, a.z / b, a.w / b);
		}
		catch(...)
		{
			throw "Logical error : Divide by zero";
		}
	}

#pragma endregion
}
