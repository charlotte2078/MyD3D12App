// Based on code by Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/shaders.hlsl

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Simple Vertex shader
PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    
    result.position = position;
    result.color = color;
    
    return result;
}

// Simple pixel shader
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
