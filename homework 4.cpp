//--------------------------------------------------------------------------------------
// File: lecture 8.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "groundwork.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

/*
Daniel Kharlamov
CST320 HW 5 P2

5 Enemies with collision detection and death on collide
Skybox
Mouse horizontal view and wasd controls with strafe
Shift enabled sprinting
Night level
Bullets that kill
Wall collision
Gun model
1280x720
*/



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11VertexShader*                 g_pVertexShaderHUD = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sky = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sphere = NULL;
ID3D11Buffer*                       g_pVertexBuffer_bomb = NULL;
ID3D11Buffer*                       g_pVertexBuffer_law = NULL;
int									model_vertex_anz = 0; 
int									bomb_vertex_anz = 0;
int									law_vertex_anz = 0;

explosion_handler					explosionhandler;

//states for turning off and on the depth buffer
ID3D11DepthStencilState				*ds_on, *ds_off;

ID3D11BlendState					*g_pBlendState = NULL;

ID3D11Buffer*                       g_pCBuffer = NULL;

ID3D11ShaderResourceView*           g_pTextureRV = NULL;
ID3D11ShaderResourceView*			g_pEnemyTex = NULL;
ID3D11ShaderResourceView*			g_pDeathTex = NULL;
ID3D11ShaderResourceView*			g_pGunTex = NULL;
ID3D11ShaderResourceView*			g_pBulletTex = NULL;
ID3D11ShaderResourceView*			g_pBombTex = NULL;
ID3D11ShaderResourceView*			g_pLawTex = NULL;

ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );
CXBOXController *gamepad = new CXBOXController(1);

camera								cam;
level								level1;
//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1280, 720 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


