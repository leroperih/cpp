#pragma once

#include "values-source.h"

#include <windows.h>
#include <iostream>
#include <dwmapi.h>

#include <condition_variable>

#include <iomanip>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

#pragma comment(lib, "dwmapi.lib")


// MY TYPE DATA
struct mouseAction
{
	int x = 0;
	int y = 0;
	std::string which_button;
};

struct EnumContext
{
	std::vector<HWND> hWnd;
	int current_counter = 0;
	bool will_SearchForGimp;
};

struct MessageWindowClassInfo
{
	const wchar_t* class_name;
	HINSTANCE hInst;
};


struct AdvWndContext
{
	HMONITOR initMonitor;
	RECT monitorRect;

};

// GLOBAL VARIABLES
HWND global_Main_hWnd;
HHOOK global_hHook;
const wchar_t global_Menu_ClassName[] = L"Um Nome de Classe de Janela do Bagulho!";

// WORKER THREAD VARIABLES
std::queue<mouseAction> actionQueue;
std::mutex queueMutex;
std::condition_variable queueCV;

POINT init_Cursor;
bool will_Allow_ArtMode = false;
bool will_Allow_MouseRemapping = true;
bool is_Brush_or_Eraser = false;
bool is_Middle_Pressed = false;
bool was_Tool_Changed = false;
bool is_Gimp_Active = false;

// PROTOTYPES
MessageWindowClassInfo my_CreateMessageWindowClass();
void my_ShowDisplacement_Function(std::string, int, int);
void my_SendCommand_Function(UINT, UINT, UINT);








// SHOW PROGRAM STATUS
void my_ShowProgramStatus_Function(BOOL success, const char which_action[], const char which_element[])
{
	static int ac_c = 0;

	if (success)
	{
		ac_c++;
		std::cout << "\n  #" << ac_c << ") You has succeeded on your attempt to " << which_action << " the [ " << which_element << " ]";
	}
	else
	{
		Beep(950, 80);
		std::cerr << "\n | Your attempt to " << which_action << " the [ " << which_element << " ] has failed! Error Code: " << GetLastError() << std::endl;
		std::cout << "\n | ._.) - Sorry, It's not me, it's probably you... "; system("pause");
	}

}


// ENUM WINDOWS
BOOL CALLBACK my_EnumWindowsProc (HWND hWnd, LPARAM lParam)
{

	// Fiutrage
	if (!IsWindowVisible(hWnd)) return TRUE;
	if (GetWindowTextLength(hWnd) == 0) return TRUE;
	if (GetWindow(hWnd, GW_OWNER) != NULL) return TRUE;

	// Checa uns estilos aí, troço complicado
	LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW)) return TRUE;

	// Qual monitor
	POINT pt; GetCursorPos(&pt);
	if ((MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)) != (MonitorFromPoint(pt, MONITOR_DEFAULTTONULL))) return TRUE;

	// Umas paradas
	int cloaked = 0;
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
	if (SUCCEEDED(hr) && cloaked != 0) return TRUE;





	// Configuração
	EnumContext* context = reinterpret_cast<EnumContext*>(lParam);
	context->current_counter++;
	context->hWnd.push_back(hWnd);
	return TRUE;

}

BOOL CALLBACK my_EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{

	std::vector<AdvWndContext>* data = reinterpret_cast<std::vector<AdvWndContext>*> (dwData);

	AdvWndContext context;
	context.initMonitor = hMonitor;
	context.monitorRect = *lprcMonitor;
	data->push_back({ context });

	return TRUE;

}








