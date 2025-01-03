// Based on code from Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/Win32Application.cpp

#include "Win32Application.h"
#include "Input.h"

HWND Win32Application::mhWnd = nullptr;

int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
	// Parse any command line parameters
	int argCount;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argCount);
	pSample->ParseCommandLineArgs(argv, argCount);
	LocalFree(argv);

	// Initialise the window class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&wcex);

	// RECT defines a rectangle by its top left and bottom right corners
	// AdjustWindowRect takes an LPRECT (pointer to a rect), a DWORD (window style), 
	// and a BOOL (whether window has a menu or not)
	RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create a window with our registered WNDCLASS instance and store its handle
	mhWnd = CreateWindow(
		wcex.lpszClassName,					// the name of our WNDCLASS structure
		pSample->GetTitle(),				// the name of our window (appears in the window's capture bar)
		WS_OVERLAPPEDWINDOW,				// the style of our window
		CW_USEDEFAULT,						// the x position at the top left corner of the window relative to the screen - CW_USEDEFAULT lets Windows choose an appropriate default
		CW_USEDEFAULT,						// the y position at the top left corner of the window relative to the screen
		windowRect.right - windowRect.left,	// the width of the window in pixels
		windowRect.bottom - windowRect.top,	// the height of the window in pixels
		nullptr,							// handle to a parent window (none in our case)
		nullptr,							// handle to a menu (none in our case)
		hInstance,							// handle to the application the window is associated with
		pSample);							// a pointer to user-defined data that you want to be avaiable to a WM_CREATE message handler

	// Initialise the sample (OnInit is defined in a child DXSample implementation)
	pSample->OnInit();

	ShowWindow(mhWnd, nCmdShow);

	// Main sample loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Release any resources
	pSample->OnDestroy();

	// Need to return part of the WM_QUIT message to Windows
	return static_cast<char>(msg.wParam);
}

// Main message handler for Sample
LRESULT CALLBACK Win32Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DXSample* pSample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	
	switch (msg)
	{
	// Need to save the DXSample* passed into CreateWindow
	case WM_CREATE:
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		return 0;
	}
	// Deal with window being closed
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	// Deal with window painting - where updating/rendering is done
	case WM_PAINT:
	{
		if (pSample)
		{
			pSample->OnUpdate();
			pSample->OnRender();
			return 0;
		}
	}
	// Key down and key release events, see Input.h
	case WM_KEYDOWN:
	{
		KeyDownEvent(static_cast<EKeyCode>(wParam));
		return 0;
	}
	case WM_KEYUP:
	{
		KeyUpEvent(static_cast<EKeyCode>(wParam));
		return 0;
	}
	// Mouse movement/button press events
	case WM_MOUSEMOVE:
	{
		MouseMoveEvent(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		KeyDownEvent(Mouse_LButton);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		KeyUpEvent(Mouse_LButton);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		KeyDownEvent(Mouse_RButton);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		KeyUpEvent(Mouse_RButton);
		return 0;
	}
	case WM_MBUTTONDOWN:
	{
		KeyDownEvent(Mouse_MButton);
		return 0;
	}
	case WM_MBUTTONUP:
	{
		KeyUpEvent(Mouse_MButton);
		return 0;
	}
	} // End of switch statement

	// Equivalent to default case - pass any other messages we don't handle
	// back to Windows for default handling
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
