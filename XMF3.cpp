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