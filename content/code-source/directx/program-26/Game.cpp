
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

	return true;

}



bool MyDX3DGame::InitializeDirectX()
{

	// --- CONFIGURAÇÃO DE INPUT ---
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = m_hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	// 1. CREATE DEVICE & CONTEXT
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
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
	if (!LoadOBJ("strange-map-vis.obj", mapVertices)) return false;
	allVertices.insert(allVertices.end(), mapVertices.begin(), mapVertices.end());
	m_VertexCount = (UINT)mapVertices.size();

	// 6.2 Carregar Porta Visual
	m_DoorStartOffset = (UINT)allVertices.size(); // Define onde a porta começa no buffer
	std::vector<VertexOBJ> doorVisualVertices;
	if (LoadOBJ("door.obj", doorVisualVertices)) {
		m_DoorVertexCount = (UINT)doorVisualVertices.size();
		allVertices.insert(allVertices.end(), doorVisualVertices.begin(), doorVisualVertices.end());
	}

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
	if (FAILED(hr)) return false;
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
	if (FAILED(hr)) return false;
	m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	psBlob->Release();

	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_WIREFRAME; // A mágica acontece aqui
	rd.CullMode = D3D11_CULL_NONE;      // Ver as linhas de todos os lados
	rd.DepthClipEnable = true;

	m_pDevice->CreateRasterizerState(&rd, &m_pWireframeRS);

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

	return true;
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








bool MyDX3DGame::LoadOBJ(const char* path, std::vector<VertexOBJ>& out_vertices)
{

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<DirectX::XMFLOAT3> temp_vertices;
	std::vector<DirectX::XMFLOAT2> temp_uvs;
	std::vector<DirectX::XMFLOAT3> temp_normals;

	std::ifstream file(path);
	if (!file.is_open()) return false;

	// Limpar as listas de colisão antes de carregar (caso chame a função mais de uma vez)
	m_FloorTriangles.clear();
	m_WallObstacles.clear();
	m_CeilingObstacles.clear();

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
			temp_uvs.push_back({ vt.x, 1.0f - vt.y });
		}
		else if (prefix == "vn") {
			DirectX::XMFLOAT3 vn;
			ss >> vn.x >> vn.y >> vn.z;
			temp_normals.push_back(vn);
		}
		else if (prefix == "f") {
			std::string vertexData;
			std::vector<unsigned int> faceVIndices;

			while (ss >> vertexData) {
				std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
				std::stringstream vss(vertexData);
				unsigned int vIdx = 0; int tIdx = -1, nIdx = -1;

				vss >> vIdx;
				faceVIndices.push_back(vIdx - 1);
				vertexIndices.push_back(vIdx - 1);

				if (vss >> tIdx) uvIndices.push_back(tIdx - 1);
				if (vss >> nIdx) normalIndices.push_back(nIdx - 1);
			}

			// --- LÓGICA DE COLISÃO ---
			if (faceVIndices.size() >= 3) {
				using namespace DirectX;
				XMVECTOR p0 = XMLoadFloat3(&temp_vertices[faceVIndices[0]]);
				XMVECTOR p1 = XMLoadFloat3(&temp_vertices[faceVIndices[1]]);
				XMVECTOR p2 = XMLoadFloat3(&temp_vertices[faceVIndices[2]]);

				// Calcula a normal geométrica da face (Produto Vetorial)
				XMVECTOR v1 = p1 - p0;
				XMVECTOR v2 = p2 - p0;
				XMVECTOR n = XMVector3Normalize(XMVector3Cross(v1, v2));
				XMFLOAT3 faceNormal;
				XMStoreFloat3(&faceNormal, n);

				// Criar AABB para a face
				AABB box;
				box.min = { FLT_MAX, FLT_MAX, FLT_MAX };
				box.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (unsigned int idx : faceVIndices) {
					XMFLOAT3 v = temp_vertices[idx];
					box.min.x = (std::min)(box.min.x, v.x);
					box.min.y = (std::min)(box.min.y, v.y);
					box.min.z = (std::min)(box.min.z, v.z);
					box.max.x = (std::max)(box.max.x, v.x);
					box.max.y = (std::max)(box.max.y, v.y);
					box.max.z = (std::max)(box.max.z, v.z);
				}

				// --- CLASSIFICAÇÃO ---
				// 1. Chão (Normal apontando para cima)
				if (faceNormal.y > 0.7f) {
					Triangle tri = { temp_vertices[faceVIndices[0]], temp_vertices[faceVIndices[1]], temp_vertices[faceVIndices[2]] };
					m_FloorTriangles.push_back(tri);
				}
				// 2. Teto (Normal apontando para baixo)
				else if (faceNormal.y < -0.7f) {
					m_CeilingObstacles.push_back(box);
				}
				// 3. Paredes (Normal horizontal / Inclinações íngremes)
				else {
					// Adicionamos um pequeno "padding" para evitar que o player fique preso na face
					box.min.x -= 0.01f; box.min.z -= 0.01f;
					box.max.x += 0.01f; box.max.z += 0.01f;
					m_WallObstacles.push_back(box);
				}
			}
		}
	}

	// Remontagem final para a GPU
	out_vertices.clear();
	out_vertices.reserve(vertexIndices.size());
	for (size_t i = 0; i < vertexIndices.size(); i++) {
		VertexOBJ v = {};
		v.pos = temp_vertices[vertexIndices[i]];
		if (!uvIndices.empty() && uvIndices[i] < temp_uvs.size()) v.tex = temp_uvs[uvIndices[i]];
		if (!normalIndices.empty() && normalIndices[i] < temp_normals.size()) v.normal = temp_normals[normalIndices[i]];
		out_vertices.push_back(v);
	}

	return true;
}



