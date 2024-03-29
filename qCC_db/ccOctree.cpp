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

#include "ccOctree.h"

//Local
#include "ccGenericPointCloud.h"
#include "ccNormalVectors.h"
#include "ccBox.h"

//CCLib
#include <ScalarFieldTools.h>
#include <Neighbourhood.h>

ccOctree::ccOctree(ccGenericPointCloud* aCloud)
	: CCLib::DgmOctree(aCloud)
	, ccHObject("Octree")
	, m_associatedCloud(aCloud)
	, m_displayType(DEFAULT_OCTREE_DISPLAY_TYPE)
	, m_displayedLevel(1)
	, m_glListID(-1)
	, m_shouldBeRefreshed(true)
{
	setVisible(false);
	lockVisibility(false);
}

void ccOctree::setDisplayedLevel(int level)
{
	if (level != m_displayedLevel)
	{
		m_displayedLevel = level;
		m_shouldBeRefreshed = true;
	}
}

void ccOctree::setDisplayType(CC_OCTREE_DISPLAY_TYPE type)
{
	if (m_displayType != type)
	{
		m_displayType = type;
		m_shouldBeRefreshed = true;
	}
}

void ccOctree::clear()
{
	if (m_glListID >= 0)
	{
		if (glIsList(m_glListID))
			glDeleteLists(m_glListID,1);
		m_glListID=-1;
	}

	DgmOctree::clear();
}

ccBBox ccOctree::getMyOwnBB()
{
	return ccBBox(m_pointsMin,m_pointsMax);
}

ccBBox ccOctree::getDisplayBB()
{
	return ccBBox(m_dimMin,m_dimMax);
}

void ccOctree::multiplyBoundingBox(const PointCoordinateType multFactor)
{
	m_dimMin *= multFactor;
	m_dimMax *= multFactor;
	m_pointsMin *= multFactor;
	m_pointsMax *= multFactor;

	for (int i=0;i<=MAX_OCTREE_LEVEL;++i)
		m_cellSize[i] *= multFactor;
}

void ccOctree::translateBoundingBox(const CCVector3& T)
{
	m_dimMin += T;
	m_dimMax += T;
	m_pointsMin += T;
	m_pointsMax += T;
}

void ccOctree::drawMeOnly(CC_DRAW_CONTEXT& context)
{
	if (m_thePointsAndTheirCellCodes.empty())
		return;

	if (MACRO_Draw3D(context))
	{
		bool pushName = MACRO_DrawEntityNames(context);

		if (pushName)
		{
			//not fast at all!
			if (MACRO_DrawFastNamesOnly(context))
				return;
			glPushName(getUniqueID());
		}

		RenderOctreeAs(m_displayType,this,m_displayedLevel,m_associatedCloud,m_glListID,m_shouldBeRefreshed);

		if (m_shouldBeRefreshed)
			m_shouldBeRefreshed = false;

		if (pushName)
			glPopName();
	}
}

/*** RENDERING METHODS ***/

void ccOctree::RenderOctreeAs(  CC_OCTREE_DISPLAY_TYPE octreeDisplayType,
								CCLib::DgmOctree* theOctree,
								unsigned char level,
								ccGenericPointCloud* theAssociatedCloud,
								int &octreeGLListID,
								bool updateOctreeGLDisplay)
{
	if (!theOctree || !theAssociatedCloud)
		return;

	glPushAttrib(GL_LIGHTING_BIT);

	if (octreeDisplayType==WIRE)
	{
		//cet affichage demande trop de memoire pour le stocker sous forme de liste OpenGL
		//donc on doit le generer dynamiquement
		
		glDisable(GL_LIGHTING); //au cas où la lumiere soit allumee
		glColor3ubv(ccColor::green);
		theOctree->executeFunctionForAllCellsAtLevel(level,&DrawCellAsABox,NULL);
	}
	else
	{
		glDrawParams glParams;
		theAssociatedCloud->getDrawingParameters(glParams);

		if (glParams.showNorms)
		{
			//DGM: Strangely, when Qt::renderPixmap is called, the OpenGL version is sometimes 1.0!
            glEnable(GL_RESCALE_NORMAL);
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,	  CC_DEFAULT_CLOUD_AMBIENT_COLOR  );
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  CC_DEFAULT_CLOUD_SPECULAR_COLOR );
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   CC_DEFAULT_CLOUD_DIFFUSE_COLOR  );
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  CC_DEFAULT_CLOUD_EMISSION_COLOR );
            glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, CC_DEFAULT_CLOUD_SHININESS);
			glEnable(GL_LIGHTING);

			glEnable(GL_COLOR_MATERIAL);
			glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		}

		if (!glParams.showColors)
			glColor3ubv(ccColor::white);

		if (updateOctreeGLDisplay || octreeGLListID<0)
		{
			if (octreeGLListID<0)
				octreeGLListID = glGenLists(1);
			else if (glIsList(octreeGLListID))
				glDeleteLists(octreeGLListID,1);
			glNewList(octreeGLListID,GL_COMPILE);

			if (octreeDisplayType == MEAN_POINTS)
			{
				void* additionalParameters[2] = {	(void*)&glParams,
													(void*)theAssociatedCloud,
				};

				glBegin(GL_POINTS);
				theOctree->executeFunctionForAllCellsAtLevel(level,&DrawCellAsAPoint,additionalParameters,0,"Render octree");
				glEnd();
			}
			else
			{
				//by default we use a box as primitive
				PointCoordinateType cs = theOctree->getCellSize(level);
				CCVector3 dims(cs,cs,cs);
				ccBox box(dims);
				box.showColors(glParams.showColors || glParams.showSF);
				box.showNormals(glParams.showNorms);

				//trick: replace all normal indexes so that they point on the first one
				{
					if (box.arePerTriangleNormalsEnabled())
						for (unsigned i=0;i<box.size();++i)
							box.setTriangleNormalIndexes(i,0,0,0);
				}

				//fake context
				CC_DRAW_CONTEXT context;
				context.flags = CC_DRAW_3D | CC_DRAW_FOREGROUND| CC_LIGHT_ENABLED;
				context._win = 0;

				void* additionalParameters[4] = {	(void*)&glParams,
													(void*)theAssociatedCloud,
													(void*)&box,
													(void*)&context
				};

				theOctree->executeFunctionForAllCellsAtLevel(level,&DrawCellAsAPrimitive,additionalParameters,0);
			}

			glEndList();
		}

		glCallList(octreeGLListID);

		if (glParams.showNorms)
		{
			glDisable(GL_COLOR_MATERIAL);
			glDisable(GL_RESCALE_NORMAL);
			glDisable(GL_LIGHTING);
		}
	}

	glPopAttrib();
}

