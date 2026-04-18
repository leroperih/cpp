
#include "game.h"





bool MyDX3DGame::Initialize(HINSTANCE hInstance, int nCmdShow)
{

	m_hInstance = hInstance;

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = m_hInstance;
	wc.lpszClassName = m_WndClassName;
	wc.lpfnWndProc = WindowProcedure;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.hIcon = NULL;
	wc.hIconSm = nullptr;
	wc.lpszMenuName = NULL;

	RegisterClassExW(&wc);



	HWND local_hWnd = CreateWindowExW(
		0,
		m_WndClassName,
		m_WndTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		m_WndWidth,
		m_WndHeight,
		NULL,
		NULL,
		m_hInstance,
		this
	);

	m_hWnd = local_hWnd;

	if (local_hWnd == NULL) return false;

	ShowWindow(m_hWnd, nCmdShow);

	if (!InitializeDirectX()) return false;

	return true;

}

bool MyDX3DGame::InitializeDirectX()
{


	// Create Device
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&m_pDevice,
		&m_ActualLevel,
		&m_pDevCon
	);
	if (FAILED(hr)) return false;




	// Create Factory
	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pFactory));
	if (FAILED(hr)) return false;





	// Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = m_WndWidth;
	sd.Height = m_WndHeight;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.Stereo = FALSE;
	sd.SampleDesc.Count = 1;      // Must be 1 for Flip Model
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;           // Flip model requires at least 2
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.Flags = 0;

	// Create the SwapChain
	IDXGISwapChain1* pSwapChain1 = nullptr;
	hr = m_pFactory->CreateSwapChainForHwnd(m_pDevice, m_hWnd, &sd, nullptr, nullptr, &pSwapChain1);
	if (FAILED(hr)) return false;

	// Cast to the base interface used in your header
	m_pSwapChain = pSwapChain1;

	// 2. CREATE THE RENDER TARGET VIEW (RTV)
	ID3D11Texture2D* pBackBuffer = nullptr;
	m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pBackBufferView);
	pBackBuffer->Release(); // Done with the texture pointer

	// 3. SET THE VIEWPORT
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_WndWidth;
	vp.Height = (FLOAT)m_WndHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pDevCon->RSSetViewports(1, &vp);

	return true;



}

void MyDX3DGame::Run()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Clear the back buffer to a color (e.g., Deep Blue)
			float color[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
			m_pDevCon->ClearRenderTargetView(m_pBackBufferView, color);

			// Set where to draw
			m_pDevCon->OMSetRenderTargets(1, &m_pBackBufferView, nullptr);

			// ... Your 3D Draw Calls Go Here ...

			// Present the frame to the screen
			m_pSwapChain->Present(1, 0);
		}
	}
}

MyDX3DGame::~MyDX3DGame()
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

















int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);


	MyDX3DGame game;


	if (game.Initialize(hInstance, nCmdShow))
	{

		game.Run();

	}

	return 0;

}





LRESULT CALLBACK MyDX3DGame::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{


	MyDX3DGame* pGame = nullptr;

	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*> (lParam);
		pGame = reinterpret_cast<MyDX3DGame*> (create->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (pGame));
	}
	else
	{
		pGame = reinterpret_cast<MyDX3DGame*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}



	switch (uMsg)
	{

		case WM_CREATE:



			{
				RECT rc;
				GetClientRect(hWnd, &rc);

				pGame->m_ClientWidth = rc.right - rc.left;
				pGame->m_ClientHeight = rc.bottom - rc.top;
			}
			break;

		case WM_CLOSE:



			if ( MessageBox(pGame->m_hWnd, L"Do you really want to quit the game?", L"Confirm Exit.", MB_ICONQUESTION | MB_YESNO) == IDYES )
			{
				DestroyWindow(pGame->m_hWnd);
			}
			return 0;
			break;

		case WM_DESTROY:



			PostQuitMessage(0);
			break;

		default:

			return DefWindowProc(hWnd, uMsg, wParam, lParam);

	}


	return 0;

}
