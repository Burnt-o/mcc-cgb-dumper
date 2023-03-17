#include "pch.h"
#include "CustomGameRefresher.h"



void CustomGameRefresher::tryGuessRefreshClickPosition()
{
	// Ratio of refresh button position into screen based on testing 
	static const float refreshXFraction = (440.f / 1220.f);
	static const float refreshYFraction = (710.f / 772.f);

	// Get the handle to the MCC window
	HWND handle = FindWindowA(NULL, "Halo: The Master Chief Collection  "); // Two spaces at the end of the string, for some reason
	PLOG_VERBOSE << "MCC Window Handle: " << handle;

	if (handle == NULL)
	{
		PLOG_ERROR << "Failed to guess refresh click position, couldn't get handle to MCC window";
		return;
	}

	// Get the dimensions of the MCC window - we can use these to calculate where the refresh button is located
	RECT windowDimensions;
	if (!GetWindowRect(handle, &windowDimensions))
	{
		PLOG_ERROR << "Failed to guess refresh click postion, couldn't get window dimensions from window handle" << std::endl
			<< "Error code: " << GetLastError();
		return;
	}

	float windowWidth = windowDimensions.right - windowDimensions.left;
	float windowHeight = windowDimensions.bottom - windowDimensions.top;

	PLOG_VERBOSE << "windowDimensions: " << std::endl
		<< "width: " << windowWidth << std::endl
		<< "height: " << windowHeight << std::endl;

	if (windowWidth <= 0 || windowHeight <= 0)
	{
		PLOG_ERROR << "windowHeight or windowWidth were invalid values";
		return;
	}

	this->mRefreshX = static_cast<WORD>(windowWidth * refreshXFraction);
	this->mRefreshY = static_cast<WORD>(windowHeight * refreshYFraction);

	mRefreshPositionInitialized = true;
	PLOG_INFO << "Guessed refresh click position as: " << this->mRefreshX << ", " << this->mRefreshY;

}




LRESULT CALLBACK NewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	PLOG_VERBOSE << "AH";

	switch (message)
	{

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}


LRESULT CALLBACK Wndproc(
	HWND unnamedParam1,
	UINT unnamedParam2,
	WPARAM unnamedParam3,
	LPARAM unnamedParam4
)
{
	PLOG_VERBOSE << "KILL ME";
}

