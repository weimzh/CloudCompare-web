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

//Always first
#include "ccIncludeGL.h"

#include "ccClipBox.h"

//qCC_db
#include "ccGenericPointCloud.h"
#include "ccCylinder.h"
#include "ccCone.h"
#include "ccSphere.h"
#include "ccTorus.h"

//system
#include <assert.h>
#include <memory>

//Components geometry
static std::shared_ptr<ccCylinder> c_arrowShaft(0);
static std::shared_ptr<ccCone> c_arrowHead(0);
static std::shared_ptr<ccSphere> c_centralSphere(0);
static std::shared_ptr<ccTorus> c_torus(0);

static void DrawUnitArrow(int ID, const CCVector3& start, const CCVector3& direction, PointCoordinateType scale, const colorType* col, CC_DRAW_CONTEXT& context)
{
	if (ID>0)
		glLoadName(ID);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

	glTranslatef(start.x,start.y,start.z);
	glScalef(scale,scale,scale);

    //we compute scalar prod between the two vectors
	CCVector3 Z(0.0,0.0,1.0);
    float ps = Z.dot(direction);
	
	if (ps < 1.0)
	{
		CCVector3 axis(1.0,0.0,0.0);
		float angle_deg = 180.0;
		
		if (ps>-1.0)
		{
			//we deduce angle from scalar prod
			angle_deg = acos(ps)*CC_RAD_TO_DEG;

			//we compute rotation axis with scalar prod
			axis = Z.cross(direction);
		}
		
		glRotatef(angle_deg, axis.x, axis.y, axis.z);
	}

	if (!c_arrowShaft)
		c_arrowShaft = std::make_shared<ccCylinder>(0.15f,0.6f,nullptr,"ArrowShaft",12);
	if (!c_arrowHead)
		c_arrowHead = std::make_shared<ccCone>(0.3f,0.0f,0.4f,0,0,nullptr,"ArrowHead",24);

	glTranslatef(0.0f,0.0f,0.3f);
	c_arrowShaft->setTempColor(col);
	c_arrowShaft->draw(context);
	glTranslatef(0.0f,0.0f,0.3f+0.2f);
	c_arrowHead->setTempColor(col);
	c_arrowHead->draw(context);

	glPopMatrix();
}

static void DrawUnitTorus(int ID, const CCVector3& center, const CCVector3& direction, PointCoordinateType scale, const colorType* col, CC_DRAW_CONTEXT& context)
{
	if (ID>0)
		glLoadName(ID);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

	glTranslatef(center.x,center.y,center.z);
	glScalef(scale,scale,scale);

    //we compute scalar prod between the two vectors
	CCVector3 Z(0.0,0.0,1.0);
    float ps = Z.dot(direction);
	
	if (ps < 1.0)
	{
		CCVector3 axis(1.0,0.0,0.0);
		float angle_deg = 180.0;
		
		if (ps>-1.0)
		{
			//we deduce angle from scalar prod
			angle_deg = acos(ps)*CC_RAD_TO_DEG;

			//we compute rotation axis with scalar prod
			axis = Z.cross(direction);
		}
		
		glRotatef(angle_deg, axis.x, axis.y, axis.z);
	}

	if (!c_torus)
		c_torus = std::make_shared<ccTorus>(0.2f,0.4f,2.0*M_PI,false,0,nullptr,"Torus",12);

	glTranslatef(0.0f,0.0f,0.3f);
	c_torus->setTempColor(col);
	c_torus->draw(context);

	glPopMatrix();
}

static void DrawUnitSphere(int ID, const CCVector3& center, PointCoordinateType radius, const colorType* col, CC_DRAW_CONTEXT& context)
{
	if (ID>0)
		glLoadName(ID);
	
	glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

	glTranslatef(center.x,center.y,center.z);
	glScalef(radius,radius,radius);

	if (!c_centralSphere)
		c_centralSphere = std::make_shared<ccSphere>(1.0f,nullptr,"CentralSphere",24);
	c_centralSphere->setTempColor(col);
	c_centralSphere->draw(context);

	glPopMatrix();
}

