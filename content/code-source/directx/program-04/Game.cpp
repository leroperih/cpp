

#include "Game.h"





bool MyDX11Game::Initialize(HINSTANCE hInstance, int nCmdShow)
{

	m_hInstance = hInstance;

	WNDCLASSEXW wc = { 0 };
	wc.cbSize        = sizeof(WNDCLASSEXW);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WindowProcedure;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = m_WndClassName;
	wc.hIconSm       = NULL;

	RegisterClassExW(&wc);

	HWND local_hWnd = CreateWindowExW(
		0,
		m_WndClassName,
		m_WndTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_WndWidth, m_WndHeight,
		NULL,
		NULL,
		m_hInstance,
		this
		);

	m_hWnd = local_hWnd;

	if (local_hWnd == NULL) return false;

	ShowWindow(m_hWnd, nCmdShow);

	// --- ADD THIS LINE ---
	if (!InitializeDirectX())
	{
		return false;
	}

	return true;

}


bool MyDX11Game::InitializeDirectX()
{

	DXGI_SWAP_CHAIN_DESC scd = { 0 };
	scd.BufferCount = 1;                                // One back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard color format
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hWnd;                          // Your stored window handle
	scd.SampleDesc.Count = 4;                           // Anti-aliasing
	scd.Windowed = TRUE;

	// Create the device, context, and swap chain
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
		D3D11_SDK_VERSION, &scd, &m_pSwapChain, &m_pDevice, NULL, &m_pDevCon);

	if (FAILED(hr)) return false;

	// --- NEW: Set up the Render Target View (The "Canvas") ---
	ID3D11Texture2D* pBackBuffer = nullptr;
	m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferView);
	pBackBuffer->Release(); // We don't need the texture pointer anymore, just the View

	if (FAILED(hr)) return false;

	// Bind the render target to the pipeline
	m_pDevCon->OMSetRenderTargets(1, &m_pBackBufferView, NULL);

	return true;

}


void MyDX11Game::Run()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// Render Logic using member variables
			float color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			m_pDevCon->ClearRenderTargetView(m_pBackBufferView, color);
			m_pSwapChain->Present(1, 0); // VSync enabled
		}
	}
}


MyDX11Game::~MyDX11Game()
{
	// 1. Libere a View do Render Target primeiro
	if (m_pBackBufferView) {
		m_pBackBufferView->Release();
		m_pBackBufferView = nullptr;
	}

	// 2. Libere a Swap Chain
	if (m_pSwapChain) {
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}

	// 3. Libere o Contexto do Dispositivo
	if (m_pDevCon) {
		m_pDevCon->Release();
		m_pDevCon = nullptr;
	}

	// 4. Libere o Dispositivo por último
	if (m_pDevice) {
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
}





int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MyDX11Game Game;


	if (Game.Initialize(hInstance, nCmdShow))
	{
		Game.Run();
	}
	else
	{
		MessageBox(NULL, L"Failed to initialize the game or DirectX!", L"Error", MB_ICONERROR);
		return -1;
	}

	return 0;

}




LRESULT CALLBACK MyDX11Game::WindowProcedure( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{

	MyDX11Game* pApp = nullptr;

	if (uMsg == WM_NCCREATE) {
		auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		pApp = reinterpret_cast<MyDX11Game*>(cs->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
	}
	else {
		pApp = reinterpret_cast<MyDX11Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}


	if (pApp)
	{

		// Here you can handle messages like WM_DESTROY
		if (uMsg == WM_CLOSE)
		{
			if (MessageBoxW(pApp->m_hWnd, L"Are you sure you want to quit?", L"Confirm Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				DestroyWindow(pApp->m_hWnd);
			}
			return 0;
		}

		if (uMsg == WM_DESTROY)
		{
			PostQuitMessage(0);
			return 0;
		}

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}
