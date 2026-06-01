
#include "game.h"

using namespace DirectX;


















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

	if (!InitializeDirectX()) {
		MessageBoxA(m_hWnd, "Falha ao inicializar a engine 3D!", "Erro", MB_ICONERROR);
		return false;
	}

	CreateDoor(DirectX::XMFLOAT3(10.0f, 0.0f, 5.0f), 0.0f, 0.0f, false);
	CreateDoor(DirectX::XMFLOAT3(25.0f, 0.0f, -12.0f), 90.0f, 90.0f, false);
	CreateDoor(DirectX::XMFLOAT3(-5.0f, 0.0f, 18.0f), 90.0f, 90.0f, true);

	if (!InitializeDirect2D()) {
		MessageBoxA(m_hWnd, "Falha ao inicializar a interface 2D/Texto!", "Erro", MB_ICONERROR);
		return false;
	}

	if (!InitializeAudio()) {
		MessageBoxA(m_hWnd, "Falha ao inicializar a interface Audio!", "Erro", MB_ICONERROR);
		return false;
	}
	return true;

}



bool MyDX3DGame::InitializeDirectX() {
	// --- CONFIGURAÇÃO DE INPUT ---
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = m_hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	// 1. CREATE DEVICE & CONTEXT (CORRIGIDO COM A FLAG D3D11_CREATE_DEVICE_BGRA_SUPPORT)
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // ESSA FLAG É OBRIGATÓRIA PARA O DIRECT2D FUNCIONAR!

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // Opcional: Ativa mensagens detalhadas de erro no modo Debug
#endif

	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
		featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
		&m_pDevice, &m_ActualLevel, &m_pDevCon);
	if (FAILED(hr)) return false;

	// 2. CREATE FACTORY & SWAP CHAIN
	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pFactory));
	if (FAILED(hr)) return false;

	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = m_WndWidth;
	sd.Height = m_WndHeight;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> pSwapChain1;
	hr = m_pFactory->CreateSwapChainForHwnd(m_pDevice.Get(), m_hWnd, &sd, nullptr, nullptr, &pSwapChain1);
	if (FAILED(hr)) return false;
	m_pSwapChain = pSwapChain1;

	// 3. CREATE RENDER TARGET VIEW
	ComPtr<ID3D11Texture2D> pBackBuffer;
	m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pBackBufferView);
	if (FAILED(hr)) return false;

	// 4. CREATE DEPTH STENCIL VIEW
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = m_WndWidth;
	descDepth.Height = m_WndHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pDepthStencil;
	m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
	hr = m_pDevice->CreateDepthStencilView(pDepthStencil.Get(), nullptr, &m_pDepthStencilView);
	if (FAILED(hr)) return false;

	// 5. BIND TARGETS & SET VIEWPORT
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)m_WndWidth, (FLOAT)m_WndHeight, 0.0f, 1.0f };
	m_pDevCon->RSSetViewports(1, &vp);

	// 6. GEOMETRY BUFFERS (MAPA E PORTA UNIFICADOS)
	std::vector<VertexOBJ> allVertices;

	// 6.1 Carregar Mapa
	std::vector<VertexOBJ> mapVertices;
	if (!LoadOBJ("mapa-detalhado.obj", "mapa-transforms.txt", mapVertices))
	{
		MessageBoxA(m_hWnd, "ERRO: Falha ao carregar o mapa!", "Erro", MB_ICONERROR);
		return false;
	}
	allVertices.insert(allVertices.end(), mapVertices.begin(), mapVertices.end());
	m_VertexCount = (UINT)mapVertices.size();

	// 6.2 Carregar o modelo 3D da Porta (Folha isolada na origem)
	m_DoorStartOffset = (UINT)allVertices.size(); // Guarda onde a porta começa no buffer
	std::vector<VertexOBJ> doorVertices;
	if (!LoadOBJ("door-detalhado.obj", doorVertices)) // Usa a versão de 2 parâmetros que você declarou na classe
	{
		MessageBoxA(m_hWnd, "ERRO: Falha ao carregar porta-modelo.obj!", "Erro", MB_ICONERROR);
		return false;
	}
	allVertices.insert(allVertices.end(), doorVertices.begin(), doorVertices.end());
	m_DoorVertexCount = (UINT)doorVertices.size(); // Quantidade de vértices da porta

	// Criar Vertex Buffer único para GPU (Contendo Mapa + Porta)
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(VertexOBJ) * (UINT)allVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vData = { allVertices.data() };
	hr = m_pDevice->CreateBuffer(&vbd, &vData, &m_pVertexBuffer);
	if (FAILED(hr)) return false;

	// 7. CONSTANT BUFFER (Matrizes)
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = (sizeof(ConstantBuffer) + 15) & ~0xf; // Alinhamento de 16 bytes
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_pDevice->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);

	// 8. SHADERS
	ID3DBlob* vsBlob = nullptr, * psBlob = nullptr, * errorBlob = nullptr;

	// Vertex Shader
	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) errorBlob->Release();
		return false;
	}
	m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVertexShader);

	// Input Layout
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_pDevice->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);
	vsBlob->Release();

	// Pixel Shader
	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) errorBlob->Release();
		return false;
	}
	m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	if (errorBlob) errorBlob->Release(); // Liberação final segur



	// --- 9. CONFIGURAÇÃO DO CUBO DE DEBUG ---
	VertexOBJ debugVertices[] = {
		// Face Frontal (Z = -0.5)
		{ {-0.5f, -0.5f, -0.5f}, {0,0}, {0,0,-1} }, { {-0.5f,  0.5f, -0.5f}, {0,0}, {0,0,-1} }, { { 0.5f, -0.5f, -0.5f}, {0,0}, {0,0,-1} },
		{ { 0.5f, -0.5f, -0.5f}, {0,0}, {0,0,-1} }, { {-0.5f,  0.5f, -0.5f}, {0,0}, {0,0,-1} }, { { 0.5f,  0.5f, -0.5f}, {0,0}, {0,0,-1} },
		// Face Traseira (Z = 0.5)
		{ {-0.5f, -0.5f,  0.5f}, {0,0}, {0,0, 1} }, { { 0.5f, -0.5f,  0.5f}, {0,0}, {0,0, 1} }, { {-0.5f,  0.5f,  0.5f}, {0,0}, {0,0, 1} },
		{ {-0.5f,  0.5f,  0.5f}, {0,0}, {0,0, 1} }, { { 0.5f, -0.5f,  0.5f}, {0,0}, {0,0, 1} }, { { 0.5f,  0.5f,  0.5f}, {0,0}, {0,0, 1} },
		// Face Esquerda (X = -0.5)
		{ {-0.5f, -0.5f,  0.5f}, {0,0}, {-1,0,0} }, { {-0.5f,  0.5f,  0.5f}, {0,0}, {-1,0,0} }, { {-0.5f, -0.5f, -0.5f}, {0,0}, {-1,0,0} },
		{ {-0.5f, -0.5f, -0.5f}, {0,0}, {-1,0,0} }, { {-0.5f,  0.5f,  0.5f}, {0,0}, {-1,0,0} }, { {-0.5f,  0.5f, -0.5f}, {0,0}, {-1,0,0} },
		// Face Direita (X = 0.5)
		{ { 0.5f, -0.5f, -0.5f}, {0,0}, { 1,0,0} }, { { 0.5f,  0.5f, -0.5f}, {0,0}, { 1,0,0} }, { { 0.5f, -0.5f,  0.5f}, {0,0}, { 1,0,0} },
		{ { 0.5f, -0.5f,  0.5f}, {0,0}, { 1,0,0} }, { { 0.5f,  0.5f, -0.5f}, {0,0}, { 1,0,0} }, { { 0.5f,  0.5f,  0.5f}, {0,0}, { 1,0,0} },
		// Face Topo (Y = 0.5)
		{ {-0.5f,  0.5f, -0.5f}, {0,0}, {0, 1,0} }, { {-0.5f,  0.5f,  0.5f}, {0,0}, {0, 1,0} }, { { 0.5f,  0.5f, -0.5f}, {0,0}, {0, 1,0} },
		{ { 0.5f,  0.5f, -0.5f}, {0,0}, {0, 1,0} }, { {-0.5f,  0.5f,  0.5f}, {0,0}, {0, 1,0} }, { { 0.5f,  0.5f,  0.5f}, {0,0}, {0, 1,0} },
		// Face Baixo (Y = -0.5)
		{ {-0.5f, -0.5f,  0.5f}, {0,0}, {0,-1,0} }, { {-0.5f, -0.5f, -0.5f}, {0,0}, {0,-1,0} }, { { 0.5f, -0.5f,  0.5f}, {0,0}, {0,-1,0} },
		{ { 0.5f, -0.5f,  0.5f}, {0,0}, {0,-1,0} }, { {-0.5f, -0.5f, -0.5f}, {0,0}, {0,-1,0} }, { { 0.5f, -0.5f, -0.5f}, {0,0}, {0,-1,0} }
	};

	D3D11_BUFFER_DESC dbd = {};
	dbd.Usage = D3D11_USAGE_DEFAULT;
	dbd.ByteWidth = sizeof(debugVertices);
	dbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA dData = { debugVertices };
	m_pDevice->CreateBuffer(&dbd, &dData, &m_pDebugCubeBuffer);
	psBlob->Release();



	D3D11_RASTERIZER_DESC wfDesc = {};
	wfDesc.FillMode = D3D11_FILL_WIREFRAME;
	wfDesc.CullMode = D3D11_CULL_NONE; // Permite ver através das caixas
	m_pDevice->CreateRasterizerState(&wfDesc, &m_pWireframeRS);


	return true;
}



