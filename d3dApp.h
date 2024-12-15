// Based on code by Frank Luna: 
// https://github.com/d3dcoder/d3d12book/blob/master/Common/d3dApp.h

#pragma once

#include "d3dUtil.h"

// Link d3d12 libraries - saves us having to set up additional dependencies
// in Linker settings
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// The D3DApp class is responsible for:
// - Creating the main application window,
// - Running the application message loop
// - Handling window messages
// - Initialising Direct3D
class D3DApp
{
protected:
	D3DApp(HINSTANCE hInstance);
};
