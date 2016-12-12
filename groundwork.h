#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <io.h>
#include "resource.h"
#include <XInput.h>

#pragma comment(lib, "XInput.lib")
using namespace std;

typedef long double TIME;

struct Ray { XMFLOAT3 P0,P1;};
XMFLOAT3 operator+(const XMFLOAT3 lhs, XMFLOAT3 rhs);
bool operator==(const XMFLOAT3 lhs, XMFLOAT3 rhs);
XMFLOAT3 operator-(const XMFLOAT3 lhs, XMFLOAT3 rhs);
XMFLOAT3 operator*(const XMMATRIX &lhs, XMFLOAT3 rhs);
XMFLOAT3 operator*(XMFLOAT3 rhs, const XMMATRIX &lhs);
XMFLOAT3 operator*(const XMFLOAT3 lhs, float rhs);
XMFLOAT3 operator*(float rhs, const XMFLOAT3 lhs);
float Vec3Length(const XMFLOAT3 &v);
float Vec3Dot(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Cross(XMFLOAT3 a, XMFLOAT3 b);
XMFLOAT3 Vec3Normalize(const  XMFLOAT3 &a);
bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count);
bool LoadOBJ(char * filename, ID3D11Device * g_pd3dDevice, ID3D11Buffer ** ppVertexBuffer, int * vertex_count);
bool LoadCatmullClark(LPCTSTR filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count); 
int D3D_intersect_RayTriangle(Ray R, XMFLOAT3 A, XMFLOAT3 B, XMFLOAT3 C, XMFLOAT3* I);
XMFLOAT3 mul(XMFLOAT3 v, XMMATRIX &M);
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
struct vertexstruct
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};
struct SimpleVertexLVL
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};
/*
struct ConstantBuffer
{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
};*/
class explosion_spots
{
public:
	XMFLOAT3 pos;
	XMFLOAT3 imp;
	float scale;

	long lifespan;
	explosion_spots()
	{
		lifespan = 0;
		pos = XMFLOAT3(0, 0, 0);
		imp = XMFLOAT3(0, 0, 0);
		scale = 1;
	}
	XMMATRIX animate(long elapsed)
	{
		lifespan += elapsed;
		pos.x = pos.x + imp.x*elapsed*0.000001;
		pos.y = pos.y + imp.y*elapsed*0.000001;
		pos.z = pos.z + imp.z*elapsed*0.000001;
		XMMATRIX S = XMMatrixScaling(scale, scale, scale);
		return S*XMMatrixTranslation(pos.x, pos.y, pos.z);
	}
};
class explosions_types
{
public:
	vector<explosion_spots> spots;
	ID3D11ShaderResourceView*           texture = NULL;
	explosions_types()
	{
		xparts = 0;
		yparts = 0;
		lifespan = 3000000;//3 seconds
	}
	bool get_spot(int i, long elapsed, XMMATRIX *world, int *tx, int *ty)
	{
		if (i >= spots.size())return FALSE;
		double maxframes = (double)xparts*yparts + 0.00000001;
		explosion_spots *ex = &spots[i];
		*world = ex->animate(elapsed);
		long time_passed = ex->lifespan;
		double frame = time_passed / (double)lifespan;
		if (frame >= 1)
		{
			spots.erase(spots.begin() + i);
			return TRUE;
		}
		double actualframe = maxframes * frame;
		int aframe = (int)actualframe;
		*tx = aframe % xparts;
		*ty = aframe / yparts;
		return TRUE;
	}
	long lifespan;
	int xparts;
	int yparts;

};
class explosions_constantbuffer
{
public:
	XMMATRIX world, view, projection;
	XMFLOAT4 animation_offset;
	explosions_constantbuffer()
	{
		world = view = projection = XMMatrixIdentity();
		animation_offset = XMFLOAT4(0, 0, 0, 0);
	}
};
class explosion_handler
{
private:
	vector<explosions_types> exp;
	ID3D11Device*                       Device;
	ID3D11DeviceContext*                DeviceContext;
	ID3D11PixelShader*                  PS;
	ID3D11VertexShader*                 VS;
	ID3D11Buffer*                       vertexbuffer;
	ID3D11Buffer*                       constantbuffer;
	ID3D11InputLayout*                  VertexLayout;
	explosions_constantbuffer			s_constantbuffer;
public:
	explosion_handler()
	{
		VertexLayout = NULL;
		vertexbuffer = NULL;
		PS = NULL;
		VS = NULL;
		Device = NULL;
		DeviceContext = NULL;
	}
	HRESULT init(ID3D11Device* device, ID3D11DeviceContext* immediatecontext)
	{
		Device = device;
		DeviceContext = immediatecontext;
		// Compile the vertex shader
		ID3DBlob* pVSBlob = NULL;
		HRESULT hr = CompileShaderFromFile(L"explosion_shader.fx", "VS", "vs_4_0", &pVSBlob);
		if (FAILED(hr))
		{
			MessageBox(NULL,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return hr;
		}

		// Create the vertex shader
		hr = Device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &VS);
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
		UINT numElements = ARRAYSIZE(layout);

		// Create the input layout
		hr = Device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
			pVSBlob->GetBufferSize(), &VertexLayout);
		pVSBlob->Release();
		if (FAILED(hr))
			return hr;

