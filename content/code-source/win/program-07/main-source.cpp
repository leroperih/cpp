#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>

// Ponteiros do DirectX 11
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Ponteiros do Direct2D e DirectWrite
ID2D1Factory* g_pD2DFactory = nullptr;
ID2D1RenderTarget* g_pD2DRenderTarget = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;
IDWriteTextFormat* g_pTextFormat = nullptr;
IDWriteTextFormat* g_pTextFormatMedium = nullptr;

// Pincéis do DirectX
ID2D1SolidColorBrush* g_pBrushTexto = nullptr;
ID2D1SolidColorBrush* g_pBrushBotao = nullptr;
ID2D1SolidColorBrush* g_pBrushBotaoTexto = nullptr;

bool is_CloseButton_hovered = false;
bool is_TomarButton_hovered = false;

int g_wnd_Height = 435;
int g_wnd_Width = 460;

bool is_Ctrl_pressed = false;

const D2D1_RECT_F CLOSE_RECT = D2D1::RectF( (float)(g_wnd_Width - 32) , 0.0f, (float)(g_wnd_Width), 32.0f);

// Posição do botão (X1, Y1, X2, Y2)
const D2D1_RECT_F BOTAO_RECT = D2D1::RectF(40.0f, 350.0f, 380.0f, 410.0f);

const std::string NOME_ARQUIVO = "historico_remedios.txt";
std::vector<std::wstring> g_historico;

bool InitDevice(HWND hWnd);
void CleanupDevice();
void RenderizarInterfaceDX();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::wstring ConversaoUTF8ParaWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    // CORREÇÃO: Passando o endereço do primeiro caractere do buffer interno (&wstrTo[0])
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string ConversaoWStringParaUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    // CORREÇÃO: Passando o endereço do primeiro caractere do buffer interno (&strTo[0])
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Converte uma string de data para time_t (segundos desde 1970) para fazer cálculos matemáticos
std::time_t ConverterStringParaTime(const std::wstring& str) {
    std::tm t = {};
    std::wistringstream ss(str);
    wchar_t descarte;
    // Formato esperado: DD/MM/AAAA HH:MM:SS
    ss >> t.tm_mday >> descarte >> t.tm_mon >> descarte >> t.tm_year >> t.tm_hour >> descarte >> t.tm_min >> descarte >> t.tm_sec;

    t.tm_mon -= 1;     // Meses no tm vão de 0 a 11
    t.tm_year -= 1900; // Anos no tm contam a partir de 1900
    t.tm_isdst = -1;   // Deixa o sistema determinar o horário de verão

    return std::mktime(&t);
}

std::wstring GetCurrentTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);
    wchar_t buffer[32];
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d/%m/%Y %H:%M:%S", &ltm);
    return std::wstring(buffer);
}

// Verifica se a hora atual passou das 18h
bool IsAfter18h() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);
    return ltm.tm_hour >= 18;
}

