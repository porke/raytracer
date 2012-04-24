#include "Scene.h"
#include "BaseUtils\BaseCore.h"

using namespace nprt;
using namespace std;


Scene::Scene() : m_Triangles(), m_ToneMappingKey(0.0f)
{ }


void Scene::LoadGeometry(const char* filename)
{
	int vertSize = NULL;
	ifstream file;
	string line, token;
	stringstream lineStream;
	vector<Point3d> vertices;
	Point3d minDomain, maxDomain;

	maxDomain.x = maxDomain.y = maxDomain.z = -numeric_limits<float>::infinity();
	minDomain.x = minDomain.y = minDomain.z = numeric_limits<float>::infinity();
	
	file.open(filename);

	if (file.is_open()) 
	{	
		while (!file.eof()) 
		{			
			getline(file, line);
			lineStream.clear();
			lineStream.str(line);
			
			while (lineStream >> token) 
			{				
				if (token == "vertices" && !lineStream.eof()) 
				{
					lineStream >> vertSize;
					vertices = vector<Point3d>(vertSize);
					
					for (int i=0; i<vertSize; i++) 
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						
						float t1, t2, t3;
						lineStream >> t1;
						lineStream >> t2;
						lineStream >> t3;

						minDomain.x = t1 < minDomain.x ? t1 : minDomain.x;
						minDomain.y = t2 < minDomain.y ? t2 : minDomain.y;
						minDomain.z = t3 < minDomain.z ? t3 : minDomain.z;

						maxDomain.x = t1 > maxDomain.x ? t1 : maxDomain.x;
						maxDomain.y = t2 > maxDomain.y ? t2 : maxDomain.y;
						maxDomain.z = t3 > maxDomain.z ? t3 : maxDomain.z;
						
						vertices[i] = Point3d(t1, t2, t3);
					}
				}
				
				int trianSize = 0;
				if (token == "triangles" && !lineStream.eof()) 
				{					
					lineStream >> trianSize;
					m_Triangles.reserve(trianSize);
					
					for (int i=0; i<trianSize; i++) 
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						int v1, v2, v3;
						lineStream >> v1;
						lineStream >> v2;
						lineStream >> v3;
						
						m_Triangles.push_back(Triangle(vertices[v1], vertices[v2], vertices[v3], i));
					}
				}

				if (token == "parts" && !lineStream.eof()) 
				{
					int i=0;
					int ind;
					const int numTriangles = m_Triangles.size();

					while (i < numTriangles) 
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);

						while (i < numTriangles && lineStream >> ind)
						{
							m_Triangles[i].setMaterialIndex(ind);
							i++;
						}
					}
				}
			}			
		}
	}
	file.close();
	
	m_Octree.buildTree(m_Triangles, minDomain, maxDomain);
}

void Scene::LoadAttributes(const char* filePath) 
{
	ifstream file;
	string line, token;
	stringstream lineStream;

	float kd, ks, wg, ka, r, g, b, kt, eta, kr;
	kd = ks = wg = ka = r = g = b = kt = eta = kr = 0.0f;

	file.open(filePath);

	if (file.is_open())
	{
		getline(file, line);
		lineStream.clear();
		lineStream.str(line);

		int attributeNumber = 0;
		lineStream >> attributeNumber;			
		getline(file, line);
		while(!file.eof())
		{
			for (int i=0; i<attributeNumber; i++) 
			{
				getline(file, line);
				getline(file, line);
				getline(file, line);
				lineStream.clear();
				lineStream.str(line);
				while(lineStream>>token) 
				{
					if (token=="kd")		
					{
						lineStream >> kd;
					}
					else if (token=="ks")		
					{
						lineStream >> ks;
					}
					else if (token=="gs")		
					{
						lineStream >> wg;
					}
					else if (token=="color")	
					{
						lineStream >> r;
						lineStream >> g;
						lineStream >> b;
					}
					else if (token=="kts")	
					{
						lineStream >> kt;
					}
					else if (token=="eta")	
					{
						lineStream >> eta;
					}
					else if (token=="kf")		
					{
						lineStream >> kr;
					}
					else if (token=="ka")		
					{
						lineStream >> ka;
					}
					else if (token=="enddef") 
					{
						Material m(kd, ks, wg, ka, r / 255.0f, g / 255.0f, b / 255.0f, kt, eta, kr);
						m.kdcG = kd;
						m.kdcB = kd;
						m.kscG = ks;
						m.kscB = ks;
						m.krcG = kt;
						m.krcB = kt;
						m.kacR = m.kacG = m.kacB = ka;
						m_Materials.push_back(m);

						break;
					}
					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
				}
				getline(file, line);
			}
			break;
		}
	}
	file.close();
}

