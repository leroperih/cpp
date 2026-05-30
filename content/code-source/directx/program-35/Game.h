#pragma once


#include <windows.h>
#include <iostream>

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <wrl.h>

#include<thread>
#include<chrono>

// DirectX 3D

	#include <d3d11.h>
	#include <dxgi1_2.h>
	#include <dxgi1_3.h>

	#include <DirectXCollision.h>
	#include <d3dcompiler.h>
	#include <DirectXMath.h>

	#pragma comment(lib, "d3dcompiler.lib")
	#pragma comment(lib, "d3d11.lib")
	#pragma comment(lib, "dxgi.lib")

// DirectX 2D

	#include <d2d1_1.h>
	#include <dwrite.h>

	#pragma comment(lib, "d2d1.lib")
	#pragma comment(lib, "dwrite.lib")

// XAudio

	#include <xaudio2.h>

	#pragma comment(lib, "xaudio2.lib")



using Microsoft::WRL::ComPtr;

const float PI = 3.1415926535f;


// STRUCTURES

	struct SoundEffect {
		WAVEFORMATEX wfx = {};
		XAUDIO2_BUFFER buffer = {};
		std::vector<BYTE> audioData;
	};

	struct AABB
	{
		DirectX::XMFLOAT3 min;
		DirectX::XMFLOAT3 max;
	};

	struct OBB
	{
		DirectX::XMFLOAT3 center;
		DirectX::XMFLOAT3 extents; // Metade do tamanho (largura, altura, profundidade)
		DirectX::XMFLOAT4 orientation; // Quatérnio de rotação (X, Y, Z, W)
	};

	struct Triangle
	{
		DirectX::XMFLOAT3 v0, v1, v2;
	};

	struct VertexOBJ
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 tex;
		DirectX::XMFLOAT3 normal;
	};

	struct Vertex
	{
		float x, y, z;
		float r, g, b, a;
	};

	struct ConstantBuffer
	{
		DirectX::XMMATRIX mWorld;
		DirectX::XMMATRIX mView;
		DirectX::XMMATRIX mProjection;
	};

	struct Door
	{
		DirectX::XMFLOAT3 position;
		float currentRotation; // Rotação atual
		float targetRotation; // 0 para fechada, 90 para aberta
		bool isOpen = false;
		AABB collisionBox; // Para colisão e detecção de proximidade
	};

	struct MenuTitle
	{
		D2D1_RECT_F rect;
		const wchar_t* text;
	};

	struct Button
	{
		int id;
		float top;
		float left;
		float right;
		D2D1_RECT_F hitbox;
		const wchar_t* text;
	};


// COLLISION FUNCTIONS

	inline bool CheckOBBCollision(const OBB& a, const OBB& b)
	{
		using namespace DirectX;

		// CORRIGIDO: Passando os membros XMFLOAT3 e XMFLOAT4 diretamente por referência,
		// sem usar XMLoadFloat para evitar a conversão errada para XMVECTOR.
		BoundingOrientedBox boxA(a.center, a.extents, a.orientation);
		BoundingOrientedBox boxB(b.center, b.extents, b.orientation);

		return boxA.Intersects(boxB);
	}

	bool Intersects(const AABB& a, const AABB& b)
	{
		return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
			(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
			(a.min.z <= b.max.z && a.max.z >= b.min.z);
	}

	bool RayTriangleIntersect(DirectX::XMVECTOR rayOrigin, DirectX::XMVECTOR rayDir, Triangle tri, float& outDist)
	{
		DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&tri.v0);
		DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&tri.v1);
		DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&tri.v2);

		float dist = 0.0f;
		if (DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, dist)) {
			outDist = dist;
			return true;
		}
		return false;
	}

	bool RayAABBIntersect(DirectX::XMVECTOR rayOrigin, DirectX::XMVECTOR rayDir, const AABB& box, float& dist)
	{
		DirectX::XMVECTOR boxMin = DirectX::XMLoadFloat3(&box.min);
		DirectX::XMVECTOR boxMax = DirectX::XMLoadFloat3(&box.max);

		float tMin = 0.0f, tMax = FLT_MAX;

		// Teste para cada eixo (X, Y, Z)
		for (int i = 0; i < 3; ++i) {
			float dir = DirectX::XMVectorGetByIndex(rayDir, i);
			float ori = DirectX::XMVectorGetByIndex(rayOrigin, i);
			float bMin = DirectX::XMVectorGetByIndex(boxMin, i);
			float bMax = DirectX::XMVectorGetByIndex(boxMax, i);

			if (abs(dir) < 1e-6f) { // Raio paralelo ao plano
				if (ori < bMin || ori > bMax) return false;
			}
			else {
				float t1 = (bMin - ori) / dir;
				float t2 = (bMax - ori) / dir;
				if (t1 > t2) std::swap(t1, t2);
				tMin = (std::max)(tMin, t1);
				tMax = (std::min)(tMax, t2);
				if (tMin > tMax) return false;
			}
		}
		dist = tMin;
		return true;
	}

	// Função auxiliar para carregar arquivos WAV (PCM padrão)
	inline bool LoadWavFile(const std::string& filename, SoundEffect& out_sound) {
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open()) return false;

		char chunkId[4];
		file.read(chunkId, 4);
		if (strncmp(chunkId, "RIFF", 4) != 0) return false;

		file.seekg(8); // Pula o tamanho do arquivo
		file.read(chunkId, 4);
		if (strncmp(chunkId, "WAVE", 4) != 0) return false;

		while (file.good()) {
			file.read(chunkId, 4);
			UINT32 chunkSize;
			file.read(reinterpret_cast<char*>(&chunkSize), 4);

			if (strncmp(chunkId, "fmt ", 4) == 0) {
				file.read(reinterpret_cast<char*>(&out_sound.wfx), std::min<size_t>(chunkSize, sizeof(WAVEFORMATEX)));
				if (chunkSize > sizeof(WAVEFORMATEX)) file.seekg(chunkSize - sizeof(WAVEFORMATEX), std::ios::cur);
			}
			else if (strncmp(chunkId, "data", 4) == 0) {
				out_sound.audioData.resize(chunkSize);
				file.read(reinterpret_cast<char*>(out_sound.audioData.data()), chunkSize);

				out_sound.buffer.AudioBytes = chunkSize;
				out_sound.buffer.pAudioData = out_sound.audioData.data();
				out_sound.buffer.Flags = XAUDIO2_END_OF_STREAM;
				break;
			}
			else {
				file.seekg(chunkSize, std::ios::cur);
			}
		}
		return true;
	}

	bool IsHovered(D2D1_RECT_F rect, float mx, float my)
	{
		return (mx >= rect.left && mx <= rect.right && my >= rect.top && my <= rect.bottom);
	}


















