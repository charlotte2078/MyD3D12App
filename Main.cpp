// Application entry point and window creation
// Based on code by Frank Luna - 3D Game Programming with DirectX12 - Appendix A (pp 758-761)

#include "Includes.h"
#include "MyD3D12App.h"

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
_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
	MyD3D12App triangleSample(1280, 720, L"My First Triangle");

	return Win32Application::Run(&triangleSample, hInstance, nShowCmd);
}