void MyDX3DGame::CreateDoor(XMFLOAT3 pos, float currentRot, float targetRot, bool isOpen) {
	Door myDoor;
	myDoor.position = pos;
	myDoor.currentRotation = currentRot;
	myDoor.targetRotation = targetRot;
	myDoor.isOpen = isOpen;

	float w = 1.0f; float h = 2.0f; float t = 0.1f;

	// Define o centro, a extensão (metade do tamanho) e a rotação inicial
	// Colocamos o centro da caixa deslocado da dobradiça
	myDoor.collisionBox.Center = XMFLOAT3(pos.x + w / 2.0f, pos.y + h / 2.0f, pos.z);
	myDoor.collisionBox.Extents = XMFLOAT3(w / 2.0f, h / 2.0f, t / 2.0f);

	// Converte a rotação para Quatérnio (necessário para OBB)
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians(currentRot), 0);
	XMStoreFloat4(&myDoor.collisionBox.Orientation, q);

	m_Doors.push_back(myDoor);
}



void MyDX3DGame::Update() {
	// 1. DELTA TIME
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0) timeStart = timeCur;
	float deltaTime = (timeCur - timeStart) / 1000.0f;
	timeStart = timeCur;

	// --- CONFIGURAÇÕES DO PLAYER ---
	float charRadius = 0.4f;
	float charHeight = 1.72f;
	float eyeHeight = 1.68f;
	float stepHeight = 0.3f;

	// 2. INPUT E VELOCIDADE HORIZONTAL
	XMMATRIX moveRotation = XMMatrixRotationRollPitchYaw(0, m_Yaw, 0);
	XMVECTOR moveForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), moveRotation);
	XMVECTOR moveRight = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), moveRotation);
	XMVECTOR velocityVec = XMVectorSet(0, 0, 0, 0);

	if (GetAsyncKeyState('W') & 0x8000) velocityVec += moveForward;
	if (GetAsyncKeyState('S') & 0x8000) velocityVec -= moveForward;
	if (GetAsyncKeyState('D') & 0x8000) velocityVec += moveRight;
	if (GetAsyncKeyState('A') & 0x8000) velocityVec -= moveRight;

	// Atalho de Debug (Ctrl + Shift + D)
	bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
	bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
	bool dKey = (GetAsyncKeyState('D') & 0x8000);
	if (ctrl && shift && dKey) {
		if (!m_ComboPressed) { m_ShowDebug = !m_ShowDebug; m_ComboPressed = true; }
	}
	else { m_ComboPressed = false; }

	if (XMVectorGetX(XMVector3LengthSq(velocityVec)) > 0.001f) {
		velocityVec = XMVector3Normalize(velocityVec) * m_MoveSpeed * deltaTime;
	}
	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, velocityVec);

	// 3. LÓGICA DE COLISÃO HORIZONTAL (AABB vs OBB)
	auto CheckHorizontalCollision = [&](AABB& box) {
		// 1. Criar BoundingBox do Player
		XMVECTOR vMin = XMVectorMin(XMLoadFloat3(&box.min), XMLoadFloat3(&box.max));
		XMVECTOR vMax = XMVectorMax(XMLoadFloat3(&box.min), XMLoadFloat3(&box.max));
		DirectX::BoundingBox playerBB;
		DirectX::BoundingBox::CreateFromPoints(playerBB, vMin, vMax);

		// 2. Colisão com Portas (OBB) - Mantendo o que já funciona
		for (const auto& door : m_Doors) {
			if (door.collisionBox.Intersects(playerBB)) return true;
		}

		// 3. Colisão com Paredes (AABB) - Reativando com BoundingBox do DirectX
		for (const auto& obs : m_WallObstacles) {
			DirectX::BoundingBox obsBB;
			XMVECTOR oMin = XMLoadFloat3(&obs.min);
			XMVECTOR oMax = XMLoadFloat3(&obs.max);
			DirectX::BoundingBox::CreateFromPoints(obsBB, oMin, oMax);

			if (playerBB.Intersects(obsBB)) return true;
		}

		return false;
		};

	// Aplicação do Movimento com folga de 0.5f do chão para não travar no piso
	m_PlayerPos.x += velocity.x;
	AABB pBoxX = { {m_PlayerPos.x - charRadius, m_PlayerPos.y + 0.5f, m_PlayerPos.z - charRadius},
				   {m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius} };
	if (CheckHorizontalCollision(pBoxX)) m_PlayerPos.x -= velocity.x;

	m_PlayerPos.z += velocity.z;
	AABB pBoxZ = { {m_PlayerPos.x - charRadius, m_PlayerPos.y + 0.5f, m_PlayerPos.z - charRadius},
				   {m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius} };
	if (CheckHorizontalCollision(pBoxZ)) m_PlayerPos.z -= velocity.z;

	// 4. FÍSICA VERTICAL (Gravidade e Pulo)
	if (m_IsGrounded) {
		m_VelocityY = 0.0f;
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			m_VelocityY = JUMP_FORCE;
			m_IsGrounded = false;
		}
	}
	else {
		m_VelocityY += GRAVITY * deltaTime;
	}
	m_PlayerPos.y += m_VelocityY * deltaTime;

	// 5. COLISÃO VERTICAL (RAYCAST PARA O CHÃO)
	m_IsGrounded = false;
	XMVECTOR rayOrigin = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, 1.0f, 0, 0);
	XMVECTOR rayDir = XMVectorSet(0, -1.0f, 0, 0);
	float closestDist = FLT_MAX;
	bool hitFloor = false;

	for (const auto& tri : m_FloorTriangles) {
		float dist = 0.0f;
		if (RayTriangleIntersect(rayOrigin, rayDir, tri, dist)) {
			if (dist < closestDist) { closestDist = dist; hitFloor = true; }
		}
	}

	if (hitFloor) {
		float floorY = XMVectorGetY(rayOrigin) - closestDist;
		// Só gruda se estivermos caindo e o chão estiver perto dos pés
		if (m_VelocityY <= 0 && floorY >= (m_PlayerPos.y - 0.2f) && floorY <= (m_PlayerPos.y + stepHeight)) {
			m_PlayerPos.y = floorY;
			m_VelocityY = 0;
			m_IsGrounded = true;
		}
	}

	// 6. INTERAÇÃO E PORTAS (OBB)
	static bool fWasPressed = false;
	bool fIsPressed = (GetAsyncKeyState('F') & 0x8000);
	bool fJustPressed = fIsPressed && !fWasPressed;
	fWasPressed = fIsPressed;

	for (auto& door : m_Doors) {
		XMVECTOR pPos = XMLoadFloat3(&m_PlayerPos);
		XMVECTOR dPos = XMLoadFloat3(&door.position);

		if (fJustPressed && XMVectorGetX(XMVector3Length(pPos - dPos)) < 2.5f) {
			door.isOpen = !door.isOpen;
			door.targetRotation = door.isOpen ? 90.0f : 0.0f;
		}

		door.currentRotation += (door.targetRotation - door.currentRotation) * 5.0f * deltaTime;

		// Atualização da OBB
		float w = 1.0f; float h = 2.0f;
		float rad = XMConvertToRadians(door.currentRotation);
		door.collisionBox.Center.x = door.position.x + cos(rad) * (w / 2.0f);
		door.collisionBox.Center.y = door.position.y + (h / 2.0f);
		door.collisionBox.Center.z = door.position.z - sin(rad) * (w / 2.0f);
		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, rad, 0);
		XMStoreFloat4(&door.collisionBox.Orientation, q);
	}

	// 7. CÂMERA
	XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
	XMVECTOR lookForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
	XMVECTOR Eye = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(Eye, Eye + lookForward, XMVectorSet(0, 1, 0, 0));

	float aspect = (m_WndHeight > 0) ? (float)m_WndWidth / m_WndHeight : 1.77f;
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspect, 0.1f, 1000.0f);
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
	m_pDevCon->Draw(m_VertexCount, 0);

	// --- 5. DESENHAR AS PORTAS (CENÁRIO DINÂMICO) ---
	for (const auto& door : m_Doors) {
		// Criar matriz de transformação da porta
		XMMATRIX mRotation = XMMatrixRotationY(XMConvertToRadians(door.currentRotation));
		XMMATRIX mTranslation = XMMatrixTranslation(door.position.x, door.position.y, door.position.z);
		XMMATRIX mWorld = mRotation * mTranslation;

		// Atualizar Constant Buffer apenas para este desenho
		ConstantBuffer cb;
		cb.mWorld = XMMatrixTranspose(mWorld);
		cb.mView = XMMatrixTranspose(m_ViewMatrix);
		cb.mProjection = XMMatrixTranspose(m_ProjectionMatrix);

		m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

		// Desenhar a malha da porta (supondo que você carregou um doorVertexBuffer)
		// Se usar o mesmo buffer do mapa, você precisa saber o offset de onde a porta começa
		m_pDevCon->Draw(m_DoorVertexCount, m_DoorStartOffset);
	}

	if (m_ShowDebug) {
		DrawDebugBoxes();
	}

	// 6. APRESENTAÇÃO
	m_pSwapChain->Present(1, 0);
}















