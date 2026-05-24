
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

	if (!InitializeDirectX()) return false;

	if (!InitializeAudio()) return false;

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
		MessageBoxA(m_hWnd, "ERRO: A função LoadOBJ não conseguiu carregar o modelo de mapa!", "Erro ao Carregar Modelo 3D", MB_ICONERROR);
		return false;
	}
	allVertices.insert(allVertices.end(), mapVertices.begin(), mapVertices.end());
	m_VertexCount = (UINT)mapVertices.size();

	// 6.2 Carregar Porta Visual
	/*
	m_DoorStartOffset = (UINT)allVertices.size(); // Define onde a porta começa no buffer
	std::vector<VertexOBJ> doorVisualVertices;
	if (!LoadOBJ("door.obj", doorVisualVertices))
	{
		MessageBoxA(m_hWnd, "ERRO: A função LoadOBJ não conseguiu carregar o modelo de mapa!", "Erro ao Carregar Modelo", MB_ICONERROR);
		return false;
	}
	else{
		m_DoorVertexCount = (UINT)doorVisualVertices.size();
		allVertices.insert(allVertices.end(), doorVisualVertices.begin(), doorVisualVertices.end());
	}
	*/

	// Criar Vertex Buffer único para GPU
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(VertexOBJ) * (UINT)allVertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vData = { allVertices.data() };
	hr = m_pDevice->CreateBuffer(&vbd, &vData, &m_pVertexBuffer);
	if (FAILED(hr)) return false;

	// 6.3 INSTANCIAR PORTAS LÓGICAS
	CreateDoor(XMFLOAT3(2.0f, 0.0f, 5.0f), 0.0f, 0.0f, false);

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

	// --- COMPILAÇÃO DO PIXEL SHADER DE CONTORNO PRETO ---
	// Ele vai ler o mesmo arquivo Shaders.hlsl, mas chamará a função "PS_Contour"
	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "PS_Contour", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) errorBlob->Release();
		return false;
	}
	m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPixelShaderContour);
	if (errorBlob) errorBlob->Release();
	psBlob->Release(); // Liberação segura



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

	D3D11_RASTERIZER_DESC wfDesc = {};
	wfDesc.FillMode = D3D11_FILL_WIREFRAME;
	wfDesc.CullMode = D3D11_CULL_NONE; // Permite ver através das caixas
	m_pDevice->CreateRasterizerState(&wfDesc, &m_pWireframeRS);

	// 1. Criar a Factory do Direct2D (Mudado para MULTITHREADED para evitar travar com Raw Input)
	D2D1_FACTORY_OPTIONS options = {};
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), &options, &m_pD2DFactory);
	if (FAILED(hr)) { OutputDebugStringA("Falha: D2D1CreateFactory\n"); return false; }

	// 2. Criar o Device Context do Direct2D a partir do Device do DX11
	ComPtr<IDXGIDevice> pDxgiDevice;
	hr = m_pDevice.As(&pDxgiDevice);
	if (FAILED(hr)) { OutputDebugStringA("Falha: m_pDevice.As DXGI\n"); return false; }

	ComPtr<ID2D1Device> pD2DDevice;
	hr = m_pD2DFactory->CreateDevice(pDxgiDevice.Get(), &pD2DDevice);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateDevice\n"); return false; }

	hr = pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DContext);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateDeviceContext\n"); return false; }

	// 3. Criar o Pincel de Cor (Brush) do Direct2D
	hr = m_pD2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pScreenBrush);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateSolidColorBrush\n"); return false; }

	// 4. Criar a Factory do DirectWrite
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_pDWriteFactory);
	if (FAILED(hr)) { OutputDebugStringA("Falha: DWriteCreateFactory\n"); return false; }

	// 5. Criar o Formato do Texto
	hr = m_pDWriteFactory->CreateTextFormat(
		L"Arial",                   // Mudado para Arial (garantido em qualquer Windows)
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"pt-BR",
		&m_pTextFormat
	);
	if (FAILED(hr)) { OutputDebugStringA("Falha: CreateTextFormat\n"); return false; }

	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// --- CONFIGURAÇÃO DO RASTERIZADOR PARA AS LINHAS DE CONTORNO ---
	D3D11_RASTERIZER_DESC contourDesc = {};
	contourDesc.FillMode = D3D11_FILL_WIREFRAME; // Desenha apenas as bordas
	contourDesc.CullMode = D3D11_CULL_NONE;      // Mostra todas as quinas
	contourDesc.DepthBias = -100;                // Empurra a linha levemente para frente da face
	contourDesc.SlopeScaledDepthBias = -0.5f;
	contourDesc.DepthClipEnable = TRUE;

	// Adicione este ComPtr<ID3D11RasterizerState> m_pContourRS no seu game.h private
	hr = m_pDevice->CreateRasterizerState(&contourDesc, &m_pContourRS);
	if (FAILED(hr)) return false;

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



