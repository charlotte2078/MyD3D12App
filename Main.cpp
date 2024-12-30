// Application entry point and window creation
// Based on code by Frank Luna - 3D Game Programming with DirectX12 - Appendix A (pp 758-761)

#include <windows.h>

// The main window handle that identifies a created window
HWND ghMainWnd = 0;

// This will create and initialise the main application window
bool InitWindowsApp(HINSTANCE instanceHandle, int show);

// Wrapper for the message loop of our app
int Run();

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Equivalent of main()
// 
// Parameters:
// HINSTANCE hInstance = a handle to the current application instance.
// HINSTANCE hPrevInstance = 0 - not used
// PSTR pCmdLine = command line argument string for running the program
// int nShowCmd = specifies how the application should be displayed (e.g. maximised, minimised, etc.)
//
// If WinMain succeeds, it should return the wParam member of the WM_QUIT message.
// If it quits without entering the message loop it should return 0.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
	if (!InitWindowsApp(hInstance, nShowCmd))
	{
		return 0;
	}

	return Run();
}

// Returns true if the initialisation was a success and false otherwise.
bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	// Create a WNDCLASS instance - this describes properties of our window
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;										// style - redraw when either the horizontal or vertical window size is changed
	wc.lpfnWndProc = WndProc;												// pointer to the windows procedure function to associate with this WNDCLASS
	wc.cbClsExtra = 0;														// extra memory slots that we don't need in this case
	wc.cbWndExtra = 0;
	wc.hInstance = instanceHandle;											// handle to the application instance (passed in through WinMain)
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);								// handle for the window's icon
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);									// handle for the window's cursor
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// brush to use for the background colour - standard white here
	wc.lpszMenuName = nullptr;													// specifies window's menu (we have none)
	wc.lpszClassName = L"BasicWndClass";									// name of our class structure - used to identify it later

	// Register our WNDCLASS instance with windows
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass FAILED", 0, 0);
		return false;
	}

	// Create a window with our registered WNDCLASS instance
	ghMainWnd = CreateWindow(
		L"BasicWndClass",		// the name of our WNDCLASS structure
		L"Win32Basic",			// the name of our window (appears in the window's capture bar)
		WS_OVERLAPPEDWINDOW,	// the style of our window
		CW_USEDEFAULT,			// the x position at the top left corner of the window relative to the screen - CW_USEDEFAULT lets Windows choose an appropriate default
		CW_USEDEFAULT,			// the y position at the top left corner of the window relative to the screen
		CW_USEDEFAULT,			// the width of the window in pixels
		CW_USEDEFAULT,			// the height of the window in pixels
		0,						// handle to a parent window (none in our case)
		0,						// handle to a menu (none in our case)
		instanceHandle,			// handle to the application the window is associated with
		0);						// a pointer to user-defined data that you want to be avaiable to a WM_CREATE message handler

	// Check that create window succeeded
	if (!ghMainWnd)
	{
		MessageBox(0, L"Create Window FAILED", 0, 0);
		return false;
	}

	// Display and refresh our window
	ShowWindow(ghMainWnd, show);
	UpdateWindow(ghMainWnd);

	return true;
}

// Wrapper for the message loop
int Run()
{
	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		// Process Windows messages if there are any
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise render/update our scene
		else
		{
			// GAME STUFF - UPDATE LATER
		}

	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
	{
		MessageBox(0, L"Hello, World!", L"Hello", MB_OK);
		return 0;
	}

	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(ghMainWnd);
			return 0;
		}

		break;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}

	// Equivalent to defualt case - pass any other messages we don't handle
	// back to Windows for default handling
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