void MyDX3DGame::DrawDebugBoxes() {

	if (m_Doors.empty()) return;
	if (m_WallObstacles.empty()) return;

	// 1. Muda para Wireframe
	m_pDevCon->RSSetState(m_pWireframeRS.Get());

	// 2. Configura o buffer do cubo de debug
	UINT stride = sizeof(VertexOBJ);
	UINT offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pDebugCubeBuffer.GetAddressOf(), &stride, &offset);
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& door : m_Doors) {
		// 1. Dimensões da folha da porta (devem ser iguais às do modelo .obj)
		float w = 1.0f;  // Largura
		float h = 2.0f;  // Altura
		float t = 0.1f;  // Espessura

		// 2. Escala o cubo unitário para o tamanho da porta
		XMMATRIX mScale = XMMatrixScaling(w, h, t);

		// 3. AJUSTE DO PIVÔ (A CHAVE DO PROBLEMA):
		// Movemos o cubo para que o "eixo Y" fique na borda (dobradiça)
		// Se a porta gira pelo lado esquerdo, movemos metade da largura para a direita
		XMMATRIX mPivot = XMMatrixTranslation(w / 2.0f, h / 2.0f, 0.0f);

		// 4. Aplica a rotação que a porta tem no momento
		XMMATRIX mRot = XMMatrixRotationY(XMConvertToRadians(door.currentRotation));

		// 5. Move para a posição real no mundo
		XMMATRIX mTrans = XMMatrixTranslation(door.position.x, door.position.y, door.position.z);

		// A ordem de multiplicação é: ESCALA -> PIVÔ -> ROTAÇÃO -> TRANSLAÇÃO
		XMMATRIX world = mScale * mPivot * mRot * mTrans;

		UpdateConstantBuffer(world);
		m_pDevCon->Draw(36, 0);
	}

	for (auto& wall : m_WallObstacles) {
		// 1. Calcular o Centro da caixa
		float centerX = (wall.max.x + wall.min.x) / 2.0f;
		float centerY = (wall.max.y + wall.min.y) / 2.0f;
		float centerZ = (wall.max.z + wall.min.z) / 2.0f;

		// 2. Calcular o Tamanho (Largura, Altura, Profundidade)
		float width = wall.max.x - wall.min.x;
		float height = wall.max.y - wall.min.y;
		float depth = wall.max.z - wall.min.z;

		// 3. Criar as matrizes usando esses valores
		// Nota: O cubo unitário (1x1x1) deve estar centralizado na origem (0,0,0)
		XMMATRIX mScale = XMMatrixScaling(width, height, depth);
		XMMATRIX mTrans = XMMatrixTranslation(centerX, centerY, centerZ);
		XMMATRIX world = mScale * mTrans;

		UpdateConstantBuffer(world);

		m_pDevCon->Draw(36, 0);
	}

	// 4. Volta para o modo normal (Preenchido)
	m_pDevCon->RSSetState(nullptr);
}



