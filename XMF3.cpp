
#include "groundwork.h"
// XMFLOAT3 a,b; a = a + b;
XMFLOAT3 operator+(const XMFLOAT3 lhs, XMFLOAT3 rhs)
{
	XMFLOAT3 res;
	res.x = lhs.x + rhs.x;
	res.y = lhs.y + rhs.y;
	res.z = lhs.z + rhs.z;
	return res;
}
// XMFLOAT3 a,b; a = a - b;
XMFLOAT3 operator-(const XMFLOAT3 lhs, XMFLOAT3 rhs)
{
	XMFLOAT3 res;
	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	return res;
}
//multiplication with matrix
// XMFLOAT3 a; XMMATRIX M;a = M*a;
XMFLOAT3 operator*(const XMMATRIX &lhs, XMFLOAT3 rhs)
{
	XMFLOAT3 res;

	XMVECTOR f = XMLoadFloat3(&rhs);
	f = XMVector3TransformCoord(f, lhs);
	XMStoreFloat3(&res, f);
	return res;
}
// XMFLOAT3 a; XMMATRIX M;a = M*a;
XMFLOAT3 operator*(XMFLOAT3 rhs, const XMMATRIX &lhs)
{
	XMFLOAT3 res;

	XMVECTOR f = XMLoadFloat3(&rhs);
	f = XMVector3TransformCoord(f, lhs);
	XMStoreFloat3(&res, f);
	return res;
}
// XMFLOAT3 a; a = a*0.01;
XMFLOAT3 operator*(const XMFLOAT3 lhs, float rhs)
{
	XMFLOAT3 res;
	res.x = lhs.x * rhs;
	res.y = lhs.y * rhs;
	res.z = lhs.z * rhs;
	return res;
}
// XMFLOAT3 a; a = 0.01*a;
XMFLOAT3 operator*(float rhs, const XMFLOAT3 lhs)
{
	XMFLOAT3 res;
	res.x = lhs.x * rhs;
	res.y = lhs.y * rhs;
	res.z = lhs.z * rhs;
	return res;
}

bool operator==(const XMFLOAT3 lhs, XMFLOAT3 rhs)
{
	return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
}


//********************************
//struct D3DXVECTOR3 {float x,y,z;};
int D3D_intersect_RayTriangle(Ray R, XMFLOAT3 A, XMFLOAT3 B, XMFLOAT3 C, XMFLOAT3* I)
{
	XMFLOAT3    u, v, n;             // triangle vectors
	XMFLOAT3    dir, w0, w;          // ray vectors

									 //	D3DXVec3Normalize(&R.P1,&R.P1);

	float     r, a, b;             // params to calc ray-plane intersect

								   // get triangle edge vectors and plane normal
								   //a,b,c V0,1,2
	u = B - A;
	v = C - A;
	n = Vec3Cross(u, v);             // cross product
	if (n == XMFLOAT3(0, 0, 0))            // triangle is degenerate
		return -1;                 // do not deal with this case

	dir = R.P1 - R.P0;//R.P1 - R.P0;             // ray direction vector
	w0 = R.P0 - A;


	a = -Vec3Dot(n, w0);
	b = Vec3Dot(n, dir);
	if (fabs(b) < 0.00001)
	{     // ray is parallel to triangle plane
		if (a == 0)                // ray lies in triangle plane
			return 2;
		else return 0;             // ray disjoint from plane
	}

	// get intersect point of ray with triangle plane
	r = a / b;
	if (r < 0.0)
	{
		dir = dir * -1;
		//w0 = R.P0 - A;
		//a = -dot(n,w0);
		b = Vec3Dot(n, dir);
		r = a / b;
	}// ray goes away from triangle

	 // for a segment, also test if (r > 1.0) => no intersect

	XMFLOAT3 RESS = R.P0 + r * dir;           // intersect point of ray and plane
	
	*I = RESS;
								   // is I inside T?
	float    uu, uv, vv, wu, wv, D;
	uu = Vec3Dot(u, u);
	uv = Vec3Dot(u, v);
	vv = Vec3Dot(v, v);
	w = *I - A;
	wu = Vec3Dot(w, u);
	wv = Vec3Dot(w, v);
	D = uv * uv - uu * vv;

	// get and test parametric coords
	float s, t;
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0 || s > 1.0)        // I is outside T
		return 0;
	t = (uv * wu - uu * wv) / D;
	if (t < 0.0 || (s + t) > 1.0)  // I is outside T
		return 0;

	return 1;                      // I is in T
}
/******************FOR THE LEVEL***************/
XMFLOAT3 mul(XMFLOAT3 v, XMMATRIX &M)
{
	XMVECTOR f = XMLoadFloat3(&v);
	f = XMVector3TransformCoord(f, M);
	XMStoreFloat3(&v, f);
	v.x += M._41;
	v.y += M._42;
	v.z += M._43;
	return v;
}

XMFLOAT2 get_level_tex_coords(int pic, XMFLOAT2 coords)
{
	float one = 1. / (float)TEXPARTS;
	int x = pic % TEXPARTS;
	int y = pic / (float)TEXPARTS;
	coords.x /= (float)TEXPARTS;
	coords.y /= (float)TEXPARTS;
	coords.x += one*x;
	coords.y += one*y;
	return coords;
}