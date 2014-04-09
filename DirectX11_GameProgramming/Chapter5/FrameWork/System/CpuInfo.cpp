/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : CpuInfo.cpp
 * File Description : 
 */
#include "pch.h"
#include "CpuInfo.h"
#include <collection.h>

using namespace Windows::Devices::Enumeration;

CpuInfo::CpuInfo():totalCores(0)
{
	this->totalCores = 0;
	auto filter = "System.Devices.InterfaceClassGuid:=\"" + GUID_DEVICE_PROCESSOR + "\"";

	Concurrency::task<DeviceInformationCollection^>(DeviceInformation::FindAllAsync(filter, nullptr))
		.then([this](DeviceInformationCollection^ interfaces)
	{
		std::for_each(begin(interfaces), end(interfaces), [this](DeviceInformation^ DeviceInfo)
		{
			this->totalCores++;
			if (this->name->IsEmpty())
			{
				this->name = DeviceInfo->Name;
			}
		});
	});
}