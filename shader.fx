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
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
int d;
int e;
int f;
};



//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
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
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
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
float4 PS( PS_INPUT input) : SV_Target
{
    float4 color = txDiffuse.Sample( samLinear, input.Tex );
	float depth = saturate(sqrt(input.Pos.z / input.Pos.w));

	if (e != 1) 
	{
		float4 fogColor = float4(0.3, 0.01, 0.0, 1.0);

		float fogFactor = depth;

		float4 colorf = (fogFactor * color + (1.00 - fogFactor) * fogColor);

		//if (depth > 0.02f)
		//	colorf = color;

	color.rgb = colorf.rgb;
	}


	if (f == 1)
	{
		color = pow(color, 0.5f);
	}

	if (d == 1)
	{
		color.r = 1;
	}
	return color;
}

