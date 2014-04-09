//--------------------------------------------------------------------------------------
// File: amp_cuda.h
//
// Implements utilties for C++ AMP to CUDA interop in namespace amp_cuda
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
// file except in compliance with the License.  You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR
// CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
//
// See the Apache Version 2.0 License for specific language governing permissions
// and limitations under the License.
//
//--------------------------------------------------------------------------------------

#pragma once
#include <amp.h>
#include <d3d11.h>
#include <memory>
#include <string>
#include <sstream>
#include <assert.h>
#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>
#include <cuda_runtime.h>

namespace amp_cuda
{

// cuda::interop_exception type
class interop_exception : public concurrency::runtime_exception
{
public:
    interop_exception() throw() 
        : concurrency::runtime_exception(E_FAIL)
    {
    }

    interop_exception(const char * message) throw()
        : concurrency::runtime_exception(message, E_FAIL)
    {
    }
};

namespace details
{
    inline void _check_and_throw_cuda_interop_exception(cudaError_t err, const char * error_msg)
    {
        if (cudaSuccess != err) 
        {
            std::stringstream msg;
            msg << error_msg << " : CUDA error : (" << static_cast<int>(err) << ") " << cudaGetErrorString(err);
            throw interop_exception(msg.str().c_str());
        }
        return;
    }

    // Deleter for the COM objects
    struct _com_deleter
    {
        void operator()(IUnknown * ptr) const
        {
			ptr->Release();
        }
    };

    // create a unique_ptr wrapper for unk_ptr
    template<typename com_type>
    std::unique_ptr<com_type, _com_deleter> _make_unique_ptr(IUnknown * unk_ptr, bool own_ref_count = true)
    {
        com_type * com_ptr = nullptr;
		if (unk_ptr != nullptr)
		{
			HRESULT hr = unk_ptr->QueryInterface( __uuidof(*com_ptr), reinterpret_cast<LPVOID *>(&com_ptr)); // increment ref count
			if (FAILED(hr)) 
			{
				// if failed, it's caller's responsibility to decrement the ref count incremented when unk_ptr was created
				com_ptr = nullptr;
			}
			else 
			{
				if (own_ref_count)
				{
					unk_ptr->Release(); // decrement the ref count incremented when unk_ptr was created
				}
			}
		}
        // move, the dtor of the returned unique_ptr eventually decrements the ref count of the underlying resource
        return std::unique_ptr<com_type, _com_deleter>(com_ptr); 
    }

} // details;

// Is the accelerator_view cuda capable
inline bool is_accelerator_view_cuda_capable(const concurrency::accelerator_view & av)
{
    bool ret = false;
    if (!av.accelerator.is_emulated && 
        (av.accelerator.device_path != concurrency::accelerator::cpu_accelerator)) 
    {
        std::unique_ptr<ID3D11Device, details::_com_deleter> d3d_device_ptr = details::_make_unique_ptr<ID3D11Device>(concurrency::direct3d::get_device(av));
        assert(d3d_device_ptr.get() != nullptr);

		std::unique_ptr<IDXGIDevice, details::_com_deleter> dxgi_device_ptr = details::_make_unique_ptr<IDXGIDevice>(d3d_device_ptr.get(), false /* own_ref_count */);
		assert(dxgi_device_ptr.get() != nullptr);
		
		IDXGIAdapter * pAdapter = nullptr;
		HRESULT hr = dxgi_device_ptr->GetAdapter(&pAdapter);
		if (hr == S_OK && pAdapter != nullptr)
		{
			int cu_device;
			cudaError_t err = cudaD3D11GetDevice(&cu_device, pAdapter);
			if (err == cudaSuccess)
			{
				// It is cuda capable
				ret = true;
			}
			else
			{
				cudaGetLastError(); // remove the error
			}
			pAdapter->Release();
		}
    }
    return ret;
}

// Is the accelerator cuda capable
inline bool is_accelerator_cuda_capable(const concurrency::accelerator & acc)
{
    return is_accelerator_view_cuda_capable(acc.default_view);
}

// An RAII wrapper to set up a cuda device for the scope
class scoped_device
{
public:
    // Construct a scoped device
    scoped_device(const concurrency::accelerator_view & av, bool reset = false)
    {
        // in clean state
        details::_check_and_throw_cuda_interop_exception(cudaGetLastError(), "cudaGetLastError");

        if (!is_accelerator_view_cuda_capable(av))
        {
            throw interop_exception("The accelerator_view argument is not CUDA capable");
        }
        
        if(reset)
        {
            details::_check_and_throw_cuda_interop_exception(cudaDeviceReset(), "cudaDeviceReset fail");
        }

        std::unique_ptr<ID3D11Device, details::_com_deleter> d3d_device_ptr = details::_make_unique_ptr<ID3D11Device>(concurrency::direct3d::get_device(av));
		assert(d3d_device_ptr.get() != nullptr);

		details::_check_and_throw_cuda_interop_exception(cudaD3D11SetDirect3DDevice(d3d_device_ptr.get()), "cudaD3D11SetDirect3DDevice fail");

		// in clean state
		details::_check_and_throw_cuda_interop_exception(cudaGetLastError(), "cudaGetLastError");
    }

