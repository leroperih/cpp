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
    float4 pos = float4(input.pos, 1.0f);

    // Se você usou XMMatrixTranspose no C++, a ordem correta aqui é:
    pos = mul(World, pos);
    pos = mul(View, pos);
    pos = mul(Projection, pos);

    output.pos = pos;
    output.tex = input.tex;

    // O mesmo para a Normal
    output.normal = mul((float3x3) World, input.normal);
    
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    // Por enquanto, vamos visualizar as Normais como cores 
    // Isso ajuda a debugar se o modelo foi carregado corretamente!
    return float4(input.normal * 0.5f + 0.5f, 1.0f);
}