void Scene::RenderToFile(const char* filename, int width, int height) const
{
	Vector3d floatColor(0, 0, 0);
	FIBITMAP* dib = FreeImage_Allocate(width, height, 24);
	const int numPixels = width * height;
	Vector3d* pixels = new Vector3d[numPixels];
	
	const Point3d observerPos(m_Camera.cameraCenter.x, m_Camera.cameraCenter.y, m_Camera.cameraCenter.z);
	m_Octree.setObserverPoint(observerPos);

	Point3d U, V, ul, P_ij;
	for (int y = 0; y < height; y++)
	{
		cout << "Completed: " << (100 * (y + 1) / height) << "%\r";
		for (int x = 0; x < width; x++)
		{
			// Calculate the ray direction based on the magic equations from the lecture
			U = m_Camera.topRight - m_Camera.topLeft;
			V = m_Camera.bottomLeft - m_Camera.topLeft;
			ul = m_Camera.topLeft;
			P_ij = ul + U * (static_cast<float>(x) / (width - 1)) + V * (static_cast<float>(y) / (height - 1));

			Vector3d rayDirection = P_ij - observerPos;
			rayDirection.normalize();
			CalculateColor(rayDirection, observerPos, floatColor);

			pixels[y * width + x].x = floatColor.x;
			pixels[y * width + x].y = floatColor.y;
			pixels[y * width + x].z = floatColor.z;
		}
	}

	// Do some tone mapping
	if(m_ToneMappingKey > FLT_EPSILON)
	{
		PerformToneMapping(pixels, numPixels);
	}

	//Determine the maximum colour value	
	RGBQUAD color = {0};

	// Normalize the colours
	for(int p = 0; p < numPixels; ++p)
	{
		pixels[p].x = Utils::Clamp(pixels[p].x, 0.0f, 1.0f);
		pixels[p].y = Utils::Clamp(pixels[p].y, 0.0f, 1.0f);
		pixels[p].z = Utils::Clamp(pixels[p].z, 0.0f, 1.0f);

		color.rgbRed = static_cast<BYTE>(255 * pixels[p].x);
		color.rgbGreen = static_cast<BYTE>(255 * pixels[p].y);
		color.rgbBlue = static_cast<BYTE>(255 * pixels[p].z);
		FreeImage_SetPixelColor(dib, p % width, height - p / width, &color);
	}

	Utils::SafeDeleteArr(pixels);

	FreeImage_Save(FIF_PNG, dib, filename, PNG_Z_BEST_SPEED);
	FreeImage_Unload(dib);
}

void Scene::CalculateColor(const Vector3d& rayDirection, const Vector3d& observerPos, Vector3d& in_color) const
{
	const int numLights = m_Lights.size();
	in_color.x = in_color.y = in_color.z = 0.0f;

	// Trace the ray in Octree
	vector<pair<Triangle, Point3d>> intersectedTriangles;
	vector<pair<Triangle, Point3d>> lightIntTriangles;
	m_Octree.traceRayForTriangles(observerPos, rayDirection, intersectedTriangles);

	if(intersectedTriangles.size() > 0)
	{
		const Triangle& hitTriangle = intersectedTriangles[0].first;
		const Vector3d intersectionPt = intersectedTriangles[0].second;
		const Material& material = m_Materials[hitTriangle.materialIndex];
		in_color.x = material.r;
		in_color.y = material.g;
		in_color.z = material.b;

		float valRed, valGreen, valBlue;
		for(int lgt = 0; lgt < numLights; ++lgt)
		{
			std::pair<Triangle, Point3d> intPair;		// shadow intersection pair
			const LightSource& light = m_Lights[lgt];
			Vector3d lgtDir = (intersectionPt - light.position);			
			lgtDir.normalize();			

			// Perform a shadow cast from the intersection point
			m_Octree.traceRayForTriangles(intersectionPt, lgtDir, lightIntTriangles);
			for(int tri = 0; tri < lightIntTriangles.size(); ++tri)
			{
				if(hitTriangle.ind == lightIntTriangles[tri].first.ind)
				{
					lightIntTriangles.erase(lightIntTriangles.begin() + tri);
					--tri;
				}
			}

			// If there are no intersections, add the light
			if(lightIntTriangles.size() == 0)
			{
				Vector3d hVec = (lgtDir + rayDirection) * 0.5f;
				hVec.normalize();

				// Calculate the diffuse component
				float intensityDiffuse = hitTriangle.norm.dotProduct(lgtDir);

				// Value clamping is being done after the rendering
				valRed = material.kdcR * intensityDiffuse * light.r;
				in_color.x += valRed;
				valGreen = material.kdcG * intensityDiffuse * light.g;
				in_color.y += valGreen;
				valBlue = material.kdcB * intensityDiffuse * light.b;
				in_color.z += valBlue;

				// Calculate the specular component
				float intensitySpecular = hitTriangle.norm.dotProduct(hVec);

				valRed = material.kscR * intensitySpecular;
				in_color.x += valRed;
				valGreen = material.kscG * intensitySpecular;
				in_color.y += valGreen;
				valBlue = material.kscB * intensitySpecular;
				in_color.z += valBlue;	
			}
		}
	}
	else
	{
		in_color *= 0.97f;
	}
}

