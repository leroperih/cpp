#pragma once


#include <windows.h>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")








class MyDX11Game
{

public:

	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();

	void Run();

	~MyDX11Game();





private:

	// Window Member Variables
	HINSTANCE m_hInstance;
	HWND m_hWnd;


	// DirectX Member Variables
	ID3D11Device*           m_pDevice = nullptr;
	ID3D11DeviceContext*    m_pDevCon = nullptr;
	IDXGISwapChain*         m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pBackBufferView = nullptr;


	// Window Member Variables
	const wchar_t* m_WndClassName = L"A Pretty Cool Name For My Game Window Class";
	const wchar_t* m_WndTitle = L"Lesro Games Productions Presents: Blank Screen";
	int m_WndWidth = 640;
	int m_WndHeight = 480;


	// Window State Member Variables
	bool m_isPaused;


	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);





};