class MyDX3DGame
{

public:


	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();
	bool InitializeDirect2D();
	bool InitializeAudio();


	void ShowMenu( std::vector<Button>& buttons, MenuTitle title);


	void PlaySoundFX(IXAudio2SourceVoice* pSourceVoice, const SoundEffect& sound);


	bool LoadOBJ(const char* objPath, const char* transformPath, std::vector<VertexOBJ>& out_vertices);
	bool LoadOBJ(const char* objPath, std::vector<VertexOBJ>& out_vertices);
	void CreateDoor(DirectX::XMFLOAT3 position, float currentRotation, float targetRotation, bool isOpen);


	void MoreInformation(int opt);
	void ButtonActions(int opt);


	void DrawDebugBoxes();
	void OnResize(UINT width, UINT height);


	void UpdateConstantBuffer(DirectX::XMMATRIX world);
	void Run();
	void Update();
	void Render();



	~MyDX3DGame();





private:



	// Initialization Data
	HINSTANCE m_hInstance = NULL;
	HWND m_hWnd = NULL;
	const wchar_t* m_WndClassName = L"A Window Class Name Very Cool!";
	const wchar_t* m_WndTitle = L"Lesro Games Productions Presents: A Simple DirectX Game!";
	int m_WndWidth = 1200;
	int m_WndHeight = 700;
	UINT m_ClientWidth = 0;
	UINT m_ClientHeight = 0;




	// DirectX

		// Direct3D
		ComPtr<ID3D11Device>                     m_pDevice = nullptr;
		ComPtr<ID3D11DeviceContext>              m_pDevCon = nullptr;
		ComPtr<IDXGISwapChain>                m_pSwapChain = nullptr;

		ComPtr<ID3D11RenderTargetView>   m_pBackBufferView = nullptr;
		ComPtr<ID3D11DepthStencilView> m_pDepthStencilView = nullptr;
		ComPtr<ID3D11Buffer>             m_pConstantBuffer = nullptr;
		ComPtr<ID3D11Buffer>               m_pVertexBuffer = nullptr;
		ComPtr<ID3D11Buffer>                m_pIndexBuffer = nullptr;

		ComPtr<ID3D11VertexShader>         m_pVertexShader = nullptr;
		ComPtr<ID3D11InputLayout>           m_pInputLayout = nullptr;
		ComPtr<ID3D11PixelShader>           m_pPixelShader = nullptr;

		ComPtr<ID3D11RasterizerState>         m_pContourRS = nullptr;
		ComPtr<ID3D11PixelShader>    m_pPixelShaderContour = nullptr;

		ComPtr<IDXGIFactory2>                   m_pFactory = nullptr;

		// Others
			D3D_FEATURE_LEVEL m_ActualLevel;

