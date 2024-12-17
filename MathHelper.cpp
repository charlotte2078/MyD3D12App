#include "MathHelper.h"

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = 3.1415926545f;

float MathHelper::AngleFromXY(float x, float y)
{
	float theta = 0.0f;

	// Quadrant 1 or 4
	if (x >= 0.0f)
	{
		theta = atan(y / x); // [-pi/2, +pi.2]

		if (theta < 0.0f)
		{
			theta += 2.0f * Pi; // [0, 2*pi)
		}
	}

	// Quadrant 2 or 3
	else
	{
		theta = atan(y / x) + Pi; // [0, 2*pi)
	}

	return theta;
}

DirectX::XMVECTOR MathHelper::RandUnitVec3()
{
	DirectX::XMVECTOR One = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	DirectX::XMVECTOR Zero = DirectX::XMVectorZero();

	while (true)
	{
		// Random point in the cube between [-1,1] on all axes
		DirectX::XMVECTOR v = DirectX::XMVectorSet(MathHelper::RandF(-1.0f, 1.0f),
			MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);
	
		// Only want a point within the spher of radius 1 centred on origin
		if (DirectX::XMVector3Greater(DirectX::XMVector3LengthSq(v), One))
		{
			continue;
		}

		return DirectX::XMVector3Normalize(v);
	}
}

DirectX::XMVECTOR MathHelper::RandHemishpereUnitVec3(DirectX::XMVECTOR n)
{
	DirectX::XMVECTOR One = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	DirectX::XMVECTOR Zero = DirectX::XMVectorZero();

	while (true)
	{
		// Random point in the cube between [-1,1] on all axes
		DirectX::XMVECTOR v = DirectX::XMVectorSet(MathHelper::RandF(-1.0f, 1.0f),
			MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// Only want a point within the spher of radius 1 centred on origin
		if (DirectX::XMVector3Greater(DirectX::XMVector3LengthSq(v), One))
		{
			continue;
		}

		// Only want points in the top hemisphere
		if (DirectX::XMVector3Less(DirectX::XMVector3Dot(n, v), Zero))
		{
			continue;
		}

		return DirectX::XMVector3Normalize(v);
	}
}
