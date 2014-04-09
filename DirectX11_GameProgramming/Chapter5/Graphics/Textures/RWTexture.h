#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : RWTexture.h
 * File Description : Creates readable / writable 2D texture
 */
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>

ref class RWTexture
{
internal:	
	RWTexture();
	void Load(DirectX::XMFLOAT2 TextureSize);
	void Load(Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D, DirectX::XMFLOAT2 TextureSize);
	
	property ID3D11ShaderResourceView** SRV
	{
		ID3D11ShaderResourceView** get()
		{
			return this->srv.GetAddressOf();
		}
	}

	property ID3D11UnorderedAccessView** UAV
	{
		ID3D11UnorderedAccessView** get()
		{
			return this->uav.GetAddressOf();
		}
	}
	
private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
};