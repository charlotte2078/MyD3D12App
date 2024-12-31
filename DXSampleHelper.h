// Based on code from Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/DXSampleHelper.h

#pragma once

#include "StdIncludes.h"
#include <stdexcept>

using Microsoft::WRL::ComPtr;

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

#define SAFE_RELEASE(p) if (p) (p)->Release()

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

// A DDS file is a DirectDrawSurface file, used to store texture data
inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, byte** data, UINT* offset, UINT* size)
{
	if (FAILED(ReadDataFromFile(filename, data, size)))
	{
		return E_FAIL;
	}

	// DDS files always start with the same magic number
	static const UINT DDS_MAGIC = 0x20534444;
	UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
	if (magicNumber != DDS_MAGIC)
	{
		return E_FAIL;
	}

	struct DDS_PIXELFORMAT
	{
		UINT size;
		UINT flags;
		UINT fourCC;
		UINT rgbBitCount;
		UINT rBitMask;
		UINT gBitMask;
		UINT bBitMask;
		UINT aBitMask;
	};

	struct DDS_HEADER
	{
		UINT size;
		UINT flags;
		UINT height;
		UINT width;
		UINT pitchOrLinearSize;
		UINT depth;
		UINT mipMapCount;
		UINT reserved1[11];
		DDS_PIXELFORMAT ddsPixelFormat;
		UINT caps;
		UINT caps2;
		UINT caps3;
		UINT caps4;
		UINT reserved2;
	};

	auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
	if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT))
	{
		return E_FAIL;
	}

	const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
	*offset = ddsDataOffset;
	*size = *size - ddsDataOffset;

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
	if (swprintf_s(fullName, L"&s[%u]", name, index) > 0)
	{
		pObject->SetName(fullName);
	}
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
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

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}
#endif

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
