#include "groundwork.h"


RenderTargetSwapChain::RenderTargetSwapChain(ID3D11Device* g_pd3dDevice, HWND g_hWnd)
{
	RTT = new RenderTextureClass;
	RTD = new RenderTextureClass;
	RTP = new RenderTextureClass;
	RTN = new RenderTextureClass;
	sRTT = new RenderTextureClass;
	sRTD = new RenderTextureClass;
	sRTP = new RenderTextureClass;
	sRTN = new RenderTextureClass;
	
	RTT->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);

	RTD->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);

	RTP->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);

	RTN->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);

	sRTT->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);

	sRTD->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);

	sRTP->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);
	
	sRTN->Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);

	swapped = false;
}

void RenderTargetSwapChain::getCurrentTargets(RenderTextureClass **out)
{
	if (!swapped)
	{
		out[TEXTURE] = RTT;
		out[DEPTH] = RTD;
		out[POSITION] = RTP;
		out[NORMAL] = RTN;
	}
	else 
	{
		out[TEXTURE] = sRTT;
		out[DEPTH] = sRTD;
		out[POSITION] = sRTP;
		out[NORMAL] = sRTN;
	}
}

void RenderTargetSwapChain::getCurrentSRVs(RenderTextureClass **out)
{
	if (!swapped)
	{
		out[TEXTURE] = sRTT;
		out[DEPTH] = sRTD;
		out[POSITION] = sRTP;
		out[NORMAL] = sRTN;
	}
	else
	{
		out[TEXTURE] = RTT;
		out[DEPTH] = RTD;
		out[POSITION] = RTP;
		out[NORMAL] = RTN;
	}


}


void RenderTargetSwapChain::Present()
{
	swapped = !swapped;
}