		ID3DBlob* pPSBlob = NULL;
		hr = CompileShaderFromFile(L"explosion_shader.fx", "PS", "ps_5_0", &pPSBlob);
		if (FAILED(hr))
		{
			MessageBox(NULL,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return hr;
		}

		// Create the pixel shader
		hr = Device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &PS);
		pPSBlob->Release();
		if (FAILED(hr))
			return hr;

		// Create vertex buffer
		vertexstruct vertices[] =
		{
			{ XMFLOAT3(-1,1,0),XMFLOAT2(0,0) },
			{ XMFLOAT3(1,1,0),XMFLOAT2(1,0) },
			{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1) },
			{ XMFLOAT3(1,1,0),XMFLOAT2(1,0) },
			{ XMFLOAT3(1,-1,0),XMFLOAT2(1,1) },
			{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1) }
		};

		//initialize d3dx verexbuff:
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertexstruct) * 6;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;
		hr = Device->CreateBuffer(&bd, &InitData, &vertexbuffer);
		if (FAILED(hr))
			return FALSE;

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(explosions_constantbuffer);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		hr = Device->CreateBuffer(&bd, NULL, &constantbuffer);
		if (FAILED(hr))
			return hr;
		return S_OK;
	}
	HRESULT init_types(LPCWSTR file, int xparts, int yparts, long lifespan)
	{
		explosions_types et;
		HRESULT hr = D3DX11CreateShaderResourceViewFromFile(Device, file, NULL, NULL, &et.texture, NULL);
		if (FAILED(hr))
			return hr;
		et.lifespan = lifespan;
		et.xparts = xparts;
		et.yparts = yparts;
		exp.push_back(et);
		return S_OK;
	}
	void new_explosion(XMFLOAT3 position, XMFLOAT3 impulse, int type, float scale)
	{
		if (exp.size() <= 0) return;
		if (type >= exp.size())type = 0;
		explosion_spots ep;
		ep.imp = impulse;
		ep.pos = position;
		ep.scale = scale;
		exp[type].spots.push_back(ep);
	}
	void render(XMMATRIX *view, XMMATRIX *projection, long elapsed)
	{
		DeviceContext->IASetInputLayout(VertexLayout);
		UINT stride = sizeof(vertexstruct);
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &vertexbuffer, &stride, &offset);
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		XMVECTOR det;
		XMMATRIX V = *view;
		V._41 = 0;
		V._42 = 0;
		V._43 = 0;
		V._44 = 1;
		V = XMMatrixInverse(&det, V);
		s_constantbuffer.view = XMMatrixTranspose(*view);
		s_constantbuffer.projection = XMMatrixTranspose(*projection);
		DeviceContext->VSSetShader(VS, NULL, 0);
		DeviceContext->PSSetShader(PS, NULL, 0);
		DeviceContext->VSSetConstantBuffers(0, 1, &constantbuffer);
		DeviceContext->PSSetConstantBuffers(0, 1, &constantbuffer);
		for (int ii = 0; ii < exp.size(); ii++)
		{
			DeviceContext->PSSetShaderResources(0, 1, &exp[ii].texture);
			for (int uu = 0; uu < exp[ii].spots.size(); uu++)
			{
				XMMATRIX world;
				int tx, ty;
				if (!exp[ii].get_spot(uu, elapsed, &world, &tx, &ty)) { uu--; continue; }
				//float scale = exp[ii].spots[uu].scale;


				s_constantbuffer.world = XMMatrixTranspose(V*world);
				s_constantbuffer.animation_offset.x = exp[ii].xparts;
				s_constantbuffer.animation_offset.y = exp[ii].yparts;
				s_constantbuffer.animation_offset.z = tx;
				s_constantbuffer.animation_offset.w = ty;
				DeviceContext->UpdateSubresource(constantbuffer, 0, NULL, &s_constantbuffer, 0, 0);
				DeviceContext->Draw(6, 0);
			}
		}
	}
};



struct SimpleVertex
	{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Norm;
	};


struct ConstantBuffer
	{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT4 w_pos;
	XMFLOAT4 data;
	};
//*****************************************



