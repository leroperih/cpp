#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>


#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

const float PI = 3.1415926535f;


using Microsoft::WRL::ComPtr;



struct AABB {
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};


struct VertexOBJ {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 tex;
	DirectX::XMFLOAT3 normal;
};

struct Vertex {

	float x, y, z;
	float r, g, b, a;

};


struct ConstantBuffer {

	DirectX::XMMATRIX mWorld;
	DirectX::XMMATRIX mView;
	DirectX::XMMATRIX mProjection;

};


struct Door {
	DirectX::XMFLOAT3 position;
	float currentRotation;     // Rotação atual
	float targetRotation; // 0 para fechada, 90 para aberta
	bool isOpen = false;
	AABB collisionBox;            // Para colisão e detecção de proximidade
};


bool CheckAABBCollision(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool Intersects(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}
















class MyDX3DGame
{

public:


	bool Initialize(HINSTANCE hInstance, int nCmdShow);
	bool InitializeDirectX();
	bool LoadOBJ(const char* path, std::vector<VertexOBJ>& out_vertices);


	void CreateDoor(DirectX::XMFLOAT3 position, float currentRotation, float targetRotation, bool isOpen);
	void Init();
	void Run();
	void OnResize(UINT width, UINT height);
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



	std::vector<AABB> m_MapObstacles;



	UINT m_VertexCount = 0;
	std::vector<VertexOBJ> m_CollisionVertices;


	// ... outras variáveis ...
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;
	std::vector<Door> m_Doors;



	DirectX::XMFLOAT3 m_PlayerPos = { 0.0f, 2.0f, -10.0f };

	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	float m_MoveSpeed = 5.0f;
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