cbuffer ConstantBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProjection;
    float4 mTexScale;  // X e Y = Escala
    float4 mTexRot;    // X = Cosseno, Y = Seno (Enviados do C++)
};

// 2. Recursos de Textura mapeados nos slots que ativamos no Render()
Texture2D shaderTexture : register(t0);
SamplerState samplerState : register(s0);

// 3. Estruturas de Dados de Entrada e Saída
struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float3 Normal : NORMAL;
};

// --- VERTEX SHADER ---
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    
    // Transforma a posição tridimensional local para o espaço de projeção da tela
    float4 worldPos = mul(float4(input.Pos, 1.0f), mWorld);
    float4 viewPos = mul(worldPos, mView);
    output.Pos = mul(viewPos, mProjection);
    
    // Passa as coordenadas UV adiante sem alterações
    output.Tex = input.Tex;
    
    // Transforma o vetor Normal para o espaço do mundo (útil para iluminação futura)
    output.Normal = mul(input.Normal, (float3x3) mWorld);
    
    return output;
}

// --- PIXEL SHADER ---
// --- PIXEL SHADER DENTRO DE SHADERS.HLSL ---
float4 PS(PS_INPUT input) : SV_TARGET
{
    // Coleta a textura exatamente como mapeada no Blender
    return shaderTexture.Sample(samplerState, input.Tex);
}