void CustomGameRefresher::forceRefresh() const
{
	if (!mRefreshPositionInitialized)
	{
			PLOG_ERROR << "Failed to force refresh, refresh click position not set!" << std::endl
				<< "Please set the click position manually.";
			return;
	}


	// Get the handle to the MCC window
	HWND handle = FindWindowA(NULL, "Halo: The Master Chief Collection  "); // Two spaces at the end of the string, for some reason
	PLOG_VERBOSE << "MCC Window Handle: " << handle;


	if (handle == NULL)
	{
		PLOG_ERROR << "Failed to force a refresh, couldn't get handle to MCC window";
		return;
	}


	LPARAM clickPosition = MAKELPARAM(this->mRefreshX, this->mRefreshY);

	// Trying CallWindowProc

	WNDPROC orgWndProc = (WNDPROC)GetWindowLongPtrW(handle, GWLP_WNDPROC);


	CallWindowProcW(orgWndProc, handle, WM_MOUSEMOVE, (WPARAM)0, clickPosition);
	CallWindowProcW(orgWndProc, handle, WM_LBUTTONDOWN, (WPARAM)1, clickPosition);
	CallWindowProcW(orgWndProc, handle, WM_LBUTTONUP, (WPARAM)0, clickPosition);

	//CallWindowProcW(*p_LastWndProc, handle, WM_LBUTTONUP, 0, clickPosition);




	// Send the fake click message at where we think the refresh button is
	//PostMessageA(handle, WM_MOUSEMOVE, (WPARAM)0, clickPosition);
	//PostMessageA(handle, WM_LBUTTONDOWN, (WPARAM)0, clickPosition);
	//PostMessageA(handle, WM_LBUTTONUP, (WPARAM)0, clickPosition);

	//PostMessageA(handle, WM_ACTIVATE, (WPARAM)WA_ACTIVE, (LPARAM)NULL);
	//PostMessageA(handle, WM_IME_SETCONTEXT, (WPARAM)TRUE, (LPARAM)NULL);





	//PostMessageA(handle, WM_MOUSEACTIVATE, (WPARAM)handle, MAKELPARAM(HTCLIENT, WM_LBUTTONDOWN));
	//PostMessageA(handle, WM_ACTIVATEAPP, TRUE, NULL);
	//PostMessageA(handle, WM_ACTIVATE, (WPARAM)WA_CLICKACTIVE, MAKELPARAM(TRUE, NULL));
	//PostMessageA(handle, WM_SETFOCUS, (WPARAM)NULL, (LPARAM)NULL);


	//LONG_PTR OldWndProc = SetWindowLongPtrW(handle, GWLP_WNDPROC, (LONG_PTR)&NewWndProc);

	//PostMessageW(handle, WM_MOUSEMOVE, (WPARAM)0, clickPosition);
	//PostMessageW(handle, WM_LBUTTONDOWN, (WPARAM)1, clickPosition);
	//PostMessageW(handle, WM_MOUSEMOVE, (WPARAM)1, clickPosition);
	//PostMessageW(handle, WM_LBUTTONUP, (WPARAM)0, clickPosition);

	//SetWindowLongPtrW(handle, GWLP_WNDPROC, (long)OldWndProc);

	PLOG_VERBOSE << "Click sent at " << this->mRefreshX << ", " << this->mRefreshY;

}


long CalculateAbsoluteCoordinateX(long x)
{
	return (x * (long)65536) / GetSystemMetrics(SM_CXSCREEN);
}

long CalculateAbsoluteCoordinateY(long y)
{
	return (y * (long)65536) / GetSystemMetrics(SM_CYSCREEN);
}


void CustomGameRefresher::moveCursorToRefreshClickPosition()
{
	if (!mRefreshPositionInitialized)
	{
		PLOG_ERROR << "Failed to force refresh, refresh click position not set!" << std::endl
			<< "Please set the click position manually.";
		return;
	}


	// Get the handle to the MCC window
	HWND handle = FindWindowA(NULL, "Halo: The Master Chief Collection  "); // Two spaces at the end of the string, for some reason
	PLOG_VERBOSE << "MCC Window Handle: " << handle;


	if (handle == NULL)
	{
		PLOG_ERROR << "Failed to force a refresh, couldn't get handle to MCC window";
		return;
	}

	// Get the dimensions of the MCC window - we can use these to calculate where the refresh button is located
	RECT windowDimensions;
	if (!GetWindowRect(handle, &windowDimensions))
	{
		PLOG_ERROR << "couldn't get window dimensions from window handle" << std::endl
			<< "Error code: " << GetLastError();
		return;
	}

	std::cout << "MCC window is positioned at " << windowDimensions.left << ", " << windowDimensions.top;

	// So to actually move the cursor we want to use SendInput instead of PostMessage
	// which is a little more involved in setup, see https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput

	tagINPUT mouseInput;
	mouseInput.type = INPUT_MOUSE;
	mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE; // we're going to use absolute coordinates
	mouseInput.mi.dx = CalculateAbsoluteCoordinateX(windowDimensions.left + (long)this->mRefreshX); // So we need to add the window position
	mouseInput.mi.dy = CalculateAbsoluteCoordinateY(windowDimensions.top + (long)this->mRefreshY); // Which also have a weird normalization thing going on
	mouseInput.mi.mouseData = 0;

	PLOG_VERBOSE << "mRefreshX as long: " << (long)this->mRefreshX;

	SendInput(1, &mouseInput, sizeof(mouseInput));
}