vector<billboard> enemies;
vector<bullet> bullets;
//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;
	
	billboard enemy1;
	enemy1.position.x = enemy1.position.z = 10;
	enemies.push_back(enemy1);
	billboard enemy2;
	enemy2.position.x = -10;
	enemy2.position.z = 12;
	enemies.push_back(enemy2);
	billboard enemy3;
	enemy3.position.z = 20;
	enemies.push_back(enemy3);
	billboard enemy4;
	enemy4.position.z = 20;
	enemy4.position.x = -10;
	enemies.push_back(enemy4);
	billboard enemy5;
	enemy5.position.z = 30;
	enemy5.position.x = 7;
	enemy5.transparency = 0.5f;
	enemies.push_back(enemy5);

	for (int i = 0; i < 6; i++)
	{
		bullet bull;
		bull.projectile.position = XMFLOAT3(0,0,100);
		bull.impulse = XMFLOAT3(0, 0, 0);
		bullets.push_back(bull);
	}

	cam.position.z = -12;
	cam.position.x = -1;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
		return hr;

	
    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;


	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_pBlendState);

	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_pBlendState, blendFactor, sampleMask);
	  

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    }
	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VSHUD", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShaderHUD);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}


	
    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;


	//create skybox vertex buffer
	SimpleVertex vertices_skybox[] =
		{
		//top
		//00x 0.25 00y 0.0 10x .5 10y .33
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.0f) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.5f, 0.0f)  },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33333333333333333333333f)  },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.0f) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33333333333333333333333f) },
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.25f, 0.33333333333333333333333f)  },
		//bottom
		//00x 0.25 00y 0.66 10x .5 10y 1
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.5f, 0.66666666666f) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.25f, 0.666666666f)  },				
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 1.0f) },				
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 1.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.25f, 0.6666666666f) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 1.0f)},
		//left
		//00x 0.0 00y .33 10x .25 10y .66
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.66f)  },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.66f) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.33333333333333333333333f) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.66f) },				
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.33333333333333333333333f) },
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.25f, 0.33333333333333333333333f)  },
		//right
		//00x .5 00y .33 10x .75 10y .66
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.75f, 0.66f) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 0.66f) },				
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.75f, 1.0f / 3.0f) },				
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.75f, 1.0f/3.0f) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 0.66f) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 1.0f/3.0f)  },
		//back
		//00x .75 00y .33 10x 1.0 10y .66
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.66f)  },
				{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.75f, 0.66f)  },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.75f, 1.0f/3.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.66f) },
				{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.75f, 1.0f/3.0f) },
				{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f/3.0f) },
		//front
		//00x .25 00y .33 10x .5 10y .66
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.66f) },				
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33f) },
				{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 0.66f) },
				{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33f) },
				{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.66f) },				
				{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.25f, 0.33f)  },
		};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices_skybox;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_sky);
	if (FAILED(hr))
		return hr;
    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.0f, -1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f ), XMFLOAT2( 0.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 0.0f ) },

		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },		
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },		
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

    };


    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 12;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );


	//Load3DS("sphere.3ds", g_pd3dDevice, &g_pVertexBuffer_sphere, &model_vertex_anz);
	LoadCatmullClark(L"ccsphere.cmp", g_pd3dDevice, &g_pVertexBuffer_sphere, &model_vertex_anz);
	LoadOBJ("Bomb.obj", g_pd3dDevice, &g_pVertexBuffer_bomb, &bomb_vertex_anz);
	Load3DS("RPG.3DS", g_pd3dDevice, &g_pVertexBuffer_law, &law_vertex_anz);


    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBuffer);
    if( FAILED( hr ) )
        return hr;
    

    // Load the Texture
    hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, L"v.jpg", NULL, NULL, &g_pTextureRV, NULL );
    if( FAILED( hr ) )
        return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"imp.png", NULL, NULL, &g_pEnemyTex, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"ded.png", NULL, NULL, &g_pDeathTex, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"mgun.png", NULL, NULL, &g_pGunTex, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"bullet.png", NULL, NULL, &g_pBulletTex, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"Bomb_D.png", NULL, NULL, &g_pBombTex, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"Tex_0006_1.jpg", NULL, NULL, &g_pLawTex, NULL);
	if (FAILED(hr))
		return hr;

	explosionhandler.init(g_pd3dDevice, g_pImmediateContext);
	explosionhandler.init_types(L"exp1.dds", 8, 8, 500000);

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );//camera position
    XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );//look at
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );// normal vector on at vector (always up)
    g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);
	

	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose( g_View );
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
    g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0 );


	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	level1.init("testLevel.bmp");
	//level1.init_texture(g_pd3dDevice, L"wall1.jpg");
	//level1.init_texture(g_pd3dDevice, L"wall2.jpg");
	//level1.init_texture(g_pd3dDevice, L"floor.jpg");
	//level1.init_texture(g_pd3dDevice, L"ceiling.jpg");
	level1.init_texture(g_pd3dDevice, L"level.jpg");
	level1.make_big_level_object(g_pd3dDevice, &g_View, &g_Projection);
	
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureRV ) g_pTextureRV->Release();
    if(g_pCBuffer) g_pCBuffer->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////

