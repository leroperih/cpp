
float4 VS_UI(float3 pos : POSITION) : SV_POSITION
{
    return float4(pos, 1.0f);
}

float4 PS_UI() : SV_Target
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f); // Verde
}