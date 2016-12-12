//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D Tx0 : register(t0);
Texture2D Tx1 : register(t1);
Texture2D Tx2 : register(t2);
Texture2D Tx3 : register(t3);
Texture2D Tx4 : register(t4);
Texture2D Tx5 : register(t5);
Texture2D Tx6 : register(t6);
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
float4 w_pos;
float4 data;
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
	float4 WorldPos : TEXCOORD1;
	float4 ScreenPos : TEXCOORD2;
	float4 ViewPos : TEXCOORD3;
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
	float4 WorldPos : TEXCOORD1;
	float4 ViewPos : TEXCOORD2;
};


struct PS_DDOutput
{

	float4 Col	: SV_Target0;
	float4 Dep	: SV_Target1;
	float4 Pos	: SV_Target2;
	float4 Norm	: SV_Target3;

};



#define DEPTH 512

//DEPTH

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;
	float4 worldpos = pos = mul(pos, World);
	output.ViewPos = output.Pos = mul(pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm = normalize(mul(input.Norm, World));
	output.WorldPos = pos;



	float3 sp = worldpos;
	sp.y = 0;


	float3 cam = w_pos;
	cam.y = 0;
	float3 p_cam_dir = normalize((sp - cam));
	float visible = dot(p_cam_dir, normalize(-input.Norm));
	float pseudo_diameter = saturate(sin(visible * 1.570796)) * 1.612847442; //!!!!! at the moment!
	float4 opposide_pos = worldpos;// +float4(p_cam_dir.xyz*pseudo_diameter, 1);
	opposide_pos = mul(opposide_pos, View);
	opposide_pos = mul(opposide_pos, Projection);
	output.ScreenPos = opposide_pos;

    return output;
}
PS_INPUT_L VSlevel(VS_INPUT_L input)
{
	PS_INPUT_L output = (PS_INPUT_L)0;
	float4 pos = input.Pos;
	output.WorldPos = pos = mul(pos, World);
	output.ViewPos = output.Pos = mul(pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Tex = input.Tex;
	
	return output;
}//ok, so what?

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

PS_DDOutput PSlevel(PS_INPUT_L input) : SV_Target
{
	float4 color = Tx2.Sample(samLinear, input.Tex.xy);
	float depth = input.Pos.z / input.Pos.w;


	float fog_d = saturate(sqrt(depth));

	float4 fogColor = float4(0.3, 0.01, 0.0, 1.0);

	float fogFactor = fog_d;

	float4 colorf = (pow(fogFactor, 1) * color + saturate(1.00 - fogFactor/6) * fogColor);

	//if (depth > 0.02f)
	//	colorf = color;

	color.rgb = colorf.rgb;
	PS_DDOutput outp;

	outp.Col = color;
	outp.Dep = depth;
	outp.Pos = input.ViewPos;
	outp.Dep.a = 1;

	/*
	outp.Col.r = input.WorldPos.x * 0.1;
	outp.Col.g = input.WorldPos.y * 0.1;
	outp.Col.b = input.WorldPos.z * 0.01;
	outp.Col.a = 1;
	*/

	return outp;
	//return color; //float4(depth, depth, depth, 1);
}
PS_DDOutput PS( PS_INPUT input) : SV_Target
{
    float4 color = Tx0.Sample( samLinear, input.Tex );
	float depth = input.Pos.z / input.Pos.w;


	float fog_d = saturate(sqrt(depth));

		float4 fogColor = float4(0.3, 0.01, 0.0, 1.0);

		float fogFactor = fog_d;

		float4 colorf = (pow(fogFactor, 2.5) * color + (1.00 - fogFactor) * fogColor);

		//if (depth > 0.02f)
		//	colorf = color;

	color.rgb = colorf.rgb;
	
	PS_DDOutput outp;

	outp.Col = color;
	outp.Dep = depth;
	outp.Pos = input.ViewPos;
	outp.Dep.a = 1;



	return outp;



	//return color; //float4(depth, depth, depth, 1);;
}

PS_DDOutput PS_SPHERE(PS_INPUT input) : SV_Target
{
	PS_DDOutput outp;
	float4 color = Tx0.Sample(samLinear, input.Tex);
	float pix_depth = input.Pos.z / input.Pos.w;
	color.a = 1;
	outp.Col = color;
	outp.Col.b = pow(outp.Col.b, 2);

	matrix ViewRot = View;
	ViewRot._41 = ViewRot._42 = ViewRot._43 = 0.0;

	outp.Norm.xyz = mul(input.Norm.xyz, ViewRot);
	//outp.Norm.xyz = input.Norm.xyz;
	outp.Norm.a = 1;
	outp.Dep.xyz = pix_depth;
	outp.Dep.a = 1;
	outp.Pos = input.ViewPos;
	return outp;	

}


PS_DDOutput PS_SPHERE_DEPTH(PS_INPUT input) : SV_Target
{
	float pix_depth = input.Pos.z / input.Pos.w;
	pix_depth = pow(pix_depth, DEPTH);

	pix_depth = 1 - pix_depth; // Inversion to match pixel depth

	PS_DDOutput outp;

	outp.Col = float4(pix_depth, pix_depth, pix_depth, 1);
	outp.Dep.rgb = pix_depth;
	outp.Dep.a = 1;
	return outp;



	//return float4(pix_depth, pix_depth, pix_depth, 1.);
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
	
	float4 result= Tx0.Sample(samLinear, input.Tex);
	
	
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


float4 PS_screen_AO(PS_INPUT input) : SV_Target
{

	float4 result = Tx0.Sample(samLinear, input.Tex);
	float4 resultd = Tx1.Sample(samLinear, input.Tex);


	resultd.a = 1;

	float depth = resultd.r;

	depth = pow(depth, 2048);

	depth = 1 - depth;

	if (depth > 0.00001)
	{
	result.rgb *= depth;
	}

	result.a = 1;

	return result;


}



float4 PS_screen_depth(PS_INPUT input) : SV_Target
{

	float4 result = Tx0.Sample(samLinear, input.Tex);


	result.a = 1;
	return result;

	float depth = result.r;


	//depth = pow(depth, DEPTH);

	//depth = 1 - depth; // Inversion to match pixel depth

	result.rgb = depth; 

	return result;


}

//???
PS_DDOutput PS_MERGE(PS_INPUT input) : SV_Target
{
	float4 sphere_color = Tx0.Sample(samLinear,	input.Tex);
	float4 sphere_depth = Tx1.Sample(samLinear, input.Tex);
	float4 sphere_position = Tx2.Sample(samLinear, input.Tex);
	float4 scene_color = Tx3.Sample(samLinear, input.Tex);
	float4 scene_depth = Tx4.Sample(samLinear, input.Tex);
	float4 scene_position = Tx5.Sample(samLinear, input.Tex);
	float4 sphere_normal = Tx6.Sample(samLinear, input.Tex);

	PS_DDOutput outp;




	float4 viewpos = input.ViewPos;
	float3 cam = w_pos;
	cam.y = 0;
	sphere_position.y = 0;
	float3 p_cam_dir = normalize(sphere_position.xyz);
	sphere_normal.w = 1;
	sphere_normal = normalize(sphere_normal);
	float visible = dot(p_cam_dir, -sphere_normal.xyz);
	float pseudo_diameter = saturate(sin(visible * 1.570796)) * 1.612847442; //!!!!! at the moment!

	float4 opposide_pos = sphere_position - float4(0, 0, pseudo_diameter, 1);

	float4 outputcolor = float4(0, 0, 0, 1);
	outputcolor = scene_color;
	//here should the mergin be
	outp.Pos = scene_position;
	outp.Dep.xyz = scene_depth.xyz;
	outp.Dep.a = 1;
	outp.Pos.a = 1;
	//outputcolor = scene_position.z;

	float sphere_on = 0;
	float sum = sphere_color.r + sphere_color.g + sphere_color.b;

	if (sphere_depth.z < scene_depth.r)
	{
		if (opposide_pos.z < scene_position.z)
		sphere_on = 1;
	}
	
	if (sphere_on && sum>0)
	{
		outputcolor = sphere_color;
		outp.Dep.xyz =sphere_depth;
		outp.Pos =sphere_position;
	}

	//all of this position must be world-coords!
	//the mergin is HERE
	outp.Col = outputcolor;
	outp.Col.a = 1;
	return outp;
}