bool MyDX3DGame::InitializeDirect2D() {

	HRESULT hr;

	// 1. Criar a Factory do Direct2D (MULTI_THREADED previne travas com Raw Input)
	D2D1_FACTORY_OPTIONS options = {};
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), &options, &m_pD2DFactory);
	if (FAILED(hr)) { OutputDebugStringA("Falha: D2D1CreateFactory\n"); return false; }

	// 2. Obter a interface DXGI a partir do dispositivo Direct3D existente
	ComPtr<IDXGIDevice> pDxgiDevice;
	hr = m_pDevice.As(&pDxgiDevice);
	if (FAILED(hr)) { OutputDebugStringA("Falha: m_pDevice.As DXGI\n"); return false; }

	// 3. Criar o Device e o Device Context do Direct2D
	ComPtr<ID2D1Device> pD2DDevice;
	hr = m_pD2DFactory->CreateDevice(pDxgiDevice.Get(), &pD2DDevice);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateDevice\n"); return false; }

	hr = pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DContext);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateDeviceContext\n"); return false; }

	// 4. Criar Recursos de Desenho (Brushes)
	hr = m_pD2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pScreenBrush);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateSolidColorBrush\n"); return false; }

	// 5. Criar a Factory do DirectWrite e Formatação de Texto
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_pDWriteFactory);
	if (FAILED(hr)) { OutputDebugStringA("Falha: DWriteCreateFactory\n"); return false; }

	hr = m_pDWriteFactory->CreateTextFormat(
		L"Arial",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"pt-BR",
		&m_pTextFormat_big
	);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateTextFormat\n"); return false; }

	hr = m_pDWriteFactory->CreateTextFormat(
		L"Segoe UI",
		nullptr,
		DWRITE_FONT_WEIGHT_MEDIUM,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		28.0f,
		L"pt-BR",
		&m_pTextFormat_elegant
	);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateTextFormat\n"); return false; }

	m_pTextFormat_elegant->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat_elegant->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_pTextFormat_big->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat_big->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


	VertexOBJ crosshairVertices[] = {
		// Linha Horizontal
		{ {-0.02f, 0.0f, 0.0f}, {0,0}, {0,0,0} },
		{ { 0.02f, 0.0f, 0.0f}, {0,0}, {0,0,0} },
		// Linha Vertical
		{ {0.0f, -0.03f, 0.0f}, {0,0}, {0,0,0} },
		{ {0.0f,  0.03f, 0.0f}, {0,0}, {0,0,0} }
	};

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(crosshairVertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = { crosshairVertices };
	m_pDevice->CreateBuffer(&bd, &initData, &m_pCrosshairBuffer);

	// 6. Vincula o Direct2D ao BackBuffer do Direct3D
	if (!m_pD2DContext || !m_pSwapChain) return false;

	// Garante que o contexto comece limpo
	m_pD2DContext->SetTarget(nullptr);
	m_pD2DBackBufferTarget.Reset(); // CORRIGIDO: Limpa o ponteiro da classe antes de recriar

	// Obtém a superfície DXGI do BackBuffer da Swap Chain
	ComPtr<IDXGISurface> pBackBufferSurface;
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferSurface));
	if (FAILED(hr)) return false;

	// Configura as propriedades do Bitmap do Direct2D para aceitar o formato DXGI
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE) // Ajustado para IGNORE igual ao swapchain
	);

	// CORRIGIDO: Salva diretamente na variável membro da classe de forma persistente
	hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pBackBufferSurface.Get(), &bitmapProperties, &m_pD2DBackBufferTarget);
	if (FAILED(hr)) return false;

	// Define o bitmap persistente da classe como o alvo atual do Direct2D
	m_pD2DContext->SetTarget(m_pD2DBackBufferTarget.Get());
	
	return true;
}



bool MyDX3DGame::InitializeAudio() {

	// 1. Inicializa o sistema COM do Windows (obrigatório para XAudio2)
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// 2. Criar a instância do XAudio2
	HRESULT hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) return false;

	// 3. Criar a Mastering Voice (Sua saída de som padrão)
	hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice);
	if (FAILED(hr)) return false;

	// 4. Carregar os arquivos .wav físicos
	if (!LoadWavFile("assets/jump.wav", m_sndJump)) {
		// Isso vai abrir uma janela real na sua tela se ele NÃO achar o arquivo
		MessageBoxA(m_hWnd, "ERRO: O arquivo 'assets/jump.wav' nao foi encontrado!", "Erro de Audio", MB_ICONERROR);
	}
	if (!LoadWavFile("assets/door.wav", m_sndDoor)) {
		// Isso vai abrir uma janela real na sua tela se ele NÃO achar o arquivo
		MessageBoxA(m_hWnd, "ERRO: O arquivo 'assets/door.wav' nao foi encontrado!", "Erro de Audio", MB_ICONERROR);
	}
	if (!LoadWavFile("assets/step.wav", m_sndStep)) {
		// Isso vai abrir uma janela real na sua tela se ele NÃO achar o arquivo
		MessageBoxA(m_hWnd, "ERRO: O arquivo 'assets/step.wav' nao foi encontrado!", "Erro de Audio", MB_ICONERROR);
	}

	m_sndStep.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	// 5. Criar as Source Voices vinculando o formato do áudio de cada um
	if (m_sndJump.buffer.AudioBytes > 0)
		m_pXAudio2->CreateSourceVoice(&m_pSourceVoiceJump, &m_sndJump.wfx);

	if (m_sndDoor.buffer.AudioBytes > 0)
		m_pXAudio2->CreateSourceVoice(&m_pSourceVoiceDoor, &m_sndDoor.wfx);

	if (m_sndStep.buffer.AudioBytes > 0)
		m_pXAudio2->CreateSourceVoice(&m_pSourceVoiceStep, &m_sndStep.wfx);

	return true;
}



void MyDX3DGame::PlaySoundFX(IXAudio2SourceVoice* pSourceVoice, const SoundEffect& sound) {
	if (!pSourceVoice || sound.audioData.empty()) return;

	pSourceVoice->Stop(0);                // Para o som se ele já estiver tocando
	pSourceVoice->FlushSourceBuffers();    // Limpa a fila antiga
	pSourceVoice->SubmitSourceBuffer(&sound.buffer); // Envia o buffer novo
	pSourceVoice->Start(0);               // Executa
}



void MyDX3DGame::ShowInitialScreen()
{
	if (!m_pD2DContext) return;

	HRESULT hr = S_OK;

	// 1. CRIA O FABRICANTE DO WIC (Decodificador de Imagens do Windows)
	ComPtr<IWICImagingFactory> pWICFactory;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pWICFactory)
	);
	if (FAILED(hr)) return;

	// 2. CARREGA O ARQUIVO PNG (Certifique-se de que "logo.png" está na pasta do executável)
	ComPtr<IWICBitmapDecoder> pDecoder;
	hr = pWICFactory->CreateDecoderFromFilename(
		L"jahy-icon.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&pDecoder
	);
	if (FAILED(hr)) return;

	// 3. PEGA O PRIMEIRO FRAME DA IMAGEM
	ComPtr<IWICBitmapFrameDecode> pFrame;
	hr = pDecoder->GetFrame(0, &pFrame);
	if (FAILED(hr)) return;

	// 4. CONVERTE O PNG PARA O FORMATO DE CORES DO DIRECT2D (BGRA de 32 bits)
	ComPtr<IWICFormatConverter> pConverter;
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return;

	hr = pConverter->Initialize(
		pFrame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeCustom
	);
	if (FAILED(hr)) return;

	// 5. CRIA O BITMAP DO DIRECT2D A PARTIR DA IMAGEM CONVERTIDA
	hr = m_pD2DContext->CreateBitmapFromWicBitmap(
		pConverter.Get(),
		nullptr,
		&m_pLogoBitmap
	);
	if (FAILED(hr)) return;

	// --- 6. RENDERIZAÇÃO DA TELA DE ABERTURA ---
	m_pD2DContext->BeginDraw();

	// Limpa a tela com fundo preto
	m_pD2DContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

	// Pega o tamanho da imagem carregada
	D2D1_SIZE_F bitmapSize = m_pLogoBitmap->GetSize();

	// Centraliza a imagem na janela do seu jogo
	float startX = ((float)m_WndWidth - bitmapSize.width) / 2.0f;
	float startY = ((float)m_WndHeight - bitmapSize.height) / 2.0f - 50.0f; // Sobe 50px para dar espaço ao texto

	D2D1_RECT_F renderRect = D2D1::RectF(
		startX,
		startY,
		startX + bitmapSize.width,
		startY + bitmapSize.height
	);

	// Desenha o PNG na tela
	m_pD2DContext->DrawBitmap(m_pLogoBitmap.Get(), renderRect);

	// Desenha o texto de abertura abaixo da logo
	D2D1_RECT_F textRect = D2D1::RectF(
		0.0f,
		250.0f,
		(float)m_WndWidth,
		(float)m_WndHeight
	);

	// Configura o seu pincel para a cor branca
	m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

	m_pD2DContext->DrawTextW(
		L"Lesro Games Productions",
		23, // Quantidade de caracteres no texto
		m_pTextFormat_elegant.Get(), // Seu formato de texto elegante declarado no game.h
		textRect,
		m_pScreenBrush.Get()
	);

	m_pD2DContext->EndDraw();

	// Envia o frame renderizado imediatamente para o monitor antes de travar o tempo
	m_pSwapChain->Present(1, 0);

	// 7. SEGURA A IMAGEM NA TELA POR 3 SEGUNDOS (3000 milissegundos)
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));

	// Libera a memória da logo, pois não vamos mais usá-la durante a gameplay
	m_pLogoBitmap.Reset();
}



void MyDX3DGame::Run()
{

	MSG msg = { 0 };

	ShowInitialScreen();

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			// Se a mensagem for de fechar a janela, encerra o loop
			if (msg.message == WM_QUIT)
				break;
		}
		else
		{
			Update();
			Render();
		}
	}
}


