			DirectX::XMMATRIX m_ViewMatrix;
			DirectX::XMMATRIX m_ProjectionMatrix;


		// Direct2D
		ComPtr<ID2D1Factory1>               m_pD2DFactory;
		ComPtr<ID2D1DeviceContext>          m_pD2DContext;
		ComPtr<ID2D1SolidColorBrush>       m_pScreenBrush;
		ComPtr<IDWriteFactory>           m_pDWriteFactory;
		ComPtr<IDWriteTextFormat>   m_pTextFormat_elegant;
		ComPtr<IDWriteTextFormat>       m_pTextFormat_big;
		ComPtr<ID2D1Bitmap1>       m_pD2DBackBufferTarget;

		// Others
			ComPtr<ID3D11Buffer> m_pCrosshairBuffer;

		//DirectXAudio
		ComPtr<IXAudio2>             m_pXAudio2 = nullptr;
		IXAudio2MasteringVoice*  m_pMasterVoice = nullptr; // Controla o som geral do PC

		// Efeitos de áudio carregados
		SoundEffect m_sndJump;
		SoundEffect m_sndDoor;
		SoundEffect m_sndStep;

		// Vozes de Origem (Source Voices) dedicadas para tocar os sons
		IXAudio2SourceVoice* m_pSourceVoiceJump = nullptr;
		IXAudio2SourceVoice* m_pSourceVoiceDoor = nullptr;
		IXAudio2SourceVoice* m_pSourceVoiceStep = nullptr;

		// Others
			float m_StepTimer = 0.0f;


	// Middle Cross

	// DEBUG
	bool m_ShowDebug = false; // Começa desligado
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pWireframeRS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pDebugCubeBuffer;





	// Obstacles & Objects

		std::vector<Triangle>    m_FloorTriangles; // Chãos e rampas precisas
		std::vector<Triangle>  m_CeilingTriangles;
		std::vector<OBB>               m_WallOBBs;	

		UINT m_VertexCount = 0;
		std::vector<VertexOBJ> m_CollisionVertices;

		UINT m_DoorStartOffset = 0;  // Onde a porta começa no buffer
		UINT m_DoorVertexCount = 0;
		std::vector<Door> m_Doors;
		DirectX::XMFLOAT3 m_DoorBaseExtents; // Guardará o tamanho padrão da porta na origem



	// PLAYER DATA
	DirectX::XMFLOAT3 m_PlayerPos = { 20.0f, 2.0f, 0.0f }; // +|- (( RIGHT|LEFT ))  (( UP|DOWN ))  (( FRONT|BACK ))

	float m_ViewRadius = 60.0f;
	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	bool m_IsRunning = false;
	float m_MoveSpeed = 6.0f;
	float m_MouseSens = 0.005f;

	float m_VelocityY = 0.0f;     // Velocidade vertical atual
	bool  m_IsGrounded = true;    // O jogador está no chão?
	const float GRAVITY = -24.0f; // Força da gravidade
	const float JUMP_FORCE = 8.0f; // Força do pulo






	// GAMEPLAY DATA
	bool m_IsMouseCaptured = false;

	bool m_ComboPressed = false; // Para evitar que o comando fique "piscando"
	bool m_ShiftPressed = false;

	bool m_WWasPressed = false; // Rastreia o estado anterior do W
	float m_WReleaseTimer = 0.0f; // Tempo decorrido desde que o W foi solto
	bool m_WaitingForDoubleTap = false; // Indica se estamos na janela de tempo do segundo clique
	const float DOUBLE_TAP_TIMEOUT = 0.15f;



	// GAME STATE
	bool m_isMaximized = false;
	bool m_isPause_screen = false;
	bool m_isSettings_screen = false;



	// MENU DATA
	std::vector<Button> m_PauseButtons;
	std::vector<Button> m_SettingsButtons;

	D2D1_RECT_F m_menu_pause_Opt1Rect;
	D2D1_RECT_F m_menu_pause_Opt2Rect;
	D2D1_RECT_F m_menu_pause_Opt3Rect;

	D2D1_RECT_F m_menu_settings_Opt1Rect;
	D2D1_RECT_F m_menu_settings_Opt2Rect;
	D2D1_RECT_F m_menu_settings_Opt3Rect;

	int m_SelectedOption = 0; // 0 = Retornar, 1 = Sair
	const int m_pause_MaxOptions = 3;
	const int m_settings_MaxOptions = 3;


	// SENSI SLIDER
	D2D1_RECT_F m_slider_TrackRect;
	D2D1_RECT_F m_slider_ThumbRect;
	bool m_isDraggingSlider = false;

	float m_MinSens = 0.001f;
	float m_MaxSens = 0.020f;

	// OTHERS



	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);



};