class CXBOXController
{
private:
	XINPUT_STATE _controllerState;
	int _controllerNum;
public:
	CXBOXController(int playerNumber)
	{
		// Set the Controller Number
		_controllerNum = playerNumber - 1;
	}

	XINPUT_STATE GetState()
	{
		// Zeroise the state
		ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

		// Get the state
		XInputGetState(_controllerNum, &_controllerState);

		return _controllerState;
	}

	bool IsConnected()
	{
		// Zeroise the state
		ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

		// Get the state
		DWORD Result = XInputGetState(_controllerNum, &_controllerState);

		if (Result == ERROR_SUCCESS)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void Vibrate(int leftVal = 0, int rightVal = 0)
	{
		// Create a Vibraton State
		XINPUT_VIBRATION Vibration;

		// Zeroise the Vibration
		ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

		// Set the Vibration Values
		Vibration.wLeftMotorSpeed = leftVal;
		Vibration.wRightMotorSpeed = rightVal;

		// Vibrate the controller
		XInputSetState(_controllerNum, &Vibration);
	}


};


class StopWatchMicro_
{
private:
	LARGE_INTEGER last, frequency;
public:
	StopWatchMicro_()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&last);

	}
	long double elapse_micro()
	{
		LARGE_INTEGER now, dif;
		QueryPerformanceCounter(&now);
		dif.QuadPart = now.QuadPart - last.QuadPart;
		long double fdiff = (long double)dif.QuadPart;
		fdiff /= (long double)frequency.QuadPart;
		return fdiff*1000000.;
	}
	long double elapse_milli()
	{
		elapse_micro() / 1000.;
	}
	void start()
	{
		QueryPerformanceCounter(&last);
	}
};
//**********************************
class billboard
{
public:
	billboard()
	{
		position = XMFLOAT3(0, 0, 0);
		scale = 1;
		transparency = 1;
		direction = false;
		dead = false;
	}
	XMFLOAT3 position; //obvious
	float scale;		//in case it can grow
	float transparency; //for later use
	bool direction;
	bool dead;
	XMMATRIX get_matrix(XMMATRIX &ViewMatrix)
	{

		XMMATRIX view, R, T, S;
		view = ViewMatrix;
		//eliminate camera translation:
		view._41 = view._42 = view._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, view);//inverse rotation
		T = XMMatrixTranslation(position.x, position.y, position.z);
		S = XMMatrixScaling(scale, scale, scale);
		return S*R*T;
	}

	XMMATRIX get_matrix_y(XMMATRIX &ViewMatrix) //enemy-type
	{

	}
};


struct bullet
{
	billboard projectile;
	XMFLOAT3 impulse;
};


class bitmap
	{

	public:
		BYTE *image;
		int array_size;
		BITMAPFILEHEADER bmfh;
		BITMAPINFOHEADER bmih;
		bitmap()
			{
			image = NULL;
			}
		~bitmap()
			{
			if(image)
				delete[] image;
			array_size = 0;
			}
		bool read_image(char *filename)
			{
			ifstream bmpfile(filename, ios::in | ios::binary);
			if (!bmpfile.is_open()) return FALSE;	// Error opening file
			bmpfile.read((char*)&bmfh, sizeof(BITMAPFILEHEADER));
			bmpfile.read((char*)&bmih, sizeof(BITMAPINFOHEADER));
			bmpfile.seekg(bmfh.bfOffBits, ios::beg);
			//make the array
			if (image)delete[] image;
			int size = bmih.biWidth*bmih.biHeight * 3;
			image = new BYTE[size];//3 because red, green and blue, each one byte
			bmpfile.read((char*)image,size);
			array_size = size;
			bmpfile.close();
			check_save();
			return TRUE;
			}
		BYTE get_pixel(int x, int y, int color_offset) //color_offset = 0,1 or 2 for red, green and blue
		{
			int array_position = x * 3 + y* bmih.biWidth * 3 + color_offset;
			if (array_position >= array_size) return 0;
			if (array_position < 0) return 0;
			return image[array_position];
		}
		BYTE set_pixel(int x, int y, int color_offset) //color_offset = 0,1 or 2 for red, green and blue
		{
			int array_position = x * 3 + y* bmih.biWidth * 3 + color_offset;
			if (array_position >= array_size) return 0;
			if (array_position < 0) return 0;
			image[array_position] = 41;
			return image[array_position];
		}
		    
		
		void check_save()
			{
			ofstream nbmpfile("newpic.bmp", ios::out | ios::binary);
			if (!nbmpfile.is_open()) return;
			nbmpfile.write((char*)&bmfh, sizeof(BITMAPFILEHEADER));
			nbmpfile.write((char*)&bmih, sizeof(BITMAPINFOHEADER));
			//offset:
			int rest = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
			if (rest > 0)
				{
				BYTE *r = new BYTE[rest];
				memset(r, 0, rest);
				nbmpfile.write((char*)&r, rest);
				}
			nbmpfile.write((char*)image, array_size);
			nbmpfile.close();

			}
	};