bool MyDX3DGame::LoadOBJ(const char* objPath, const char* transformPath, std::vector<VertexOBJ>& out_vertices) {

	// --- PASSO 1: CARREGAMENTO DE TODA A GEOMETRIA BRUTA (TABELA GLOBAL) ---
	std::vector<DirectX::XMFLOAT3> temp_vertices;
	std::vector<DirectX::XMFLOAT2> temp_uvs;
	std::vector<DirectX::XMFLOAT3> temp_normals;

	std::ifstream fileGeometry(objPath);
	if (!fileGeometry.is_open()) return false;

	std::string line;
	while (std::getline(fileGeometry, line)) {
		if (line.empty()) continue;
		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "v") {
			DirectX::XMFLOAT3 v;
			ss >> v.x >> v.y >> v.z;
			temp_vertices.push_back(v);
		}
		else if (prefix == "vt") {
			DirectX::XMFLOAT2 vt;
			ss >> vt.x >> vt.y;
			temp_uvs.push_back({ vt.x, 1.0f - vt.y });
		}
		else if (prefix == "vn") {
			DirectX::XMFLOAT3 vn;
			ss >> vn.x >> vn.y >> vn.z;
			temp_normals.push_back(vn);
		}
	}
	fileGeometry.close();

	// --- PASSO 2: LEITURA DE ESTRUTURAS, FACES E ASSOCIAÇÃO COM O TXT ---
	std::ifstream fileStructure(objPath);
	if (!fileStructure.is_open()) return false;

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<unsigned int> currentMeshVIndices;

	// Acumuladores locais de vértices para medir o tamanho exato da OBB na origem
	DirectX::XMFLOAT3 wallMin = { FLT_MAX, FLT_MAX, FLT_MAX };
	DirectX::XMFLOAT3 wallMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	std::string currentObjectName = "";
	bool isFloorObject = false;
	bool isWallObject = false;

	auto ProcessCollisionMesh = [&]() {
		if ((!isFloorObject && !isWallObject) || currentObjectName.empty()) return;

		std::ifstream transFile(transformPath);
		if (!transFile.is_open()) return;

		std::string tLine;
		while (std::getline(transFile, tLine)) {
			if (tLine.empty()) continue;
			std::stringstream tss(tLine);
			std::string type, name;
			float rawX, rawY, rawZ, rawRX, rawRY, rawRZ, rawRW;

			tss >> type >> name >> rawX >> rawY >> rawZ >> rawRX >> rawRY >> rawRZ >> rawRW;

			std::string normName = name;
			std::string normCurrent = currentObjectName;
			std::replace(normName.begin(), normName.end(), '-', '_');
			std::replace(normCurrent.begin(), normCurrent.end(), '-', '_');

			std::transform(normName.begin(), normName.end(), normName.begin(), ::tolower);
			std::transform(normCurrent.begin(), normCurrent.end(), normCurrent.begin(), ::tolower);

			size_t dotName = normName.find('.');
			if (dotName != std::string::npos) normName = normName.substr(0, dotName);

			size_t dotCurrent = normCurrent.find('.');
			if (dotCurrent != std::string::npos) normCurrent = normCurrent.substr(0, dotCurrent);

			bool nomeBateu = (normCurrent.find(normName) != std::string::npos || normName.find(normCurrent) != std::string::npos);

			if (nomeBateu) {
				using namespace DirectX;

				// Posicionamento Y-Up padrão do conversor do Blender
				float px = rawX;
				float py = rawZ;
				float pz = rawY;

				XMVECTOR qRot = XMVectorSet(rawRX, rawRZ, rawRY, rawRW);

				bool agirComoChao = (type == "FLOOR" || normCurrent.find("floor") != std::string::npos);

				if (agirComoChao && !currentMeshVIndices.empty()) {
					for (size_t i = 0; i < currentMeshVIndices.size(); i += 3) {
						if (i + 2 >= currentMeshVIndices.size()) break;

						if (currentMeshVIndices[i] >= temp_vertices.size() ||
							currentMeshVIndices[i + 1] >= temp_vertices.size() ||
							currentMeshVIndices[i + 2] >= temp_vertices.size()) continue;

						XMVECTOR v0 = XMLoadFloat3(&temp_vertices[currentMeshVIndices[i]]);
						XMVECTOR v1 = XMLoadFloat3(&temp_vertices[currentMeshVIndices[i + 1]]);
						XMVECTOR v2 = XMLoadFloat3(&temp_vertices[currentMeshVIndices[i + 2]]);

						Triangle tri;
						XMStoreFloat3(&tri.v0, v0);
						XMStoreFloat3(&tri.v1, v2);
						XMStoreFloat3(&tri.v2, v1);

						// 1. Encontra os limites mínimos e máximos (AABB) do triângulo nos eixos X e Z
						float minX = (std::min)({ tri.v0.x, tri.v1.x, tri.v2.x });
						float maxX = (std::max)({ tri.v0.x, tri.v1.x, tri.v2.x });
						float minZ = (std::min)({ tri.v0.z, tri.v1.z, tri.v2.z });
						float maxZ = (std::max)({ tri.v0.z, tri.v1.z, tri.v2.z });

						int startX, startZ, endX, endZ;
						m_SpatialGrid.GetCellIndices(minX, minZ, startX, startZ);
						m_SpatialGrid.GetCellIndices(maxX, maxZ, endX, endZ);

						// 2. Registra o mesmo triângulo em todas as células que ele abrange horizontalmente
						for (int cx = startX; cx <= endX; ++cx) {
							for (int cz = startZ; cz <= endZ; ++cz) {
								std::string key = m_SpatialGrid.GetCellKey(cx, cz);
								m_SpatialGrid.cells[key].floorTriangles.push_back(tri);
							}
						}
					}
				}
				else if (type == "WALL") {
					using namespace DirectX;
					OBB box;

					// 1. O CENTRO REAL: Como o .obj já está no espaço do mundo, o centro geométrico 
					// calculado a partir dos vértices globais (wallMin e wallMax) está perfeitamente correto.
					box.center.x = (wallMax.x + wallMin.x) * 0.5f;
					box.center.y = (wallMax.y + wallMin.y) * 0.5f;
					box.center.z = (wallMax.z + wallMin.z) * 0.5f;

					// 2. A ORIENTAÇÃO REAL: Pegamos a rotação original vinda diretamente do arquivo .txt
					// Corrigindo o mapeamento de eixos do Blender (Z-up) para o DirectX (Y-up) e invertendo o W
					XMVECTOR qRot = XMVectorSet(rawRX, rawRZ, rawRY, -rawRW);
					XMStoreFloat4(&box.orientation, qRot);

					// 3. CORREÇÃO DO TAMANHO (EXTENTS) GIGANTE:
					// Para descobrir o tamanho real do cubo sem a inflação da rotação, nós pegamos os 
					// vértices globais, trazemos para a origem (-centro) e aplicamos o inverso da rotação.
					DirectX::XMFLOAT3 localMin = { FLT_MAX, FLT_MAX, FLT_MAX };
					DirectX::XMFLOAT3 localMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
					XMVECTOR invQ = XMQuaternionInverse(qRot);
					XMVECTOR vCenter = XMLoadFloat3(&box.center);

					for (unsigned int idx : currentMeshVIndices) {
						if (idx >= temp_vertices.size()) continue;

						// Carrega o vértice global do mundo
						XMVECTOR vGlobal = XMLoadFloat3(&temp_vertices[idx]);

						// Transforma o vértice de volta para o espaço local do objeto (Zera posição e rotação)
						XMVECTOR vLocal = XMVector3Rotate(vGlobal - vCenter, invQ);

						DirectX::XMFLOAT3 v;
						XMStoreFloat3(&v, vLocal);

						// Mede o tamanho real do bloco puro
						localMin.x = (std::min)(localMin.x, v.x); localMin.y = (std::min)(localMin.y, v.y); localMin.z = (std::min)(localMin.z, v.z);
						localMax.x = (std::max)(localMax.x, v.x); localMax.y = (std::max)(localMax.y, v.y); localMax.z = (std::max)(localMax.z, v.z);
					}

					// Define o tamanho real (metade da largura, altura e profundidade)
					box.extents.x = (localMax.x - localMin.x) * 0.5f;
					box.extents.y = (localMax.y - localMin.y) * 0.5f;
					box.extents.z = (localMax.z - localMin.z) * 0.5f;

					// NOVO: Adiciona no grid 2D usando o centro da OBB
					int cx, cz;
					m_SpatialGrid.GetCellIndices(box.center.x, box.center.z, cx, cz);
					std::string key = m_SpatialGrid.GetCellKey(cx, cz);
					m_SpatialGrid.cells[key].wallOBBs.push_back(box);
				}
				break;
			}
		}
		};

	// Variável de controle para sabermos a qual objeto as linhas 'v' pertencem sequencialmente
	size_t globalVertexCounter = 0;

	while (std::getline(fileStructure, line)) {
		if (line.empty()) continue;

		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "o" || prefix == "g") {
			ProcessCollisionMesh(); // Salva o objeto anterior com os limites calculados

			ss >> currentObjectName;
			std::string lowerName = currentObjectName;
			std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

			isFloorObject = (lowerName.find("floor") != std::string::npos);
			isWallObject = (lowerName.find("wall") != std::string::npos);

			currentMeshVIndices.clear();

			// Reseta os limites estritamente para o novo objeto que começou
			wallMin = { FLT_MAX, FLT_MAX, FLT_MAX };
			wallMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		}
		else if (prefix == "v") {
			// CORREÇÃO DE TAMANHO LOCAL: Lemos os vértices da malha atual.
			// Como o Blender agrupa os 'v' embaixo da linha 'o', medimos o tamanho local real do objeto aqui!
			if (isWallObject && globalVertexCounter < temp_vertices.size()) {
				DirectX::XMFLOAT3 v = temp_vertices[globalVertexCounter];
				wallMin.x = (std::min)(wallMin.x, v.x); wallMin.y = (std::min)(wallMin.y, v.y); wallMin.z = (std::min)(wallMin.z, v.z);
				wallMax.x = (std::max)(wallMax.x, v.x); wallMax.y = (std::max)(wallMax.y, v.y); wallMax.z = (std::max)(wallMax.z, v.z);
			}
			globalVertexCounter++;
		}
		else if (prefix == "f") {
			std::string vertexData;
			std::vector<unsigned int> faceVIndices;
			std::vector<int> faceTIndices;
			std::vector<int> faceNIndices;

			while (ss >> vertexData) {
				std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
				std::stringstream vss(vertexData);

				unsigned int vIdx = 0;
				int tIdx = -1, nIdx = -1;

				vss >> vIdx;
				if (vIdx == 0) continue;
				unsigned int internalIndex = vIdx - 1;

				faceVIndices.push_back(internalIndex);
				if (vss >> tIdx) faceTIndices.push_back(tIdx - 1);
				if (vss >> nIdx) faceNIndices.push_back(nIdx - 1);
			}

			if (isFloorObject || isWallObject) {
				for (unsigned int idx : faceVIndices) {
					currentMeshVIndices.push_back(idx);
				}
			}

			if (!isFloorObject && !isWallObject) {
				for (size_t i = 0; i < faceVIndices.size(); i++) {
					vertexIndices.push_back(faceVIndices[i]);
					if (i < faceTIndices.size()) uvIndices.push_back(faceTIndices[i]);
					if (i < faceNIndices.size()) normalIndices.push_back(faceNIndices[i]);
				}
			}
		}
	}

	ProcessCollisionMesh(); // Captura o último objeto do arquivo
	fileStructure.close();

	// Montagem do buffer de vértices visuais
	out_vertices.reserve(vertexIndices.size());
	for (size_t i = 0; i < vertexIndices.size(); i++) {
		if (vertexIndices[i] >= temp_vertices.size()) continue;
		VertexOBJ v = {};
		v.pos = temp_vertices[vertexIndices[i]];
		if (!uvIndices.empty() && uvIndices[i] >= 0 && uvIndices[i] < temp_uvs.size()) v.tex = temp_uvs[uvIndices[i]];
		if (!normalIndices.empty() && normalIndices[i] >= 0 && normalIndices[i] < temp_normals.size()) v.normal = temp_normals[normalIndices[i]];
		out_vertices.push_back(v);
	}

	m_VertexCount = (UINT)out_vertices.size();
	return true;
}



bool MyDX3DGame::LoadOBJ(const char* objPath, std::vector<VertexOBJ>& out_vertices) {
	// --- PASSO 1: CARREGAMENTO DE TODA A GEOMETRIA BRUTA ---
	std::vector<DirectX::XMFLOAT3> temp_vertices;
	std::vector<DirectX::XMFLOAT2> temp_uvs;
	std::vector<DirectX::XMFLOAT3> temp_normals;

	std::ifstream file(objPath);
	if (!file.is_open()) return false;

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::string line;

	while (std::getline(file, line)) {
		if (line.empty()) continue;
		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "v") {
			DirectX::XMFLOAT3 v;
			ss >> v.x >> v.y >> v.z;
			temp_vertices.push_back(v);
		}
		else if (prefix == "vt") {
			DirectX::XMFLOAT2 vt;
			ss >> vt.x >> vt.y;
			// Inverte o eixo Y da textura para o padrão do DirectX
			temp_uvs.push_back({ vt.x, 1.0f - vt.y });
		}
		else if (prefix == "vn") {
			DirectX::XMFLOAT3 vn;
			ss >> vn.x >> vn.y >> vn.z;
			temp_normals.push_back(vn);
		}
		else if (prefix == "f") {
			std::string vertexData;
			while (ss >> vertexData) {
				std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
				std::stringstream vss(vertexData);

				unsigned int vIdx = 0;
				int tIdx = -1, nIdx = -1;

				vss >> vIdx;
				if (vIdx == 0) continue;

				vertexIndices.push_back(vIdx - 1);
				if (vss >> tIdx) uvIndices.push_back(tIdx - 1);
				if (vss >> nIdx) normalIndices.push_back(nIdx - 1);
			}
		}
	}
	file.close();

	// --- PASSO 2: MONTAGEM DO BUFFER DE VÉRTICES VISUAIS ---
	out_vertices.reserve(vertexIndices.size());
	for (size_t i = 0; i < vertexIndices.size(); i++) {
		if (vertexIndices[i] >= temp_vertices.size()) continue;

		VertexOBJ v = {};
		v.pos = temp_vertices[vertexIndices[i]];

		if (!uvIndices.empty() && uvIndices[i] >= 0 && uvIndices[i] < temp_uvs.size()) {
			v.tex = temp_uvs[uvIndices[i]];
		}
		if (!normalIndices.empty() && normalIndices[i] >= 0 && normalIndices[i] < temp_normals.size()) {
			v.normal = temp_normals[normalIndices[i]];
		}

		out_vertices.push_back(v);
	}

	return true;
}