XMVECTOR *det = new XMVECTOR;
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{
	static int clip = 0;
	if (clip == 6)
		clip = 0;
	XMFLOAT3 shootdirection = XMFLOAT3(0, 0, 1);
	XMMATRIX R = cam.get_matrix(&g_View);

	R._41 = R._42 = R._43 = 0.0f;
	R = XMMatrixInverse(det, R);

	shootdirection = R * shootdirection;

	bullets[clip].impulse = shootdirection;
	bullets[clip].projectile.position = XMFLOAT3(-cam.position.x, -cam.position.y, -cam.position.z);
	clip++;
	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
	{

	}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
	{
	//Ray outray;
	//outray.P0 = XMFLOAT3(cam.position.x, cam.position.y, cam.position.z);
	//outray.P1 = XMFLOAT3(outray.P0.x + cam.fwd.x, outray.P0.y + cam.fwd.y, outray.P0.z + cam.fwd.z);
	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
	{


	}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////

bool dead = false;
void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
	{
	static int lastX = x;
	static int flag = 0;
	if (flag == 1)
	{
		lastX = x;
		flag = 0;
		return;
	}
	
	if (!dead)
	{
	int diff = lastX - x;
	cam.rotation.y += diff * 0.01f;
	lastX = x;

	RECT r;

	GetWindowRect(hwnd, &r);

	int mx = (r.left + r.right) / 2;
	int my = (r.top + r.bottom) / 2;

	SetCursorPos(mx,my);
	flag = 1;

	}
	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
		{
		}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
		{
		}
	
	}

int plane = 0;		//global defined
void mHideCursor() 	//hides cursor
{
	while (plane >= 0)
		plane = ShowCursor(FALSE);
}
void mShowCursor() 	//shows it again
{
	while (plane<0)
		plane = ShowCursor(TRUE);
}



BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
	{
	RECT rc;
	GetWindowRect(hwnd, &rc);
	SetCursorPos((rc.right + rc.left)/2, (rc.bottom + rc.top)/2);
	
	mHideCursor();

	return TRUE;
	}
void OnTimer(HWND hwnd, UINT id)
	{

	}
bool shiftDown = false;
//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
	switch (vk)
		{
			case 65:cam.a = 0;//a
				break;
			case 68: cam.d = 0;//d
				break;
			case 32: //space
				break;
			case 87: cam.w = 0; //w
				break;
			case 16: shiftDown = false;//escape
				break;
			case 83:cam.s = 0; //s
			default:break;

		}

	}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
	int x = 0;
	switch (vk)
		{
			default:break;
			case 65:cam.a = 1;//a
				break;
			case 68: cam.d = 1;//d
				break;
			case 32: //space
			break;
			case 87: cam.w = 1; //w
				break;
			case 83:cam.s = 1; //s
				break;
			case 16: shiftDown = true;//escape
				break;
			case 27: PostQuitMessage(0);//escape
				break;
		}
	}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
#include <windowsx.h>
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
	HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
	HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLBU);
	HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMM);
	HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
	HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
	HANDLE_MSG(hWnd, WM_KEYUP, OnKeyUp);
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
			mShowCursor();
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}



//--------------------------------------------------------------------------------------
// sprites
//--------------------------------------------------------------------------------------
class sprites
	{
	public:
		XMFLOAT3 position;
		XMFLOAT3 impulse;
		float rotation_x;
		float rotation_y;
		float rotation_z;
		sprites()
			{
			impulse = position = XMFLOAT3(0, 0, 0);
			rotation_x = rotation_y = rotation_z;
			}
		XMMATRIX animation() 
			{
			//update position:
			position.x = position.x + impulse.x; //newtons law
			position.y = position.y + impulse.y; //newtons law
			position.z = position.z + impulse.z; //newtons law

			XMMATRIX M;
			//make matrix M:
			XMMATRIX R,Rx,Ry,Rz,T;
			T = XMMatrixTranslation(position.x, position.y, position.z);
			Rx = XMMatrixRotationX(rotation_x);
			Ry = XMMatrixRotationX(rotation_y);
			Rz = XMMatrixRotationX(rotation_z);
			R = Rx*Ry*Rz;
			M = R*T;
			return M;
			}
	};
sprites mario;


