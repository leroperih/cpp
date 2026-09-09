#include <windows.h>
#include <d3d11.h>

// Automatically link the necessary library
#pragma comment (lib, "d3d11.lib")

// Window procedure to handle basic messages like closing the window
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Setup and Register the Window Class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WindowProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, L"DXClass", NULL };
    RegisterClassEx(&wc);

    // 2. Create the Window
    HWND hWnd = CreateWindowEx(0, L"DXClass", L"Simple DirectX Window", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);

    // 3. Initialize DirectX 11 and the Swap Chain
    DXGI_SWAP_CHAIN_DESC scd = { 0 };
    scd.BufferCount = 1;                                    // One back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // Standard color format
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // Use as a render target
    scd.OutputWindow = hWnd;                                // Target our window
    scd.SampleDesc.Count = 4;                               // Basic anti-aliasing
    scd.Windowed = TRUE;                                    // Windowed mode

    ID3D11Device* device;
    ID3D11DeviceContext* devcon;
    IDXGISwapChain* swapchain;

    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &scd, &swapchain, &device, NULL, &devcon);

    // 4. Main Game Loop
    MSG msg;
    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Render: Clear the screen to a specific color (Dark Blue)
        float color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        ID3D11Texture2D* pBackBuffer;
        swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

        ID3D11RenderTargetView* backbuffer;
        device->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
        pBackBuffer->Release();

        devcon->ClearRenderTargetView(backbuffer, color);
        swapchain->Present(0, 0); // Display the frame
        backbuffer->Release();
    }

    return msg.wParam;
}