////////////////////////////////////////////////////////////////////////////////
//lets assume a wall is 10/10 big!
#define FULLWALL 2
#define HALFWALL 1
class wall
	{
	public:
		XMFLOAT3 position;
			int texture_no;
			int rotation; //0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
			int collision;
			wall()
				{
				texture_no = 0;
				rotation = 0;
				position = XMFLOAT3(0,0,0);
				collision = 0;
				}
			XMMATRIX get_matrix()
				{
				XMMATRIX R, T, T_offset;				
				R = XMMatrixIdentity();
				T_offset = XMMatrixTranslation(0, 0, -HALFWALL);
				T = XMMatrixTranslation(position.x, position.y, position.z);
				switch (rotation)//0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
					{
						default:
						case 0:	R = XMMatrixRotationY(XM_PI);		T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 1: R = XMMatrixRotationY(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 2:										T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 3: R = XMMatrixRotationY(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
						case 4: R = XMMatrixRotationX(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
						case 5: R = XMMatrixRotationX(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
					}
				return T_offset * R * T;
				}
	};
/*************************************************************************/

#define MAXTEXTURE 30
#define TEXPARTS 3
XMFLOAT2 get_level_tex_coords(int pic, XMFLOAT2 coords);
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

class level
{
private:
	bitmap leveldata;
	vector<wall*> walls;						//all wall positions
	ID3D11Buffer*                       VertexBuffer = NULL;
	int texture_count;
	ID3D11VertexShader*                 vertexshader;
	ID3D11PixelShader*                  pixelshader;
	ID3D11InputLayout*                  VertexLayout = NULL;
	ID3D11ShaderResourceView* texture;
	void process_level()
	{
		//we have to get the level to the middle:
		int x_offset = (leveldata.bmih.biWidth / 2)*FULLWALL;

		//lets go over each pixel without the borders!, only the inner ones
		for (int yy = 1; yy < (leveldata.bmih.biHeight - 1); yy++)
			for (int xx = 1; xx < (leveldata.bmih.biWidth - 1); xx++)
			{
				//wall information is the interface between pixels:
				//blue to something not blue: wall. texture number = 255 - blue
				//green only: floor. texture number = 255 - green
				//red only: ceiling. texture number = 255 - red
				//green and red: floor and ceiling ............
				BYTE red, green, blue;

				blue = leveldata.get_pixel(xx, yy, 0);
				green = leveldata.get_pixel(xx, yy, 1);
				red = leveldata.get_pixel(xx, yy, 2);

				if (blue > 0)//wall possible
				{
					int texno = 255 - blue;
					BYTE left_red = leveldata.get_pixel(xx - 1, yy, 2);
					BYTE left_green = leveldata.get_pixel(xx - 1, yy, 1);
					BYTE right_red = leveldata.get_pixel(xx + 1, yy, 2);
					BYTE right_green = leveldata.get_pixel(xx + 1, yy, 1);
					BYTE top_red = leveldata.get_pixel(xx, yy + 1, 2);
					BYTE top_green = leveldata.get_pixel(xx, yy + 1, 1);
					BYTE bottom_red = leveldata.get_pixel(xx, yy - 1, 2);
					BYTE bottom_green = leveldata.get_pixel(xx, yy - 1, 1);

					if (left_red>0 || left_green > 0)//to the left
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 3, texno);
					if (right_red>0 || right_green > 0)//to the right
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 1, texno);
					if (top_red>0 || top_green > 0)//to the top
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 2, texno);
					if (bottom_red>0 || bottom_green > 0)//to the bottom
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 0, texno);
				}
				if (red > 0)//ceiling
				{
					int texno = 255 - red;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 5, texno);
				}
				if (green > 0)//floor
				{
					int texno = 255 - green;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 4, texno);
				}
			}
	}
	void init_wall(XMFLOAT3 pos, int rotation, int texture_no)
	{
		wall *w = new wall;
		walls.push_back(w);
		w->position = pos;
		w->rotation = rotation;
		w->texture_no = texture_no;
	}
	SimpleVertexLVL *pvertices;