//FONCTION "CELLULAIRE" D'AFFICHAGE "FIL DE FER"
bool ccOctree::DrawCellAsABox(const CCLib::DgmOctree::octreeCell& cell, void** additionalParameters)
{
	CCVector3 bbMin,bbMax;
	cell.parentOctree->computeCellLimits(cell.truncatedCode,cell.level,bbMin.u,bbMax.u,true);

	glColor3ubv(ccColor::green);
	glBegin(GL_LINE_LOOP);
	glVertex3fv(bbMin.u);
	glVertex3f(bbMax.x,bbMin.y,bbMin.z);
	glVertex3f(bbMax.x,bbMax.y,bbMin.z);
	glVertex3f(bbMin.x,bbMax.y,bbMin.z);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(bbMin.x,bbMin.y,bbMax.z);
	glVertex3f(bbMax.x,bbMin.y,bbMax.z);
	glVertex3fv(bbMax.u);
	glVertex3f(bbMin.x,bbMax.y,bbMax.z);
	glEnd();

	glBegin(GL_LINES);
	glVertex3fv(bbMin.u);
	glVertex3f(bbMin.x,bbMin.y,bbMax.z);
	glVertex3f(bbMax.x,bbMin.y,bbMin.z);
	glVertex3f(bbMax.x,bbMin.y,bbMax.z);
	glVertex3f(bbMax.x,bbMax.y,bbMin.z);
	glVertex3fv(bbMax.u);
	glVertex3f(bbMin.x,bbMax.y,bbMin.z);
	glVertex3f(bbMin.x,bbMax.y,bbMax.z);
	glEnd();

	return true;
}

//FONCTION "CELLULAIRE" D'AFFICHAGE "POINT MOYEN"
bool ccOctree::DrawCellAsAPoint(const CCLib::DgmOctree::octreeCell& cell, void** additionalParameters)
{
	//variables additionnelles
	glDrawParams* glParams						= (glDrawParams*)additionalParameters[0];
	ccGenericPointCloud* theAssociatedCloud		= (ccGenericPointCloud*)additionalParameters[1];

	if (glParams->showSF)
	{
		ScalarType dist = CCLib::ScalarFieldTools::computeMeanScalarValue(cell.points);
		const colorType* col = theAssociatedCloud->geScalarValueColor(dist);
		glColor3ubv(col ? col : ccColor::lightGrey);
	}
	else if (glParams->showColors)
	{
		colorType col[3];
		ComputeAverageColor(cell.points,theAssociatedCloud,col);
		glColor3ubv(col);
	}

	if (glParams->showNorms)
	{
		GLfloat N[3];
		ComputeAverageNorm(cell.points,theAssociatedCloud,N);
		glNormal3fv(N);
	}

	const CCVector3* gravityCenter = CCLib::Neighbourhood(cell.points).getGravityCenter();
	glVertex3fv(gravityCenter->u);

	return true;
}