void MyDX3DGame::Run()
{

	MSG msg = { 0 };

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

				bool agirComoChao = (type == "FLOOR" || normCurrent.find("floor") != std::string::npos || normCurrent.find("rampa") != std::string::npos);

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

						m_FloorTriangles.push_back(tri);
					}
				}
				else if (type == "WALL") {
					OBB box;

					// 1. O centro real é a média exata dos limites da malha rotacionada vinda do .obj
					box.center.x = (wallMax.x + wallMin.x) * 0.5f;
					box.center.y = (wallMax.y + wallMin.y) * 0.5f;
					box.center.z = (wallMax.z + wallMin.z) * 0.5f;

					// 2. SOLUÇÃO DA LARGURA GIGANTE: Como o cubo já veio rotacionado do Blender,
					// nós NÃO aplicamos a rotação do .txt na caixa de debug.
					// Configurando a orientação como IDENTIDADE (0,0,0,1), o tamanho da OBB
					// encolhe e se ajusta perfeitamente às quinas reais do modelo!
					box.extents.x = (wallMax.x - wallMin.x) * 0.5f;
					box.extents.y = (wallMax.y - wallMin.y) * 0.5f;
					box.extents.z = (wallMax.z - wallMin.z) * 0.5f;

					// Rotação Identidade Pura (Impede que a caixa infle)
					box.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };

					m_WallOBBs.push_back(box);
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
		if (ctrl && shift && iKey )
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




	// Audio
	bool isMoving = (GetAsyncKeyState('W') || GetAsyncKeyState('S') || GetAsyncKeyState('A') || GetAsyncKeyState('D'));

	if (m_IsGrounded && isMoving) {
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

	// SE ESTIVER PAUSADO, PARA O UPDATE AQUI
	if (m_isPaused)
	{
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(m_hWnd, &mousePos);

		float mouseX = (float)mousePos.x;
		float mouseY = (float)mousePos.y;

		int hoverOption = -1;

		if (mouseX >= m_menuOpt1Rect.left && mouseX <= m_menuOpt1Rect.right &&
			mouseY >= m_menuOpt1Rect.top && mouseY <= m_menuOpt1Rect.bottom)
		{
			hoverOption = 0;
		}
		else if (mouseX >= m_menuOpt2Rect.left && mouseX <= m_menuOpt2Rect.right &&
			mouseY >= m_menuOpt2Rect.top && mouseY <= m_menuOpt2Rect.bottom)
		{
			hoverOption = 1;
		}

		if (hoverOption != -1)
		{
			m_SelectedOption = hoverOption;
		}

		return; // Mantém o congelamento da física do jogo
	}



	// --- CONFIGURAÇÕES DO PLAYER ---
	float charRadius = 0.3f;
	float charHeight = 1.73f;
	float eyeHeight = 1.62f;
	float stepHeight = 0.3f; // Altura máxima que o player "ignora" para subir degraus

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
	if (m_IsRunning) {
		m_MoveSpeed = 12.0f;
	}
	else {
		m_MoveSpeed = 6.0f;
	}

	// Normaliza e aplica magnitude
	if (XMVectorGetX(XMVector3LengthSq(velocityVec)) > 0.001f) {
		velocityVec = XMVector3Normalize(velocityVec) * m_MoveSpeed * deltaTime;
	}
	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, velocityVec);


	// 3. MOVIMENTO E COLISÃO HORIZONTAL (X e Z) usando OBB para Paredes e Portas
	auto CheckHorizontalCollision = [&](const AABB& pBox) {
		using namespace DirectX;

		// Converte a AABB estrutural do Player para o formato BoundingBox do DirectX
		XMVECTOR pMin = XMLoadFloat3(&pBox.min);
		XMVECTOR pMax = XMLoadFloat3(&pBox.max);
		XMVECTOR pCenter = (pMin + pMax) * 0.5f;
		XMVECTOR pExtents = (pMax - pMin) * 0.5f;

		BoundingBox playerDirectXBox;
		XMStoreFloat3(&playerDirectXBox.Center, pCenter);
		XMStoreFloat3(&playerDirectXBox.Extents, pExtents);

		// Varre as OBBs de Paredes usando a assinatura direta correta do construtor
		for (const auto& wall : m_WallOBBs) {
			DirectX::BoundingOrientedBox wallOriented;
			DirectX::XMStoreFloat3(&wallOriented.Center, DirectX::XMLoadFloat3(&wall.center));
			DirectX::XMStoreFloat3(&wallOriented.Extents, DirectX::XMLoadFloat3(&wall.extents));
			
			// GARANTE A SINCRONIA: Lê a orientação real salva na OBB (Identidade)
			DirectX::XMStoreFloat4(&wallOriented.Orientation, DirectX::XMLoadFloat4(&wall.orientation));

			if (wallOriented.Intersects(playerDirectXBox)) return true;
		}

		// Varre as portas lógicas do mapa
		for (const auto& door : m_Doors) {
			XMVECTOR dMin = XMLoadFloat3(&door.collisionBox.min);
			XMVECTOR dMax = XMLoadFloat3(&door.collisionBox.max);
			BoundingBox doorDxBox;
			XMStoreFloat3(&doorDxBox.Center, (dMin + dMax) * 0.5f);
			XMStoreFloat3(&doorDxBox.Extents, (dMax - dMin) * 0.5f);

			if (doorDxBox.Intersects(playerDirectXBox)) return true;
		}

		return false;
		};

	// Eixo X
	m_PlayerPos.x += velocity.x;
	AABB playerBoxX = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	if (CheckHorizontalCollision(playerBoxX)) m_PlayerPos.x -= velocity.x;

	// Eixo Z
	m_PlayerPos.z += velocity.z;
	AABB playerBoxZ = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	if (CheckHorizontalCollision(playerBoxZ)) m_PlayerPos.z -= velocity.z;

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

		for (const auto& tri : m_FloorTriangles) {
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

	// 6. INTERAÇÃO E PORTAS LÓGICAS COM ROTAÇÃO E RIO DE JANEIRO (OBB)
	bool fPressed = GetAsyncKeyState('F') & 0x0001;
	XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
	XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
	XMVECTOR eyePos = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);

	for (auto& door : m_Doors) {
		using namespace DirectX;

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
			if (fPressed) {
				door.isOpen = !door.isOpen;
				door.targetRotation = door.isOpen ? 90.0f : 0.0f;
			}
		}

		// Animação suave da dobradiça
		door.currentRotation += (door.targetRotation - door.currentRotation) * 5.0f * deltaTime;

		// --- ATUALIZAÇÃO DA COLISÃO DINÂMICA (Atualiza a bounding box alinhada para simplificar) ---
		float w = 1.0f; float t = 0.15f; float h = 2.0f;
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

	float targetFOV = m_IsRunning ? 80.0f : 60.0f;
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

	// PASSE B: Desenha as linhas de contorno pretas nas quinas (Edges)
	ComPtr<ID3D11RasterizerState> pOldRS;
	m_pDevCon->RSGetState(&pOldRS);

	// 1. Vincula o estado de wireframe com bias e a topologia de linha
	m_pDevCon->RSSetState(m_pContourRS.Get());
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 2. TROCA O PIXEL SHADER: Desvincula o colorido e ativa o preto puro
	m_pDevCon->PSSetShader(m_pPixelShaderContour.Get(), nullptr, 0);

	// 3. Desenha os contornos pretos aproveitando os mesmos vértices
	m_pDevCon->Draw(m_VertexCount, 0);

	// RESTAURAÇÃO: Devolve os Shaders e Estados originais para o restante do frame
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDevCon->PSSetShader(m_pPixelShader.Get(), nullptr, 0); // Volta pro shader colorido
	m_pDevCon->RSSetState(pOldRS.Get());


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
	if (m_isPaused && m_pD2DContext)
	{
		// PASSO CRUCIAL A: Obter a superfície DXGI do Back Buffer para o Direct2D saber onde desenhar
		ComPtr<IDXGISurface> pBackBufferSurface;
		HRESULT hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferSurface));

		if (SUCCEEDED(hr))
		{
			// PASSO CRUCIAL B: Criar um Bitmap do Direct2D apontando diretamente para essa superfície
			ComPtr<ID2D1Bitmap1> pD2DBackBufferTarget;
			D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
			);

			hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pBackBufferSurface.Get(), &bp, &pD2DBackBufferTarget);

			if (SUCCEEDED(hr))
			{
				// PASSO CRUCIAL C: Definir o bitmap como alvo atual do nosso contexto Direct2D
				m_pD2DContext->SetTarget(pD2DBackBufferTarget.Get());

				// PASSO CRUCIAL D: Desvincular Shaders do DirectX 11 para não gerar conflito de estados com o Direct2D
				m_pDevCon->VSSetShader(nullptr, nullptr, 0);
				m_pDevCon->PSSetShader(nullptr, nullptr, 0);

				m_pD2DContext->BeginDraw();

				// 1. Desenha o fundo escuro semi-transparente
				D2D1_RECT_F screenRect = D2D1::RectF(0.0f, 0.0f, (float)m_WndWidth, (float)m_WndHeight);
				m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.6f));
				m_pD2DContext->FillRectangle(&screenRect, m_pScreenBrush.Get());

				// 2. Título
				m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
				D2D1_RECT_F titleRect = D2D1::RectF(0.0f, (float)m_WndHeight * 0.3f, (float)m_WndWidth, (float)m_WndHeight * 0.4f);
				m_pD2DContext->DrawTextW(L"JOGO PAUSADO", 12, m_pTextFormat.Get(), titleRect, m_pScreenBrush.Get());

				// --- 1. CONFIGURAÇÕES GERAIS DAS CAIXAS DE FUNDO ---
				float boxWidth = 350.0f; // Largura da caixinha de fundo em pixels
				float leftPos = ((float)m_WndWidth - boxWidth) / 2.0f;
				float rightPos = ((float)m_WndWidth + boxWidth) / 2.0f;

				// --- 2. OPÇÃO 1: CONTINUAR ---
				D2D1_RECT_F opt1Rect = D2D1::RectF(0.0f, (float)m_WndHeight * 0.5f, (float)m_WndWidth, (float)m_WndHeight * 0.55f);

				// Cria o retângulo menor centralizado para o fundo da Opção 1
				m_menuOpt1Rect = D2D1::RectF(leftPos, opt1Rect.top, rightPos, opt1Rect.bottom);

				// Se estiver selecionado/hover, usa um fundo cinza mais claro, senão usa um escuro bem sutil
				if (m_SelectedOption == 0) {
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 0.15f)); // Branco com 15% de opacidade
					m_pD2DContext->FillRectangle(&m_menuOpt1Rect, m_pScreenBrush.Get());
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow)); // Texto amarelo
				}
				else {
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.3f)); // Preto com 30% de opacidade
					m_pD2DContext->FillRectangle(&m_menuOpt1Rect, m_pScreenBrush.Get());
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White)); // Texto branco
				}

				const wchar_t menu_text_opt_01[] = L"Continuar";
				m_pD2DContext->DrawTextW(menu_text_opt_01, (sizeof(menu_text_opt_01) / sizeof(const wchar_t)), m_pTextFormat.Get(), opt1Rect, m_pScreenBrush.Get());



				// --- 3. OPÇÃO 2: SAIR DO JOGO ---
				D2D1_RECT_F opt2Rect = D2D1::RectF(0.0f, (float)m_WndHeight * 0.58f, (float)m_WndWidth, (float)m_WndHeight * 0.63f);

				// Cria o retângulo menor centralizado para o fundo da Opção 2
				m_menuOpt2Rect = D2D1::RectF(leftPos, opt2Rect.top, rightPos, opt2Rect.bottom);

				if (m_SelectedOption == 1) {
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 0.15f));
					m_pD2DContext->FillRectangle(&m_menuOpt2Rect, m_pScreenBrush.Get());
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
				}
				else {
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.3f));
					m_pD2DContext->FillRectangle(&m_menuOpt2Rect, m_pScreenBrush.Get());
					m_pScreenBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
				}

				const wchar_t menu_text_opt_02[] = L"Sair do Jogo";
				m_pD2DContext->DrawTextW(menu_text_opt_02, (sizeof(menu_text_opt_02) / sizeof(const wchar_t)), m_pTextFormat.Get(), opt2Rect, m_pScreenBrush.Get());

				m_pD2DContext->EndDraw();

				// PASSO CRUCIAL E: Desvincular o alvo para liberar o buffer para o próximo frame
				m_pD2DContext->SetTarget(nullptr);
			}
		}
	}

	// 8. APRESENTAÇÃO
	m_pSwapChain->Present(1, 0);
}


