void MyDX3DGame::CreateDoor(XMFLOAT3 position, float currentRotation, float targetRotation, bool isOpen)
{
	Door myDoor;
	myDoor.position = position;
	myDoor.currentRotation = currentRotation;
	myDoor.targetRotation = targetRotation;
	myDoor.isOpen = isOpen;
	/*
	// Define o tamanho da folha da porta (ex: 1m de largura, 2m de altura, 0.2m de espessura)
	float width = 3.0f;
	float height = 2.0f;
	float thickness = 0.1f;

	myDoor.collisionBox = {
		{ position.x - 0.2f , position.y         , position.z - thickness },
		{ position.x + width, position.y + height, position.z + thickness }
	};
	*/
	m_Doors.push_back(myDoor); // Salva na lista da classe!
}



void MyDX3DGame::Update() {

	// 1. DELTA TIME
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0) timeStart = timeCur;
	float deltaTime = (timeCur - timeStart) / 1000.0f;
	timeStart = timeCur;

	// PAUSE
		if (m_isPause_screen)
		{
			POINT mousePos;
			GetCursorPos(&mousePos);
			ScreenToClient(m_hWnd, &mousePos);

			float mouseX = (float)mousePos.x;
			float mouseY = (float)mousePos.y;

			int hoverOption = -1;

			if (IsHovered(m_menu_pause_Opt1Rect, mouseX, mouseY))
			{
				hoverOption = 0;
			}
			else if (IsHovered(m_menu_pause_Opt2Rect, mouseX, mouseY))
			{
				hoverOption = 1;
			}
			else if (IsHovered(m_menu_pause_Opt3Rect, mouseX, mouseY))
			{
				hoverOption = 2;
			}

			if (hoverOption != -1)
			{
				m_SelectedOption = hoverOption;
			}

			return; // Mantém o congelamento da física do jogo
		}

		if (m_isSettings_screen)
		{
			POINT mousePos;
			GetCursorPos(&mousePos);
			ScreenToClient(m_hWnd, &mousePos); // Converte para coordenadas de dentro da janela

			float mouseX = (float)mousePos.x;
			float mouseY = (float)mousePos.y;

			// --- NOVA LÓGICA DE ARRASTO DIRETO NO UPDATE ---
			if (m_isDraggingSlider)
			{
				// Se o usuário soltar o clique esquerdo do mouse em qualquer lugar, para o arrasto
				if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
				{
					m_isDraggingSlider = false;
					ReleaseCapture();
				}
				else
				{
					// Calcula o percentual horizontal do mouse na pista
					float width = m_slider_TrackRect.right - m_slider_TrackRect.left;
					float pct = (mouseX - m_slider_TrackRect.left) / width;
					pct = std::clamp(pct, 0.0f, 1.0f); // Trava entre as extremidades

					// Atualiza o valor da sensibilidade instantaneamente
					m_MouseSens = m_MinSens + pct * (m_MaxSens - m_MinSens);
				}
			}
			else
			{
				// Se NÃO estiver arrastando, mantém a lógica original de hover dos botões
				int hoverOption = -1;

				if (IsHovered(m_menu_settings_Opt1Rect, mouseX, mouseY))
				{
					hoverOption = 0;
				}
				else if (IsHovered(m_menu_settings_Opt2Rect, mouseX, mouseY))
				{
					hoverOption = 1;
				}
				else if (IsHovered(m_menu_settings_Opt3Rect, mouseX, mouseY))
				{
					hoverOption = 2;
				}

				if (hoverOption != -1)
				{
					m_SelectedOption = hoverOption;
				}
			}

			return; // Mantém o congelamento da física do jogo
		}

		// DEBUG SHORTCUTS

		bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
		bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
		bool dKey = (GetAsyncKeyState('D') & 0x8000);
		bool iKey = (GetAsyncKeyState('I') & 0x8000);

		// DEUBUG BOXES
		if (ctrl && shift && dKey) {
			if (!m_ComboPressed) { m_ShowDebug = !m_ShowDebug; m_ComboPressed = true; }
		}
		else { m_ComboPressed = false; }

		// RELATÓRIO
		if (ctrl && shift && iKey)
		{
			MoreInformation(1);
		}

	// Audio
		bool isMoving = (GetAsyncKeyState('W') || GetAsyncKeyState('S') || GetAsyncKeyState('A') || GetAsyncKeyState('D'));

		if (m_IsGrounded && isMoving && !m_IsCrouching) {
			// Se o som não estiver tocando, inicia ele em modo loop
			XAUDIO2_VOICE_STATE state;
			m_pSourceVoiceStep->GetState(&state);

			if (state.BuffersQueued == 0) {
				// Envia o buffer configurado com Loop Infinito
				m_pSourceVoiceStep->SubmitSourceBuffer(&m_sndStep.buffer);
				m_pSourceVoiceStep->Start(0);
			}

			// Ajusta a velocidade de reprodução (frequência) do áudio baseado na corrida
			// Se estiver correndo (m_IsRunning), o som toca 1.6x mais rápido. Se andando, velocidade normal (1.0f)
			float frequencyRatio = m_IsRunning ? 1.6f : 1.0f;
			m_pSourceVoiceStep->SetFrequencyRatio(frequencyRatio);
		}
		else {
			// SE PAROU DE ANDAR OU SAIU DO CHÃO: Corta o som imediatamente!
			m_pSourceVoiceStep->Stop(0);
			m_pSourceVoiceStep->FlushSourceBuffers(); // Limpa a fila para não dar estalos ou "sobras"
		}




	// --- CONFIGURAÇÕES DO PLAYER ---
	float charRadius = 0.3f;
	float stepHeight = 0.3f; // Altura máxima que o player "ignora" para subir degraus

	// 1. CAPTURA O INPUT DO AGACHAMENTO (Segurando a tecla CTRL Esquerdo)
	m_IsCrouching = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;

	// Se estiver agachado, as alturas alvo diminuem. Se em pé, voltam ao normal.
	float targetCharHeight = m_IsCrouching ? 1.0f : 1.73f; // Corpo encolhe para 1 metro
	float targetEyeHeight = m_IsCrouching ? 0.9f : 1.62f; // Olhos descem para 90 centímetros

	// Interpolação suave para a transição não ser instantânea (Efeito amortecido)
	m_CurrentCharHeight += (targetCharHeight - m_CurrentCharHeight) * 10.0f * deltaTime;
	m_CurrentEyeHeight += (targetEyeHeight - m_CurrentEyeHeight) * 10.0f * deltaTime;

	// Substitui os valores fixos pelas variáveis dinâmicas interpoladas
	float charHeight = m_CurrentCharHeight;
	float eyeHeight = m_CurrentEyeHeight;

	// 2. INPUT E VELOCIDADE HORIZONTAL
	XMMATRIX moveRotation = XMMatrixRotationRollPitchYaw(0, m_Yaw, 0);
	XMVECTOR moveForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), moveRotation);
	XMVECTOR moveRight = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), moveRotation);
	XMVECTOR velocityVec = XMVectorSet(0, 0, 0, 0);


	bool wIsPressed = (GetAsyncKeyState('W') & 0x8000) != 0;

	// Detecta o exato frame em que o 'W' foi pressionado (Key Down Event)
	if (wIsPressed && !m_WWasPressed) {
		if (m_WaitingForDoubleTap && m_WReleaseTimer <= DOUBLE_TAP_TIMEOUT) {
			// O segundo clique ocorreu dentro da janela de tempo aceitável
			m_IsRunning = true;
			m_WaitingForDoubleTap = false;
		}
	}

	// Detecta o exato frame em que o 'W' foi solto (Key Up Event)
	if (!wIsPressed && m_WWasPressed) {
		if (m_IsRunning) {
			// Se o jogador estava correndo e soltou o W, ele para de correr imediatamente
			m_IsRunning = false;
		}
		else {
			// Se não estava correndo, inicia a contagem para o possível clique duplo
			m_WaitingForDoubleTap = true;
			m_WReleaseTimer = 0.0f;
		}
	}

	// Atualiza o temporizador se estiver aguardando o clique duplo
	if (m_WaitingForDoubleTap) {
		m_WReleaseTimer += deltaTime;
		if (m_WReleaseTimer > DOUBLE_TAP_TIMEOUT) {
			m_WaitingForDoubleTap = false; // Tempo esgotado
		}
	}

	// Guarda o estado atual para o próximo frame
	m_WWasPressed = wIsPressed;

	// Coleta a direção do input
	if (GetAsyncKeyState('W') & 0x8000) velocityVec += moveForward;
	if (GetAsyncKeyState('S') & 0x8000) velocityVec -= moveForward;
	if (GetAsyncKeyState('D') & 0x8000) velocityVec += moveRight;
	if (GetAsyncKeyState('A') & 0x8000) velocityVec -= moveRight;

	// Define a velocidade com base no estado atual de corrida
	m_MoveSpeed = (m_IsCrouching) ? 2.0f : ((m_IsRunning) ? 12.0f : 6.0f) ;

	// Normaliza e aplica magnitude
	if (XMVectorGetX(XMVector3LengthSq(velocityVec)) > 0.001f) {
		velocityVec = XMVector3Normalize(velocityVec) * m_MoveSpeed * deltaTime;
	}
	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, velocityVec);


	int pCellX, pCellZ;
	m_SpatialGrid.GetCellIndices(m_PlayerPos.x, m_PlayerPos.z, pCellX, pCellZ);
	std::vector<Triangle> localFloorTriangles;
	std::vector<OBB> localWallOBBs;
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			std::string key = m_SpatialGrid.GetCellKey(pCellX + dx, pCellZ + dz);
			if (m_SpatialGrid.cells.find(key) != m_SpatialGrid.cells.end()) {
				const auto& cell = m_SpatialGrid.cells[key];
				localFloorTriangles.insert(localFloorTriangles.end(), cell.floorTriangles.begin(), cell.floorTriangles.end());
				localWallOBBs.insert(localWallOBBs.end(), cell.wallOBBs.begin(), cell.wallOBBs.end());
			}
		}
	}

	// ###############################################################################################

	// 3. MOVIMENTO E COLISÃO HORIZONTAL (X e Z) usando OBB para Paredes e Portas
	// --- LAMBDA DE COLISÃO TOTALMENTE CORRIGIDA (SEM DEPENDÊNCIA INDIRETA DO GRID) ---
	auto GetWallCollisionNormal = [&](const AABB& pBox, XMVECTOR& outNormal) -> bool {
		using namespace DirectX;

		// 1. Converte a caixa do jogador (pBox) enviada pelo passo X ou Z
		XMVECTOR pMin = XMLoadFloat3(&pBox.min);
		XMVECTOR pMax = XMLoadFloat3(&pBox.max);
		XMVECTOR pCenter = (pMin + pMax) * 0.5f;
		XMVECTOR pExtents = (pMax - pMin) * 0.5f;

		BoundingBox playerBoxCorrect;
		XMStoreFloat3(&playerBoxCorrect.Center, pCenter);
		XMStoreFloat3(&playerBoxCorrect.Extents, pExtents);

		// --- PASSO A: TESTA AS PAREDES ESTÁTICAS (Utiliza a otimização local do Grid) ---
		for (const auto& wall : localWallOBBs) {
			BoundingOrientedBox wallOriented;
			XMStoreFloat3(&wallOriented.Center, XMLoadFloat3(&wall.center));
			XMStoreFloat3(&wallOriented.Extents, XMLoadFloat3(&wall.extents));
			XMStoreFloat4(&wallOriented.Orientation, XMLoadFloat4(&wall.orientation));

			if (wallOriented.Intersects(playerBoxCorrect)) {
				XMVECTOR wCenter = XMLoadFloat3(&wallOriented.Center);
				XMVECTOR collisionDir = pCenter - wCenter;

				XMVECTOR obbX = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), XMLoadFloat4(&wallOriented.Orientation));
				XMVECTOR obbY = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), XMLoadFloat4(&wallOriented.Orientation));
				XMVECTOR obbZ = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4(&wallOriented.Orientation));

				float dotX = XMVectorGetX(XMVector3Dot(collisionDir, obbX));
				float dotY = XMVectorGetX(XMVector3Dot(collisionDir, obbY));
				float dotZ = XMVectorGetX(XMVector3Dot(collisionDir, obbZ));

				if (fabs(dotX) > fabs(dotY) && fabs(dotX) > fabs(dotZ)) { outNormal = (dotX > 0.0f) ? obbX : -obbX; }
				else if (fabs(dotY) > fabs(dotX) && fabs(dotY) > fabs(dotZ)) { outNormal = (dotY > 0.0f) ? obbY : -obbY; }
				else { outNormal = (dotZ > 0.0f) ? obbZ : -obbZ; }

				outNormal = XMVectorSetY(outNormal, 0.0f);
				outNormal = XMVector3Normalize(outNormal);
				return true; // Colidiu com uma parede, para o processamento
			}
		}

		// --- PASSO B: TESTA AS PORTAS DINÂMICAS (Global - Ignora o filtro do Grid) ---
		// Como as portas foram criadas no Initialize(), varremos m_Doors diretamente
		for (const auto& door : m_Doors) {
			// Dimensões exatas do modelo da sua porta
			float w = 1.0f; float h = 2.0f; float t = 0.1f;

			// 1. Recria exatamente a mesma árvore de transformações usada no Render() e no Blender
			// Como a dobradiça é a origem (0,0,0) do .obj, o centro local da folha da porta é (w/2, h/2, 0)
			XMVECTOR localCenter = XMVectorSet(w * 0.5f, h * 0.5f, 0.0f, 1.0f);

			// Cria as matrizes de rotação e translação idênticas às do pipeline gráfico
			XMMATRIX mScale = XMMatrixScaling(w * 0.5f, h * 0.5f, t * 0.5f); // Extensões da OBB
			XMMATRIX mRot = XMMatrixRotationY(XMConvertToRadians(door.currentRotation));
			XMMATRIX mTrans = XMMatrixTranslation(door.position.x, door.position.y, door.position.z);
			XMMATRIX mWorld = mRot * mTrans; // Matriz de mundo combinada

			// 2. Transforma o centro local para o espaço do mundo usando a matriz real
			XMVECTOR worldCenterVec = XMVector3TransformCoord(localCenter, mWorld);
			XMFLOAT3 doorCenter;
			XMStoreFloat3(&doorCenter, worldCenterVec);

			// 3. Extrai as extensões reais (metade do tamanho total)
			XMFLOAT3 doorExtents = { w * 0.5f, h * 0.5f, t * 0.5f };

			// 4. Extrai o quatérnio de rotação real direto da matriz mRot
			XMVECTOR dQuat = XMQuaternionRotationMatrix(mRot);
			XMFLOAT4 doorOrientation;
			XMStoreFloat4(&doorOrientation, dQuat);

			// 5. Inicializa a OBB Rígida perfeitamente sincronizada com o visual tridimensional
			BoundingOrientedBox doorOrientedBox(doorCenter, doorExtents, doorOrientation);

			// 6. Teste físico contra o BoundingBox correto do jogador
			if (doorOrientedBox.Intersects(playerBoxCorrect)) {
				XMVECTOR dCenterVec = XMLoadFloat3(&doorCenter);
				XMVECTOR collisionDir = pCenter - dCenterVec;

				// Extrai os eixos direcionais locais diretamente usando o quatérnio sincronizado
				XMVECTOR doorX = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), dQuat);
				XMVECTOR doorY = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), dQuat);
				XMVECTOR doorZ = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), dQuat);

				float dotX = XMVectorGetX(XMVector3Dot(collisionDir, doorX));
				float dotY = XMVectorGetX(XMVector3Dot(collisionDir, doorY));
				float dotZ = XMVectorGetX(XMVector3Dot(collisionDir, doorZ));

				if (fabs(dotX) > fabs(dotY) && fabs(dotX) > fabs(dotZ)) { outNormal = (dotX > 0.0f) ? doorX : -doorX; }
				else if (fabs(dotY) > fabs(dotX) && fabs(dotY) > fabs(dotZ)) { outNormal = (dotY > 0.0f) ? doorY : -doorY; }
				else { outNormal = (dotZ > 0.0f) ? doorZ : -doorZ; }

				// Zera o componente Y para deslizar apenas no plano do chão
				outNormal = XMVectorSetY(outNormal, 0.0f);
				outNormal = XMVector3Normalize(outNormal);
				return true; // Bloqueia o movimento do jogador imediatamente
			}
		}

		return false; // Nenhuma colisão detectada neste frame
	};

	m_PlayerPos.x += velocity.x;
	AABB playerBoxX = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};

	XMVECTOR normalX;
	if (GetWallCollisionNormal(playerBoxX, normalX)) {
		m_PlayerPos.x -= velocity.x;
		XMVECTOR currentVel = XMLoadFloat3(&velocity);
		XMVECTOR penetration = XMVector3Dot(currentVel, normalX);
		if (XMVectorGetX(penetration) < 0.0f) {
			XMVECTOR slideVel = currentVel - (penetration * normalX);
			m_PlayerPos.x += XMVectorGetX(slideVel);
		}
	}

	// 3. TERCEIRO: CRIAÇÃO DO PLAYERBOXZ (Garante que ela exista antes de chamar a função)
	m_PlayerPos.z += velocity.z;
	AABB playerBoxZ = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};

	// 4. QUARTO: CHAMADA DA FUNÇÃO NO EIXO Z (O trecho da sua imagem)
	XMVECTOR normalZ;
	if (GetWallCollisionNormal(playerBoxZ, normalZ)) { // << AGORA VAI RECONHECER PERFEITAMENTE!
		m_PlayerPos.z -= velocity.z;
		XMVECTOR currentVel = XMLoadFloat3(&velocity);
		XMVECTOR penetration = XMVector3Dot(currentVel, normalZ);
		if (XMVectorGetX(penetration) < 0.0f) {
			XMVECTOR slideVel = currentVel - (penetration * normalZ);
			m_PlayerPos.z += XMVectorGetZ(slideVel);
		}
	}


	// 4. FÍSICA VERTICAL (Gravidade e Pulo - Removido m_CeilingOBBs completamente)
	if (m_IsGrounded) {
		m_VelocityY = 0.0f;
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			m_VelocityY = JUMP_FORCE;
			m_IsGrounded = false;
			PlaySoundFX(m_pSourceVoiceJump, m_sndJump);
		}
	}
	else {
		m_VelocityY += GRAVITY * deltaTime;
	}

	float moveY = m_VelocityY * deltaTime;
	float nextY = m_PlayerPos.y + moveY;


	if (m_VelocityY > 0) { // Subindo (Pulo)
		// Volume que a cabeça vai percorrer neste frame
		float currentHeadTop = m_PlayerPos.y + charHeight;
		float futureHeadTop = nextY + charHeight;

		AABB headSweepBox;
		headSweepBox.min = { m_PlayerPos.x - charRadius, currentHeadTop - 0.1f, m_PlayerPos.z - charRadius };
		headSweepBox.max = { m_PlayerPos.x + charRadius, futureHeadTop, m_PlayerPos.z + charRadius };

		// Converte SweepBox para a BoundingBox do DirectX
		XMVECTOR hsMin = XMLoadFloat3(&headSweepBox.min);
		XMVECTOR hsMax = XMLoadFloat3(&headSweepBox.max);
		BoundingBox hsDxBox;
		XMStoreFloat3(&hsDxBox.Center, (hsMin + hsMax) * 0.5f);
		XMStoreFloat3(&hsDxBox.Extents, (hsMax - hsMin) * 0.5f);

		bool hitCeiling = false;
		for (const auto& ceil : m_CeilingTriangles) {
			// Carrega os três vértices do triângulo atual
			XMVECTOR v0 = XMLoadFloat3(&ceil.v0);
			XMVECTOR v1 = XMLoadFloat3(&ceil.v1);
			XMVECTOR v2 = XMLoadFloat3(&ceil.v2);

			// Testa interseção entre a caixa da cabeça e o triângulo
			if (hsDxBox.Intersects(v0, v1, v2)) {
				// Calcula a altura exata do teto usando a média geométrica simples dos vértices do triângulo
				float triangleCenterY = (ceil.v0.y + ceil.v1.y + ceil.v2.y) / 3.0f;

				// Trava a subida imediatamente abaixo do triângulo
				m_PlayerPos.y = triangleCenterY - charHeight - 0.001f;
				m_VelocityY = 0;
				hitCeiling = true;
				break;
			}
		}
		if (!hitCeiling) m_PlayerPos.y = nextY;
	}
	else {
		m_PlayerPos.y = nextY;
	}



	// 5. COLISÃO VERTICAL COM RAYCAST EM TRIÂNGULOS DO MUNDO (Ajuste de Altura)
		m_IsGrounded = false;
		float closestDist = FLT_MAX;
		bool hit = false;

		float checkRadius = charRadius * 0.8f;
		DirectX::XMFLOAT3 testPoints[5] = {
			{ m_PlayerPos.x, m_PlayerPos.y + 2.0f, m_PlayerPos.z }, // Centro
			{ m_PlayerPos.x + checkRadius, m_PlayerPos.y + 2.0f, m_PlayerPos.z },
			{ m_PlayerPos.x - checkRadius, m_PlayerPos.y + 2.0f, m_PlayerPos.z },
			{ m_PlayerPos.x, m_PlayerPos.y + 2.0f, m_PlayerPos.z + checkRadius },
			{ m_PlayerPos.x, m_PlayerPos.y + 2.0f, m_PlayerPos.z - checkRadius }
		};

		for (int i = 0; i < 5; i++) {
			DirectX::XMVECTOR rayOrigin = DirectX::XMLoadFloat3(&testPoints[i]);
			DirectX::XMVECTOR rayDir = DirectX::XMVectorSet(0, -1.0f, 0, 0);

			for (const auto& tri : localFloorTriangles) {
				float dist = 0.0f;
				DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&tri.v0);
				DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&tri.v1);
				DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&tri.v2);

				// TESTE 1: Sentido normal do triângulo
				if (DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, dist)) {
					if (dist < closestDist) {
						closestDist = dist;
						hit = true;
					}
				}
				// TESTE 2: Sentido invertido (Garante a colisão mesmo se a normal no Blender estiver de cabeça para baixo)
				else if (DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v2, v1, dist)) {
					if (dist < closestDist) {
						closestDist = dist;
						hit = true;
					}
				}
			}
		}

		// ###################################################################################################

		if (hit) {
			float floorY = (m_PlayerPos.y + 2.0f) - closestDist;

			if (m_VelocityY <= 0) {
				// Aumentamos ligeiramente a tolerância para capturar o player mesmo em quedas de frames estáveis
				if (floorY >= (m_PlayerPos.y - 0.5f) && floorY <= (m_PlayerPos.y + stepHeight)) {
					m_PlayerPos.y = floorY;
					m_VelocityY = 0.0f;
					m_IsGrounded = true;
				}
			}
		}

	// 6. INTERAÇÃO E PORTAS LÓGICAS COM ROTAÇÃO (OBB)
		bool fPressed = GetAsyncKeyState('F') & 0x0001;
		XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
		XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
		XMVECTOR eyePos = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);

		for (auto& door : m_Doors) {

			using namespace DirectX;

			if (fabs(door.targetRotation - door.currentRotation) > 2.0f) {
				door.isMoving = true;
			}
			else {
				door.isMoving = false;
				door.currentRotation = door.targetRotation; // Trava no valor exato ao terminar
			}

			// Converte a caixa de colisão da porta para OBB nativa do DirectX para testar o olhar
			XMVECTOR dMin = XMLoadFloat3(&door.collisionBox.min);
			XMVECTOR dMax = XMLoadFloat3(&door.collisionBox.max);
			XMVECTOR dCenter = (dMin + dMax) * 0.5f;
			XMVECTOR dExtents = (dMax - dMin) * 0.5f;

			// Como a porta gira no eixo Y da sua dobradiça local, usamos o ângulo atual
			XMVECTOR dQuat = XMQuaternionRotationRollPitchYaw(0.0f, XMConvertToRadians(door.currentRotation), 0.0f);

			BoundingOrientedBox doorBox;
			XMStoreFloat3(&doorBox.Center, dCenter);
			XMStoreFloat3(&doorBox.Extents, dExtents);
			XMStoreFloat4(&doorBox.Orientation, dQuat);

			float distToDoor = 0.0f;
			bool isLookingAt = doorBox.Intersects(eyePos, lookDir, distToDoor);

			// Se estiver olhando para a porta E perto dela
			if (isLookingAt && distToDoor < 2.5f) {
				if (fPressed && !door.isMoving) {
					door.isOpen = !door.isOpen;
					door.targetRotation = door.isOpen ? 90.0f : 0.0f;
					door.isMoving = true;
					PlaySoundFX(m_pSourceVoiceDoor, m_sndDoor);
				}
			}

			// Animação suave da dobradiça
			door.currentRotation += (door.targetRotation - door.currentRotation) * 5.0f * deltaTime;

			// --- ATUALIZAÇÃO DA COLISÃO DINÂMICA (Atualiza a bounding box alinhada para simplificar) ---
			float w = 1.0f; float t = 0.1f; float h = 2.0f;
			float cosA = cosf(XMConvertToRadians(door.currentRotation));
			float sinA = sinf(XMConvertToRadians(door.currentRotation));

			door.collisionBox.min.x = (std::min)(door.position.x, door.position.x + w * cosA) - t;
			door.collisionBox.min.z = (std::min)(door.position.z, door.position.z - w * sinA) - t;
			door.collisionBox.min.y = door.position.y;
			door.collisionBox.max.x = (std::max)(door.position.x, door.position.x + w * cosA) + t;
			door.collisionBox.max.z = (std::max)(door.position.z, door.position.z - w * sinA) + t;
			door.collisionBox.max.y = door.position.y + h;
		}



	// 7. CÂMERA
		XMVECTOR lookForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
		XMVECTOR Eye = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);
		m_ViewMatrix = XMMatrixLookAtLH(Eye, Eye + lookForward, XMVectorSet(0, 1, 0, 0));

		float targetFOV = m_IsCrouching ? 60 : (m_IsRunning ? 80.0f : 60.0f);
		m_ViewRadius += (targetFOV - m_ViewRadius) * 5.0f * deltaTime;
		if (fabs(m_ViewRadius - targetFOV) < 0.01f) m_ViewRadius = targetFOV;

		float aspect = (m_WndHeight > 0) ? (float)m_WndWidth / m_WndHeight : 1.77f;
		m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_ViewRadius), aspect, 0.1f, 1000.0f);
}



