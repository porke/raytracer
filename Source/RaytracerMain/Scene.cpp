#include "Scene.h"
#include "BaseUtils\BaseCore.h"
#include <Windows.h>
#include <omp.h>
#include <cmath>
#include <numeric>
#include "Math/Math.h"

using namespace nprt;
using namespace std;

static const float REDIRECTED_RAY_MIRROR_FACTOR = 0.001f;


Scene::Scene() : m_Triangles(), m_ToneMappingKey(0.0f), m_WallTexture(1176, 1216, TextureType::Bricks)
{ }

void Scene::LoadScene(const char* filename)
{
	m_Triangles.clear();
	m_Materials.clear();
	m_Lights.clear();

	ifstream file;
	string line, token;
	stringstream lineStream;

	Vector3d* vertices;
	Vector3d minDomain, maxDomain;

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
				if (token == "points_count" && !lineStream.eof())
				{
					int verticesCount;
					lineStream >> verticesCount;
					vertices = new Vector3d[verticesCount];

					for (int i = 0; i < verticesCount; ++i)
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						
						float t1, t2, t3;
						lineStream >> t3;
						lineStream >> t2;
						lineStream >> t1;

						minDomain.x = t1 < minDomain.x ? t1 : minDomain.x;
						minDomain.y = t2 < minDomain.y ? t2 : minDomain.y;
						minDomain.z = t3 < minDomain.z ? t3 : minDomain.z;

						maxDomain.x = t1 > maxDomain.x ? t1 : maxDomain.x;
						maxDomain.y = t2 > maxDomain.y ? t2 : maxDomain.y;
						maxDomain.z = t3 > maxDomain.z ? t3 : maxDomain.z;
						
						vertices[i] = Vector3d(t1, t2, t3);
					}
				}
				else if (token == "triangles_count" && !lineStream.eof())
				{
					int trianglesCount;
					lineStream >> trianglesCount;
					m_Triangles.reserve(trianglesCount);
					
					for (int i = 0; i < trianglesCount; ++i) 
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
				else if (token == "parts_count" && !lineStream.eof())
				{
					int i=0;
					int ind;
					const int numTriangles = m_Triangles.size();

					while (i < numTriangles) 
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);

						while (lineStream >> ind) 
						{
							m_Triangles[i].setMaterialIndex(ind);
							i++;
						}
					}
				}
				else if (token == "materials_count" && !lineStream.eof())
				{
					int materialsCount;
					lineStream >> materialsCount;
					m_Materials.reserve(materialsCount);

					int i = 0;
					Material material;

					while (i < materialsCount)
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						lineStream >> token;

						if (token == "mat_name")
						{
							if (i > 0)
							{
								m_Materials.push_back(material);
							}

							material.texture = 0;
							lineStream >> material.name;
							i++;
						}
						else if (token == "rgb")
						{
							lineStream >> material.r;
							lineStream >> material.g;
							lineStream >> material.b;
						}
						else if (token == "kdCr")
							lineStream >> material.kdc;
						else if (token == "kdCg")
							lineStream >> material.kdc;
						else if (token == "kdCb")
							lineStream >> material.kdc;
						else if (token == "ksCr")
							lineStream >> material.ksc;
						else if (token == "ksCg")
							lineStream >> material.ksc;
						else if (token == "ksCb")
							lineStream >> material.ksc;
						else if (token == "kaCr")
							lineStream >> material.kac;
						else if (token == "kaCg")
							lineStream >> material.kac;
						else if (token == "kaCb")
							lineStream >> material.kac;
						else if(token == "texture")
							lineStream >> material.texture;
						else if(token == "kt")
							lineStream >> material.kt;
						else if(token == "eta")
							lineStream >> material.eta;
					}
					m_Materials.push_back(material);
				}
				else if (token == "texcoord_count" && !lineStream.eof())
				{
					int texcoordCount;
					lineStream >> texcoordCount;
					
					getline(file, line);
					lineStream.clear();
					lineStream.str(line);

					for (int i = 0; i < texcoordCount; ++i)
					{
						int triangle;
						float u1, v1, u2, v2, u3, v3;
						lineStream >> triangle;
						
						lineStream >> u1;
						lineStream >> v1;
						lineStream >> u2;
						lineStream >> v2;
						lineStream >> u3;
						lineStream >> v3;

						m_Triangles[triangle].SetTexcoords(u1, v1, u2, v2, u3, v3);

						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
					}
				}
				else if (token == "lights_count" && !lineStream.eof())
				{
					int lightsCount;
					lineStream >> lightsCount;

					// Move the reader to the first light name
					while (token != "light_name")
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						lineStream >> token;
					}

					for (int i = 0; i < lightsCount; ++i)
					{
						LightSource lightSource;
						
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);
						lineStream >> token;

						// Read light data
						while (token != "gonio_count")
						{
							getline(file, line);
							lineStream.clear();
							lineStream.str(line);
							lineStream >> token;

							if (token == "rgb" && !lineStream.eof())
							{
								lineStream >> lightSource.r;
								lineStream >> lightSource.g;
								lineStream >> lightSource.b;
							}
							else if (token == "pos" && !lineStream.eof())
							{
								lineStream >> lightSource.position.z;
								lineStream >> lightSource.position.y;
								lineStream >> lightSource.position.x;
							}
							else if (token == "power" && !lineStream.eof())
							{
								lineStream >> lightSource.power;
							}
						}

						m_Lights.push_back(lightSource);
					}
				}				
				else if (token == "cam_name" && !lineStream.eof())
				{
					Vector3d pos, lookAt;
					float fov = 0, rotation = 0;
					int resX = 0, resY = 0;
					
					bool usingOldFormat = false;
					Vector3d topLeft, topRight, bottomLeft;
					float x, y, z;

					int params = 0;
					while (params < 9)
					{
						getline(file, line);
						lineStream.clear();
						lineStream.str(line);

						lineStream >> token;
						{
							params++;

							if (token == "pos")
							{
								lineStream >> pos.z;
								lineStream >> pos.y;
								lineStream >> pos.x;
							}
							else if (token == "lookAt")
							{
								lineStream >> lookAt.z;
								lineStream >> lookAt.y;
								lineStream >> lookAt.x;
							}
							else if (token == "resolution")
							{
								lineStream >> resX;
								lineStream >> resY;
							}
							else if (token == "rotation")
							{
								lineStream >> rotation;
								break;
							}
							else if (token == "fov")
								lineStream >> fov;
							
							// old format camera
							else if (token == "viewpoint")
							{
								usingOldFormat = true;

								lineStream >> x;
								lineStream >> y;
								lineStream >> z;
								pos = Vector3d(x, y, z);
							}

							else if (token == "screen_topLeft")
							{
								lineStream >> x;
								lineStream >> y;
								lineStream >> z;
								topLeft = Vector3d(x, y, z);
							}
							else if (token == "screen_topRight")
							{
								lineStream >> x;
								lineStream >> y;
								lineStream >> z;
								topRight = Vector3d(x, y, z);
							}
							else if (token == "screen_bottomLeft")
							{							
								lineStream >> x;
								lineStream >> y;
								lineStream >> z;
								bottomLeft = Vector3d(x, y, z);
							}
							else
								params--;
						}
					}

					if (usingOldFormat)
					{
						m_Camera.initialize(pos, topLeft, bottomLeft, topRight, resX, resY);
					}
					else
					{
						m_Camera.initialize(pos, lookAt, fov);
						m_Camera.setResolution(resX, resY);
					}
				}
			}
		}
	}
	file.close();

	for (int i = 0, n = m_Triangles.size(); i < n; ++i)
	{
		Triangle& triangle = m_Triangles[i];

		triangle.hasDisplacement = (triangle.texture != 0);
		triangle.texture = &m_WallTexture;
		Vector3d triangleCenter = triangle.p1 + (triangle.p2 - triangle.p1) * 0.5f + (triangle.p3 - triangle.p1) * 0.5f;

		if (triangle.norm.dotProduct(triangleCenter - m_Camera.cameraCenter) > 0)
		{
			const Vector3d tmp(triangle.p1);			
			triangle.setP1(triangle.p3);
			triangle.setP3(tmp);
		}
	}
	delete[] vertices;

	m_Octree.buildTree(m_Triangles, minDomain, maxDomain);
}