void MyDX3DGame::OnResize(UINT width, UINT height)
{
	if (!m_pDevice || !m_pSwapChain) return;

	// 0. Atualiza as variáveis de tamanho da classe
	m_WndWidth = width;
	m_WndHeight = height;

	// 1. Limpar estados e referências
	// Remove os buffers do pipeline para que possam ser liberados
	m_pDevCon->OMSetRenderTargets(0, nullptr, nullptr);

	m_pBackBufferView.Reset();
	m_pDepthStencilView.Reset();

	// Limpa a fila de comandos da GPU para garantir que nada está usando os buffers
	m_pDevCon->Flush();

	// 2. Redimensionar buffers internos
	// O uso de DXGI_FORMAT_UNKNOWN mantém o formato atual
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

	// 5. Vincular novamente os novos targets ao pipeline
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

	// 1. Recursos de Renderização (Views, Buffers, Shaders)
	if (m_pInputLayout) { m_pInputLayout.Reset(); }

	// 2. Recursos de Renderização (Views, Buffers, Shaders)
	if (m_pDepthStencilView) { m_pDepthStencilView.Reset(); }

	// 3. Recursos de Renderização (Views, Buffers, Shaders)
	if (m_pBackBufferView) { m_pBackBufferView.Reset(); }

	// 4. Infraestrutura de exibição (Swap Chain)
	if (m_pSwapChain) { m_pSwapChain.Reset(); }

	// 5. A Factory (que criou a Swap Chain)
	if (m_pFactory) { m_pFactory.Reset(); }

	// 6. O Contexto (quem executa os comandos)
	if (m_pDevCon) { m_pDevCon.Reset(); }

	// 7. O Dispositivo (O "Pai" de todos, deve ser o último)
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
	else
	{
		game.~MyDX3DGame();
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



	switch (uMsg)
	{

		case WM_CLOSE:



				if (MessageBox(pGame->m_hWnd, L"Do you really want to quit the game?", L"Confirm Exit.", MB_ICONQUESTION | MB_YESNO) == IDYES)
				{
					DestroyWindow(pGame->m_hWnd);
				}

			return 0;
			break;

		case WM_DESTROY:



				PostQuitMessage(0);

			break;

		case WM_SIZE:
		case WM_LBUTTONDOWN:
		case WM_KEYDOWN:
		case WM_INPUT:



				pGame->HandleMessage(uMsg, wParam, lParam);

			break;


	}

	DefWindowProc(hWnd, uMsg, wParam, lParam);

}



LRESULT MyDX3DGame::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{

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



				if (!m_IsMouseCaptured)
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
					if (m_IsMouseCaptured)
					{
						m_IsMouseCaptured = false;
						ShowCursor(TRUE); // Mostra o mouse
						ClipCursor(NULL); // Libera o mouse da janela
					}
				}
				return 0;

		case WM_INPUT:



				{
				if (!m_IsMouseCaptured) break;

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