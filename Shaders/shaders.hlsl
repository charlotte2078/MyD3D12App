// Based on code by Microsoft and Frank Luna
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/shaders.hlsl
// Frank Luna DirectX 12 2ed pp. 232-240

struct ObjectConstants
{
    float4x4 gWorld;
};
ConstantBuffer<ObjectConstants> gObjConstants : register(b0);

struct PassConstants
{
    float4x4 gViewProj;
};
ConstantBuffer<PassConstants> gPassConstants : register(b1);

struct VSInput
{ 
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Simple Vertex shader
PSInput VSMain(VSInput input)
{
    PSInput result;
    
    float4 worldPos = mul(input.position, gObjConstants.gWorld);
    result.position = mul(worldPos, gPassConstants.gViewProj);
    
    result.color = input.color;
    
    return result;
}

// Simple pixel shader
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