void Scene::PerformToneMapping(Vector3d* pixels, const int numPixels) const
{
	// HDR logarithm scaling
	// Calculate the low average luminance
	const float rgbToLum[] = {0.27f, 0.67f, 0.06f};
	const float delta = 0.01f;
	float lw = 0.0f;
	float lsum = 0.0f;
	float pixelLum = 0.0f;
	for(int p = 0; p < numPixels; ++p)
	{
		pixels[p].x = Utils::Clamp(pixels[p].x, 0.0f, std::numeric_limits<float>::max());
		pixels[p].y = Utils::Clamp(pixels[p].y, 0.0f, std::numeric_limits<float>::max());
		pixels[p].z = Utils::Clamp(pixels[p].z, 0.0f, std::numeric_limits<float>::max());

		pixelLum = rgbToLum[0] * pixels[p].x 
				 + rgbToLum[1] * pixels[p].y 
				 + rgbToLum[2] * pixels[p].z;

		lsum += log(delta + pixelLum);
	}
	
	lw = 1.0f / numPixels * lsum;

	// Apply the luminance scaling operator
	float* luminance = new float[numPixels];	
	for(int p = 0; p < numPixels; ++p)
	{
		pixelLum = rgbToLum[0] * pixels[p].x 
				 + rgbToLum[1] * pixels[p].y 
				 + rgbToLum[2] * pixels[p].z;
		luminance[p] = m_ToneMappingKey * pixelLum / lw;
		luminance[p] /= (1.0f + luminance[p]);
		
		pixels[p] *= luminance[p];
		
		pixels[p].x /= (1.0f + pixels[p].x);
		pixels[p].y /= (1.0f + pixels[p].y);
		pixels[p].z /= (1.0f + pixels[p].z);
	}

	Utils::SafeDeleteArr(luminance);
}

void Scene::LoadLights(const char* filename)
{
	ifstream file;
	string line, token;
	stringstream lineStream;
	float x, y, z, flux, r, g, b;
	int lightCount = 0;

	file.open(filename);

	if (file.is_open()) 
	{
		while(!file.eof()) 
		{
			getline(file, line);
			lineStream.clear();
			lineStream.str(line);
			while(lineStream >> token) 
			{
				if (token == "lights")
				{
					getline(file, line);
					lineStream.clear();
					lineStream.str(line);

					lineStream >> lightCount;
				}

				if (token == "Position")
				{
					for (int i = 0; i < lightCount; i++)
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);

						lineStream >> x;
						lineStream >> y;
						lineStream >> z;
						lineStream >> flux;
						lineStream >> r;
						lineStream >> g;
						lineStream >> b;

						m_Lights.push_back(LightSource(x, y, z, flux, r, g, b));
					}
				}
			}
		}
	}
	file.close();
}

void Scene::LoadCamera(const char* filePath) 
{
	ifstream file;
	string line, token;
	stringstream lineStream;
	Point3d cameraCenter, topLeft, bottomLeft, topRight;
	int xRes = -1, yRes = -1;

	file.open(filePath);

	if (file.is_open()) 
	{
		while(!file.eof())
		{
			getline(file, line);
			lineStream.clear();
			lineStream.str(line);
			while (lineStream >> token)
			{
				if (token == "viewpoint")
				{
					float x, y, z;
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					cameraCenter = Point3d(x, y, z);
				}

				if (token == "screen")
				{
					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
					float x, y, z;
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					topLeft = Point3d(x, y, z);

					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					topRight = Point3d(x, y, z);

					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					bottomLeft = Point3d(x, y, z);
				}

				if (token == "resolution")
				{
					lineStream >> xRes;
					lineStream >> yRes;
				}
			}
		}
	}
	file.close();
	m_Camera = Camera(cameraCenter, topLeft, bottomLeft, topRight, xRes, yRes);
}