// Nova lógica de validação do botão
bool PodeTomarRemedio(std::wstring& motivoBloqueio) {
    std::time_t agora = std::time(nullptr);
    std::tm ltmAgora;
    localtime_s(&ltmAgora, &agora);

    // 1. Carrega a última linha válida do arquivo
    std::ifstream arquivo(NOME_ARQUIVO);
    std::string linha;
    std::string ultimaLinhaValida = "";

    if (arquivo.is_open()) {
        while (std::getline(arquivo, linha)) {
            while (!linha.empty() && (linha.back() == '\n' || linha.back() == '\r')) {
                linha.pop_back();
            }
            if (!linha.empty()) {
                ultimaLinhaValida = linha;
            }
        }
        arquivo.close();
    }

    // 2. Se houver histórico, extrai o timestamp do último registro
    bool temHistorico = false;
    std::time_t tempoUltimaDose = -1;

    if (!ultimaLinhaValida.empty()) {
        std::wstring ultimaLinhaW = ConversaoUTF8ParaWString(ultimaLinhaValida);
        tempoUltimaDose = ConverterStringParaTime(ultimaLinhaW);
        if (tempoUltimaDose != -1) {
            temHistorico = true;
        }
    }

    // REGRA 1: Janela Noturna (Após as 18h de hoje)
    if (ltmAgora.tm_hour >= 18) {
        std::tm tmHoje18h = ltmAgora;
        tmHoje18h.tm_hour = 18;
        tmHoje18h.tm_min = 0;
        tmHoje18h.tm_sec = 0;
        std::time_t hoje18h = std::mktime(&tmHoje18h);

        if (temHistorico && tempoUltimaDose >= hoje18h) {
            motivoBloqueio = L"Dose da noite registrada";
            return false;
        }

        motivoBloqueio = L"Marcar Dose da Noite";
        return true;
    }

    // REGRA 2: Janela Diurna (Antes das 18h de hoje)
    if (temHistorico) {
        std::tm tmOntem18h = ltmAgora;
        tmOntem18h.tm_hour = 18;
        tmOntem18h.tm_min = 0;
        tmOntem18h.tm_sec = 0;

        std::time_t ontem18h = std::mktime(&tmOntem18h) - (24 * 3600);

        // Se o remédio já foi tomado após as 18h de ontem, exibe a contagem regressiva até as 18h de hoje
        if (tempoUltimaDose >= ontem18h) {
            std::tm tmHoje18h = ltmAgora;
            tmHoje18h.tm_hour = 18;
            tmHoje18h.tm_min = 0;
            tmHoje18h.tm_sec = 0;
            std::time_t hoje18h = std::mktime(&tmHoje18h);

            // Calcula a diferença em segundos entre agora e as 18h de hoje
            long long segundosRestantes = static_cast<long long>(std::difftime(hoje18h, agora));

            if (segundosRestantes > 0) {
                long long horas = segundosRestantes / 3600;
                long long minutos = (segundosRestantes % 3600) / 60;
                long long segundos = segundosRestantes % 60;

                wchar_t buffer[64];
                swprintf_s(buffer, L"%02lld:%02lld:%02lld", horas, minutos, segundos);
                motivoBloqueio = buffer;
            }
            else {
                motivoBloqueio = L"Liberado";
            }

            return false;
        }
    }

    motivoBloqueio = L"Marcar como Tomado";
    return true;
}

void SalvarNoArquivo(const std::wstring& texto) {
    std::wofstream arquivo(NOME_ARQUIVO, std::ios::app);
    if (arquivo.is_open()) {
        arquivo << texto << L"\n";
        arquivo.close();
    }
}

// Modificado para guardar estritamente os últimos 12 registros
void CarregarHistorico() {
    g_historico.clear();
    std::wifstream arquivo(NOME_ARQUIVO);
    std::wstring linha;
    std::vector<std::wstring> todosRegistros;

    if (arquivo.is_open()) {
        while (std::getline(arquivo, linha)) {
            if (!linha.empty()) {
                todosRegistros.push_back(linha);
            }
        }
        arquivo.close();
    }

    // Pega apenas os últimos 12 elementos do vetor (ou todos, se tiver menos que 12)
    size_t total = todosRegistros.size();
    size_t inicio = (total > 12) ? (total - 12) : 0;

    for (size_t i = inicio; i < total; ++i) {
        g_historico.push_back(todosRegistros[i]);
    }
}