void MyDX3DGame::Render() {
	// 1. LIMPEZA DOS BUFFERS
	float bgColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	m_pDevCon->ClearRenderTargetView(m_pBackBufferView.Get(), bgColor);
	m_pDevCon->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 2. CONFIGURAÇÃO DO PIPELINE
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDevCon->IASetInputLayout(m_pInputLayout.Get());

	UINT stride = sizeof(VertexOBJ);
	UINT offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	m_pDevCon->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDevCon->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDevCon->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// 3. PREPARAÇÃO DO CONSTANT BUFFER (View e Projection comuns a todos)
	ConstantBuffer cb;
	cb.mView = XMMatrixTranspose(m_ViewMatrix);
	cb.mProjection = XMMatrixTranspose(m_ProjectionMatrix);

	// --- 4. DESENHAR O MAPA (CENÁRIO ESTÁTICO) ---
	cb.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// PASSE A: Desenha o mapa preenchido colorido normal
	m_pDevCon->Draw(m_VertexCount, 0);


	// --- 5. DESENHAR AS PORTAS (CENÁRIO DINÂMICO) ---
	for (const auto& door : m_Doors) {
		XMMATRIX mRotation = XMMatrixRotationY(XMConvertToRadians(door.currentRotation));
		XMMATRIX mTranslation = XMMatrixTranslation(door.position.x, door.position.y, door.position.z);
		XMMATRIX mWorld = mRotation * mTranslation;

		cb.mWorld = XMMatrixTranspose(mWorld);
		cb.mView = XMMatrixTranspose(m_ViewMatrix);
		cb.mProjection = XMMatrixTranspose(m_ProjectionMatrix);

		m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
		m_pDevCon->Draw(m_DoorVertexCount, m_DoorStartOffset);
	}

	if (m_ShowDebug) {
		DrawDebugBoxes();
	}

	// --- 6. DESENHAR A MIRA (UI 3D/LINHAS) ---
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), nullptr);

	stride = sizeof(VertexOBJ);
	offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pCrosshairBuffer.GetAddressOf(), &stride, &offset);
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	ConstantBuffer cbUI;
	cbUI.mWorld = cbUI.mView = cbUI.mProjection = XMMatrixTranspose(XMMatrixIdentity());
	m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cbUI, 0, 0);

	m_pDevCon->Draw(4, 0);

	// --- 7. NOVO: RENDERIZAÇÃO DO MENU DE PAUSE (DIRECT 2D) ---
	if (m_isPause_screen && m_pD2DContext)
	{
		m_PauseButtons.clear();

		// --- CONFIGURAÇÕES DE LAYOUT DOS BOTÕES ---
		float boxWidth = 350.0f;
		float leftPos = ((float)m_WndWidth - boxWidth) / 2.0f;
		float rightPos = ((float)m_WndWidth + boxWidth) / 2.0f;

		m_PauseButtons.push_back({ 0 ,  0.5f , leftPos , rightPos , m_menu_pause_Opt1Rect , L"Continuar" });
		m_PauseButtons.push_back({ 1 , 0.58f , leftPos , rightPos , m_menu_pause_Opt2Rect , L"Configuracoes" });
		m_PauseButtons.push_back({ 2 , 0.66f , leftPos , rightPos , m_menu_pause_Opt3Rect , L"Sair do Jogo" });

		MenuTitle title;
		title.rect = D2D1::RectF(0.0f, (float)m_WndHeight * 0.3f, (float)m_WndWidth, (float)m_WndHeight * 0.4f);
		title.text = L" JOGO PAUSADO";

		ShowMenu(m_PauseButtons, title);
	}

	if (m_isSettings_screen && m_pD2DContext)
	{
		m_SettingsButtons.clear();

		float boxWidth = 350.0f;
		float leftPos = ((float)m_WndWidth - boxWidth) / 2.0f;
		float rightPos = ((float)m_WndWidth + boxWidth) / 2.0f;

		m_SettingsButtons.push_back({ 0 ,  0.38f , leftPos , rightPos , m_menu_settings_Opt1Rect , L"Voltar" });
		m_SettingsButtons.push_back({ 1 ,  0.46f , leftPos , rightPos , m_menu_settings_Opt2Rect , L"Relatorio" });
		m_SettingsButtons.push_back({ 2 ,  0.54f , leftPos , rightPos , m_menu_settings_Opt3Rect , L"Debug Boxes" });

		MenuTitle title;
		title.rect = D2D1::RectF(0.0f, (float)m_WndHeight * 0.15f, (float)m_WndWidth, (float)m_WndHeight * 0.25f);
		title.text = L" CONFIGURACOES";

		// Deixe apenas isso aqui! O resto vai para dentro do ShowMenu
		ShowMenu(m_SettingsButtons, title);
	}

	// 8. APRESENTAÇÃO
	m_pSwapChain->Present(1, 0);
}



















