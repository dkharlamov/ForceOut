//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txDepth : register(t1);
Texture2D tx : register(t2);
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
float3 w_pos;
};



//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 WorldPos : TEXCOORD1;
	float4 Norm : Normal0;
};

struct VS_INPUT_L
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT_L
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};



//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	pos = mul(pos, World);
	output.Pos = mul(pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm = normalize(mul(input.Norm, World));
	output.WorldPos = pos;
    
    return output;
}
PS_INPUT_L VSlevel(VS_INPUT_L input)
{
	VS_INPUT_L output = (PS_INPUT_L)0;
	output.Pos = mul(input.Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;

	return output;
}

PS_INPUT VSHUD(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, World);
	//output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;

	return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PSlevel(PS_INPUT_L input) : SV_Target
{
	float4 color = tx.Sample(samLinear, input.Tex.xy);
	float depth = saturate(input.Pos.z / input.Pos.w);
	float4 fogColor = float4(0.3, 0.01, 0.0, 1.0);

	float fogFactor = depth * 3;

	float4 colorf = (pow(fogFactor, 1) * color + saturate(1.00 - fogFactor/6) * fogColor);

	//if (depth > 0.02f)
	//	colorf = color;

	color.rgb = colorf.rgb;
	return color; //float4(depth, depth, depth, 1);
}
float4 PS( PS_INPUT input) : SV_Target
{
    float4 color = txDiffuse.Sample( samLinear, input.Tex );
	float depth = saturate(sqrt(input.Pos.z / input.Pos.w));


		float4 fogColor = float4(0.3, 0.01, 0.0, 1.0);

		float fogFactor = depth;

		float4 colorf = (pow(fogFactor, 2.5) * color + (1.00 - fogFactor) * fogColor);

		//if (depth > 0.02f)
		//	colorf = color;

	color.rgb = colorf.rgb;


	return color; //float4(depth, depth, depth, 1);;
}

float4 PS_SPHERE(PS_INPUT input) : SV_Target
{
	float4 color = txDiffuse.Sample(samLinear, input.Tex);
	float4 result = txDepth.Sample(samLinear, float2(input.Pos.x/1280, input.Pos.y/720));


	result.a = 1;

	float depth = result.r;

	depth = pow(depth, 512);

	depth = 1 - depth;

	float pix_depth = saturate(sqrt(input.Pos.z / input.Pos.w));

	float3 sp = input.WorldPos;
	sp.y = 0;

	float3 cam = w_pos;
	cam.y = 0;
	

	float3 r = normalize((sp - cam));

	//closer -> 1

	if (pix_depth > depth)
		color.a = 0;
	//color.r = depth;
	//color.g = pix_depth;
	//color.b = 0;


	return color;
}


float4 PS_SPHERE_DEPTH(PS_INPUT input) : SV_Target
{
	float pix_depth = saturate(sqrt(input.Pos.z / input.Pos.w));

	return float4(pix_depth, pix_depth, pix_depth, 1.);
}


PS_INPUT VS_screen(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	output.Pos = pos;
	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm.xyz = input.Norm;
	output.Norm.w = 1;




	return output;
}

float4 PS_screen(PS_INPUT input) : SV_Target
{
	
	float4 result= txDiffuse.Sample(samLinear, input.Tex);
	
	
	result.a = 1;

	/*
	float depth = (result.r * 2.0f) - 1.0f;

	depth = pow(depth, 128);

	result.rgb = 0;
	if (depth < 0.25)
		result.g = depth;
	if (depth < 0.5)
		result.r = depth;
	if (depth > 0.5)
		result.b = depth;
	
	*/


	return result;
	

}


float4 PS_screen_depth(PS_INPUT input) : SV_Target
{

	float4 result = txDiffuse.Sample(samLinear, input.Tex);


	result.a = 1;

	float depth = result.r;


	depth = pow(depth, 512);

	depth = 1 - depth; // Inversion to match pixel depth

	result.rgb = depth; 

	return result;


}