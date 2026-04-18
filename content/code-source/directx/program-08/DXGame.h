#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")






class MyDX3DGame
{

public:

	// Initialization Functions
	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();


	// Run Time Functions
	void Run();




	// Termination Functions
	~MyDX3DGame();





private:

	// App Data Variables
	HINSTANCE m_hInstance = NULL;
	HWND m_hWnd = NULL;



	// DirectX Variables and Interfaces
	ID3D11;



	// Window Data Variables
	const wchar_t* m_WndClassName = L"A Window Class Name Very Cool!";
	const wchar_t* m_WndTitle = L"Lesro Games Productions Presents: An Another Blank Screen!";
	int m_WndWidth = 640;
	int m_WndHeight = 480;
	UINT m_ClientWidth = 0;
	UINT m_ClientHeight = 0;



	// Game Stata Variables
	bool m_isPaused;



	// Window Messages Functions
	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);





};