void MyDX3DGame::ShowMenu(std::vector<Button>& buttons, MenuTitle title)
{
	if (buttons.empty() || !m_pD2DBackBufferTarget) return;

	m_pD2DContext->SetTarget(m_pD2DBackBufferTarget.Get());

	m_pDevCon->VSSetShader(nullptr, nullptr, 0);
	m_pDevCon->PSSetShader(nullptr, nullptr, 0);

	m_pD2DContext->BeginDraw();

	// 1. Desenha o fundo escuro semi-transparente
	D2D1_RECT_F screenRect = D2D1::RectF(0.0f, 0.0f, (float)m_WndWidth, (float)m_WndHeight);
	m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.6f));
	m_pD2DContext->FillRectangle(&screenRect, m_pScreenBrush.Get());

	// 2. Título 
	m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
	UINT32 titleLength = static_cast<UINT32>(wcslen(title.text));
	m_pD2DContext->DrawTextW(title.text, titleLength, m_pTextFormat_big.Get(), title.rect, m_pScreenBrush.Get());

	// 3. RENDERIZAÇÃO DOS BOTÕES
	for (auto& button : buttons)
	{
		float topPos = (float)m_WndHeight * button.top;
		float bottomPos = topPos + ((float)m_WndHeight * 0.05f);

		button.hitbox = D2D1::RectF(button.left, topPos, button.right, bottomPos);

		if (m_isPause_screen)
		{
			if (button.id == 0) m_menu_pause_Opt1Rect = button.hitbox;
			else if (button.id == 1) m_menu_pause_Opt2Rect = button.hitbox;
			else if (button.id == 2) m_menu_pause_Opt3Rect = button.hitbox;
		}
		else if (m_isSettings_screen)
		{
			if (button.id == 0) m_menu_settings_Opt1Rect = button.hitbox;
			else if (button.id == 1) m_menu_settings_Opt2Rect = button.hitbox;
			else if (button.id == 2) m_menu_settings_Opt3Rect = button.hitbox;
		}

		if (m_SelectedOption == button.id)
		{
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 0.15f));
			m_pD2DContext->FillRectangle(&button.hitbox, m_pScreenBrush.Get());
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
		}
		else
		{
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.3f));
			m_pD2DContext->FillRectangle(&button.hitbox, m_pScreenBrush.Get());
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
		}

		UINT32 textLength = (UINT32)wcslen(button.text);
		m_pD2DContext->DrawTextW(button.text, textLength, m_pTextFormat_elegant.Get(), button.hitbox, m_pScreenBrush.Get());
	}

	// --- NOVO: DESENHAR O SLIDER SE ESTIVER NA TELA DE CONFIGURAÇÕES ---
	if (m_isSettings_screen)
	{
		// Posição vertical do slider fixa em 68% da altura da tela
		float sliderY = (float)m_WndHeight * 0.68f;
		float sliderWidth = 350.0f;
		float sliderLeft = ((float)m_WndWidth - sliderWidth) / 2.0f;
		float sliderRight = sliderLeft + sliderWidth;

		// Desenha a Pista (Linha horizontal)
		m_slider_TrackRect = D2D1::RectF(sliderLeft, sliderY - 4.0f, sliderRight, sliderY + 4.0f);
		m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::DimGray, 0.8f));
		m_pD2DContext->FillRectangle(&m_slider_TrackRect, m_pScreenBrush.Get());

		// Calcula a posição do Botão deslizante baseado no valor atual da sensibilidade
		float pct = (m_MouseSens - m_MinSens) / (m_MaxSens - m_MinSens);
		float thumbX = sliderLeft + (pct * sliderWidth);
		float thumbSize = 16.0f;

		m_slider_ThumbRect = D2D1::RectF(thumbX - (thumbSize / 2.0f), sliderY - (thumbSize / 2.0f), thumbX + (thumbSize / 2.0f), sliderY + (thumbSize / 2.0f));

		// Altera a cor se o usuário estiver ativamente arrastando
		if (m_isDraggingSlider)
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
		else
			m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

		m_pD2DContext->FillRectangle(&m_slider_ThumbRect, m_pScreenBrush.Get());

		// Desenha o texto informativo acima do slider
		int number_of_digits = ((m_MouseSens * 1000) > 9.99999) ? 4 : 3;
		std::wstring sensText = L"Sensibilidade: " + std::to_wstring(m_MouseSens * 1000).substr(0, number_of_digits);
		D2D1_RECT_F textRect = D2D1::RectF(sliderLeft, sliderY - 45.0f, sliderRight, sliderY - 10.0f);
		m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
		m_pD2DContext->DrawTextW(sensText.c_str(), (UINT32)sensText.length(), m_pTextFormat_elegant.Get(), textRect, m_pScreenBrush.Get());
	}

	m_pD2DContext->EndDraw();
	m_pD2DContext->SetTarget(nullptr);
}



