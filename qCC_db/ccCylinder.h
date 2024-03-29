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

#ifndef CC_CYLINDER_PRIMITIVE_HEADER
#define CC_CYLINDER_PRIMITIVE_HEADER

#include "ccCone.h"
#include <string>

//! Cylinder (primitive)
/** 3D cylinder primitive
**/
#ifdef QCC_DB_USE_AS_DLL
#include "qCC_db_dll.h"
class QCC_DB_DLL_API ccCylinder : public ccCone
#else
class ccCylinder : public ccCone
#endif
{
public:

	//! Default constructor
	/** Cylinder axis corresponds to the 'Z' dimension.
		Internally represented by a cone with the same top and bottom radius.
		\param radius cylinder radius
		\param height cylinder height (transformation should point to the axis center)
		\param transMat optional 3D transformation (can be set afterwards with ccDrawableObject::setGLTransformation)
		\param name name
		\param precision drawing precision (angular step = 360/precision)
	**/
	ccCylinder(PointCoordinateType radius,
				PointCoordinateType height,
				const ccGLMatrix* transMat = 0,
				std::string name = "Cylinder",
				unsigned precision = 24);

	//! Simplified constructor
	/** For ccHObject factory only!
	**/
	ccCylinder(std::string name = "Cylinder");

	//! Returns class ID
	virtual CC_CLASS_ENUM getClassID() const { return CC_CYLINDER; }

	//inherited from ccGenericPrimitive
	virtual std::string getTypeName() const { return "Cylinder"; }
	virtual ccGenericPrimitive* clone() const;

protected:
    
};

#endif //CC_CYLINDER_PRIMITIVE_HEADER
