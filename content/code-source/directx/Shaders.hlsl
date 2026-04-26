cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD; // Mudamos de COLOR para TEXCOORD
    float3 normal : NORMAL; // Adicionamos NORMAL
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float3 normal : NORMAL;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    
    // Transformação de posição
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, World);
    pos = mul(pos, View);
    pos = mul(pos, Projection);
    
    output.pos = pos;
    output.tex = input.tex;
    
    // Transforma a normal para o espaço do mundo (importante para iluminação futura)
    output.normal = mul(input.normal, (float3x3) World);
    
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    // Por enquanto, vamos visualizar as Normais como cores 
    // Isso ajuda a debugar se o modelo foi carregado corretamente!
    return float4(input.normal * 0.5f + 0.5f, 1.0f);
}