static void DrawUnitCross(int ID, const CCVector3& center, PointCoordinateType scale, const colorType* col, CC_DRAW_CONTEXT& context)
{
	if (ID>0)
		glLoadName(ID);
	
	scale /= 2.0;
	DrawUnitArrow(0, center, CCVector3(-1.0, 0.0, 0.0), scale, col, context);
	DrawUnitArrow(0, center, CCVector3( 1.0, 0.0, 0.0), scale, col, context);
	DrawUnitArrow(0, center, CCVector3( 0.0,-1.0, 0.0), scale, col, context);
	DrawUnitArrow(0, center, CCVector3( 0.0, 1.0, 0.0), scale, col, context);
	DrawUnitArrow(0, center, CCVector3( 0.0, 0.0,-1.0), scale, col, context);
	DrawUnitArrow(0, center, CCVector3( 0.0, 0.0, 1.0), scale, col, context);
}

ccClipBox::ccClipBox(ccHObject* associatedEntity, std::string name/*= "clipping box"*/)
	: ccHObject(name)
	, m_associatedEntity(0)
	, m_activeComponent(NONE)
	, boxModified(NULL)
{
	setSelectionBehavior(SELECTION_IGNORED);

	setAssociatedEntity(associatedEntity);
}

ccClipBox::~ccClipBox()
{
	setAssociatedEntity(0);
}

void ccClipBox::reset()
{
	m_box.clear();
	razGLTransformation();

	if (m_associatedEntity)
	{
		m_box = m_associatedEntity->getBB();
	}

	update();

	//send 'modified' signal
	/*emit*/if (boxModified) boxModified(&m_box);
}

void ccClipBox::setAssociatedEntity(ccHObject* associatedEntity)
{
	//release precedent one
	if (m_associatedEntity && m_associatedEntity->isKindOf(CC_POINT_CLOUD))
	{
		static_cast<ccGenericPointCloud*>(m_associatedEntity)->unallocateVisibilityArray();
	}
	m_associatedEntity = 0;

	//try to initialize new one
	if (associatedEntity)
	{
		if (!associatedEntity->isKindOf(CC_POINT_CLOUD))
		{
			ccLog::Error("Unhandled entity! Clipping box will be deactivated...");
		}
		else
		{
			if (static_cast<ccGenericPointCloud*>(associatedEntity)->razVisibilityArray())
			{
				m_associatedEntity = associatedEntity;
			}
			else
			{
				ccLog::Error("Not enough memory! Clipping box will be deactivated...");
			}
		}
	}

	reset();
}

void ccClipBox::setActiveComponent(int id)
{
	switch(id)
	{
	case 1:
		m_activeComponent = X_MINUS_ARROW;
		break;
	case 2:
		m_activeComponent = X_PLUS_ARROW;
		break;
	case 3:
		m_activeComponent = Y_MINUS_ARROW;
		break;
	case 4:
		m_activeComponent = Y_PLUS_ARROW;
		break;
	case 5:
		m_activeComponent = Z_MINUS_ARROW;
		break;
	case 6:
		m_activeComponent = Z_PLUS_ARROW;
		break;
	case 7:
		m_activeComponent = CROSS;
		break;
	case 8:
		m_activeComponent = SPHERE;
		break;
	case 9:
		m_activeComponent = X_MINUS_TORUS;
		break;
	case 10:
		m_activeComponent = Y_MINUS_TORUS;
		break;
	case 11:
		m_activeComponent = Z_MINUS_TORUS;
		break;
	case 12:
		m_activeComponent = X_PLUS_TORUS;
		break;
	case 13:
		m_activeComponent = Y_PLUS_TORUS;
		break;
	case 14:
		m_activeComponent = Z_PLUS_TORUS;
		break;
	default:
		m_activeComponent = NONE;
	}
}

static CCVector3 PointToVector(int x, int y, int screenWidth, int screenHeight)
{
	//convert mouse position to vector (screen-centered)
	CCVector3 v;
	v.x = float(2.0 * std::max(std::min(x,screenWidth-1),-screenWidth+1) - screenWidth) / (float)screenWidth;
	v.y = float(screenHeight - 2.0 * std::max(std::min(y,screenHeight-1),-screenHeight+1)) / (float)screenHeight;
	v.z = 0;

	//square 'radius'
	float d2 = v.x*v.x + v.y*v.y;

	//projection on the unit sphere
	if (d2 > 1.0f)
	{
		float d = sqrt(d2);
		v.x /= d;
		v.y /= d;
	}
	else
	{
		v.z = (PointCoordinateType)sqrt(1.0f-d2);
	}

	return v;
}

