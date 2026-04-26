
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

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = m_hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));





	// 1. CREATE DEVICE & CONTEXT
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &m_pDevice, &m_ActualLevel, &m_pDevCon);
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





	// 6.1 GEOMETRY BUFFERS (VISUAL)
	std::vector<VertexOBJ> visibleVertices;
	LoadOBJ("strange-map.obj", visibleVertices);
	m_VertexCount = (UINT)visibleVertices.size();

	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(VertexOBJ) * m_VertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vData = { visibleVertices.data() };
	m_pDevice->CreateBuffer(&vbd, &vData, &m_pVertexBuffer);




	// 6.2 GEOMETRY (COLLISION - INVISIBLE)
	std::vector<VertexOBJ> collisionVertices;
	if (LoadOBJ("strange-map.obj", collisionVertices))
	{
		for (size_t i = 0; i < collisionVertices.size(); i += 3)
		{
			// 1. Pegar os 3 vértices do triângulo como vetores matemáticos
			XMVECTOR v0 = XMLoadFloat3(&collisionVertices[i].pos);
			XMVECTOR v1 = XMLoadFloat3(&collisionVertices[i + 1].pos);
			XMVECTOR v2 = XMLoadFloat3(&collisionVertices[i + 2].pos);

			// 2. Calcular a Normal da face (Produto vetorial de dois lados)
			XMVECTOR edge1 = v1 - v0;
			XMVECTOR edge2 = v2 - v0;
			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(edge1, edge2));

			// 3. Verificar a inclinação através do componente Y da Normal
			float upComponent = XMVectorGetY(normal);

			if (upComponent > 0.6f) // Faces horizontais ou rampas leves
			{
				Triangle tri;
				// Armazena convertendo de XMVECTOR de volta para XMFLOAT3
				XMStoreFloat3(&tri.v0, v0);
				XMStoreFloat3(&tri.v1, v1);
				XMStoreFloat3(&tri.v2, v2);
				m_MapFloors.push_back(tri);
			}
			else // Se for muito inclinado (Paredes, Tetos), gera uma AABB
			{
				AABB box;
				// Extrai os valores min/max entre os 3 vértices
				box.min.x = (std::min)({ XMVectorGetX(v0), XMVectorGetX(v1), XMVectorGetX(v2) });
				box.min.y = (std::min)({ XMVectorGetY(v0), XMVectorGetY(v1), XMVectorGetY(v2) });
				box.min.z = (std::min)({ XMVectorGetZ(v0), XMVectorGetZ(v1), XMVectorGetZ(v2) });

				box.max.x = (std::max)({ XMVectorGetX(v0), XMVectorGetX(v1), XMVectorGetX(v2) });
				box.max.y = (std::max)({ XMVectorGetY(v0), XMVectorGetY(v1), XMVectorGetY(v2) });
				box.max.z = (std::max)({ XMVectorGetZ(v0), XMVectorGetZ(v1), XMVectorGetZ(v2) });

				m_MapWalls.push_back(box);
			}
		}
	}





	// 7. CONSTANT BUFFER
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = (sizeof(ConstantBuffer) + 15) & ~0xf;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_pDevice->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);





	ID3DBlob* vsBlob = nullptr, * psBlob = nullptr, * errorBlob = nullptr;

	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		return false;
	}
	m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVertexShader);

	// Layout de entrada (Garanta que a struct VertexOBJ no C++ seja idêntica a esta ordem)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 12 bytes de offset (pos)
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 20 bytes (pos+tex)
	};
	m_pDevice->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);

	hr = D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) { return false; }
	m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPixelShader);

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

	std::string line;
	while (std::getline(file, line))
	{

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
			while (ss >> vertexData) {

				// Substitui '/' por espaço para facilitar a leitura
				std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
				std::stringstream vss(vertexData);
				unsigned int vIdx = 0, tIdx = 0, nIdx = 0;

				vss >> vIdx;
				vertexIndices.push_back(vIdx - 1);

				// Só lê tIdx e nIdx se existirem
				if (vss >> tIdx) uvIndices.push_back(tIdx - 1);
				if (vss >> nIdx) normalIndices.push_back(nIdx - 1);
			}
		}

	}

	// Remontar os vértices
	out_vertices.reserve(vertexIndices.size());
	for (size_t i = 0; i < vertexIndices.size(); i++) {
		VertexOBJ v = {};
		if (!temp_vertices.empty()) v.pos = temp_vertices[vertexIndices[i]];
		if (!temp_uvs.empty())      v.tex = temp_uvs[uvIndices[i]];
		if (!temp_normals.empty())  v.normal = temp_normals[normalIndices[i]];
		out_vertices.push_back(v);
	}
	return true;
}



