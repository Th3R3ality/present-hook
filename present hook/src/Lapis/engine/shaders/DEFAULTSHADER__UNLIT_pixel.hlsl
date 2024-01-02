#include "DefaultStructs.hlsli"

float4 main(VOut input) : SV_TARGET
{
    return float4(input.uv.xyz, input.color.a);
    return input.color;
}