bool ccClipBox::move2D(int x, int y, int dx, int dy, int screenWidth, int screenHeight)
{
	if (m_activeComponent != SPHERE || !m_box.isValid())
		return false;

	//convert mouse position to vector (screen-centered)
	CCVector3 currentOrientation = PointToVector(x,y,screenWidth,screenHeight);

	ccGLMatrix rotMat = ccGLMatrix::FromToRotation(m_lastOrientation,currentOrientation);

	CCVector3 C = m_box.getCenter();

	ccGLMatrix transMat;
	transMat.setTranslation(-C);
	transMat = rotMat * transMat;
	transMat.setTranslation(CCVector3(transMat.getTranslation())+C);

	//rotateGL(transMat);
    m_glTrans = transMat.inverse() * m_glTrans;
    enableGLTransformation(true);

	m_lastOrientation = currentOrientation;

	update();

	return true;
}

void ccClipBox::setClickedPoint(int x, int y, int screenWidth, int screenHeight, const ccGLMatrix& viewMatrix)
{
	m_lastOrientation = PointToVector(x,y,screenWidth,screenHeight);
	m_viewMatrix = viewMatrix;
}

bool ccClipBox::move3D(const CCVector3& uInput)
{
	if (m_activeComponent == NONE || !m_box.isValid())
		return false;

	CCVector3 u = uInput;

	//Arrows
	if (m_activeComponent >= X_MINUS_ARROW && m_activeComponent <= CROSS)
	{
		if (m_glTransEnabled)
			m_glTrans.inverse().applyRotation(u);

		switch(m_activeComponent)
		{
		case X_MINUS_ARROW:
			m_box.minCorner().x += u.x;
			if (m_box.minCorner().x > m_box.maxCorner().x)
				m_box.minCorner().x = m_box.maxCorner().x;
			break;
		case X_PLUS_ARROW:
			m_box.maxCorner().x += u.x;
			if (m_box.minCorner().x > m_box.maxCorner().x)
				m_box.maxCorner().x = m_box.minCorner().x;
			break;
		case Y_MINUS_ARROW:
			m_box.minCorner().y += u.y;
			if (m_box.minCorner().y > m_box.maxCorner().y)
				m_box.minCorner().y = m_box.maxCorner().y;
			break;
		case Y_PLUS_ARROW:
			m_box.maxCorner().y += u.y;
			if (m_box.minCorner().y > m_box.maxCorner().y)
				m_box.maxCorner().y = m_box.minCorner().y;
			break;
		case Z_MINUS_ARROW:
			m_box.minCorner().z += u.z;
			if (m_box.minCorner().z > m_box.maxCorner().z)
				m_box.minCorner().z = m_box.maxCorner().z;
			break;
		case Z_PLUS_ARROW:
			m_box.maxCorner().z += u.z;
			if (m_box.minCorner().z > m_box.maxCorner().z)
				m_box.maxCorner().z = m_box.minCorner().z;
			break;
		case CROSS:
			m_box.minCorner() += u;
			m_box.maxCorner() += u;
			break;
		default:
			assert(false);
			return false;
		}
		
		//send 'modified' signal
		/*emit*/if (boxModified) boxModified(&m_box);
	}
	else if (m_activeComponent == SPHERE)
	{
		//handled by move2D!
		return false;
	}
	else if (m_activeComponent >= X_MINUS_TORUS && m_activeComponent <= Z_PLUS_TORUS)
	{
		//we guess the rotation order by comparing the current screen 'normal'
		//and the vector prod of u and the current rotation axis
		CCVector3 Rb(0.0,0.0,0.0);
		switch(m_activeComponent)
		{
		case X_MINUS_TORUS:
			Rb.x = -1.0;
			break;
		case X_PLUS_TORUS:
			Rb.x = 1.0;
			break;
		case Y_MINUS_TORUS:
			Rb.y = -1.0;
			break;
		case Y_PLUS_TORUS:
			Rb.y = 1.0;
			break;
		case Z_MINUS_TORUS:
			Rb.z = -1.0;
			break;
		case Z_PLUS_TORUS:
			Rb.z = 1.0;
			break;
		default:
			assert(false);
			return false;
		}
		
		CCVector3 R = Rb;
		if (m_glTransEnabled)
			m_glTrans.applyRotation(R);

		CCVector3 RxU = R.cross(u);

		//look for the most parallel dimension
		int minDim = 0;
		PointCoordinateType maxDot = CCVector3(m_viewMatrix.getColumn(0)).dot(RxU);
		for (int i=1;i<3;++i)
		{
			PointCoordinateType dot = CCVector3(m_viewMatrix.getColumn(i)).dot(RxU);
			if (fabs(dot) > fabs(maxDot))
			{
				maxDot = dot;
				minDim = i;
			}
		}

		//angle is proportional to absolute displacement
		PointCoordinateType angle_rad = u.norm()/m_box.getDiagNorm() * M_PI;
		if (maxDot < 0.0)
			angle_rad = -angle_rad;

		ccGLMatrix rotMat;
		rotMat.initFromParameters(angle_rad,Rb,CCVector3(0.0,0.0,0.0));

		CCVector3 C = m_box.getCenter();
		ccGLMatrix transMat;
		transMat.setTranslation(-C);
		transMat = rotMat * transMat;
		transMat.setTranslation(CCVector3(transMat.getTranslation())+C);

		m_glTrans = m_glTrans * transMat.inverse();
		enableGLTransformation(true);
	}
	else
	{
		assert(false);
		return false;
	}

	update();

	return true;
}

