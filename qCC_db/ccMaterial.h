//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#ifndef CC_MATERIAL_HEADER
#define CC_MATERIAL_HEADER

#include <string>
#include <vector>
#include <memory>

struct Rgb
{
    unsigned char r, g, b;
};

struct Image
{
    Image() : _width(0), _height(0) {}
    int _width, _height;
    std::vector<Rgb> _data;
    bool isNull() const { return _width == 0 || _height == 0; }
    int width() const { return _width; }
    int height() const { return _height; }
    Rgb pixel(int x, int y) const { return _data[y * _width + x]; }
};

//! Mesh (triangle) material
#ifdef QCC_DB_USE_AS_DLL
#include "qCC_db_dll.h"
struct QCC_DB_DLL_API ccMaterial
#else
struct ccMaterial
#endif
{
	typedef std::shared_ptr<ccMaterial> Shared;

	std::string name;
	Image texture;
	float diffuseFront[4];
	float diffuseBack[4];
	float ambient[4];
	float specular[4];
	float emission[4];
	float shininessFront;
	float shininessBack;

	unsigned texID;

	/*float reflect;
	float refract;
	float trans;
	float glossy;
	float refract_index;
	//*/

	//! Default constructor
	ccMaterial(std::string name = "default");

	//! Copy constructor
	ccMaterial(const ccMaterial& mtl);

	//! Sets diffuse color (both front and back)
	void setDiffuse(const float color[4]);

	//! Sets shininess (both front - 100% - and back - 80%)
	void setShininess(float val);

	//! Sets transparency (all colors)
	void setTransparency(float val);

	//! Apply parameters (OpenGL)
	void applyGL(bool lightEnabled, bool skipDiffuse) const;
};

#endif //CC_MATERIAL_HEADER