void MyDX3DGame::DrawDebugBoxes() {

	// 1. Evita processamento se não houver nada para desenhar
	if (m_Doors.empty() && m_WallOBBs.empty() && m_FloorTriangles.empty()) return;

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

	// --- RENDERIZAÇÃO DAS PAREDES ROTACIONADAS (m_WallOBBs) ---
	for (const auto& wall : m_WallOBBs) {
		XMMATRIX mScale = XMMatrixScaling(wall.extents.x * 2.0f, wall.extents.y * 2.0f, wall.extents.z * 2.0f);
		XMVECTOR qRot = XMLoadFloat4(&wall.orientation);
		XMMATRIX mRot = XMMatrixRotationQuaternion(qRot);
		XMMATRIX mTrans = XMMatrixTranslation(wall.center.x, wall.center.y, wall.center.z);
		XMMATRIX world = mScale * mRot * mTrans;

		UpdateConstantBuffer(world);
		m_pDevCon->Draw(36, 0);
	}

	// --- NOVO: RENDERIZAÇÃO WIREFRAME DAS RAMPAS E CHÃOS (m_FloorTriangles) ---
	if (!m_FloorTriangles.empty()) {
		// Desvencilha o Vertex Buffer de cubos para podermos desenhar geometria dinâmica via Subresource
		m_pDevCon->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);

		// Altera a topologia para lista de linhas (LINELIST), ideal para contornos matemáticos
		m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		// Como os triângulos já estão transformados em coordenadas globais no LoadOBJ, 
		// mandamos uma matriz Identidade para o Buffer de Constantes do Vertex Shader
		UpdateConstantBuffer(XMMatrixIdentity());

		// Cada triângulo precisa de 3 linhas (6 vértices totais: v0-v1, v1-v2, v2-v0)
		std::vector<VertexOBJ> lineVertices;
		lineVertices.reserve(m_FloorTriangles.size() * 6);

		for (const auto& tri : m_FloorTriangles) {
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

		// Cria e popula um buffer temporário dinâmico na GPU para enviar as linhas criadas
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

	// 4. Restaura o Rasterizer State original para não quebrar o mapa visual detalhado
	m_pDevCon->RSSetState(pOldRS.Get());
}



void MyDX3DGame::OnResize(UINT width, UINT height)
{
	if (!m_pDevice || !m_pSwapChain) return;

	// 0. Atualiza as variáveis de tamanho da classe
	m_WndWidth = width;
	m_WndHeight = height;

	// 1. Limpar estados e referências do DirectX 11 e Direct2D
	m_pDevCon->OMSetRenderTargets(0, nullptr, nullptr);

	if (m_pD2DContext)
	{
		m_pD2DContext->SetTarget(nullptr); // Remove o vínculo do Direct2D com o Back Buffer antigo
	}

	m_pBackBufferView.Reset();
	m_pDepthStencilView.Reset();

	// Limpa a fila de comandos da GPU para garantir que nada está usando os buffers
	m_pDevCon->Flush();

	// 2. Redimensionar buffers internos
	HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) return;

	// 3. Recriar a Render Target View (Back Buffer)
	ComPtr<ID3D11Texture2D> pBackBuffer;
	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (SUCCEEDED(hr))
	{
		m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pBackBufferView);
	}

	// 4. Recriar o Depth Stencil com o NOVO tamanho
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pDepthStencil;
	hr = m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
	if (SUCCEEDED(hr))
	{
		m_pDevice->CreateDepthStencilView(pDepthStencil.Get(), nullptr, &m_pDepthStencilView);
	}

	// 5. Vincular novamente os novos targets ao pipeline do DX11
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());

	// 6. Atualizar a Viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pDevCon->RSSetViewports(1, &vp);
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
	if (m_pTextFormat) { m_pTextFormat.Reset(); }
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
					m_isPaused = true;
					m_IsMouseCaptured = false;
					ClipCursor(NULL);

					// CORRIGIDO: Força o contador do Windows a ficar visível (>= 0)
					while (ShowCursor(TRUE) < 0); 
					
					m_SelectedOption = 0; 
				}
				}
				return 0;

		case WM_SIZE:



				if (wParam != SIZE_MINIMIZED)
				{
					// Atualiza as variáveis da classe para o novo tamanho da janela
					m_WndWidth = LOWORD(lParam);
					m_WndHeight = HIWORD(lParam);
					// Se você já tiver o método OnResize, chame-o aqui
					OnResize(m_WndWidth, m_WndHeight);
				}
				return 0;

		case WM_LBUTTONDOWN: // Clicou na tela


				if (m_isPaused)
				{
					// Captura a posição atual do ponteiro
					float mouseX = (float)LOWORD(lParam);
					float mouseY = (float)HIWORD(lParam);

					// Testa usando os retângulos oficiais salvos na classe
					if (mouseX >= m_menuOpt1Rect.left && mouseX <= m_menuOpt1Rect.right && mouseY >= m_menuOpt1Rect.top && mouseY <= m_menuOpt1Rect.bottom)
					{
						m_isPaused = false;
						m_IsMouseCaptured = true;

						// Força o cursor a sumir de verdade
						while (ShowCursor(FALSE) >= 0);

						RECT rect;
						GetWindowRect(m_hWnd, &rect);
						ClipCursor(&rect);
					}
					else if (mouseX >= m_menuOpt2Rect.left && mouseX <= m_menuOpt2Rect.right && mouseY >= m_menuOpt2Rect.top && mouseY <= m_menuOpt2Rect.bottom)
					{
						PostQuitMessage(0);
					}
				}
				
				else if (!m_IsMouseCaptured)
				{
					m_IsMouseCaptured = true;
					ShowCursor(FALSE); // Esconde o mouse
					// Trava o mouse dentro da nossa janela
					RECT rect;
					GetWindowRect(m_hWnd, &rect);
					ClipCursor(&rect);
				}
				return 0;

		case WM_KEYDOWN:



				if (wParam == VK_ESCAPE) // Apertou ESC
				{
					m_isPaused = !m_isPaused; // Inverte o estado de pause

					if (m_isPaused)
					{
						// Entrou no Pause: Libera o mouse para clicar ou navegar
						m_IsMouseCaptured = false;
						ShowCursor(TRUE);
						ClipCursor(NULL);
						m_SelectedOption = 0; // Reseta a seleção para a primeira opção
					}
					else
					{
						// Saiu do Pause: Recaptura o mouse para o gameplay
						m_IsMouseCaptured = true;
						ShowCursor(FALSE);
						RECT rect;
						GetWindowRect(m_hWnd, &rect);
						ClipCursor(&rect);
					}
					return 0;
				}

				// Se o jogo estiver pausado, processa a navegação do menu
				if (m_isPaused)
				{
					if (wParam == VK_UP || wParam == 'W')
					{
						m_SelectedOption = (m_SelectedOption - 1 + m_MaxOptions) % m_MaxOptions;
					}
					if (wParam == VK_DOWN || wParam == 'S')
					{
						m_SelectedOption = (m_SelectedOption + 1) % m_MaxOptions;
					}
					if (wParam == VK_RETURN) // Apertou ENTER
					{
						if (m_SelectedOption == 0) // Retornar
						{
							m_isPaused = false;
							m_IsMouseCaptured = true;
							ShowCursor(FALSE);
							RECT rect;
							GetWindowRect(m_hWnd, &rect);
							ClipCursor(&rect);
						}
						else if (m_SelectedOption == 1) // Sair
						{
							PostQuitMessage(0);
						}
					}
					return 0; // Impede que o comando passe para o jogo se estiver pausado
				}
				return 0;

		case WM_INPUT:



				{
				if (!m_IsMouseCaptured) break;

				if (m_isPaused) break;

				UINT dwSize = sizeof(RAWINPUT);
				static BYTE lpb[sizeof(RAWINPUT)];

				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
				RAWINPUT* raw = (RAWINPUT*)lpb;

				if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					float mouseX = (float)raw->data.mouse.lLastX;
					float mouseY = (float)raw->data.mouse.lLastY;

					m_Yaw += mouseX * m_MouseSens;
					m_Pitch += mouseY * m_MouseSens;

					// Trava o olhar vertical em 85 graus
					m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);
				}
				}
				return 0;

	}

	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);


}