void MyDX3DGame::ButtonActions(int opt)
{
	if (m_isPause_screen)
	{
		if (opt == 0) // PAUSE >> JOGO
		{
			m_isPause_screen = false;
			m_IsMouseCaptured = true;
			while (ShowCursor(FALSE) >= 0);

			RECT rect;
			GetWindowRect(m_hWnd, &rect);
			ClipCursor(&rect);
		}
		else if (opt == 1) // PAUSE >> SETTINGS
		{
			m_isPause_screen = false;
			m_isSettings_screen = true;
			m_SelectedOption = 0;
		}
		else if (opt == 2) // PAUSE >> EXIT
		{
			PostQuitMessage(0);
		}
	}
	else if (m_isSettings_screen)
	{
		if (opt == 0) // SETTINGS >> PAUSE
		{
			m_isPause_screen = true;
			m_isSettings_screen = false;
			m_SelectedOption = 0;
		}
		else if (opt == 1) // SETTINGS > RELATÓRIO
		{
			MoreInformation(1);
		}
		else if (opt == 2) // SETTINGS >> DEBUG BOXES
		{
			m_ShowDebug = !m_ShowDebug;
		}
	}
	else // JOGO >> PAUSE
	{
		m_isPause_screen = true;
		m_isSettings_screen = false;
		m_IsMouseCaptured = false;
		ClipCursor(NULL);

		while (ShowCursor(TRUE) < 0);
		m_SelectedOption = 0;
	}
}




















void MyDX3DGame::MoreInformation(int which)
{

	switch (which)
	{
		case 1:

			{
				std::string relatorio = "=== DIAGNOSTICO REAL DA MEMORIA ===\n\n"
					"Vertices Visuais na GPU: " + std::to_string(m_VertexCount) + "\n"
					"Triangulos de Chao carregados: " + std::to_string(m_FloorTriangles.size()) + "\n"
					"Caixas de Parede (OBB) carregadas: " + std::to_string(m_WallOBBs.size()) + "\n\n";

				if (m_FloorTriangles.empty() && m_WallOBBs.empty()) {
					relatorio += "ANALISE: A Engine nao conseguiu extrair NENHUMA colisao! Verifique os nomes dos objetos.";
				}
				else {
					relatorio += "ANALISE: As colisoes estao na memoria! Se o visual sumiu, o arquivo .obj contem apenas dados de colisao.";
				}

				MessageBoxA(m_hWnd, relatorio.c_str(), "Rastreamento de Vetores", MB_ICONINFORMATION);
			}
			break;
		case 2:

			break;
	}

}



void MyDX3DGame::DrawDebugBoxes() {

	// 1. Evita processamento se não houver células e nem portas
	if (m_SpatialGrid.cells.empty() && m_Doors.empty()) return;

	// 2. Salva o Rasterizer State atual do pipeline do DX11
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> pOldRS;
	m_pDevCon->RSGetState(&pOldRS);

	// 3. Muda o pipeline para o modo Wireframe de Debug
	m_pDevCon->RSSetState(m_pWireframeRS.Get());

	// --- RENDERIZAÇÃO DAS PORTAS ---
	UINT stride = sizeof(VertexOBJ);
	UINT offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pDebugCubeBuffer.GetAddressOf(), &stride, &offset);
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& door : m_Doors) {
		float w = 1.0f; float h = 2.0f; float t = 0.1f;
		XMMATRIX mScale = XMMatrixScaling(w, h, t);
		XMMATRIX mPivot = XMMatrixTranslation(w / 2.0f, h / 2.0f, 0.0f);
		XMMATRIX mRot = XMMatrixRotationY(XMConvertToRadians(door.currentRotation));
		XMMATRIX mTrans = XMMatrixTranslation(door.position.x, door.position.y, door.position.z);
		XMMATRIX world = mScale * mPivot * mRot * mTrans;

		UpdateConstantBuffer(world);
		m_pDevCon->Draw(36, 0);
	}

	// --- NOVO: ACUMULADOR DE TRIÂNGULOS DE CHÃO ENCONTRADOS NO GRID ---
	std::vector<VertexOBJ> lineVertices;

	// --- PERCORRE TODAS AS CÉLULAS ATIVAS DO GRID ESPACIAL ---
	for (const auto& pair : m_SpatialGrid.cells) {
		const GridCell& cell = pair.second; // Pega os dados estruturados da caixa atual

		// --- RENDERIZAÇÃO DAS PAREDES ROTACIONADAS DA CÉLULA ---
		// Como o Buffer de Cubos já está vinculado acima, podemos desenhar as paredes direto
		m_pDevCon->IASetVertexBuffers(0, 1, m_pDebugCubeBuffer.GetAddressOf(), &stride, &offset);
		m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (const auto& wall : cell.wallOBBs) {
			XMMATRIX mScale = XMMatrixScaling(wall.extents.x * 2.0f, wall.extents.y * 2.0f, wall.extents.z * 2.0f);
			XMVECTOR qRot = XMLoadFloat4(&wall.orientation);
			XMMATRIX mRot = XMMatrixRotationQuaternion(qRot);
			XMMATRIX mTrans = XMMatrixTranslation(wall.center.x, wall.center.y, wall.center.z);
			XMMATRIX world = mScale * mRot * mTrans;

			UpdateConstantBuffer(world);
			m_pDevCon->Draw(36, 0);
		}

		// --- ACUMULA OS TRIÂNGULOS DE CHÃO DA CÉLULA PARA O FILME EM LINHAS ---
		for (const auto& tri : cell.floorTriangles) {
			VertexOBJ p0 = { tri.v0, {0, 0}, {0, 1, 0} };
			VertexOBJ p1 = { tri.v1, {0, 0}, {0, 1, 0} };
			VertexOBJ p2 = { tri.v2, {0, 0}, {0, 1, 0} };

			// Linha 1: De v0 até v1
			lineVertices.push_back(p0); lineVertices.push_back(p1);
			// Linha 2: De v1 até v2
			lineVertices.push_back(p1); lineVertices.push_back(p2);
			// Linha 3: De v2 até v0
			lineVertices.push_back(p2); lineVertices.push_back(p0);
		}
	}

	// --- RENDERIZAÇÃO WIREFRAME DAS RAMPAS E CHÃOS ACUMULADOS ---
	if (!lineVertices.empty()) {
		// Desvencilha o Vertex Buffer de cubos
		m_pDevCon->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);

		// Altera a topologia para lista de linhas (LINELIST)
		m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		// Aplica matriz Identidade para coordenadas globais
		UpdateConstantBuffer(XMMatrixIdentity());

		// Cria e popula o buffer temporário dinâmico na GPU
		D3D11_BUFFER_DESC lbd = {};
		lbd.Usage = D3D11_USAGE_DEFAULT;
		lbd.ByteWidth = sizeof(VertexOBJ) * (UINT)lineVertices.size();
		lbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA lData = { lineVertices.data() };
		Microsoft::WRL::ComPtr<ID3D11Buffer> pLineBuffer;
		HRESULT hr = m_pDevice->CreateBuffer(&lbd, &lData, &pLineBuffer);

		if (SUCCEEDED(hr)) {
			m_pDevCon->IASetVertexBuffers(0, 1, pLineBuffer.GetAddressOf(), &stride, &offset);
			m_pDevCon->Draw((UINT)lineVertices.size(), 0);
		}
	}

	// 4. Restaura o Rasterizer State original
	m_pDevCon->RSSetState(pOldRS.Get());
}



void MyDX3DGame::OnResize(UINT width, UINT height)
{
	// Se a janela foi minimizada ou o tamanho for inválido, ignora para evitar quebras
	if (width == 0 || height == 0 || !m_pDevice || !m_pDevCon) return;

	m_WndWidth = width;
	m_WndHeight = height;

	// 1.1 Desvincular os alvos do pipeline do Direct3D
	m_pDevCon->OMSetRenderTargets(0, nullptr, nullptr);

	// 1.2 IMPORTANTE: Desvincular e DESTRUIR o alvo do Direct2D
	if (m_pD2DContext)
	{
		m_pD2DContext->SetTarget(nullptr); // Desvincula o bitmap
	}
	m_pD2DBackBufferTarget.Reset(); // Libera a referência COM do bitmap antigo

	// 1.3 Destruir as Views antigas do Direct3D
	m_pBackBufferView.Reset();
	m_pDepthStencilView.Reset();

	// Agora que TODAS as referências ao BackBuffer sumiram, a Swap Chain aceita o comando livremente
	HRESULT hr = m_pSwapChain->ResizeBuffers(
		2, // Mantém o BufferCount idêntico ao seu Initialize (2 buffers para FLIP_DISCARD)
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0
	);

	if (FAILED(hr))
	{
		MessageBoxW(m_hWnd, L"Falha crítica ao redimensionar a Swap Chain!", L"Erro Buffer Resize", MB_ICONERROR);
		return;
	}


	// 3.1 Recriar o Render Target View
	ComPtr<ID3D11Texture2D> pBackBuffer;
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (SUCCEEDED(hr))
	{
		m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pBackBufferView);
	}

	// 3.2 Recriar o Buffer de Profundidade (Depth Stencil) com a nova resolução
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pDepthStencil;
	hr = m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
	if (SUCCEEDED(hr))
	{
		m_pDevice->CreateDepthStencilView(pDepthStencil.Get(), nullptr, &m_pDepthStencilView);
	}

	// 3.3 Re-vincular os targets e redefinir a Viewport do Direct3D
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());

	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
	m_pDevCon->RSSetViewports(1, &vp);

	if (m_pD2DContext && m_pSwapChain)
	{
		ComPtr<IDXGISurface> pBackBufferSurface;
		hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferSurface));
		if (SUCCEEDED(hr))
		{
			D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
			);

			// Vincula a nova superfície ao bitmap membro da classe de forma persistente
			hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pBackBufferSurface.Get(), &bitmapProperties, &m_pD2DBackBufferTarget);
			if (SUCCEEDED(hr))
			{
				m_pD2DContext->SetTarget(m_pD2DBackBufferTarget.Get());
			}
		}
	}
}