void ccClipBox::setBox(const ccBBox& box)
{
	m_box = box;

	update();

	//send 'modified' signal
	/*emit*/if (boxModified) boxModified(&m_box);
}

void ccClipBox::shift(const CCVector3& v)
{
	m_box.minCorner() += v;
	m_box.maxCorner() += v;
		
	update();

	//send 'modified' signal
	/*emit*/if (boxModified) boxModified(&m_box);
}

void ccClipBox::update(bool shrink/*=false*/)
{
	if (!m_associatedEntity || !m_associatedEntity->isKindOf(CC_POINT_CLOUD))
		return;

	ccGenericPointCloud* cloud = static_cast<ccGenericPointCloud*>(m_associatedEntity);
	unsigned count = cloud->size();
	if (count==0 || !cloud->isVisibilityTableInstantiated())
	{
		assert(false);
		return;
	}

	ccGenericPointCloud::VisibilityTableType* visTable = cloud->getTheVisibilityArray();

	if (m_glTransEnabled)
	{
		ccGLMatrix transMat;
		//CCVector3 C = m_box.getCenter();
		//transMat.setTranslation(-C);
		//transMat = m_glTrans.inverse() * transMat;
		//transMat.setTranslation(CCVector3(transMat.getTranslation())+C);
		transMat = m_glTrans.inverse();

		for (unsigned i=0; i<count; ++i)
		{
			if (!shrink || visTable->getValue(i) == POINT_VISIBLE)
			{
				CCVector3 P = *cloud->getPoint(i);
				transMat.apply(P);
				visTable->setValue(i,m_box.contains(P) ? POINT_VISIBLE : POINT_HIDDEN);
			}
		}
	}
	else
	{
		for (unsigned i=0; i<count; ++i)
		{
			if (!shrink || visTable->getValue(i) == POINT_VISIBLE)
			{
				const CCVector3* P = cloud->getPoint(i);
				visTable->setValue(i,m_box.contains(*P) ? POINT_VISIBLE : POINT_HIDDEN);
			}
		}
	}
}

ccBBox ccClipBox::getMyOwnBB()
{
	return m_box;
}

ccBBox ccClipBox::getDisplayBB()
{
	ccBBox bbox = m_box;

	PointCoordinateType scale = computeArrowsScale();
	bbox.minCorner() -= CCVector3(scale,scale,scale);
	bbox.maxCorner() += CCVector3(scale,scale,scale);

	return bbox;
}

PointCoordinateType ccClipBox::computeArrowsScale() const
{
	PointCoordinateType scale = m_box.getDiagNorm()/(PointCoordinateType)10.0;

	if (m_associatedEntity)
		scale = std::max<PointCoordinateType>(scale,m_associatedEntity->getMyOwnBB().getDiagNorm()/(PointCoordinateType)100.0);

	return scale;
}

