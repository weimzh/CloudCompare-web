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

#include "ccBox.h"

//qCC_db
#include "ccPlane.h"
#include "ccPointCloud.h"

ccBox::ccBox(const CCVector3& dims, const ccGLMatrix* transMat/*= 0*/, std::string name/*="Box"*/)
	: ccGenericPrimitive(name,transMat)
	, m_dims(dims)
{
	buildUp();
	applyTransformationToVertices();
}

bool ccBox::buildUp()
{
	//upper plane
	ccGLMatrix upperMat;
	upperMat.getTranslation()[2] = m_dims.z/2.0f;
	*this += ccPlane(m_dims.x,m_dims.y,&upperMat);
	//lower plane
	ccGLMatrix lowerMat;
	lowerMat.initFromParameters(-(PointCoordinateType)M_PI,CCVector3(1,0,0),CCVector3(0,0,-m_dims.z/2.0f));
	*this += ccPlane(m_dims.x,m_dims.y,&lowerMat);
	//left plane
	ccGLMatrix leftMat;
	leftMat.initFromParameters(-(PointCoordinateType)(M_PI/2.0),CCVector3(0,1,0),CCVector3(-m_dims.x/2.0f,0,0));
	*this += ccPlane(m_dims.z,m_dims.y,&leftMat);
	//right plane
	ccGLMatrix rightMat;
	rightMat.initFromParameters((PointCoordinateType)(M_PI/2.0),CCVector3(0,1,0),CCVector3(m_dims.x/2.0f,0,0));
	*this += ccPlane(m_dims.z,m_dims.y,&rightMat);
	//front plane
	ccGLMatrix frontMat;
	frontMat.initFromParameters((PointCoordinateType)(M_PI/2.0),CCVector3(1,0,0),CCVector3(0,-m_dims.y/2.0f,0));
	*this += ccPlane(m_dims.x,m_dims.z,&frontMat);
	//back plane
	ccGLMatrix backMat;
	backMat.initFromParameters(-(PointCoordinateType)(M_PI/2.0),CCVector3(1,0,0),CCVector3(0,m_dims.y/2.0f,0));
	*this += ccPlane(m_dims.x,m_dims.z,&backMat);

	return (vertices() && vertices()->size() == 24 && vertices()->hasNormals() && this->size() == 12);
}

ccBox::ccBox(std::string name/*"Box"*/)
	: ccGenericPrimitive(name)
	, m_dims(0,0,0)
{
}

ccGenericPrimitive* ccBox::clone() const
{
	return finishCloneJob(new ccBox(m_dims,&m_transformation,getName()));
}