MyDX3DGame::~MyDX3DGame()
{
	// --- RECURSOS DE ÁUDIO (XAudio2) ---
	if (m_pSourceVoiceJump) { m_pSourceVoiceJump->DestroyVoice(); m_pSourceVoiceJump = nullptr; }
	if (m_pSourceVoiceDoor) { m_pSourceVoiceDoor->DestroyVoice(); m_pSourceVoiceDoor = nullptr; }
	if (m_pSourceVoiceStep) { m_pSourceVoiceStep->DestroyVoice(); m_pSourceVoiceStep = nullptr; }

	if (m_pMasterVoice) { m_pMasterVoice->DestroyVoice(); m_pMasterVoice = nullptr; }
	if (m_pXAudio2) { m_pXAudio2.Reset(); }

	CoUninitialize(); // Finaliza o sistema COM

	// 1. DESVINCULAR DO PIPELINE
	if (m_pDevCon) {
		m_pDevCon->OMSetRenderTargets(0, nullptr, nullptr);
		m_pDevCon->ClearState();
		m_pDevCon->Flush();
	}

	// 2. RECURSOS DE INTERFACE E TEXTO (Direct2D e DirectWrite)
	if (m_pTextFormat_elegant) { m_pTextFormat_elegant.Reset(); }
	if (m_pTextFormat_big) { m_pTextFormat_big.Reset(); }
	if (m_pDWriteFactory) { m_pDWriteFactory.Reset(); }
	if (m_pScreenBrush) { m_pScreenBrush.Reset(); }
	if (m_pD2DContext) { m_pD2DContext->SetTarget(nullptr); m_pD2DContext.Reset(); }
	if (m_pD2DFactory) { m_pD2DFactory.Reset(); }

	// 3. RECURSOS DE GEOMETRIA E MIRA
	if (m_pVertexBuffer) { m_pVertexBuffer.Reset(); }
	if (m_pCrosshairBuffer) { m_pCrosshairBuffer.Reset(); } // Libera a mira da tela
	if (m_pDebugCubeBuffer) { m_pDebugCubeBuffer.Reset(); }
	if (m_pConstantBuffer) { m_pConstantBuffer.Reset(); }

	// 4. ESTADOS DE RASTERIZADOR 
	if (m_pWireframeRS) { m_pWireframeRS.Reset(); }

	// 5. SHADERS E LAYOUTS
	if (m_pVertexShader) { m_pVertexShader.Reset(); }
	if (m_pPixelShader) { m_pPixelShader.Reset(); }
	if (m_pInputLayout) { m_pInputLayout.Reset(); }

	// 6. VIEWS DE RENDERIZAÇÃO
	if (m_pDepthStencilView) { m_pDepthStencilView.Reset(); }
	if (m_pBackBufferView) { m_pBackBufferView.Reset(); }

	// 7. INFRAESTRUTURA DXGI
	if (m_pSwapChain) { m_pSwapChain.Reset(); }
	if (m_pFactory) { m_pFactory.Reset(); }

	// 8. DISPOSITIVOS NUCLEARES (Devem ser rigorosamente os últimos)
	if (m_pDevCon) { m_pDevCon.Reset(); }
	if (m_pDevice) { m_pDevice.Reset(); }
}


















int WINAPI WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPSTR pCmdLine,_In_ int nCmdShow)
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


















void MyDX3DGame::UpdateConstantBuffer(XMMATRIX world) {
	ConstantBuffer cb;
	// Transposta necessária porque o DirectX (CPU) e HLSL (GPU) leem matrizes de formas opostas
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(m_ViewMatrix); // Certifique-se de ter m_ViewMatrix atualizada
	cb.mProjection = XMMatrixTranspose(m_ProjectionMatrix);

	// Atualiza o buffer que você criou no Initialize
	m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// Ativa o buffer no Vertex Shader
	m_pDevCon->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}



LRESULT CALLBACK MyDX3DGame::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{


	MyDX3DGame* pGame = nullptr;

	if (uMsg == WM_NCCREATE)
	{

		CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
		pGame = reinterpret_cast<MyDX3DGame*>(create->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pGame));

	}
	else
	{
		pGame = reinterpret_cast<MyDX3DGame*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}


	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0); // Coloca WM_QUIT na fila, fazendo o loop do Run() quebrar
		return 0;
	}

	if (uMsg == WM_GETMINMAXINFO)
	{
		MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);

		// Define o tamanho mínimo em pixels (Largura x Altura)
		// Ajuste para os valores que melhor preservam a proporção da sua UI
		mmi->ptMinTrackSize.x = 800;  // Largura mínima: 800px
		mmi->ptMinTrackSize.y = 600;  // Altura mínima: 600px

		return 0; // Informa ao Windows que processamos a mensagem
	}



	switch (uMsg)
	{

		case WM_SIZE:
		case WM_LBUTTONDOWN:
		case WM_KEYDOWN:
		case WM_INPUT:
		case WM_ACTIVATE:



				pGame->HandleMessage(uMsg, wParam, lParam);

			break;


	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}



LRESULT MyDX3DGame::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:



			{
				WORD activationState = LOWORD(wParam);

				if (activationState == WA_INACTIVE)
				{
					m_isPause_screen = true;
					m_isSettings_screen = false;
					m_IsMouseCaptured = false;
					ClipCursor(NULL);

					while (ShowCursor(TRUE) < 0);
					m_SelectedOption = 0;
				}
			}
			return 0;

	case WM_SIZE:



			{
				if (wParam == SIZE_MINIMIZED)
				{
					m_isMaximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					m_isMaximized = true;
					m_WndWidth = LOWORD(lParam);
					m_WndHeight = HIWORD(lParam);
					OnResize(m_WndWidth, m_WndHeight);
				}
				else if (wParam == SIZE_RESTORED)
				{
					m_isMaximized = false;
					m_WndWidth = LOWORD(lParam);
					m_WndHeight = HIWORD(lParam);
					OnResize(m_WndWidth, m_WndHeight);
				}
			}
			return 0;

	case WM_LBUTTONDOWN:


			{
				float mouseX = (float)LOWORD(lParam);
				float mouseY = (float)HIWORD(lParam);

				if (m_isSettings_screen)
				{
					// --- NOVA LÓGICA DO SLIDER ---
					// Verifica se o clique ocorreu na hitbox do botão (Thumb) ou da pista (Track)
					if (IsHovered(m_slider_ThumbRect, mouseX, mouseY) || IsHovered(m_slider_TrackRect, mouseX, mouseY))
					{
						m_isDraggingSlider = true;
						SetCapture(m_hWnd); // Impede que o Windows perca o mouse se o usuário arrastar rápido para fora da janela

						// Atualiza o valor imediatamente no local do clique
						float width = m_slider_TrackRect.right - m_slider_TrackRect.left;
						float pct = (mouseX - m_slider_TrackRect.left) / width;
						pct = std::clamp(pct, 0.0f, 1.0f);

						// Interpolação linear (Lerp) para definir a nova sensibilidade
						m_MouseSens = m_MinSens + pct * (m_MaxSens - m_MinSens);
						return 0;
					}

					// Seus botões originais da tela de configurações
					if (IsHovered(m_menu_settings_Opt1Rect, mouseX, mouseY)) ButtonActions(0);
					else if (IsHovered(m_menu_settings_Opt2Rect, mouseX, mouseY)) ButtonActions(1);
					else if (IsHovered(m_menu_settings_Opt3Rect, mouseX, mouseY)) ButtonActions(2);
				}
				else if (m_isPause_screen)
				{
					if (IsHovered(m_menu_pause_Opt1Rect, mouseX, mouseY)) ButtonActions(0);
					else if (IsHovered(m_menu_pause_Opt2Rect, mouseX, mouseY)) ButtonActions(1);
					else if (IsHovered(m_menu_pause_Opt3Rect, mouseX, mouseY)) ButtonActions(2);
				}
				else if (!m_IsMouseCaptured)
				{
					m_IsMouseCaptured = true;
					while (ShowCursor(FALSE) >= 0);

					RECT rect;
					GetWindowRect(m_hWnd, &rect);
					ClipCursor(&rect);
				}
			}
			return 0;



	case WM_MOUSEMOVE:



			{
				if (m_isSettings_screen && m_isDraggingSlider)
				{
					float mouseX = (float)LOWORD(lParam);

					float width = m_slider_TrackRect.right - m_slider_TrackRect.left;
					float pct = (mouseX - m_slider_TrackRect.left) / width;
					pct = std::clamp(pct, 0.0f, 1.0f);

					m_MouseSens = m_MinSens + pct * (m_MaxSens - m_MinSens);

					return 0;
				}
			}
			break;

	case WM_LBUTTONUP:



			{
				if (m_isDraggingSlider)
				{
					m_isDraggingSlider = false;
					ReleaseCapture();
					return 0;
				}
			}
			break;

	case WM_KEYDOWN:



			{
				// 1. ESC
				if (wParam == VK_ESCAPE)
				{
					if (m_isSettings_screen)
					{
						ButtonActions(0);
					}
					else if (m_isPause_screen)
					{
						ButtonActions(0);
					}
					else
					{
						ButtonActions(0);
					}	
					return 0;
				}

				// 2. F11
				if (wParam == VK_F11)
				{
					if (m_isMaximized)
					{ ShowWindow(m_hWnd, SW_NORMAL); }
					else
					{ ShowWindow(m_hWnd, SW_MAXIMIZE); }

					UpdateWindow(m_hWnd);
					return 0;
				}

				// 3.NAVEGAR PELAS OPÇÕES USANDO TECLAS
				if (m_isSettings_screen || m_isPause_screen)
				{
					int maxOptions;
					if (m_isSettings_screen) maxOptions = m_settings_MaxOptions;
					else if (m_isPause_screen) maxOptions = m_pause_MaxOptions;

					if (wParam == VK_UP || wParam == 'W')
						m_SelectedOption = (m_SelectedOption - 1 + maxOptions) % maxOptions;

					if (wParam == VK_DOWN || wParam == 'S')
						m_SelectedOption = (m_SelectedOption + 1) % maxOptions;

					if (wParam == VK_RETURN)
						ButtonActions(m_SelectedOption);

					return 0;
				}
			}
			return 0;

	case WM_INPUT:



			{
				if (!m_IsMouseCaptured || m_isPause_screen || m_isSettings_screen) break;

				UINT dwSize = sizeof(RAWINPUT);
				static BYTE lpb[sizeof(RAWINPUT)];

				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
				RAWINPUT* raw = (RAWINPUT*)lpb;

				if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
					{
						float mouseX = (float)raw->data.mouse.lLastX;
						float mouseY = (float)raw->data.mouse.lLastY;

						m_Yaw += mouseX * m_MouseSens;
						m_Pitch += mouseY * m_MouseSens;

						m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);
					}
				}
			}
			return 0;

	case WM_DESTROY:



			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}