void Scene::LoadGeometry(const char* filename)
{
	int vertSize = NULL;
	ifstream file;
	string line, token;
	stringstream lineStream;
	Vector3d* vertices;
	Vector3d minDomain, maxDomain;

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
					vertices = new Vector3d[vertSize];
					
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
						
						vertices[i] = Vector3d(t1, t2, t3);
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
	delete[] vertices;
	
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
			int tex = 0;
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
					else if (token=="ka")		
					{
						lineStream >> ka;
					}
					else if(token=="texture")
					{
						lineStream >> tex;
					}
					else if (token=="enddef") 
					{
						Material m(kd, ks, wg, ka, r / 255.0f, g / 255.0f, b / 255.0f, kt, eta, tex);
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

void Scene::RenderToFile(const char* filename) const
{
	RenderToFile(filename, m_Camera.xResolution, m_Camera.yResolution);
}

void Scene::RenderToFile(const char* filename, int width, int height) const
{
	FIBITMAP* dib = FreeImage_Allocate(width, height, 24);
	const int numPixels = width * height;
	Vector3d* pixels = new Vector3d[numPixels];
	
	const Vector3d observerPos(m_Camera.cameraCenter);
	const Vector3d U = m_Camera.topRight - m_Camera.topLeft;
	const Vector3d V = m_Camera.bottomLeft - m_Camera.topLeft;

	volatile long rowsDoneCount = 0;

	const int linesForOnePercent = height / 100;

#ifndef _DEBUG
	#pragma omp parallel for
#endif
	for (int y = 0; y < height; y++)
	{
		if (rowsDoneCount % linesForOnePercent == 0 || y == height-1)
			cout << "Completed: " << (100 * (static_cast<int>(rowsDoneCount) + 1) / height) << "%\t\t\t\t\t\r";

		Vector3d rayDirection;
		Vector3d floatColor(0, 0, 0);

		for (int x = 0; x < width; x++)
		{
			m_WallTexture.currentX = x;

			// Calculate the ray direction based on the magic equations from the lecture
			Vector3d P_ij = m_Camera.topLeft + U * (static_cast<float>(x) / static_cast<float>(width - 1)) + V * (static_cast<float>(y) / static_cast<float>(height - 1));
			rayDirection = P_ij - observerPos;
			rayDirection.normalize();
			CalculateColor(rayDirection, observerPos, m_NumReflections, floatColor);

			pixels[y * width + x].set(floatColor);
		}

		_InterlockedIncrement(&rowsDoneCount);
	}


	// Do some tone mapping
	if(m_EnableToneMapping)
		PerformToneMapping(pixels, numPixels);

	if(m_EnableGamma)
		PerformGammaCorrection(pixels, numPixels);

	//Determine the maximum colour value	
	RGBQUAD color = {0};

	// Normalize the colours
	for(int p = 0; p < numPixels; ++p)
	{
		Vector3d& pixel = pixels[p];
		pixel.x = Utils::Clamp(pixel.x, 0.0f, 1.0f);
		pixel.y = Utils::Clamp(pixel.y, 0.0f, 1.0f);
		pixel.z = Utils::Clamp(pixel.z, 0.0f, 1.0f);

		BYTE r = Utils::Clamp(static_cast<BYTE>(255.0f * pixel.x), static_cast<BYTE>(0), static_cast<BYTE>(255));
		BYTE g = Utils::Clamp(static_cast<BYTE>(255.0f * pixel.y), static_cast<BYTE>(0), static_cast<BYTE>(255));
		BYTE b = Utils::Clamp(static_cast<BYTE>(255.0f * pixel.z), static_cast<BYTE>(0), static_cast<BYTE>(255));

		color.rgbRed = r;
		color.rgbGreen = g;
		color.rgbBlue = b;
		FreeImage_SetPixelColor(dib, p % width, height - p / width, &color);
	}

	Utils::SafeDeleteArr(pixels);
	FreeImage_Save(FIF_PNG, dib, filename, PNG_Z_BEST_SPEED);
	FreeImage_Unload(dib);
}

inline
void Scene::CalculateColor(	const Vector3d& rayDirection, 
							const Vector3d& observerPos, 
							int numReflections, 
							Vector3d& in_color) const
{
	const int numLights = m_Lights.size();

	// Trace the ray in Octree
	pair<Triangle, AlignedVector3d> intersectedTriangle;
	
	if(m_Octree.castRayForTriangle(observerPos, rayDirection, intersectedTriangle))
	{
		const Triangle& hitTriangle = intersectedTriangle.first;
		const Vector3d intersectionPt = intersectedTriangle.second;// - rayDirection * 0.001f;
		const Vector3d observerDir = -rayDirection;
		const Material& material = m_Materials[hitTriangle.materialIndex];

		if (hitTriangle.norm.dotProduct(rayDirection) > 0)
		{
			// HACK fix for bad data given from dr
			(const_cast<Vector3d*>(&hitTriangle.norm))->set(hitTriangle.norm.x * (-1.0f), hitTriangle.norm.y * (-1.0f), hitTriangle.norm.z * (-1.0f));
		}

		Vector3d reflectedRay = hitTriangle.norm * 2 * observerDir.dotProduct(hitTriangle.norm) - observerDir;
		reflectedRay.normalize();

		if(material.texture == 0)
		{
			// Apply the material
			ApplyMaterialColor(material, in_color);
		}
		else
		{
			// Apply a texture
			ApplyTexture(hitTriangle, intersectedTriangle.second, in_color);
		}

		// Mirror reflection component
		if (m_EnableReflection)
			CalculateReflectionComponent(in_color, intersectionPt, material, reflectedRay, numReflections - 1);
		 
		// Refraction component
		if (m_EnableRefraction)
			CalculateRefractionComponent(in_color, intersectionPt, observerDir, hitTriangle, material, numReflections - 1);

		Vector3d lightIntensity(0, 0, 0);
		for(int lgt = 0; lgt < numLights; ++lgt)
		{
			const LightSource& light = m_Lights[lgt];
			const Vector3d lgtColor(light.r, light.g, light.b);
			const Vector3d lgtPos = light.position;			
			Vector3d lgtDir = (lgtPos - intersectionPt);
			static const float a = 1.1f;
			static const float b = 1.4f;
			static const float c = 0.8f;
			const float d = lgtDir.length();
			float attenuation = 1.0f / (a + b*d + c*d*d) * 2.5f;
			lgtDir.normalize();

			// Perform a shadow cast from the intersection point
			bool shadow = false;
			if(m_EnableShadows)
			{
				pair<Triangle, AlignedVector3d> t;				
				shadow = m_Octree.castRayForTriangle(intersectionPt + lgtDir * REDIRECTED_RAY_MIRROR_FACTOR, lgtDir, t);

				// Check if the intersection does not occur behind the light source
				float intDist = (t.second - intersectionPt).length();
				float lgtDist = (light.position - intersectionPt).length();

				if(shadow && intDist < lgtDist)
				{
					continue;
				}
			}

			// If there are no intersections (shadows) then add the light.

			// Calculate the diffuse component
			float dot = hitTriangle.norm.dotProduct(lgtDir);			
			float diffuseFactor = material.kdc * dot;
			
			if (diffuseFactor > 0) {
				lightIntensity += (in_color * lgtColor * diffuseFactor * attenuation);
			}

			// Calculate specular
			Vector3d lightReflect = hitTriangle.norm * (hitTriangle.norm*2).dotProduct(lgtDir) - lgtDir;
			const float cosFactor = hitTriangle.norm.dotProduct(lightReflect);

			if (cosFactor) {
				lightIntensity += (lgtColor * material.ksc * powf(cosFactor, material.wg) * attenuation);
			}
		}

		// Apply the light component to the output pixel

		const float colorFactor = m_EnableGamma || m_EnableToneMapping ? 1.0f : (1.0f - 1.0f/static_cast<float>(numLights));
		in_color.set(in_color * colorFactor + lightIntensity);
	}
}

inline
void Scene::CalculateReflectionComponent(nprt::Vector3d& in_color, 
										const nprt::Vector3d& intersectionPt,
										const nprt::Material& material,
										const nprt::Vector3d& reflectedRay, 
										int numReflections) const 
{
	if(material.ksc > 0 && numReflections > 0)
	{
		Vector3d in_refl_color(0, 0, 0);
		CalculateColor(reflectedRay, intersectionPt + reflectedRay * REDIRECTED_RAY_MIRROR_FACTOR, numReflections - 1, in_refl_color);
		
		// Scaling of the reflection intensity
		float div = static_cast<float>((m_NumReflections) ? m_NumReflections - 1 : 0);
		float intensity = (m_NumReflections - numReflections) / div;
		in_color += in_refl_color * material.ksc * intensity;
	}
}

inline
void Scene::CalculateRefractionComponent(nprt::Vector3d& in_color, const nprt::Vector3d& intersectionPt, 
										const nprt::Vector3d& observerDir, const nprt::Triangle& hitTriangle, 
										const nprt::Material& material, int numReflections) const 
{
	if(material.kt > 0 && numReflections > 0)
	{
		Vector3d in_refr_color(0, 0, 0);
		Vector3d horz_vec = (observerDir - hitTriangle.norm * observerDir.dotProduct(hitTriangle.norm)) * (-material.eta);

		float ndo = hitTriangle.norm.dotProduct(observerDir);
		float vert_vec_scale = -nprt::math::fsqrt(1 - material.eta * material.eta * (1 - ndo * ndo));
		Vector3d vert_vec = hitTriangle.norm * vert_vec_scale;
		Vector3d refractedRay = horz_vec + vert_vec;
		refractedRay.normalize();

		CalculateColor(refractedRay, intersectionPt + refractedRay * REDIRECTED_RAY_MIRROR_FACTOR, numReflections - 1, in_refr_color);
		in_color += in_refr_color * material.kt;
	}
}

void Scene::PerformGammaCorrection(Vector3d* pixels, const int numPixels) const
{
	Vector3d pixel;
	for(int p = 0; p < numPixels; ++p)
	{
		pixels[p].x = powf(pixels[p].x, 1.0f / m_Gamma);
		pixels[p].y = powf(pixels[p].y, 1.0f / m_Gamma);
		pixels[p].z = powf(pixels[p].z, 1.0f / m_Gamma);
	}

	return;

	for(int p = 0; p < numPixels; ++p)
	{
		pixels[p].x = powf(pixels[p].x, m_Gamma);
		pixels[p].y = powf(pixels[p].y, m_Gamma);
		pixels[p].z = powf(pixels[p].z, m_Gamma);

		pixels[p].x /= (1.0f + pixels[p].x);
		pixels[p].y /= (1.0f + pixels[p].y);
		pixels[p].z /= (1.0f + pixels[p].z);
	}
}

void Scene::PerformToneMapping(Vector3d* pixels, const int numPixels) const
{
	// Logarithmic luminance scaling
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

	//// Apply the luminance scaling operator
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
	float x, y, z, r, g, b, flux;
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

						m_Lights.push_back(LightSource(x, y, z, r, g, b, flux));
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
	Vector3d cameraCenter, topLeft, bottomLeft, topRight;
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
					cameraCenter = Vector3d(x, y, z);
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
					topLeft = Vector3d(x, y, z);

					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					topRight = Vector3d(x, y, z);

					getline(file, line);
					lineStream.clear();
					lineStream.str(line);
					lineStream >> x;
					lineStream >> y;
					lineStream >> z;
					bottomLeft = Vector3d(x, y, z);
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

inline
void Scene::ApplyMaterialColor(const Material& material, Vector3d& in_color) const
{
	in_color.x = material.r * (1.0f - material.kt);
	in_color.y = material.g * (1.0f - material.kt);
	in_color.z = material.b * (1.0f - material.kt);
}

inline
void Scene::ApplyTexture(const Triangle& hitTriangle, const Vector3d& hitPoint, Vector3d& in_color) const
{
	Vector3d barycentric;
	hitTriangle.getUV(hitPoint, barycentric.x, barycentric.y);
	barycentric.z = 1.0f - barycentric.x - barycentric.y;
	
	float u = barycentric.x * hitTriangle.u1 + barycentric.y * hitTriangle.u2 + barycentric.z * hitTriangle.u3;
	float v = barycentric.x * hitTriangle.v1 + barycentric.y * hitTriangle.v2 + barycentric.z * hitTriangle.v3;
	const Vector3d& texel = m_WallTexture.GetTexel(u, v);
	in_color.set(texel);

	hitTriangle.texture = &m_WallTexture;
	//hitTriangle.hasDisplacement = true;
}

inline
bool Scene::ManualRaytrace(const Vector3d& rayPos, const Vector3d& rayDir, std::pair<Triangle, AlignedVector3d>& out) const
{
	float minDist = std::numeric_limits<float>::max();
	float dist = 0;
	int minTri = -1;

	for (int i = 0; i < m_Triangles.size(); ++i)
	{
		const Triangle& tri = m_Triangles[i];
		dist = tri.intersection(rayPos, rayDir);

		if (dist > 0 
			&& dist < minDist)
		{
			minDist = dist;
			minTri = i;
		}
	}

	if (minTri != -1)
	{
		out.first = m_Triangles[minTri];
		out.second = rayPos + rayDir * minDist;
		return true;
	}

	return false;
}