// WORKER THREAD
void my_WorkerThread_Function()
{

	while (true)
	{



		mouseAction action;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCV.wait(lock, [] { return !actionQueue.empty(); });
			action = actionQueue.front();
			actionQueue.pop();
		}





		// RIGHT MOUSE BUTTON
		if (action.which_button == "Right ")
		{


			my_ShowDisplacement_Function("> " + action.which_button, action.x, action.y);

			// X Displacement
			if ((abs(action.x) > MIN_DISPLACEMENT_DRAG) && (abs(action.y) <= MAX_DEVIATION_THRESHOLD))
			{
				// TO RIGHT
				if (action.x < 0)
				{
					my_SendCommand_Function(0, 0, VK_RIGHT);
				}


				// TO LEFT
				else
				{
					my_SendCommand_Function(0, 0, VK_LEFT);
				}
			}

			// Y Displacement
			if ((abs(action.y) > MIN_DISPLACEMENT_DRAG) && (abs(action.x) <= MAX_DEVIATION_THRESHOLD))
			{

				// TO UPSIDE
				if (action.y < 0)
				{
					EnumContext context;
					EnumWindows(my_EnumWindowsProc, reinterpret_cast<LPARAM>(&context));
					if (context.current_counter > 0)
					{
						HWND wnd = context.hWnd[context.current_counter - 1];
						if (IsIconic(wnd)) { ShowWindow(wnd, SW_RESTORE); }
						else { ShowWindow(wnd, SW_SHOW); }
						SetForegroundWindow(wnd);
					}
				}

				// TO DOWNSIDE
				else
				{
					EnumContext context;
					EnumWindows(my_EnumWindowsProc, reinterpret_cast<LPARAM> (&context));
					if (context.current_counter > 0)
					{
						if ((GetForegroundWindow()) != context.hWnd[0])
						{
							SetForegroundWindow(context.hWnd[0]);
						}
					}
					my_SendCommand_Function(VK_CONTROL, 0, VK_TAB);
				}

			}

			// No Displacement
			if ((abs(action.x) <= MAX_REMAPLESS_DISPLACEMENT) && (abs(action.y) <= MAX_REMAPLESS_DISPLACEMENT))
			{
				INPUT inputs[2] = {};
				inputs[0].type = INPUT_MOUSE; inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				inputs[1].type = INPUT_MOUSE; inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				SendInput(2, inputs, sizeof(INPUT));
			}


		}


		// MIDDLE MOUSE BUTTON
		if (action.which_button == "Middle")
		{


			my_ShowDisplacement_Function("x " + action.which_button, action.x, action.y);

			// X Displacement
			if ((abs(action.x) > MIN_DISPLACEMENT_DRAG) && (abs(action.y) <= MAX_DEVIATION_THRESHOLD))
			{
				// TO RIGHT
				if (action.x < 0)
				{
					my_SendCommand_Function(VK_LWIN, VK_CONTROL, VK_RIGHT);
				}


				// TO LEFT
				else
				{
					my_SendCommand_Function(VK_LWIN, VK_CONTROL, VK_LEFT);
				}
			}

			// Y Displacement
			if ((abs(action.y) > MIN_DISPLACEMENT_DRAG) && (abs(action.x) <= MAX_DEVIATION_THRESHOLD))
			{
				// TO UPSIDE
				if (action.y < 0)
				{
					my_SendCommand_Function(VK_LWIN, 0, VK_TAB);
				}


				// TO DOWNSIDE
				else
				{
					EnumContext context;
					EnumWindows(my_EnumWindowsProc, reinterpret_cast<LPARAM>(&context));
					if (context.current_counter > 0)
					{
						for (int i = 0; i < context.current_counter; i++)
						{
							if (!IsIconic(context.hWnd[i])) { ShowWindow(context.hWnd[i], SW_MINIMIZE); }
						}
					}
				}
			}

			// No Diplacement
			if ((abs(action.x) <= MAX_REMAPLESS_DISPLACEMENT) && (abs(action.y) <= MAX_REMAPLESS_DISPLACEMENT))
			{
				INPUT inputs[2] = {};
				inputs[0].type = INPUT_MOUSE; inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
				inputs[1].type = INPUT_MOUSE; inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
				SendInput(2, inputs, sizeof(INPUT));
			}


		}



	}

}








// SHOW TOGGLE REMAP
void my_ShowToggleAdvice(const wchar_t* message , bool value )
{

	// Garante que usamos a classe correta
	static MessageWindowClassInfo context = my_CreateMessageWindowClass();

	std::wstring msgText = message;




	std::thread([ msgText , value ]() {


		// STYLE VALUES
		COLORREF magicColor = RGB(255, 0, 255);
		int w = 350;
		int h = 60;
		int padding = 12;

		int fontSize = 24;

		HFONT hFont = CreateFontW(
			fontSize,
			0, 0, 0,
			FW_SEMIBOLD,
			FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			VARIABLE_PITCH,
			L"Segoe UI Variable Display"
		);



		// CREATES THE WINDOW(S)
		std::vector<AdvWndContext> myAdvWndContext;
		EnumDisplayMonitors(NULL, NULL, my_EnumMonitorsProc, reinterpret_cast<LPARAM> (&myAdvWndContext));
		std::vector<HWND> createdWnds;


		for ( int i = 0 ; i < myAdvWndContext.size() ; i++ )
		{ 

				RECT r = myAdvWndContext[i].monitorRect;
				int monWidth = r.right - r.left;
				int monHeight = r.bottom - r.top;

				int x = r.left + 32;
				int y = r.top + 32;



				 createdWnds.push_back({ (CreateWindowExW(
					WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
					context.class_name,
					NULL,
					WS_POPUP | WS_VISIBLE,
					x, y, w, h,
					NULL, NULL, context.hInst, nullptr
				)) });

				if (!createdWnds[i])
				{
					my_ShowProgramStatus_Function( ATTEMPT_FAILED , "create" , "Advice Window #" + i );
					return;
				}

				SetWindowRgn(createdWnds[i], CreateRoundRectRgn(0, 0, w, h, 25, 25), TRUE);
				SetLayeredWindowAttributes(createdWnds[i], 0, 255, LWA_ALPHA);

				SetWindowPos(createdWnds[i], HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
				UpdateWindow(createdWnds[i]);



				int textW = w - (padding * 2);
				int textH = h - (padding * 2);

				HWND hStatic = CreateWindowExW(
					0, L"STATIC",
					(msgText + ( value ? L" On" : L" Off" )).c_str(),
					WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
					padding,
					padding,
					w - (padding * 2),
					h - (padding * 2),
					createdWnds[i], NULL, context.hInst, NULL
				);

				SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);


		}




		// DESTROY THE WINDOW(S)
		auto start = std::chrono::steady_clock::now();
		MSG msg = {};

		while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(2500)) {
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		for (HWND hWnd : createdWnds) {
			if (hWnd) DestroyWindow(hWnd);
		}

		}).detach();


}