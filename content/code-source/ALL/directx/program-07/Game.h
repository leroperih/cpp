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


	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();



	void Run();



	~MyDX3DGame();





private:


	HINSTANCE m_hInstance = NULL;
	HWND m_hWnd = NULL;



	ID3D11Device*           m_pDevice = nullptr;
	ID3D11DeviceContext*    m_pDevCon = nullptr;
	IDXGISwapChain*         m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pBackBufferView = nullptr;

	D3D_FEATURE_LEVEL       m_ActualLevel;
	IDXGIFactory2*          m_pFactory = nullptr;



	const wchar_t* m_WndClassName = L"A Window Class Name Very Cool!";
	const wchar_t* m_WndTitle = L"Lesro Games Productions Presents: An Another Blank Screen!";
	int m_WndWidth = 640;
	int m_WndHeight = 480;
	UINT m_ClientWidth = 0;
	UINT m_ClientHeight = 0;



	bool m_isPaused;



	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);





};




