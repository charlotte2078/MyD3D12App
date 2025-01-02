// Based on code by Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/shaders.hlsl

struct PSInput
{
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

// Simple Vertex shader
PSInput VSMain(float4 position : POSITIONT, float4 colour : COLOUR)
{
    PSInput result;
    
    result.position = position;
    result.colour = colour;
    
    return result;
}

// Simple pixel shader
float PSMain(PSInput input) : SV_TARGET
{
    return input.colour;
}