void RenderizarInterfaceDX() {
    if (!g_pd3dDeviceContext || !g_mainRenderTargetView || !g_pD2DRenderTarget) return;

    const float corFundo[] = { 0.08f, 0.12f, 0.20f, 1.00f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, corFundo);

    g_pD2DRenderTarget->BeginDraw();

    std::wstring textoClose = L"X";
    if (is_CloseButton_hovered)
    {
        g_pBrushBotao->SetColor(D2D1::ColorF(0.65f, 0.0f, 0.0f, 1.0f)); // Vermelho escuro
    }
    else
    {
        g_pBrushBotao->SetColor(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f)); // Vermelho forte
    }
    g_pD2DRenderTarget->FillRectangle(CLOSE_RECT, g_pBrushBotao);
    g_pBrushBotaoTexto->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    D2D1_RECT_F close_Rect = D2D1::RectF(CLOSE_RECT.left, CLOSE_RECT.top - 8.0f, CLOSE_RECT.right, CLOSE_RECT.bottom - 8.0f);
    g_pD2DRenderTarget->DrawTextW(textoClose.c_str(), (UINT32)textoClose.length(), g_pTextFormatMedium, close_Rect, g_pBrushBotaoTexto);
    g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    // Título do Histórico
    g_pD2DRenderTarget->DrawTextW(L"Últimas 12 doses tomadas:", 25, g_pTextFormat, D2D1::RectF(20, 20, 400, 50), g_pBrushTexto);

    // Listar Histórico limitado
    float yPos = 60.0f;
    if (g_historico.empty()) {
        g_pD2DRenderTarget->DrawTextW(L"Nenhum registro encontrado.", 27, g_pTextFormat, D2D1::RectF(40, yPos, 400, yPos + 30), g_pBrushTexto);
    }
    else {
        for (const auto& dose : g_historico) {
            std::wstring item = L"• " + dose;
            g_pD2DRenderTarget->DrawTextW(item.c_str(), (UINT32)item.length(), g_pTextFormat, D2D1::RectF(40, yPos, 400, yPos + 25), g_pBrushTexto);
            yPos += 22.0f;
        }
    }

    // Validar estado do botão para atualizar a cor e o texto em tempo real
    std::wstring textoBotao;
    bool liberado = PodeTomarRemedio(textoBotao);

    if (liberado) {
        
        if (is_TomarButton_hovered)
        {
            g_pBrushBotao->SetColor(D2D1::ColorF(0.05f, 0.3f, 0.1f, 1.0f)); // Verde
        }
        else
        { 
            g_pBrushBotao->SetColor(D2D1::ColorF(0.1f, 0.6f, 0.2f, 1.0f));
        }
        
        g_pBrushBotaoTexto->SetColor(D2D1::ColorF(D2D1::ColorF::White));

        g_pD2DRenderTarget->FillRectangle(BOTAO_RECT, g_pBrushBotao);

        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        D2D1_RECT_F button_Rect = D2D1::RectF(BOTAO_RECT.left, BOTAO_RECT.top + 20.0f, BOTAO_RECT.right, BOTAO_RECT.bottom + 20.0f);
        g_pD2DRenderTarget->DrawTextW(textoBotao.c_str(), (UINT32)textoBotao.length(), g_pTextFormat, button_Rect, g_pBrushBotaoTexto);
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    else {
        g_pBrushBotao->SetColor(D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f)); // Cinza Escuro
        g_pBrushBotaoTexto->SetColor(D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f)); // Texto Opaco

        g_pD2DRenderTarget->FillRectangle(BOTAO_RECT, g_pBrushBotao);

        // Desenhar Texto Centralizado no Botão
        D2D1_RECT_F button_Rect = D2D1::RectF(BOTAO_RECT.left, BOTAO_RECT.top + 8.0f, BOTAO_RECT.right, BOTAO_RECT.bottom + 8.0f);
        g_pD2DRenderTarget->DrawTextW(textoBotao.c_str(), (UINT32)textoBotao.length(), g_pTextFormatMedium, button_Rect, g_pBrushBotaoTexto);
    }


    g_pD2DRenderTarget->EndDraw();
    g_pSwapChain->Present(1, 0);
}

bool g_MouseTrackeado = false;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCHITTEST: {
        float mouseX = (float)LOWORD(lParam);
        float mouseY = (float)HIWORD(lParam);

        POINT pt = { (LONG)mouseX, (LONG)mouseY };
        ::ScreenToClient(hWnd, &pt);

        // Se o mouse estiver sobre os botões, avise ao Windows que é uma área de clique normal (HTCLIENT).
        // Isso força o Windows a disparar o WM_MOUSEMOVE corretamente aqui dentro!
        if ((pt.x >= BOTAO_RECT.left && pt.x <= BOTAO_RECT.right && pt.y >= BOTAO_RECT.top && pt.y <= BOTAO_RECT.bottom) ||
            (pt.x >= CLOSE_RECT.left && pt.x <= CLOSE_RECT.right && pt.y >= CLOSE_RECT.top && pt.y <= CLOSE_RECT.bottom)) {
            return HTCLIENT;
        }

        return HTCAPTION;
    }

    case WM_MOUSEMOVE: {
        float mouseX = (float)LOWORD(lParam);
        float mouseY = (float)HIWORD(lParam);

        // Ativa o rastreamento para detectar quando o mouse sair TOTALMENTE da janela
        if (!g_MouseTrackeado) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
            g_MouseTrackeado = true;
        }

        // Verifica o botão de fechar
        bool novoCloseHover = (mouseX >= CLOSE_RECT.left && mouseX <= CLOSE_RECT.right &&
            mouseY >= CLOSE_RECT.top && mouseY <= CLOSE_RECT.bottom);

        // Verifica o botão de tomar remédio
        bool novoTomarHover = (mouseX >= BOTAO_RECT.left && mouseX <= BOTAO_RECT.right &&
            mouseY >= BOTAO_RECT.top && mouseY <= BOTAO_RECT.bottom);

        // Só força o redesenho se o estado de hover mudou (evita lag de renderização desnecessária)
        if (novoCloseHover != is_CloseButton_hovered || novoTomarHover != is_TomarButton_hovered) {
            is_CloseButton_hovered = novoCloseHover;
            is_TomarButton_hovered = novoTomarHover;
            InvalidateRect(hWnd, nullptr, FALSE); // Força o DirectX a redesenhar com a cor nova
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        // O mouse saiu da janela inteira, reseta todos os hovers imediatamente
        is_CloseButton_hovered = false;
        is_TomarButton_hovered = false;
        g_MouseTrackeado = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        float mouseX = (float)LOWORD(lParam);
        float mouseY = (float)HIWORD(lParam);

        if (mouseX >= CLOSE_RECT.left && mouseX <= CLOSE_RECT.right &&
            mouseY >= CLOSE_RECT.top && mouseY <= CLOSE_RECT.bottom) {
            PostQuitMessage(0);
            return 0;
        }

        if (mouseX >= BOTAO_RECT.left && mouseX <= BOTAO_RECT.right &&
            mouseY >= BOTAO_RECT.top && mouseY <= BOTAO_RECT.bottom) {

            std::wstring motivo;
            if (PodeTomarRemedio(motivo)) {
                std::wstring horario = GetCurrentTimeString();
                SalvarNoArquivo(horario);
                CarregarHistorico();
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }
        return 0;
    }

    case WM_KEYDOWN: {
        if (is_Ctrl_pressed && wParam == 'W') { PostQuitMessage(0); }
        if (wParam == VK_CONTROL) { is_Ctrl_pressed = true; }
        break;
    }

    case WM_KEYUP: {
        if (wParam == VK_CONTROL) { is_Ctrl_pressed = false; }
        break;
    }

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}




