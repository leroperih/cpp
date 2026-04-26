
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



bool MyDX3DGame::InitializeDirectX() {
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

	// 6. GEOMETRY BUFFERS (VISUAL)
	std::vector<VertexOBJ> visibleVertices;
	LoadOBJ("mapa_visual.obj", visibleVertices);
	m_VertexCount = (UINT)visibleVertices.size();

	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(VertexOBJ) * m_VertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vData = { visibleVertices.data() };
	m_pDevice->CreateBuffer(&vbd, &vData, &m_pVertexBuffer);

	// 6.1 GEOMETRY (COLLISION - INVISIBLE)
	std::vector<VertexOBJ> collisionVertices; // Criamos o vetor local para processar
	LoadOBJ("mapa_visual.obj", collisionVertices);

	for (size_t i = 0; i < collisionVertices.size(); i += 3) {
		AABB box;
		auto& v1 = collisionVertices[i].pos;
		auto& v2 = collisionVertices[i + 1].pos;
		auto& v3 = collisionVertices[i + 2].pos;

		box.min.x = (std::min)({ v1.x, v2.x, v3.x });
		box.min.y = (std::min)({ v1.y, v2.y, v3.y });
		box.min.z = (std::min)({ v1.z, v2.z, v3.z }); // Corrigido de x para z

		box.max.x = (std::max)({ v1.x, v2.x, v3.x });
		box.max.y = (std::max)({ v1.y, v2.y, v3.y });
		box.max.z = (std::max)({ v1.z, v2.z, v3.z }); // Corrigido de x para z

		m_MapObstacles.push_back(box);
	}

	// 7. CONSTANT BUFFER
	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = (sizeof(ConstantBuffer) + 15) & ~0xf;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_pDevice->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);

	// 8. SHADERS
	ID3DBlob* vsBlob, * psBlob, * errorBlob;
	D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVertexShader);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_pDevice->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);

	D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errorBlob);
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






void MyDX3DGame::Update() {
	// 1. DELTA TIME
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0) timeStart = timeCur;
	float deltaTime = (timeCur - timeStart) / 1000.0f;
	timeStart = timeCur;

	// --- CONFIGURAÇÃO ---
	float charRadius = 0.4f;
	float charHeight = 1.8f;
	float eyeHeight = 1.7f;
	float stepHeight = 0.3f; // NOVO: Altura que o jogador ignora na colisão lateral (pé)
	float floorLevel = 0.0f;

	// 2. CALCULA VELOCIDADE DESEJADA
	XMMATRIX moveRotation = XMMatrixRotationRollPitchYaw(0, m_Yaw, 0);
	XMVECTOR moveForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), moveRotation);
	XMVECTOR moveRight = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), moveRotation);
	XMVECTOR velocityVec = XMVectorSet(0, 0, 0, 0);

	if (GetAsyncKeyState('W') & 0x8000) velocityVec += moveForward;
	if (GetAsyncKeyState('S') & 0x8000) velocityVec -= moveForward;
	if (GetAsyncKeyState('D') & 0x8000) velocityVec += moveRight;
	if (GetAsyncKeyState('A') & 0x8000) velocityVec -= moveRight;

	if (XMVectorGetX(XMVector3LengthSq(velocityVec)) > 0.001f)
		velocityVec = XMVector3Normalize(velocityVec) * m_MoveSpeed * deltaTime;

	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, velocityVec);

	// 3. MOVIMENTO E COLISÃO EIXO X (Ignora o que estiver abaixo do pé)
	m_PlayerPos.x += velocity.x;
	AABB playerBoxX = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	for (const auto& obs : m_MapObstacles) {
		if (Intersects(playerBoxX, obs)) {
			m_PlayerPos.x -= velocity.x;
			break;
		}
	}

	// 4. MOVIMENTO E COLISÃO EIXO Z (Ignora o que estiver abaixo do pé)
	m_PlayerPos.z += velocity.z;
	AABB playerBoxZ = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y + stepHeight, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	for (const auto& obs : m_MapObstacles) {
		if (Intersects(playerBoxZ, obs)) {
			m_PlayerPos.z -= velocity.z;
			break;
		}
	}

	// 5. FÍSICA VERTICAL (Pulo e Gravidade)
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

	// 6. COLISÃO VERTICAL (Chão e Topo de Objetos)
	m_IsGrounded = false;
	if (m_PlayerPos.y <= floorLevel) {
		m_PlayerPos.y = floorLevel;
		m_VelocityY = 0.0f;
		m_IsGrounded = true;
	}

	// Checa colisão com o topo dos cubos do mapa
	AABB playerBoxY = {
		{m_PlayerPos.x - charRadius, m_PlayerPos.y, m_PlayerPos.z - charRadius},
		{m_PlayerPos.x + charRadius, m_PlayerPos.y + charHeight, m_PlayerPos.z + charRadius}
	};
	for (const auto& obs : m_MapObstacles) {
		if (Intersects(playerBoxY, obs)) {
			if (m_VelocityY < 0) { // Caindo em cima de algo
				m_PlayerPos.y = obs.max.y;
				m_IsGrounded = true;
				m_VelocityY = 0;
			}
			else if (m_VelocityY > 0) { // Batendo a cabeça
				m_PlayerPos.y = obs.min.y - charHeight;
				m_VelocityY = 0;
			}
			break;
		}
	}

	// 7. CÂMERA E MATRIZES
	XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
	XMVECTOR lookForward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRotation);
	XMVECTOR Eye = XMLoadFloat3(&m_PlayerPos) + XMVectorSet(0, eyeHeight, 0, 0);
	XMMATRIX mView = XMMatrixLookAtLH(Eye, Eye + lookForward, XMVectorSet(0, 1, 0, 0));

	float aspect = (m_WndHeight > 0) ? (float)m_WndWidth / m_WndHeight : 1.0f;
	XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.01f, 1000.0f);

	// 8. ATUALIZA CONSTANT BUFFER
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	cb.mView = XMMatrixTranspose(mView);
	cb.mProjection = XMMatrixTranspose(mProjection);
	m_pDevCon->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
}







void MyDX3DGame::Render()
{
	float bgColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	m_pDevCon->ClearRenderTargetView(m_pBackBufferView.Get(), bgColor);
	m_pDevCon->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_pDevCon->OMSetRenderTargets(1, m_pBackBufferView.GetAddressOf(), m_pDepthStencilView.Get());

	m_pDevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDevCon->IASetInputLayout(m_pInputLayout.Get());

	UINT stride = sizeof(VertexOBJ); // Agora coincide com o buffer criado
	UINT offset = 0;
	m_pDevCon->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	m_pDevCon->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDevCon->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDevCon->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// CORREÇÃO: Usar Draw em vez de DrawIndexed para modelos carregados via LoadOBJ
	m_pDevCon->Draw(m_VertexCount, 0);

	m_pSwapChain->Present(1, 0);
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