const colorType c_lightComp = MAX_COLOR_COMP/2;
const colorType c_lightRed[3]	= {MAX_COLOR_COMP,c_lightComp,c_lightComp};
const colorType c_lightGreen[3]	= {c_lightComp,MAX_COLOR_COMP,c_lightComp};
const colorType c_lightBlue[3]	= {c_lightComp,c_lightComp,MAX_COLOR_COMP};
void ccClipBox::drawMeOnly(CC_DRAW_CONTEXT& context)
{
	if (!MACRO_Draw3D(context))
		return;

	if (!m_box.isValid())
		return;

	//m_box.draw(m_selected ? context.bbDefaultCol : ccColor::magenta);
	m_box.draw(ccColor::yellow);
	
	//standard case: list names pushing
	bool pushName = MACRO_DrawEntityNames(context);
	if (pushName)
		glPushName(getUniqueID());

	if (m_selected)
	{
		//draw the interactors
		const CCVector3& minC = m_box.minCorner();
		const CCVector3& maxC = m_box.maxCorner();
		CCVector3 center = m_box.getCenter();
	
		PointCoordinateType scale = computeArrowsScale();

		//custom arrow 'context'
		CC_DRAW_CONTEXT componentContext = context;
		componentContext.flags &= (~CC_DRAW_ENTITY_NAMES); //we must remove the 'push name flag' so that the arows don't push their own!
		componentContext._win = 0;

		//1 if names shall be pushed, 0 otherwise
		if (pushName)
			glPushName(0); //fake ID, will be replaced by the arrows one if any

		DrawUnitArrow(X_MINUS_ARROW*pushName,CCVector3(minC.x,center.y,center.z),CCVector3(-1.0, 0.0, 0.0),scale,ccColor::red,componentContext);
		DrawUnitArrow(X_PLUS_ARROW*pushName,CCVector3(maxC.x,center.y,center.z),CCVector3( 1.0, 0.0, 0.0),scale,ccColor::red,componentContext);
		DrawUnitArrow(Y_MINUS_ARROW*pushName,CCVector3(center.x,minC.y,center.z),CCVector3( 0.0,-1.0, 0.0),scale,ccColor::green,componentContext);
		DrawUnitArrow(Y_PLUS_ARROW*pushName,CCVector3(center.x,maxC.y,center.z),CCVector3( 0.0, 1.0, 0.0),scale,ccColor::green,componentContext);
		DrawUnitArrow(Z_MINUS_ARROW*pushName,CCVector3(center.x,center.y,minC.z),CCVector3( 0.0, 0.0,-1.0),scale,ccColor::blue,componentContext);
		DrawUnitArrow(Z_PLUS_ARROW*pushName,CCVector3(center.x,center.y,maxC.z),CCVector3( 0.0, 0.0, 1.0),scale,ccColor::blue,componentContext);
		DrawUnitCross(CROSS*pushName,minC-CCVector3(scale,scale,scale)/2.0,scale,ccColor::yellow,componentContext);
		//DrawUnitSphere(SPHERE*pushName,maxC+CCVector3(scale,scale,scale)/2.0,scale/2.0,ccColor::yellow,componentContext);
		DrawUnitTorus(X_MINUS_TORUS*pushName,CCVector3(minC.x,center.y,center.z),CCVector3(-1.0, 0.0, 0.0),scale,c_lightRed,componentContext);
		DrawUnitTorus(Y_MINUS_TORUS*pushName,CCVector3(center.x,minC.y,center.z),CCVector3( 0.0,-1.0, 0.0),scale,c_lightGreen,componentContext);
		DrawUnitTorus(Z_MINUS_TORUS*pushName,CCVector3(center.x,center.y,minC.z),CCVector3( 0.0, 0.0,-1.0),scale,c_lightBlue,componentContext);
		DrawUnitTorus(X_PLUS_TORUS*pushName,CCVector3(maxC.x,center.y,center.z),CCVector3( 1.0, 0.0, 0.0),scale,c_lightRed,componentContext);
		DrawUnitTorus(Y_PLUS_TORUS*pushName,CCVector3(center.x,maxC.y,center.z),CCVector3( 0.0, 1.0, 0.0),scale,c_lightGreen,componentContext);
		DrawUnitTorus(Z_PLUS_TORUS*pushName,CCVector3(center.x,center.y,maxC.z),CCVector3( 0.0, 0.0, 1.0),scale,c_lightBlue,componentContext);

		if (pushName)
			glPopName();
	}

	if (pushName)
		glPopName();
}