// Inicialização estável do DirectX 11 + Direct2D
bool InitDevice(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        nullptr,
        &g_pd3dDeviceContext
    );
    if (FAILED(res)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (FAILED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)))) return false;

    res = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    if (FAILED(res)) {
        pBackBuffer->Release();
        return false;
    }

    res = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
    if (FAILED(res)) {
        pBackBuffer->Release();
        return false;
    }

    IDXGISurface* pDXGISurface = nullptr;
    res = pBackBuffer->QueryInterface(IID_PPV_ARGS(&pDXGISurface));
    if (SUCCEEDED(res)) {
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        res = g_pD2DFactory->CreateDxgiSurfaceRenderTarget(pDXGISurface, &props, &g_pD2DRenderTarget);
        pDXGISurface->Release();
    }
    pBackBuffer->Release();

    if (FAILED(res)) return false;

    res = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pDWriteFactory);
    if (FAILED(res)) return false;

    res = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.0f,
        L"pt-BR",
        &g_pTextFormat
    );

    res = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        32.0f,
        L"pt-BR",
        &g_pTextFormatMedium
    );
    if (FAILED(res)) return false;

    g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_pTextFormatMedium->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    g_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &g_pBrushTexto);
    g_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f), &g_pBrushBotao);
    g_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &g_pBrushBotaoTexto);

    return true;
}




int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW | CS_VREDRAW, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"ClasseDXPuro24h", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Gerenciador de Remédios - DirectX Puro", WS_POPUP | WS_VISIBLE, 100, 100, g_wnd_Width, g_wnd_Height, nullptr, nullptr, hInstance, nullptr);

    if (!hwnd || !InitDevice(hwnd)) {
        CleanupDevice();
        return 1;
    }

    CarregarHistorico();

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }
        RenderizarInterfaceDX();
    }

    CleanupDevice();
    return (int)msg.wParam;
}

void CleanupDevice() {
    if (g_pBrushTexto) { g_pBrushTexto->Release(); g_pBrushTexto = nullptr; }
    if (g_pBrushBotao) { g_pBrushBotao->Release(); g_pBrushBotao = nullptr; }
    if (g_pBrushBotaoTexto) { g_pBrushBotaoTexto->Release(); g_pBrushBotaoTexto = nullptr; }

    if (g_pTextFormatMedium) { g_pTextFormatMedium->Release(); g_pTextFormatMedium = nullptr; }
    if (g_pTextFormat) { g_pTextFormat->Release(); g_pTextFormat = nullptr; }

    if (g_pDWriteFactory) { g_pDWriteFactory->Release(); g_pDWriteFactory = nullptr; }
    if (g_pD2DRenderTarget) { g_pD2DRenderTarget->Release(); g_pD2DRenderTarget = nullptr; }
    if (g_pD2DFactory) { g_pD2DFactory->Release(); g_pD2DFactory = nullptr; }
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