//FONCTION "CELLULAIRE" D'AFFICHAGE "CUBE MOYEN"
bool ccOctree::DrawCellAsAPrimitive(const CCLib::DgmOctree::octreeCell& cell, void** additionalParameters)
{
	//variables additionnelles
	glDrawParams* glParams						= (glDrawParams*)additionalParameters[0];
	ccGenericPointCloud* theAssociatedCloud	    = (ccGenericPointCloud*)additionalParameters[1];
	ccGenericPrimitive*	primitive				= (ccGenericPrimitive*)additionalParameters[2];
	CC_DRAW_CONTEXT* context					= (CC_DRAW_CONTEXT*)additionalParameters[3];

	GLfloat cellCenter[3];
	cell.parentOctree->computeCellCenter(cell.truncatedCode,cell.level,cellCenter,true);

	if (glParams->showSF)
	{
		ScalarType dist = CCLib::ScalarFieldTools::computeMeanScalarValue(cell.points);
		const colorType* col = theAssociatedCloud->geScalarValueColor(dist);
		primitive->setColor(col);
	}
	else if (glParams->showColors)
	{
		colorType col[3];
		ComputeAverageColor(cell.points,theAssociatedCloud,col);
		primitive->setColor(col);
	}

	if (glParams->showNorms)
	{
		GLfloat N[3];
		ComputeAverageNorm(cell.points,theAssociatedCloud,N);
		if (primitive->getTriNormsTable())
			primitive->getTriNormsTable()->setValue(0,ccNormalVectors::GetNormIndex(N));
	}

	glPushMatrix();
	glTranslatef(cellCenter[0],cellCenter[1],cellCenter[2]);
	primitive->draw(*context);
	glPopMatrix();

	return true;
}

void ccOctree::ComputeAverageColor(CCLib::ReferenceCloud* subset, ccGenericPointCloud* sourceCloud, colorType meanCol[])
{
	if (!subset || subset->size()==0 || !sourceCloud)
		return;

	assert(sourceCloud->hasColors());
	assert(subset->getAssociatedCloud() == static_cast<CCLib::GenericIndexedCloud*>(sourceCloud));

	double Rsum=0.0;
	double Gsum=0.0;
	double Bsum=0.0;

	unsigned n=subset->size();
	for (unsigned i=0;i<n;++i)
	{
		const colorType* _theColors = sourceCloud->getPointColor(subset->getPointGlobalIndex(i));
		Rsum += (double)*_theColors++;
		Gsum += (double)*_theColors++;
		Bsum += (double)*_theColors++;
	}

	meanCol[0] = colorType( Rsum/(double)n );
	meanCol[1] = colorType( Gsum/(double)n );
	meanCol[2] = colorType( Bsum/(double)n );
}

void ccOctree::ComputeAverageNorm(CCLib::ReferenceCloud* subset, ccGenericPointCloud* sourceCloud, float norm[])
{
	if (!subset || subset->size()==0 || !sourceCloud)
		return;

	assert(sourceCloud->hasNormals());
	assert(subset->getAssociatedCloud() == static_cast<CCLib::GenericIndexedCloud*>(sourceCloud));

	norm[0] = norm[1] = norm[2] = 0.0f;

	unsigned n=subset->size();
	for (unsigned i=0;i<n;++i)
	{
		const PointCoordinateType* N = sourceCloud->getPointNormal(subset->getPointGlobalIndex(i));
		CCVector3::vadd(norm,N,norm);
	}

	CCVector3::vnormalize(norm);
}

void ccOctree::ComputeRobustAverageNorm(CCLib::ReferenceCloud* subset, ccGenericPointCloud* sourceCloud, PointCoordinateType norm[])
{
	if (!subset || subset->size()==0 || !sourceCloud)
		return;

	assert(sourceCloud->hasNormals());
	assert(subset->getAssociatedCloud() == static_cast<CCLib::GenericIndexedCloud*>(sourceCloud));

	//we compute the (least square) best fit plane
	CCVector3 Nplane;
	const PointCoordinateType* Nlsq = CCLib::Neighbourhood(subset).getLSQPlane();
	if (Nlsq)
	{
		Nplane = CCVector3(Nlsq);
	}
	else //not enough points?
	{
		//we simply take the first one!
		const PointCoordinateType* N = sourceCloud->getPointNormal(subset->getPointGlobalIndex(0));
		Nplane = CCVector3(N);
	}

	//new we can sum up the vectors, using the plane normal as reference
	norm[0] = norm[1] = norm[2] = 0.0f;
	unsigned n=subset->size();
	for (unsigned i=0;i<n;++i)
	{
		const PointCoordinateType* N = sourceCloud->getPointNormal(subset->getPointGlobalIndex(0));
		//calcul du produit scalaire entre la ieme normale et la normale du plan aux moindres carres
		//(pour savoir de quel cote pointe la normale)
		float ps = CCVector3::vdot(N,Nplane.u);
		if (ps < 0)
		{
			CCVector3::vsubstract(norm,N,norm);
		}
		else
		{
			CCVector3::vadd(norm,N,norm);
		}
	}

	CCVector3::vnormalize(norm);
}