void MyDX3DGame::Update()
{

	// --- 1. DELTA TIME ---
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0) timeStart = timeCur;
	float deltaTime = (timeCur - timeStart) / 1000.0f;
	timeStart = timeCur;

	// --- CONFIGURAÇÕES DO PLAYER ---
	float charRadius = 0.4f;
	float charHeight = 1.74f;
	float eyeHeight = 1.68f;
	float stepHeight = 0.3f; // Permite subir degraus pequenos sem pular

	// --- 2. INPUT DE MOVIMENTAÇÃO ---
	XMMATRIX moveRotation = XMMatrixRotationRollPitchYaw(0, m_Yaw, 0);
	XMVECTOR moveForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), moveRotation);
	XMVECTOR moveRight = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), moveRotation);
	XMVECTOR velocityVec = XMVectorSet(0, 0, 0, 0);

	if (GetAsyncKeyState('W') & 0x8000) velocityVec += moveForward;
	if (GetAsyncKeyState('S') & 0x8000) velocityVec -= moveForward;
	if (GetAsyncKeyState('D') & 0x8000) velocityVec += moveRight;
	if (GetAsyncKeyState('A') & 0x8000) velocityVec -= moveRight;

	if (XMVectorGetX(XMVector3LengthSq(velocityVec)) > 0.001f) {
		velocityVec = XMVector3Normalize(velocityVec) * m_MoveSpeed * deltaTime;
	}
	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, velocityVec);

	// --- 3. MOVIMENTO E COLISÃO HORIZONTAL (Paredes) ---
	// Eixo X
	m_PlayerPos.x += velocity.x;
	AABB playerBoxX = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	for (const auto& wall : m_MapWalls) {
		if (Intersects(playerBoxX, wall)) { m_PlayerPos.x -= velocity.x; break; }
	}

	// Eixo Z
	m_PlayerPos.z += velocity.z;
	AABB playerBoxZ = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	for (const auto& wall : m_MapWalls) {
		if (Intersects(playerBoxZ, wall)) { m_PlayerPos.z -= velocity.z; break; }
	}

	// --- 4. FÍSICA VERTICAL E RAMPAS (Raycasting) ---
	float floorY = GetFloorHeight(m_PlayerPos); // Pega a altura real do seu modelo

	if (m_IsGrounded) {
		m_PlayerPos.y = floorY; // Gruda no chão (sobe rampa)
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

	// Checa se tocou no chão ao cair
	if (m_VelocityY <= 0 && m_PlayerPos.y <= floorY + 0.1f) {
		m_PlayerPos.y = floorY;
		m_VelocityY = 0;
		m_IsGrounded = true;
	}
	else if (m_PlayerPos.y > floorY + 0.2f) {
		m_IsGrounded = false; // Começa a cair se o chão sumir
	}


	// 8. CÂMERA (Calcula as matrizes que o Render() usará)
	XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
	XMVECTOR lookForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
	XMVECTOR Eye = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);

	m_ViewMatrix = XMMatrixLookAtLH(Eye, Eye + lookForward, XMVectorSet(0, 1, 0, 0));

	float aspectRatio = (m_WndHeight > 0) ? (float)m_WndWidth / m_WndHeight : 1.77f;
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f);
}



void MyDX3DGame::Render() {
	// 1. LIMPEZA E VINCULAÇÃO (Obrigatório em cada frame para FLIP_DISCARD)
	float bgColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());
	m_pDevCon->ClearRenderTargetView(m_pBackBufferView.Get(), bgColor);
	m_pDevCon->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 2. PIPELINE
	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDevCon->IASetInputLayout(m_pInputLayout.Get());

	UINT stride = sizeof(VertexOBJ);
	UINT offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	m_pDevCon->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDevCon->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDevCon->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// 3. MATRIZES (DirectXMath usa Row-Major, no HLSL mul(pos, Matrix) é o padrão)
	ConstantBuffer cb;
	cb.mView = XMMatrixTranspose(m_ViewMatrix);
	cb.mProjection = XMMatrixTranspose(m_ProjectionMatrix);

	// --- 4. DESENHAR O MAPA ---
	cb.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_pDevCon->Draw(m_VertexCount, 0);

	// --- 5. DESENHAR AS PORTAS ---
	// NOTA: Se as portas usam um modelo diferente, você precisaria de um 
	// m_pDevCon->IASetVertexBuffers diferente aqui apontando para os vértices do cubo.
	for (const auto& door : m_Doors) {
		XMMATRIX world = XMMatrixRotationY(XMConvertToRadians(door.currentRotation)) *
			XMMatrixTranslation(door.position.x, door.position.y, door.position.z);

		cb.mWorld = XMMatrixTranspose(world);
		m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

		// Se você não carregou vértices de cubo, mude '36' para algo que exista no buffer
		// ou o Draw falhará silenciosamente por falta de dados.
		if (m_VertexCount >= 36) m_pDevCon->Draw(36, 0);
	}

	m_pSwapChain->Present(1, 0);
}











float MyDX3DGame::GetFloorHeight(DirectX::XMFLOAT3 pos)
{
	float highestY = -100.0f;

	// 1. O raio começa acima da posição do jogador
	float rayOffset = 1.0f; // Aumentado para detectar rampas melhor
	DirectX::XMVECTOR rayOrigin = DirectX::XMVectorSet(pos.x, pos.y + rayOffset, pos.z, 0.0f);
	DirectX::XMVECTOR rayDir = DirectX::XMVectorSet(0, -1, 0, 0);

	// 2. O loop percorre os triângulos
	for (const auto& tri : m_MapFloors) {
		float dist = 0;

		// 3. Converte XMFLOAT3 (seguro) para XMVECTOR (matemático) DENTRO do loop
		DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&tri.v0);
		DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&tri.v1);
		DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&tri.v2);

		// 4. Teste de intersecção
		if (DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, dist)) {
			float hitY = (pos.y + rayOffset) - dist;
			if (hitY > highestY) highestY = hitY;
		}
	}
	return highestY;
}



void MyDX3DGame::CreateDoor(XMFLOAT3 position, float currentRotation, float targetRotation, bool isOpen)
{
	Door myDoor;
	myDoor.position = position;
	myDoor.currentRotation = currentRotation;
	myDoor.targetRotation = targetRotation;
	myDoor.isOpen = isOpen;

	// Define o tamanho da folha da porta (ex: 1m de largura, 2m de altura, 0.2m de espessura)
	float halfWidth = 0.5f;
	float height = 2.0f;
	float thickness = 0.1f;

	myDoor.collisionBox = {
		{ position.x - halfWidth, position.y         , position.z - thickness },
		{ position.x + halfWidth, position.y + height, position.z + thickness }
	};

	m_Doors.push_back(myDoor); // Salva na lista da classe!
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