    // Destructor
    ~scoped_device()
    {
        cudaDeviceReset();
        assert(cudaGetLastError() == cudaSuccess);
    }
};

// map kind
enum scoped_buffer_map_kind
{
    scoped_buffer_read_write_kind,
    scoped_buffer_read_only_kind,
    scoped_buffer_write_discard_kind
};

// An RAII wrapper to set up a cuda buffer for the scope
// Note: Does not support re-mapping of the scoped_buffer to different map flags, it's fixed for lifetime.
class scoped_buffer
{
public:
    // constructor that takes array
    template<typename T, int R>
    scoped_buffer(concurrency::array<T, R> & arr, scoped_buffer_map_kind kind = scoped_buffer_read_write_kind)
    {
        this->init(arr, kind);
    }

    // constructor that takes const array, thus read-only
    template<typename T, int R>
    scoped_buffer(const concurrency::array<T, R> & arr)
    {
        this->init(arr, scoped_buffer_read_only_kind);
    }

    // destructor
    ~scoped_buffer()
    {
        cudaGraphicsUnmapResources(1, &cuda_resource);
        assert(cudaGetLastError() == cudaSuccess);

        cudaGraphicsUnregisterResource(cuda_resource);
        assert(cudaGetLastError() == cudaSuccess);
    }

    // return device pointer 
    void * cuda_ptr() const
    {
        return cuda_device_buffer_ptr;
    }

    // return device pointer 
    template<typename T>
    T * cuda_ptr() const
    {
        return reinterpret_cast<T *>(cuda_device_buffer_ptr);
    }

    // return size of the buffer
    size_t size() const
    {
        return cuda_device_buffer_size;
    }
    
private:
    // Init
    template<typename T, int R>
    void init(const concurrency::array<T, R> & arr, scoped_buffer_map_kind kind)
    {

        std::unique_ptr<ID3D11Buffer, details::_com_deleter> d3d_buffer_ptr = details::_make_unique_ptr<ID3D11Buffer>(concurrency::direct3d::get_buffer(arr));
		assert(d3d_buffer_ptr.get() != nullptr);

		// in clean state
		details::_check_and_throw_cuda_interop_exception(cudaGetLastError(), "cudaGetLastError");

		// register and map
		details::_check_and_throw_cuda_interop_exception(cudaGraphicsD3D11RegisterResource(&cuda_resource, d3d_buffer_ptr.get(), cudaGraphicsRegisterFlagsNone),
			"cudaGraphicsD3D11RegisterResource fail");
		details::_check_and_throw_cuda_interop_exception(cudaGraphicsResourceSetMapFlags(cuda_resource, get_cuda_resouce_map_flags(kind)),
			"cudaGraphicsResourceSetMapFlags fail");
		details::_check_and_throw_cuda_interop_exception(cudaGraphicsMapResources(1, &cuda_resource),
			"cudaGraphicsMapResources fail");        
		details::_check_and_throw_cuda_interop_exception(cudaGraphicsResourceGetMappedPointer(&cuda_device_buffer_ptr, &cuda_device_buffer_size, cuda_resource), 
			"cudaGraphicsResourceGetMappedPointer fail");

		// in clean state
		details::_check_and_throw_cuda_interop_exception(cudaGetLastError(), "cudaGetLastError");

    }

    unsigned int get_cuda_resouce_map_flags(scoped_buffer_map_kind kind)
    {
        unsigned int flag;
        switch(kind)
        {
        case scoped_buffer_read_write_kind:
            flag = cudaGraphicsMapFlagsNone;
            break;
        case scoped_buffer_read_only_kind:
            flag = cudaGraphicsMapFlagsReadOnly;
            break;
        case scoped_buffer_write_discard_kind:
            flag = cudaGraphicsMapFlagsWriteDiscard;
            break;
        default:
            assert(false);
            break;
        }
        return flag;
    }

    size_t cuda_device_buffer_size;
    void * cuda_device_buffer_ptr;
    cudaGraphicsResource_t cuda_resource;
};

} // namespace amp_cuda
