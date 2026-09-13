// Based on code from Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/DXSampleHelper.h

#pragma once

#include "Includes.h"
#include <stdexcept>
#include <dxcapi.h>
#include <filesystem>
#include <fstream>

using Microsoft::WRL::ComPtr;

namespace DXHelpers
{
	inline std::string HrToString(HRESULT hr)
	{
		char s_str[64] = {};
		sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
		return std::string(s_str);
	}

	class HrException : public std::runtime_error
	{
	public:
		HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), mHr(hr) {}
		HRESULT Error() const { return mHr; }
	private:
		const HRESULT mHr;
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw HrException(hr);
		}
	}

	inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
	{
		if (path == nullptr)
		{
			throw std::exception();
		}

		DWORD size = GetModuleFileName(nullptr, path, pathSize);
		if (size == 0 || size == pathSize)
		{
			// Method failed or path was truncated
			throw std::exception();
		}

		WCHAR* lastSlash = wcsrchr(path, L'\\');
		if (lastSlash)
		{
			*(lastSlash + 1) = L'\0';
		}
	}

	inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
	{
		using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
		CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
		extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
		extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
		extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
		extendedParams.lpSecurityAttributes = nullptr;
		extendedParams.hTemplateFile = nullptr;

		Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
		Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif

		// Check the handle is valid
		if (file.Get() == INVALID_HANDLE_VALUE)
		{
			throw std::exception();
		}

		// Check the file info is valid
		FILE_STANDARD_INFO fileInfo = {};
		if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
		{
			throw std::exception();
		}

		// Check the EndOfFile is in the Low Part of the large integer - needed for next part
		if (fileInfo.EndOfFile.HighPart != 0)
		{
			throw std::exception();
		}

		*data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
		*size = fileInfo.EndOfFile.LowPart;

		// Check we can read the file
		if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
		{
			throw std::exception();
		}

		return S_OK;
	}

	// Assign a name to the object for debugging
#if defined(_DEBUG) || defined(DBG)
	inline void SetName(ID3D12Object* pObject, LPCWSTR name)
	{
		pObject->SetName(name);
	}
	inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
	{
		WCHAR fullName[50];
		//if (swprintf_s(fullName, L"&s[%u]", name, index) > 0)
		if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		{
			pObject->SetName(fullName);
		}
	}
#else
	inline void SetName(ID3D12Object*, LPCWSTR)
	{}
	inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
	{}
#endif

	// Naming helper for ComPtr<T>.
	// Assigns the name of the variable as the name of the object.
	// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

// Calculates size of constant buffers. The constant buffer must be a multiple of the
// minimum hardware allocation size
	inline UINT CalculateConstantBufferByteSize(UINT byteSize)
	{
		// e.g. in the case D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT = 256
		// Say byteSize = 300
		// (300 + 256 - 1) & ~(256 - 1)
		// = 555 & ~255
		// = 0x022B & ~0x00FF
		// = 0x022B & 0xFF00
		// = 0x0200
		// = 512 (which is the smallest multiple of 256 above 300)
		return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	}

	// Frank Luna helper function https://github.com/d3dcoder/d3d12book_2ed/blob/main/Common/d3dUtil.h
	inline std::wstring AnsiToWString(const std::string& str)
	{
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}

	// Frank Luna helper function https://github.com/d3dcoder/d3d12book_2ed/blob/main/Common/d3dUtil.cpp
	inline void WriteBinaryToFile(IDxcBlob* blob, const std::wstring& filename)
	{
		std::ofstream fout(filename, std::ios::binary);
		fout.write((char*)blob->GetBufferPointer(), blob->GetBufferSize());
		fout.close();
	}

	// Frank Luna DirectX 12 2ed pp. 259-260
	inline ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filename,
		std::vector<LPCWSTR>& compileArgs
	)
	{
		static ComPtr<IDxcUtils> utils = nullptr;
		static ComPtr<IDxcCompiler3> compiler = nullptr;
		static ComPtr<IDxcIncludeHandler> defaultIncludeHandler = nullptr;

		if (!compiler)
		{
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
			ThrowIfFailed(utils->CreateDefaultIncludeHandler(&defaultIncludeHandler));
		}

		uint32_t codePage = CP_UTF8;
		ComPtr<IDxcBlobEncoding> sourceBlob = nullptr;
		ThrowIfFailed(utils->LoadFile(filename.c_str(), &codePage, &sourceBlob));

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
		sourceBuffer.Size = sourceBlob->GetBufferSize();
		sourceBuffer.Encoding = 0;

		ComPtr<IDxcResult> result = nullptr;
		HRESULT hr = compiler->Compile(
			&sourceBuffer,
			compileArgs.data(),
			static_cast<UINT32>(compileArgs.size()),
			defaultIncludeHandler.Get(),
			IID_PPV_ARGS(result.GetAddressOf()));

		if (SUCCEEDED(hr))
		{
			result->GetStatus(&hr);
		}

		ComPtr<IDxcBlobUtf8> errorMsgs = nullptr;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMsgs), nullptr);

		if (errorMsgs && errorMsgs->GetStringLength())
		{
			OutputDebugStringA(errorMsgs->GetStringPointer());
			ThrowIfFailed(E_FAIL);
		}

		ComPtr<IDxcBlob> dxil = nullptr;
		ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), nullptr));

#if defined(DEBUG) || defined(_DEBUG)  
		// Write PDB data for PIX debugging.
		const std::string pdbDirectory = "HLSL PDB/";
		if (!std::filesystem::exists(pdbDirectory))
		{
			std::filesystem::create_directory(pdbDirectory);
		}

		ComPtr<IDxcBlob> pdbData = nullptr;
		ComPtr<IDxcBlobUtf16> pdbPathFromCompiler = nullptr;
		ThrowIfFailed(result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbData), &pdbPathFromCompiler));
		WriteBinaryToFile(pdbData.Get(),
			AnsiToWString(pdbDirectory) +
			std::wstring(pdbPathFromCompiler->GetStringPointer()));
#endif

		return dxil;
	}

	inline D3D12_SHADER_BYTECODE ByteCodeFromBlob(IDxcBlob* shader)
	{
		return { reinterpret_cast<BYTE*>(shader->GetBufferPointer()), shader->GetBufferSize() };
	}

	// Resets all elements in a ComPtr array
	template<class T>
	void ResetComPtrArray(T* comPtrArray)
	{
		for (auto& i : *comPtrArray)
		{
			i.Reset();
		}
	}

	// Resets all elements in a unique_ptr array
	template<class T>
	void ResetUniquePtrArray(T* uniquePtrArray)
	{
		for (auto& i : *uniquePtrArray)
		{
			i.reset();
		}
	}
}
