#pragma once


#include <windows.h>
#include <iostream>


#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <wrl.h>


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






using Microsoft::WRL::ComPtr;





const float PI = 3.1415926535f;




// STRUCTURES

	struct AABB
	{
		DirectX::XMFLOAT3 min;
		DirectX::XMFLOAT3 max;
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
		float currentRotation;     // Rotação atual
		float targetRotation; // 0 para fechada, 90 para aberta
		bool isOpen = false;
		AABB collisionBox;            // Para colisão e detecção de proximidade
	};


// COLLISION FUNCTIONS

	bool CheckAABBCollision(const AABB& a, const AABB& b)
	{
		return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
			(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
			(a.min.z <= b.max.z && a.max.z >= b.min.z);
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
















class MyDX3DGame
{

public:


	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();


	bool LoadOBJ(const char* path, std::vector<VertexOBJ>& out_vertices);
	void CreateDoor(DirectX::XMFLOAT3 position, float currentRotation, float targetRotation, bool isOpen);

	void DrawDebugBoxes();
	void OnResize(UINT width, UINT height);


	void UpdateConstantBuffer(DirectX::XMMATRIX world);
	void Run();
	void Update();
	void Render();



	~MyDX3DGame();





private:


	HINSTANCE m_hInstance = NULL;
	HWND m_hWnd = NULL;



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

	D3D_FEATURE_LEVEL m_ActualLevel;
	ComPtr<IDXGIFactory2>                   m_pFactory = nullptr;



	ComPtr<ID3D11Buffer> m_pCrosshairBuffer;



	bool m_ShowDebug = false; // Começa desligado
	bool m_ComboPressed = false; // Para evitar que o comando fique "piscando"
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pWireframeRS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pDebugCubeBuffer;



	std::vector<AABB> m_MapObstacles;
	std::vector<Triangle> m_FloorTriangles;
	std::vector<AABB> m_FloorObstacles;
	std::vector<AABB> m_WallObstacles;
	std::vector<AABB> m_CeilingObstacles;



	UINT m_VertexCount = 0;
	std::vector<VertexOBJ> m_CollisionVertices;


	// ... outras variáveis ...
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;

	UINT m_DoorStartOffset = 0;  // Onde a porta começa no buffer
	UINT m_DoorVertexCount = 0;
	std::vector<Door> m_Doors;



	DirectX::XMFLOAT3 m_PlayerPos = { 2.0f, 0.0f, 0.0f };

	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	float m_MoveSpeed = 6.0f;
	float m_MouseSens = 0.005f;


	float m_VelocityY = 0.0f;     // Velocidade vertical atual
	bool  m_IsGrounded = true;    // O jogador está no chão?
	const float GRAVITY = -24.0f; // Força da gravidade
	const float JUMP_FORCE = 8.0f; // Força do pulo



	const wchar_t* m_WndClassName = L"A Window Class Name Very Cool!";
	const wchar_t* m_WndTitle = L"Lesro Games Productions Presents: Another Blank Screen!";
	int m_WndWidth = 1200;
	int m_WndHeight = 700;
	UINT m_ClientWidth = 0;
	UINT m_ClientHeight = 0;




	// Others
	bool m_IsMouseCaptured = false;



	// GAME STATE
	bool m_isPaused;



	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);





};