void animateEnemies(TIME t)
{
	if (enemies[0].position.x < 0)
		enemies[0].direction = true;
	if(enemies[0].position.x > 10)
		enemies[0].direction = false;

	if (!enemies[0].direction)
		enemies[0].position.x -= t * 0.0000007f;
	else
		enemies[0].position.x += t * 0.0000007f;

	if (enemies[1].position.x < -10)
		enemies[1].direction = true;
	if (enemies[1].position.x > 0)
		enemies[1].direction = false;

	if (!enemies[1].direction)
		enemies[1].position.x -= t * 0.0000005f;
	else
		enemies[1].position.x += t * 0.0000005f;

	if (enemies[2].position.x < -5)
		enemies[2].direction = true;
	if (enemies[2].position.x > 10)
		enemies[2].direction = false;

	if (!enemies[2].direction)
		enemies[2].position.x -= t * 0.0000009f;
	else
		enemies[2].position.x += t * 0.0000009f;

	if (enemies[3].position.z < 12)
		enemies[3].direction = true;
	if (enemies[3].position.z > 22)
		enemies[3].direction = false;

	if (!enemies[3].direction)
		enemies[3].position.z -= t * 0.0000005f;
	else
		enemies[3].position.z += t * 0.0000005f;

	static float tm = 0.0f;
	tm += t / 6000000.0;
	enemies[4].position.x = 7.0 + (3 * sin(tm));
	enemies[4].position.z = 34.0 + (2 * cos(tm));


}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
UINT stride = sizeof(SimpleVertex);
UINT offset = 0;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // Update our time
    static float t = 0.0f;
	t += 0.001;
	static StopWatchMicro_ timer;
	TIME elapsed = timer.elapse_micro();
	timer.start();

    ConstantBuffer constantbuffer;

	if (gamepad->IsConnected())
	{
		if (gamepad->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_Y)
			cam.w = 1;
		else
			cam.w = 0;
		if (gamepad->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_A)
		{

			cam.s = 1;
		}
		else
			cam.s = 0;

	}
	SHORT lx = gamepad->GetState().Gamepad.sThumbLX;
	SHORT ly = gamepad->GetState().Gamepad.sThumbLY;

	SHORT rx = gamepad->GetState().Gamepad.sThumbRX;
	SHORT ry = gamepad->GetState().Gamepad.sThumbRY;

	if (abs(ry) > 3000)
	{
		float angle_x = (float)ry / 32000.0;
		angle_x *= 0.05;
		cam.rotation.x += angle_x;
	}
	if (abs(rx) > 3000)
	{
		float angle_y = (float)rx / 32000.0;
		angle_y *= 0.05;
		cam.rotation.y -= angle_y;
	}


    // Clear the back buffer
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    // Clear the depth buffer to 1.0 (max depth)
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );


	
	if (dead)
	{
		constantbuffer.d = 1;
	}
	else
	{
		if (shiftDown)
			cam.animation(elapsed, 0.00001f, &level1);
		else
			cam.animation(elapsed, 0.000005f, &level1);
	}

	XMMATRIX view = cam.get_matrix(&g_View);


	XMMATRIX T = XMMatrixTranslation(-cam.position.x, -cam.position.y, -cam.position.z);

    // Update skybox constant buffer
	constantbuffer.World	=	XMMatrixTranspose(T);
	constantbuffer.View		=	XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
    g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0 );
    // Render skybox
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);	
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBuffer);   
    g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureRV );
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );

	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
    g_pImmediateContext->Draw( 36, 0 );

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);


	//render all the walls of the level
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	level1.render_level(g_pImmediateContext, &view, &g_Projection, g_pCBuffer);

	for (int i = 0; i < 5; i++)
	{
		if (!enemies[i].dead)
		{
		for (int j = 0; j < 6; j++)
		{
			XMFLOAT3 toVector;
			toVector.x = enemies[i].position.x - bullets[j].projectile.position.x;
			toVector.z = enemies[i].position.z - bullets[j].projectile.position.z;

			float length = sqrt(pow(toVector.x, 2) + pow(toVector.z, 2));

			if (length < 1.0)
				enemies[i].dead = true;
				
		}

			animateEnemies(elapsed);

			XMFLOAT3 toVector;
			toVector.x = enemies[i].position.x - (cam.position.x * -1);
			toVector.z = enemies[i].position.z - (cam.position.z * -1);

			float length = sqrt(pow(toVector.x, 2) + pow(toVector.z, 2));

			if (length < 1.0)
		//		dead = true;

			constantbuffer.World = XMMatrixTranspose(enemies[i].get_matrix(view));
			constantbuffer.View = XMMatrixTranspose(view);
			constantbuffer.Projection = XMMatrixTranspose(g_Projection);
			g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
			g_pImmediateContext->PSSetShaderResources(0, 1, &g_pEnemyTex);

		//	g_pImmediateContext->Draw(12, 0);
		}
	}

	/*
	//flashlight
	XMMATRIX GT = XMMatrixTranslation(-0.3f, 0, 10);
	XMMATRIX GS = XMMatrixScaling(2,2,1);

	constantbuffer.World = XMMatrixTranspose(GS * GT);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.e = 1;
	constantbuffer.f = 1;
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pFlashTex);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	g_pImmediateContext->VSSetShader(g_pVertexShaderHUD, NULL, 0);
	g_pImmediateContext->Draw(12, 0);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	constantbuffer.e = 0;
	constantbuffer.f = 0;
	*/

	//drawBullets
	for (int i = 0; i < 6; i++)
	{


		XMMATRIX S = XMMatrixScaling(0.2f, 0.2f, 1);
		XMMATRIX T = XMMatrixTranslation(0.0f, -0.5f, 1.0f);
		bullets[i].projectile.position = bullets[i].projectile.position + (bullets[i].impulse * 0.2f);

		if (abs(bullets[i].projectile.position.z) <= 80 || abs(bullets[i].projectile.position.x) <= 80)
		{
		constantbuffer.World = XMMatrixTranspose(S * T * bullets[i].projectile.get_matrix(view));
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBulletTex);

		g_pImmediateContext->Draw(12, 0);
		}
	}

	
	//Draw gun
	if (!dead)
	{
	XMMATRIX GT = XMMatrixTranslation(0.2f , -3, 9);
	constantbuffer.World = XMMatrixTranspose(GT);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.e = 1;
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pGunTex);
	
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	g_pImmediateContext->VSSetShader(g_pVertexShaderHUD, NULL, 0);
	//g_pImmediateContext->Draw(12, 0);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	}
	constantbuffer.e = 0;


	XMMATRIX M = XMMatrixIdentity();
	T = XMMatrixTranslation(-1, 0, 8);
	M = XMMatrixScaling(0.01f, 0.01f, 0.01f) * T;
	constantbuffer.World = XMMatrixTranspose(M);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.sphere = 1;
	constantbuffer.sp_pos = XMFLOAT3(0,0,8);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sphere, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);


	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	//g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(model_vertex_anz, 0);
	//g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	constantbuffer.sphere = 0;


	// GUN XMMatrixScaling(0.01, 0.01, 0.01) * XMMatrixRotationX(XM_PIDIV2) * XMMatrixRotationZ(XM_PI) * XMMatrixRotationY(XM_PI) * T;

	M = XMMatrixIdentity();
	T = XMMatrixTranslation(0, 0, 10);
	M =  T;
	constantbuffer.World = XMMatrixTranspose(M);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.sp_pos = XMFLOAT3(0, 0, 8);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBombTex);
	g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_bomb, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);


	//g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(bomb_vertex_anz, 0);


	M = XMMatrixIdentity();
	
	T = XMMatrixTranslation(0.1f, -0.1f, -0.2f);
	M = XMMatrixScaling(0.01, 0.01, 0.01) * XMMatrixRotationX(XM_PIDIV2) * XMMatrixRotationZ(XM_PI) * XMMatrixRotationY(XM_PI) * T;
	constantbuffer.World = XMMatrixTranspose(M);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.sp_pos = XMFLOAT3(0, 0, 8);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShaderHUD, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pLawTex);
	g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_law, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);


	//g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	g_pImmediateContext->Draw(law_vertex_anz, 0);
	

	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	explosionhandler.render(&view, &g_Projection, elapsed);
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

	static int flag = 0;
	if(flag == 0)
		explosionhandler.new_explosion(XMFLOAT3(0, 0, 10), XMFLOAT3(1,1,1), 0, 4.0);
	flag = 1;
    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 0, 0 );
}