public:
	level()
	{

		pvertices = NULL;
		texture_count = 0;

		texture = NULL;
	}
	void init(char *level_bitmap)
	{
		if (!leveldata.read_image(level_bitmap))return;
		process_level();
	}
	
	bool init_texture(ID3D11Device* pd3dDevice, LPCWSTR filename)
	{
		// Load the Texture
		ID3D11ShaderResourceView *tex;
		HRESULT hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, filename, NULL, NULL, &tex, NULL);
		if (FAILED(hr))
			return FALSE;
		texture = tex;
		return TRUE;
	}

	XMMATRIX get_wall_matrix(int no)
	{
		if (no < 0 || no >= walls.size()) return XMMatrixIdentity();
		return walls[no]->get_matrix();
	}
	int get_wall_count()
	{
		return walls.size();
	}
	void render_level(ID3D11DeviceContext* ImmediateContext, XMMATRIX *view, XMMATRIX *projection, ID3D11Buffer* dx_cbuffer)
	{
		//set up everything for the waqlls/floors/ceilings:
		UINT stride = sizeof(SimpleVertexLVL);
		UINT offset = 0;
		ID3D11InputLayout*           old_VertexLayout = NULL;
		ImmediateContext->IAGetInputLayout(&old_VertexLayout);
		ImmediateContext->IASetInputLayout(VertexLayout);
		ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
		ConstantBuffer constantbuffer;
		constantbuffer.View = XMMatrixTranspose(*view);
		constantbuffer.Projection = XMMatrixTranspose(*projection);
		XMMATRIX wall_matrix, S;

		//S = XMMatrixScaling(FULLWALL, FULLWALL, FULLWALL);

		constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		ImmediateContext->VSSetShader(vertexshader, NULL, 0);
		ImmediateContext->PSSetShader(pixelshader, NULL, 0);
		ImmediateContext->UpdateSubresource(dx_cbuffer, 0, NULL, &constantbuffer, 0, 0);
		ImmediateContext->VSSetConstantBuffers(0, 1, &dx_cbuffer);
		ImmediateContext->PSSetConstantBuffers(0, 1, &dx_cbuffer);
		ImmediateContext->PSSetShaderResources(2, 1, &texture);
		ImmediateContext->Draw(walls.size() * 12, 0);

		ImmediateContext->IASetInputLayout(old_VertexLayout);

	}
	bool check_wall_vertex(Ray B, XMFLOAT3 *I)
	{
		vector<XMFLOAT3> collisions;
		XMFLOAT3 *solutions = new XMFLOAT3;
		for (int i = 0; i < (walls.size() * 12); i += 6)
		{
			XMFLOAT3 a = pvertices[i + 0].Pos;
			XMFLOAT3 b = pvertices[i + 1].Pos;
			XMFLOAT3 c = pvertices[i + 2].Pos;

			XMFLOAT3 diff;
			diff.x = B.P0.x - a.x;
			diff.y = B.P0.y - a.y;
			diff.z = B.P0.z - a.z;

			float length = sqrt(pow(diff.x, 2) + pow(diff.z, 2));

			int res;
			if (length < 8)
			{
				res = D3D_intersect_RayTriangle(B, a, b, c, solutions);
				if (res == 1)
				{
					//wall *w = walls.at(i / 12);
					//set_col(w->position);
					//w->collision++;
					collisions.push_back(*solutions);
				}
			}

			XMFLOAT3 a2 = pvertices[i + 3].Pos;
			XMFLOAT3 b2 = pvertices[i + 4].Pos;
			XMFLOAT3 c2 = pvertices[i + 5].Pos;

			diff.x = B.P0.x - a.x;
			diff.y = B.P0.y - a.y;
			diff.z = B.P0.z - a.z;

			length = sqrt(pow(diff.x, 2) + pow(diff.z, 2));

			if (length < 8)
			{
				res = D3D_intersect_RayTriangle(B, a2, b2, c2, solutions);
				if (res == 1)
				{
				//	wall *w = walls.at(i /12);
				//	w->collision++;
				//	set_col(w->position);

					collisions.push_back(*solutions);
				}
			}
		}
		if (collisions.size() > 0)
		{
			float closest = 99999999999.999;
			for (int i = 0; i < collisions.size(); i++)
			{

				XMFLOAT3 camfwd;
				camfwd.x = B.P1.x - B.P0.x;
				camfwd.y = B.P1.y - B.P0.y;
				camfwd.z = B.P1.z - B.P0.z;


				XMFLOAT3 diff;
				diff.x = B.P0.x - collisions[i].x;
				diff.y = B.P0.y - collisions[i].y;
				diff.z = B.P0.z - collisions[i].z;

				float len = Vec3Length(diff);
				

				float dot = Vec3Dot(Vec3Normalize(diff), Vec3Normalize(camfwd));


				if (closest > len && dot < 0)
				{
					closest = len;
					I->x = collisions[i].x;
					I->y = collisions[i].y;
					I->z = collisions[i].z;
				}
			}
			return true;
		}
		return false;
	}
	void set_col(XMFLOAT3 pos)
	{
		pos.x = -pos.x;
		int x_offset = 50;
		float x, z;
		pos.x /= 2;
		pos.z /= 2;
		x = (pos.x / FULLWALL) + x_offset + 0.5;
		z = (pos.z / FULLWALL) + 0.5f;
		leveldata.set_pixel(x, z, 2);

	}

	bool check_col(XMFLOAT3 pos, vector<XMFLOAT3> sphere_pos)
	{

		for (int i = 0; i < sphere_pos.size(); i++)
		{
			float leng = Vec3Length(pos - sphere_pos[i]);
			if (abs(leng) < 1.0f)
				return false;
		}

		int x_offset = 50;
		float x, z;
		pos.x /= 2;
		pos.z /= 2;
		x = (pos.x / FULLWALL) +x_offset+0.5;
		z = (pos.z / FULLWALL)+0.5f;
		BYTE red = leveldata.get_pixel(x, z, 2);
		if (red > 10)
			return false;
		return true;
	}
	void make_big_level_object(ID3D11Device* g_pd3dDevice, XMMATRIX *view, XMMATRIX *projection)
	{
		if (pvertices)return;
		// Compile the pixel shader
		ID3DBlob* pPSBlob = NULL;
		HRESULT hr = CompileShaderFromFile(L"shader.fx", "PSlevel", "ps_4_0", &pPSBlob);
		if (FAILED(hr))
		{
			MessageBox(NULL,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return;
		}

		// Create the pixel shader
		hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelshader);
		pPSBlob->Release();
		if (FAILED(hr))
			return;
		ID3DBlob* pVSBlob = NULL;
		hr = CompileShaderFromFile(L"shader.fx", "VSlevel", "vs_4_0", &pVSBlob);
		if (FAILED(hr))
		{
			MessageBox(NULL,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return;
		}
		// Create the vertex shader
		hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexshader);
		if (FAILED(hr))
		{
			pVSBlob->Release();
			return;
		}
		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		// Create the input layout
		hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
			pVSBlob->GetBufferSize(), &VertexLayout);

		SimpleVertexLVL vertices[] = //12 vertices
		{
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },

			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

		};
		pvertices = new SimpleVertexLVL[12 * walls.size()];
		SimpleVertexLVL ver[100];
		XMMATRIX S = XMMatrixScaling(2, 2, 2);
		int originalVert = 12;
		int expVert = 6;
		int oo = 0;
		for (int ii = 0; ii < walls.size(); ii++)
		{
			XMMATRIX wall_matrix = walls[ii]->get_matrix();
			for (int uu = 0; uu < 12; uu++)
			{
				pvertices[ii * 12 + uu].Pos = mul(vertices[uu].Pos, S* wall_matrix);
				pvertices[ii * 12 + uu].Tex = get_level_tex_coords(walls[ii]->texture_no, vertices[uu].Tex);
			}
		}
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertexLVL) * walls.size() * 12;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = pvertices;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &VertexBuffer);
		if (FAILED(hr))
			return;
		int z;
		z = 0;
	}
};
//********************************************************************************************
/*

class level
	{
	private:
		bitmap leveldata;
		vector<wall*> walls;						//all wall positions
		vector<XMFLOAT3*> wall_vertices;
		vector<ID3D11ShaderResourceView*> textures;	//all wall textures
		void process_level()
			{
			//we have to get the level to the middle:
			int x_offset = (leveldata.bmih.biWidth/2)*FULLWALL;

			//lets go over each pixel without the borders!, only the inner ones
			for (int yy = 1; yy < (leveldata.bmih.biHeight - 1);yy++)
				for (int xx = 1; xx < (leveldata.bmih.biWidth - 1); xx++)
					{
					//wall information is the interface between pixels:
					//blue to something not blue: wall. texture number = 255 - blue
					//green only: floor. texture number = 255 - green
					//red only: ceiling. texture number = 255 - red
					//green and red: floor and ceiling ............
					BYTE red, green, blue;

					blue = leveldata.get_pixel(xx, yy, 0);
					green = leveldata.get_pixel(xx, yy, 1);
					red = leveldata.get_pixel(xx, yy, 2);
					
					if (blue > 0)//wall possible
						{
						int texno = 255 - blue;
						BYTE left_red = leveldata.get_pixel(xx - 1, yy, 2);
						BYTE left_green = leveldata.get_pixel(xx - 1, yy, 1);
						BYTE right_red = leveldata.get_pixel(xx + 1, yy, 2);
						BYTE right_green = leveldata.get_pixel(xx + 1, yy, 1);
						BYTE top_red = leveldata.get_pixel(xx, yy+1, 2);
						BYTE top_green = leveldata.get_pixel(xx, yy+1, 1);
						BYTE bottom_red = leveldata.get_pixel(xx, yy-1, 2);
						BYTE bottom_green = leveldata.get_pixel(xx, yy-1, 1);

						if (left_red>0 || left_green > 0)//to the left
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 3, texno);
						if (right_red>0 || right_green > 0)//to the right
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 1, texno);
						if (top_red>0 || top_green > 0)//to the top
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 2, texno);
						if (bottom_red>0 || bottom_green > 0)//to the bottom
							init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 0, texno);
						}
					if (red > 0)//ceiling
						{
						int texno = 255 - red;
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0,yy*FULLWALL), 5, texno);
						}
					if (green > 0)//floor
						{
						int texno = 255 - green;
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0,yy*FULLWALL), 4, texno);
						}
					}
			}
		void init_wall(XMFLOAT3 pos, int rotation, int texture_no)
			{
			wall *w = new wall;
			walls.push_back(w);
			w->position = pos;
			w->rotation = rotation;
			w->texture_no = texture_no;
			}
	public:
		level()
			{
			}
		void init(char *level_bitmap)
			{
			if(!leveldata.read_image(level_bitmap))return;
			process_level();

			}
		bool init_texture(ID3D11Device* pd3dDevice,LPCWSTR filename)
			{
			// Load the Texture
			ID3D11ShaderResourceView *texture;
			HRESULT hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, filename, NULL, NULL, &texture, NULL);
			if (FAILED(hr))
				return FALSE;
			textures.push_back(texture);
			return TRUE;
			}
		ID3D11ShaderResourceView *get_texture(int no)
			{
			if (no < 0 || no >= textures.size()) return NULL;
			return textures[no];
			}

		bool check_col(XMFLOAT3 pos)
		{
			int x_offset = 50;
			float x, z;
			x = (pos.x / FULLWALL) + x_offset + 0.5f;
			z = (pos.z / FULLWALL) + 0.5f;
			BYTE red = leveldata.get_pixel(x, z, 2);
			if (red > 10)
				return false;
			return true;
		}

		XMMATRIX get_wall_matrix(int no)
			{
			if (no < 0 || no >= walls.size()) return XMMatrixIdentity();
			return walls[no]->get_matrix();
			}
		int get_wall_count()
			{
			return walls.size();
			}

		void render_level(bool ded, ID3D11DeviceContext* ImmediateContext,ID3D11Buffer *vertexbuffer_wall,XMMATRIX *view, XMMATRIX *projection, ID3D11Buffer* dx_cbuffer)
			{
			//set up everything for the waqlls/floors/ceilings:
			UINT stride = sizeof(SimpleVertex);
			UINT offset = 0;			
			ImmediateContext->IASetVertexBuffers(0, 1, &vertexbuffer_wall, &stride, &offset);
			ConstantBuffer constantbuffer;			
			if (ded)
				constantbuffer.d = 1;
			constantbuffer.View = XMMatrixTranspose(*view);
			constantbuffer.Projection = XMMatrixTranspose(*projection);			
			XMMATRIX wall_matrix,S;
			ID3D11ShaderResourceView* tex;
			//S = XMMatrixScaling(FULLWALL, FULLWALL, FULLWALL);
			S = XMMatrixScaling(1, 1, 1);
			for (int ii = 0; ii < walls.size(); ii++)
				{
				wall_matrix = walls[ii]->get_matrix();
				int texno = walls[ii]->texture_no;
				if (texno >= textures.size())
					texno = 0;
				tex = textures[texno];
				wall_matrix = wall_matrix;// *S;

				constantbuffer.World = XMMatrixTranspose(wall_matrix);
				constantbuffer.w_pos = XMFLOAT3(wall_matrix._41, wall_matrix._42, wall_matrix._43);
				
				ImmediateContext->UpdateSubresource(dx_cbuffer, 0, NULL, &constantbuffer, 0, 0);
				ImmediateContext->VSSetConstantBuffers(0, 1, &dx_cbuffer);
				ImmediateContext->PSSetConstantBuffers(0, 1, &dx_cbuffer);
				ImmediateContext->PSSetShaderResources(0, 1, &tex);
				ImmediateContext->Draw(6, 0);
				}
			}


	};
	
	*/
	class camera
		{
		private:

		public:
			int w, s, a, d;
			XMFLOAT3 position;
			XMFLOAT3 rotation;
			XMFLOAT3 fwd;
			camera()
				{
				w = s = a = d = 0;
				position = rotation = fwd = XMFLOAT3(0, 0, 0);
				}
			void animation(TIME t, float factor, level *levl, vector<XMFLOAT3> *sphere_positions)
				{


				XMMATRIX R, T;
				R = XMMatrixRotationY(-rotation.y);

				XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
				XMVECTOR f = XMLoadFloat3(&forward);
				f = XMVector3TransformCoord(f, R);
				XMStoreFloat3(&forward, f);
				XMFLOAT3 side = XMFLOAT3(1, 0, 0);
				XMVECTOR si = XMLoadFloat3(&side);
				si = XMVector3TransformCoord(si, R);
				XMStoreFloat3(&side, si);

				fwd = forward;

				if (w)
					{
					position.x -= (forward.x * t * factor);
					position.y -= (forward.y * t * factor);
					position.z -= (forward.z * t * factor);
					bool topRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool topLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool bottomRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					bool bottomLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
						if (topRight || topLeft || bottomLeft || bottomRight)
						{	
							position.x += (forward.x * t * factor);
							position.y += (forward.y * t * factor);
							position.z += (forward.z * t * factor);

						}

					}
				if (s)
					{
					position.x += forward.x * t * factor;
					position.y += forward.y * t * factor;
					position.z += forward.z * t * factor;
					bool topRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool topLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool bottomRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					bool bottomLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					if (topRight || topLeft || bottomLeft || bottomRight)
					{
						position.x -= forward.x * t * factor;
						position.y -= forward.y * t * factor;
						position.z -= forward.z * t * factor;
					}
					}
				if (d)
					{
					position.x -= side.x * t * factor;
					position.y -= side.y * t * factor;
					position.z -= side.z * t * factor;
					bool topRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool topLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool bottomRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					bool bottomLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					if (topRight || topLeft || bottomLeft || bottomRight)
					{
						position.x += side.x * t * factor;
						position.y += side.y * t * factor;
						position.z += side.z * t * factor;
					}
					}
				if (a)
					{
					position.x += side.x * t * factor;
					position.y += side.y * t * factor;
					position.z += side.z * t * factor;
					bool topRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool topLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z - 0.25f), *sphere_positions);
					bool bottomRight = levl->check_col(XMFLOAT3(-position.x - 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					bool bottomLeft = levl->check_col(XMFLOAT3(-position.x + 0.25f, -position.y, -position.z + 0.25f), *sphere_positions);
					if (topRight || topLeft || bottomLeft || bottomRight)
					{
						position.x -= side.x * t * factor;
						position.y -= side.y * t * factor;
						position.z -= side.z * t * factor;
					}
					}
				}



			XMMATRIX get_matrix(XMMATRIX *view)
				{
				XMMATRIX R, T;
				R = XMMatrixRotationY(rotation.y) * XMMatrixRotationX(rotation.x);
				T = XMMatrixTranslation(position.x, position.y, position.z);
				return T*(*view)*R;
				}
		};


		class RenderTextureClass
		{
		private:
			ID3D11Texture2D*			m_renderTargetTexture;
			ID3D11Texture3D*			m_renderTargetTexture3D;
			ID3D11RenderTargetView*		m_renderTargetView;
			ID3D11DepthStencilView*		m_DepthStencilView;
			ID3D11UnorderedAccessView*  m_pUAVs;
			bool uav;
		public:
			ID3D11ShaderResourceView*	m_shaderResourceView;
			RenderTextureClass()
			{
				uav = FALSE;
				m_renderTargetTexture3D = NULL;
				m_renderTargetTexture = NULL;
				m_renderTargetView = NULL;
				m_shaderResourceView = NULL;
				m_DepthStencilView = NULL;
				m_pUAVs = NULL;
			}
			RenderTextureClass(const RenderTextureClass&) {}
			~RenderTextureClass() { Shutdown(); }
			ID3D11RenderTargetView* GetRenderTarget() { return m_renderTargetView; }
			bool Initialize(ID3D11Device* device, HWND hwnd, int width = -1, int height = -1, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = FALSE);
			bool Initialize_depth(ID3D11Device* device, HWND hwnd, int width = -1, int height = -1);
			bool Initialize_3DTex(ID3D11Device* device, int width, int height, int depth, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = FALSE);
			void Shutdown();
			ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }
			ID3D11ShaderResourceView* GetShaderResourceView();
			ID3D11UnorderedAccessView*  GetUAV() { return m_pUAVs; }

		};

		class RenderTargetSwapChain
		{
		private:
			RenderTextureClass *RTD;
			RenderTextureClass *RTP;
			RenderTextureClass *RTN;
			RenderTextureClass *sRTD;
			RenderTextureClass *sRTP;
			RenderTextureClass *sRTN;
		public:
			bool swapped;
			RenderTextureClass *RTT;
			RenderTextureClass *sRTT;
			static const int TEXTURE = 0;
			static const int DEPTH = 1;
			static const int POSITION = 2;
			static const int NORMAL = 3;
			RenderTargetSwapChain(ID3D11Device* g_pd3dDevice, HWND g_hWnd);
			void getCurrentTargets(RenderTextureClass **out);
			void getCurrentSRVs(RenderTextureClass **out);
			void Present();
		};