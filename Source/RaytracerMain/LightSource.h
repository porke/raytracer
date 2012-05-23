#pragma once

#include "Geometry\Vector3d.h"


namespace nprt
{
	class LightSource
	{	
		public:
			LightSource() {	}
			LightSource(float x, float y, float z, float r, float g, float b);

			Vector3d position;
			float r, g, b;
	};
}
