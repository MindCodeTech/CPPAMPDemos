#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : CpuInfo.h
 * File Description : Returns the information of the CPU
 */
#include <wrl/client.h>

#define GUID_DEVICE_PROCESSOR "{97FADB10-4E33-40AE-359C-8BEF029DBDD0}"

using namespace Platform;

ref class CpuInfo
{
internal:
	CpuInfo();
private:
	byte totalCores;
	String^ name;
public:
	property String^ Name
	{
		String^ get()
		{
			return this->name;
		}
	}
	property byte TotalCores
	{
		byte get()
		{
			return this->totalCores;
		}
	}
};

