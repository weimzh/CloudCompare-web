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

#include "ccMesh.h"

//Local
#include "ccGenericPointCloud.h"
#include "ccNormalVectors.h"
#include "ccPointCloud.h"
#include "ccNormalVectors.h"
#include "ccMaterialSet.h"

//CCLib
#include <ManualSegmentationTools.h>
#include <ReferenceCloud.h>

//System
#include <string.h>
#include <assert.h>

ccMesh::ccMesh(ccGenericPointCloud* vertices)
	: ccGenericMesh(vertices,"Mesh")
	, m_triIndexes(0)
	, m_globalIterator(0)
	, m_triMtlIndexes(0)
	, m_materialsShown(false)
	, m_texCoordIndexes(0)
	, m_triNormalIndexes(0)
	, m_triNormsShown(false)
	, m_stippling(false)
{
	m_triIndexes = new triangleIndexesContainer();
	m_triIndexes->link();
}

ccMesh::ccMesh(CCLib::GenericIndexedMesh* giMesh, ccGenericPointCloud* giVertices)
	: ccGenericMesh(giVertices, "Mesh")
	, m_triIndexes(0)
	, m_globalIterator(0)
	, m_triMtlIndexes(0)
	, m_materialsShown(false)
	, m_texCoordIndexes(0)
	, m_triNormalIndexes(0)
	, m_triNormsShown(false)
	, m_stippling(false)
{
	m_triIndexes = new triangleIndexesContainer();
	m_triIndexes->link();

	unsigned i,triNum = giMesh->size();
	if (!reserve(triNum))
		return;

	giMesh->placeIteratorAtBegining();
	for (i=0;i<triNum;++i)
	{
		const CCLib::TriangleSummitsIndexes* tsi = giMesh->getNextTriangleIndexes();
		addTriangle(tsi->i1,tsi->i2,tsi->i3);
	}

	if (!giVertices->hasNormals())
		computeNormals();
	showNormals(true);

	if (giVertices->hasColors())
		showColors(giVertices->colorsShown());

	if (giVertices->hasDisplayedScalarField())
		showSF(giVertices->sfShown());
}

ccMesh::~ccMesh()
{
	if (m_triIndexes)
		m_triIndexes->release();
	if (m_texCoordIndexes)
		m_texCoordIndexes->release();
	if (m_triMtlIndexes)
		m_triMtlIndexes->release();
	if (m_triNormalIndexes)
		m_triNormalIndexes->release();
}

ccGenericMesh* ccMesh::clone(ccGenericPointCloud* vertices/*=0*/,
	ccMaterialSet* clonedMaterials/*=0*/,
	NormsIndexesTableType* clonedNormsTable/*=0*/,
	TextureCoordsContainer* cloneTexCoords/*=0*/)
{
	assert(m_associatedCloud);

	//vertices
	unsigned i,vertNum = m_associatedCloud->size();
	//triangles
	unsigned triNum = size();

	//temporary structure to check that vertices are really used (in case of vertices set sharing)
	unsigned* usedVerts = 0;
	CCLib::TriangleSummitsIndexes* tsi=0;

	ccGenericPointCloud* newVertices = vertices;

	//no input vertices set
	if (!newVertices)
	{
		//let's check the real vertex count
		usedVerts = new unsigned[vertNum];
		if (!usedVerts)
		{
			ccLog::Error("[ccMesh::clone] Not enough memory!");
			return 0;
		}
		memset(usedVerts,0,sizeof(unsigned)*vertNum);

		placeIteratorAtBegining();
		for (i=0;i<triNum;++i)
		{
			tsi = getNextTriangleIndexes();
			usedVerts[tsi->i1]=1;
			usedVerts[tsi->i2]=1;
			usedVerts[tsi->i3]=1;
		}

		//we check that all points in 'associatedCloud' are used by this mesh
		unsigned realVertCount=0;
		for (i=0;i<vertNum;++i)
			usedVerts[i]=(usedVerts[i]==1 ? realVertCount++ : vertNum);

		//the associated cloud is already the exact vertices set --> nothing to change
		if (realVertCount == vertNum)
		{
			newVertices = m_associatedCloud->clone();
		}
		else
		{
			//we create a temporary entity with used vertices only
			CCLib::ReferenceCloud* rc = new CCLib::ReferenceCloud(m_associatedCloud);
			if (rc->reserve(realVertCount))
			{
				for (i=0;i<vertNum;++i)
					if (usedVerts[i]!=vertNum)
						rc->addPointIndex(i); //can't fail, see above

				//and the associated vertices set
				assert(m_associatedCloud->isA(CC_POINT_CLOUD));
				newVertices = static_cast<ccPointCloud*>(m_associatedCloud)->partialClone(rc);
				if (newVertices && newVertices->size() < rc->size())
				{
					//not enough memory!
					delete newVertices;
					newVertices=0;
				}
			}

			delete rc;
			rc=0;
		}
	}

	//failed to create a new vertices set!
	if (!newVertices)
	{
		if (usedVerts)
			delete[] usedVerts;
		ccLog::Error("[ccMesh::clone] Not enough memory!");
		return 0;
	}

	//mesh clone
	ccMesh* cloneMesh = new ccMesh(newVertices);
	if (!cloneMesh->reserve(triNum))
	{
		if (!vertices)
			delete newVertices;
		if (usedVerts)
			delete[] usedVerts;
		delete cloneMesh;
		ccLog::Error("[ccMesh::clone] Not enough memory!");
		return 0;
	}

	//let's create the new triangles
	if (usedVerts) //in case we have an equivalence table
	{
		placeIteratorAtBegining();
		for (i=0;i<triNum;++i)
		{
			tsi = getNextTriangleIndexes();
			cloneMesh->addTriangle(usedVerts[tsi->i1],usedVerts[tsi->i2],usedVerts[tsi->i3]);
		}

		delete[] usedVerts;
		usedVerts=0;
	}
	else
	{
		placeIteratorAtBegining();
		for (i=0;i<triNum;++i)
		{
			tsi = getNextTriangleIndexes();
			cloneMesh->addTriangle(tsi->i1,tsi->i2,tsi->i3);
		}
	}

	//triangle normals
	if (m_triNormals && m_triNormalIndexes)
	{
		//1st: try to allocate per-triangle normals indexes
		if (cloneMesh->reservePerTriangleNormalIndexes())
		{
			//2nd: clone the main array if not already done
			if (!clonedNormsTable)
			{
				clonedNormsTable = m_triNormals->clone(); //TODO: keep only what's necessary!
				if (clonedNormsTable)
					cloneMesh->addChild(clonedNormsTable);
				else
				{
					ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone per-triangle normals!");
					cloneMesh->removePerTriangleNormalIndexes(); //don't need this anymore!
				}
			}

			//if we have both the main array and per-triangle normals indexes, we can finish the job
			if (cloneMesh)
			{
				cloneMesh->setTriNormsTable(clonedNormsTable);
				assert(cloneMesh->m_triNormalIndexes);
				m_triNormalIndexes->copy(*cloneMesh->m_triNormalIndexes); //should be ok as array is already reserved!
			}
		}
		else
		{
			ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone per-triangle normal indexes!");
		}
	}

	//materials
	if (m_materials && m_triMtlIndexes)
	{
		//1st: try to allocate per-triangle materials indexes
		if (cloneMesh->reservePerTriangleMtlIndexes())
		{
			//2nd: clone the main array if not already done
			if (!clonedMaterials)
			{
				clonedMaterials = getMaterialSet()->clone(); //TODO: keep only what's necessary!
				if (clonedMaterials)
				{
					cloneMesh->addChild(clonedMaterials);
				}
				else
				{
					ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone materials set!");
					cloneMesh->removePerTriangleMtlIndexes(); //don't need this anymore!
				}
			}

			//if we have both the main array and per-triangle materials indexes, we can finish the job
			if (clonedMaterials)
			{
				cloneMesh->setMaterialSet(clonedMaterials);
				assert(cloneMesh->m_triMtlIndexes);
				m_triMtlIndexes->copy(*cloneMesh->m_triMtlIndexes); //should be ok as array is already reserved!
			}
		}
		else
		{
			ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone per-triangle materials!");
		}
	}

	//texture coordinates
	if (m_texCoords && m_texCoordIndexes)
	{
		//1st: try to allocate per-triangle texture info
		if (cloneMesh->reservePerTriangleTexCoordIndexes())
		{
			//2nd: clone the main array if not already done
			if (!cloneTexCoords)
			{
				cloneTexCoords = m_texCoords->clone(); //TODO: keep only what's necessary!
				if (cloneTexCoords)
					cloneMesh->addChild(cloneTexCoords);
				else
				{
					ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone texture coordinates!");
					cloneMesh->removePerTriangleTexCoordIndexes(); //don't need this anymore!
				}
			}

			//if we have both the main array and per-triangle texture info, we can finish the job
			if (cloneTexCoords)
			{
				cloneMesh->setTexCoordinatesTable(cloneTexCoords);
				assert(cloneMesh->m_texCoordIndexes);
				m_texCoordIndexes->copy(*cloneMesh->m_texCoordIndexes); //should be ok as array is already reserved!
			}
		}
		else
		{
			ccLog::Warning("[ccMesh::clone] Not enough memory: failed to clone per-triangle texture info!");
		}
	}

	if (!vertices)
	{
		if (hasNormals() && !m_triNormals)
			cloneMesh->computeNormals();
		newVertices->setEnabled(false);
		//we link the mesh structure with the new vertex set
		cloneMesh->addChild(newVertices);
		cloneMesh->setDisplay_recursive(getDisplay());
	}

	//stippling
	cloneMesh->m_stippling = m_stippling;

	cloneMesh->showNormals(normalsShown());
	cloneMesh->showColors(colorsShown());
	cloneMesh->showSF(sfShown());
	cloneMesh->showMaterials(materialsShown());
	cloneMesh->setName(getName()+".clone");
	cloneMesh->setVisible(isVisible());
	cloneMesh->setEnabled(isEnabled());

	return cloneMesh;
}

unsigned ccMesh::size() const
{
	return m_triIndexes->currentSize();
}

unsigned ccMesh::maxSize() const
{
	return m_triIndexes->capacity();
}

//std methods
void ccMesh::forEach(genericTriangleAction& anAction)
{
	m_triIndexes->placeIteratorAtBegining();
	for (unsigned i=0;i<m_triIndexes->currentSize();++i)
	{
		const unsigned* tri = m_triIndexes->getCurrentValue();
		m_currentTriangle.A = m_associatedCloud->getPoint(tri[0]);
		m_currentTriangle.B = m_associatedCloud->getPoint(tri[1]);
		m_currentTriangle.C = m_associatedCloud->getPoint(tri[2]);
		anAction(m_currentTriangle);
		m_triIndexes->forwardIterator();
	}
}

void ccMesh::placeIteratorAtBegining()
{
	m_globalIterator = 0;
}

CCLib::GenericTriangle* ccMesh::_getNextTriangle()
{
	if (m_globalIterator<m_triIndexes->currentSize())
		return _getTriangle(m_globalIterator++);

	return NULL;
}

CCLib::GenericTriangle* ccMesh::_getTriangle(unsigned triangleIndex) //temporary
{
	assert(triangleIndex<m_triIndexes->currentSize());

	const unsigned* tri = m_triIndexes->getValue(triangleIndex);
	m_currentTriangle.A = m_associatedCloud->getPoint(tri[0]);
	m_currentTriangle.B = m_associatedCloud->getPoint(tri[1]);
	m_currentTriangle.C = m_associatedCloud->getPoint(tri[2]);

	return &m_currentTriangle;
}

void ccMesh::getTriangleSummits(unsigned triangleIndex, CCVector3& A, CCVector3& B, CCVector3& C)
{
	assert(triangleIndex<m_triIndexes->currentSize());

	const unsigned* tri = m_triIndexes->getValue(triangleIndex);
	m_associatedCloud->getPoint(tri[0],A);
	m_associatedCloud->getPoint(tri[1],B);
	m_associatedCloud->getPoint(tri[2],C);
}

void ccMesh::refreshBB()
{
	if (!m_associatedCloud)
		return;

	if (!m_bBox.isValid() || getLastModificationTime() < m_associatedCloud->getLastModificationTime_recursive())
	{
		m_bBox.clear();

		unsigned i,count=m_triIndexes->currentSize();
		m_triIndexes->placeIteratorAtBegining();
		for (i=0;i<count;++i)
		{
			const unsigned* tri = m_triIndexes->getCurrentValue();
			assert(tri[0]<m_associatedCloud->size() && tri[1]<m_associatedCloud->size() && tri[2]<m_associatedCloud->size());
			m_bBox.add(*m_associatedCloud->getPoint(tri[0]));
			m_bBox.add(*m_associatedCloud->getPoint(tri[1]));
			m_bBox.add(*m_associatedCloud->getPoint(tri[2]));
			m_triIndexes->forwardIterator();
		}
	}
}

void ccMesh::getBoundingBox(PointCoordinateType bbMin[], PointCoordinateType bbMax[])
{
	refreshBB();

	memcpy(bbMin, m_bBox.minCorner().u, 3*sizeof(PointCoordinateType));
	memcpy(bbMax, m_bBox.maxCorner().u, 3*sizeof(PointCoordinateType));
}

ccBBox ccMesh::getMyOwnBB()
{
	refreshBB();

	return m_bBox;
}

//specific methods
void ccMesh::addTriangle(unsigned i1, unsigned i2, unsigned i3)
{
	CCLib::TriangleSummitsIndexes t(i1,i2,i3);
	m_triIndexes->addElement(t.i);
}

bool ccMesh::reserve(unsigned n)
{
	if (m_triNormalIndexes)
		if (!m_triNormalIndexes->reserve(n))
			return false;

	if (m_triMtlIndexes)
		if (!m_triMtlIndexes->reserve(n))
			return false;

	if (m_texCoordIndexes)
		if (!m_texCoordIndexes->reserve(n))
			return false;

	return m_triIndexes->reserve(n);
}

bool ccMesh::resize(unsigned n)
{
	m_bBox.setValidity(false);
	updateModificationTime();

	if (m_triMtlIndexes)
	{
		if (!m_triMtlIndexes->resize(n,true,-1))
			return false;
	}

	if (m_texCoordIndexes)
	{
		int defaultValues[3]={-1,-1,-1};
		if (!m_texCoordIndexes->resize(n,true,defaultValues))
			return false;
	}

	if (m_triNormalIndexes)
	{
		int defaultValues[3]={-1,-1,-1};
		if (!m_triNormalIndexes->resize(n,true,defaultValues))
			return false;
	}

	return m_triIndexes->resize(n);
}

CCLib::TriangleSummitsIndexes* ccMesh::getTriangleIndexes(unsigned triangleIndex)
{
	return (CCLib::TriangleSummitsIndexes*)m_triIndexes->getValue(triangleIndex);
}

const CCLib::TriangleSummitsIndexes* ccMesh::getTriangleIndexes(unsigned triangleIndex) const
{
	return (CCLib::TriangleSummitsIndexes*)m_triIndexes->getValue(triangleIndex);
}

CCLib::TriangleSummitsIndexes* ccMesh::getNextTriangleIndexes()
{
	if (m_globalIterator<m_triIndexes->currentSize())
		return getTriangleIndexes(m_globalIterator++);

	return NULL;
}

#define GL_SET_NORM(vertexIndex) (glNormal3fv(compressedNormals->getNormal(normalsIndexesTable->getValue(vertexIndex))))

//Vertex indexes for OpenGL "arrays" drawing
static PointCoordinateType s_xyzBuffer[MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*3*3];
static PointCoordinateType s_normBuffer[MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*3*3];
static colorType s_rgbBuffer[MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*3*3];

//static unsigned s_vertIndexes[MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*3];
static unsigned s_vertWireIndexes[MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*6];
static bool s_vertIndexesInitialized = false;
static PointCoordinateType s_blankNorm[3]={0.0,0.0,0.0};

//stipple mask (for semi-transparent display of meshes)
static const GLubyte s_byte0 = 1 | 4 | 16 | 64;
static const GLubyte s_byte1 = 2 | 8 | 32 | 128;
//static const GLubyte s_byte0 = 1 | 16;
//static const GLubyte s_byte1 = 0;
static const GLubyte s_stippleMask[4*32] = {s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1,
	s_byte0,s_byte0,s_byte0,s_byte0,
	s_byte1,s_byte1,s_byte1,s_byte1};

void ccMesh::drawMeOnly(CC_DRAW_CONTEXT& context)
{
	if (!m_associatedCloud)
		return;

	//first, call parent method
	ccGenericMesh::drawMeOnly(context);

	//3D pass
	if (MACRO_Draw3D(context))
	{
		//any triangle?
		unsigned n,triNum=m_triIndexes->currentSize();
		if (triNum==0)
			return;

		//L.O.D.
		bool lodEnabled = (triNum > MAX_LOD_FACES_NUMBER && context.decimateMeshOnMove && MACRO_LODActivated(context));
		int decimStep = (lodEnabled ? (int)ceil((float)triNum*3 / (float)MAX_LOD_FACES_NUMBER) : 1);

		//display parameters
		glDrawParams glParams;
		getDrawingParameters(glParams);
		glParams.showNorms &= bool(MACRO_LightIsEnabled(context));

		//vertices visibility
		const ccGenericPointCloud::VisibilityTableType* verticesVisibility = m_associatedCloud->getTheVisibilityArray();
		bool visFiltering = (verticesVisibility && verticesVisibility->isAllocated());

		//wireframe ? (not compatible with LOD)
		bool showWired = m_showWired && !lodEnabled;

		//per-triangle normals?
		bool showTriNormals = (hasTriNormals() && triNormsShown());

		//materials & textures
		bool applyMaterials = (hasMaterials() && materialsShown());
		bool showTextures = (hasTextures() && materialsShown() && !lodEnabled);

		//GL name pushing
		bool pushName = MACRO_DrawEntityNames(context);
		//special case: triangle names pushing (for picking)
		bool pushTriangleNames = MACRO_DrawTriangleNames(context);
		pushName |= pushTriangleNames;

		if (pushName)
		{
			//not fast at all!
			if (MACRO_DrawFastNamesOnly(context))
				return;
			glPushName(getUniqueID());
			//minimal display for picking mode!
			glParams.showNorms = false;
			glParams.showColors = false;
			//glParams.showSF --> we keep it only if SF 'NaN' values are hidden
			showTriNormals = false;
			applyMaterials = false;
			showTextures = false;
		}

		//in the case we need to display scalar field colors
		ccScalarField* currentDisplayedScalarField = 0;
		bool greyForNanScalarValues = true;
		unsigned colorRampSteps = 0;
		ccColorScale::Shared colorScale(0);

		if (glParams.showSF)
		{
			assert(m_associatedCloud->isA(CC_POINT_CLOUD));
			ccPointCloud* cloud = static_cast<ccPointCloud*>(m_associatedCloud);

			greyForNanScalarValues = (cloud->getCurrentDisplayedScalarField() && cloud->getCurrentDisplayedScalarField()->areNaNValuesShownInGrey());
			if (greyForNanScalarValues && pushName)
			{
				//in picking mode, no need to take SF into account if we don't hide any points!
				glParams.showSF = false;
			}
			else
			{
				currentDisplayedScalarField = cloud->getCurrentDisplayedScalarField();
				colorScale = currentDisplayedScalarField->getColorScale();
				colorRampSteps = currentDisplayedScalarField->getColorRampSteps();

				assert(colorScale);
				//get default color ramp if cloud has no scale associated?!
				if (!colorScale)
					colorScale = ccColorScalesManager::GetUniqueInstance()->getDefaultScale(ccColorScalesManager::BGYR);
			}
		}

		//materials or color?
		bool colorMaterial = false;
		if (glParams.showSF || glParams.showColors)
		{
			applyMaterials = false;
			colorMaterial = true;
			glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
		}

		//in the case we need to display vertex colors
		ColorsTableType* rgbColorsTable = 0;
		if (glParams.showColors)
		{
			if (isColorOverriden())
			{
				glColor3ubv(m_tempColor);
				glParams.showColors = false;
			}
			else
			{
				assert(m_associatedCloud->isA(CC_POINT_CLOUD));
				rgbColorsTable = static_cast<ccPointCloud*>(m_associatedCloud)->rgbColors();
			}
		}
		else
		{
			glColor3fv(context.defaultMat.diffuseFront);
		}

		if (glParams.showNorms)
		{
			//DGM: Strangely, when Qt::renderPixmap is called, the OpenGL version can fall to 1.0!
			glEnable(GL_RESCALE_NORMAL);
			glEnable(GL_LIGHTING);
			context.defaultMat.applyGL(true,colorMaterial);
		}

		//in the case we need normals (i.e. lighting)
		NormsIndexesTableType* normalsIndexesTable = 0;
		ccNormalVectors* compressedNormals = 0;
		if (glParams.showNorms)
		{
			assert(m_associatedCloud->isA(CC_POINT_CLOUD));
			normalsIndexesTable = static_cast<ccPointCloud*>(m_associatedCloud)->normals();
			compressedNormals = ccNormalVectors::GetUniqueInstance();
		}

		//stipple mask
		if (m_stippling)
		{
#ifdef __EMSCRIPTEN__
			// glPolygonStipple is not supported
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
			glPolygonStipple(s_stippleMask);
			glEnable(GL_POLYGON_STIPPLE);
#endif
		}

		if (!pushTriangleNames && !visFiltering && !(applyMaterials || showTextures) && (!glParams.showSF || greyForNanScalarValues))
		{
#define OPTIM_MEM_CPY //use optimized mem. transfers
#ifdef OPTIM_MEM_CPY
			const unsigned step = 3*(decimStep-1);
#else
			const unsigned step = 3*decimStep;
#endif

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,s_xyzBuffer);

			if (glParams.showNorms)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT,0,s_normBuffer);
			}
			if (glParams.showSF || glParams.showColors)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3,GL_UNSIGNED_BYTE,0,s_rgbBuffer);
			}

			//we can scan and process each chunk separately in an optimized way
			unsigned k,chunks = m_triIndexes->chunksCount();
			const PointCoordinateType* P=0;
			const PointCoordinateType* N=0;
			const colorType* col=0;
			for (k=0;k<chunks;++k)
			{
				const unsigned chunkSize = m_triIndexes->chunkSize(k);

				//vertices
				const unsigned* _vertIndexes = m_triIndexes->chunkStartPtr(k);
				PointCoordinateType* _vertices = s_xyzBuffer;
#ifdef OPTIM_MEM_CPY
				for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
				{
					P = m_associatedCloud->getPoint(*_vertIndexes++)->u;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
					P = m_associatedCloud->getPoint(*_vertIndexes++)->u;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
					P = m_associatedCloud->getPoint(*_vertIndexes++)->u;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
					*(_vertices)++ = *(P)++;
				}
#else
				for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
				{
					memcpy(_vertices,m_associatedCloud->getPoint(_vertIndexes[0])->u,sizeof(PointCoordinateType)*3);
					_vertices+=3;
					memcpy(_vertices,m_associatedCloud->getPoint(_vertIndexes[1])->u,sizeof(PointCoordinateType)*3);
					_vertices+=3;
					memcpy(_vertices,m_associatedCloud->getPoint(_vertIndexes[2])->u,sizeof(PointCoordinateType)*3);
					_vertices+=3;
				}
#endif

				//scalar field
				if (glParams.showSF)
				{
					colorType* _rgbColors = s_rgbBuffer;
					_vertIndexes = m_triIndexes->chunkStartPtr(k);
					assert(colorScale);
#ifdef OPTIM_MEM_CPY
					for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
					{
						col = currentDisplayedScalarField->getValueColor(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;

						col = currentDisplayedScalarField->getValueColor(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;

						col = currentDisplayedScalarField->getValueColor(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
					}
#else
					for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
					{
						col = currentDisplayedScalarField->getValueColor(_vertIndexes[0]);
						memcpy(_rgbColors,col,sizeof(colorType)*3);
						_rgbColors += 3;
						col = currentDisplayedScalarField->getValueColor(_vertIndexes[1]);
						memcpy(_rgbColors,col,sizeof(colorType)*3);
						_rgbColors += 3;
						col = currentDisplayedScalarField->getValueColor(_vertIndexes[2]);
						memcpy(_rgbColors,col,sizeof(colorType)*3);
						_rgbColors += 3;
					}
#endif
				}
				//colors
				else if (glParams.showColors)
				{
					colorType* _rgbColors = s_rgbBuffer;
					_vertIndexes = m_triIndexes->chunkStartPtr(k);
#ifdef OPTIM_MEM_CPY
					for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
					{
						col = rgbColorsTable->getValue(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;

						col = rgbColorsTable->getValue(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;

						col = rgbColorsTable->getValue(*_vertIndexes++);
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
						*(_rgbColors)++ = *(col)++;
					}
#else
					for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
					{
						memcpy(_rgbColors,rgbColorsTable->getValue(_vertIndexes[0]),sizeof(colorType)*3);
						_rgbColors += 3;
						memcpy(_rgbColors,rgbColorsTable->getValue(_vertIndexes[1]),sizeof(colorType)*3);
						_rgbColors += 3;
						memcpy(_rgbColors,rgbColorsTable->getValue(_vertIndexes[2]),sizeof(colorType)*3);
						_rgbColors += 3;
					}
#endif
				}

				//normals
				if (glParams.showNorms)
				{
					PointCoordinateType* _normals = s_normBuffer;
					if (showTriNormals)
					{
						assert(m_triNormalIndexes);
						int* _triNormalIndexes = m_triNormalIndexes->chunkStartPtr(k);
#ifdef OPTIM_MEM_CPY
						for (n=0;n<chunkSize;n+=decimStep,_triNormalIndexes+=step)
						{
							assert(*_triNormalIndexes<(int)m_triNormals->currentSize());
							N = (*_triNormalIndexes>=0 ? compressedNormals->getNormal(m_triNormals->getValue(*_triNormalIndexes)) : s_blankNorm);
							++_triNormalIndexes;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;

							assert(*_triNormalIndexes<(int)m_triNormals->currentSize());
							N = (*_triNormalIndexes>=0 ? compressedNormals->getNormal(m_triNormals->getValue(*_triNormalIndexes)) : s_blankNorm);
							++_triNormalIndexes;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;

							assert(*_triNormalIndexes<(int)m_triNormals->currentSize());
							N = (*_triNormalIndexes>=0 ? compressedNormals->getNormal(m_triNormals->getValue(*_triNormalIndexes)) : s_blankNorm);
							++_triNormalIndexes;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
						}
#else
						for (n=0;n<chunkSize;n+=decimStep,_triNormalIndexes+=step)
						{
							assert(_triNormalIndexes[0]<(int)m_triNormals->currentSize());
							N = (_triNormalIndexes[0]>=0 ? compressedNormals->getNormal(m_triNormals->getValue(_triNormalIndexes[0])) : s_blankNorm);
							memcpy(_normals,N,sizeof(PointCoordinateType)*3);
							_normals+=3;
							assert(_triNormalIndexes[1]<(int)m_triNormals->currentSize());
							N = (_triNormalIndexes[0]==_triNormalIndexes[1] ? N : _triNormalIndexes[1]>=0 ? compressedNormals->getNormal(m_triNormals->getValue(_triNormalIndexes[1])) : s_blankNorm);
							memcpy(_normals,N,sizeof(PointCoordinateType)*3);
							_normals+=3;
							assert(_triNormalIndexes[2]<(int)m_triNormals->currentSize());
							N = (_triNormalIndexes[0]==_triNormalIndexes[2] ? N : _triNormalIndexes[2]>=0 ? compressedNormals->getNormal(m_triNormals->getValue(_triNormalIndexes[2])) : s_blankNorm);
							memcpy(_normals,N,sizeof(PointCoordinateType)*3);
							_normals+=3;
						}
#endif
					}
					else
					{
						_vertIndexes = m_triIndexes->chunkStartPtr(k);
#ifdef OPTIM_MEM_CPY
						for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
						{
							N = compressedNormals->getNormal(normalsIndexesTable->getValue(*_vertIndexes++));
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;

							N = compressedNormals->getNormal(normalsIndexesTable->getValue(*_vertIndexes++));
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;

							N = compressedNormals->getNormal(normalsIndexesTable->getValue(*_vertIndexes++));
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
							*(_normals)++ = *(N)++;
						}
#else
						for (n=0;n<chunkSize;n+=decimStep,_vertIndexes+=step)
						{
							memcpy(_normals,compressedNormals->getNormal(normalsIndexesTable->getValue(_vertIndexes[0])),sizeof(PointCoordinateType)*3);
							_normals+=3;
							memcpy(_normals,compressedNormals->getNormal(normalsIndexesTable->getValue(_vertIndexes[1])),sizeof(PointCoordinateType)*3);
							_normals+=3;
							memcpy(_normals,compressedNormals->getNormal(normalsIndexesTable->getValue(_vertIndexes[2])),sizeof(PointCoordinateType)*3);
							_normals+=3;
						}
#endif
					}
				}

				if (!showWired)
				{
					glDrawArrays(lodEnabled ? GL_POINTS : GL_TRIANGLES,0,(chunkSize/decimStep)*3);
					//glDrawElements(lodEnabled ? GL_POINTS : GL_TRIANGLES,(chunkSize/decimStep)*3,GL_UNSIGNED_INT,s_vertIndexes);
				}
				else
				{
					//on first display of a wired mesh, we need to init the corresponding vertex indexes array!
					if (!s_vertIndexesInitialized)
					{
						unsigned* _vertWireIndexes = s_vertWireIndexes;
						for (unsigned i=0;i<MAX_NUMBER_OF_ELEMENTS_PER_CHUNK*3;++i)
						{
							//s_vertIndexes[i]=i;
							*_vertWireIndexes++=i;
							*_vertWireIndexes++=((i+1)%3 == 0 ? i-2 : i+1);
						}
						s_vertIndexesInitialized=true;
					}
					glDrawElements(GL_LINES,(chunkSize/decimStep)*6,GL_UNSIGNED_INT,s_vertWireIndexes);
				}
			}

			//disable arrays
			glDisableClientState(GL_VERTEX_ARRAY);
			if (glParams.showNorms)
				glDisableClientState(GL_NORMAL_ARRAY);
			if (glParams.showSF || glParams.showColors)
				glDisableClientState(GL_COLOR_ARRAY);
		}
		else
		{
			//current vertex color
			const colorType *col1=0,*col2=0,*col3=0;
			//current vertex normal
			const PointCoordinateType *N1=0,*N2=0,*N3=0;
			//current vertex texture coordinates
			const float *Tx1=0,*Tx2=0,*Tx3=0;

			//loop on all triangles
			m_triIndexes->placeIteratorAtBegining();

			int lasMtlIndex = -1;

			if (showTextures)
			{
				//#define TEST_TEXTURED_BUNDLER_IMPORT
#ifdef TEST_TEXTURED_BUNDLER_IMPORT
				glPushAttrib(GL_COLOR_BUFFER_BIT);
				glEnable(GL_BLEND);
				glBlendFunc(context.sourceBlend, context.destBlend);
#endif

				glEnable(GL_TEXTURE_2D);
			}

			if (pushTriangleNames)
				glPushName(0);

			GLenum triangleDisplayType = lodEnabled ? GL_POINTS : showWired ? GL_LINE_LOOP : GL_TRIANGLES;
			glBegin(triangleDisplayType);

			for (n=0;n<triNum;++n)
			{
				//current triangle vertices
				const CCLib::TriangleSummitsIndexes* tsi = (CCLib::TriangleSummitsIndexes*)m_triIndexes->getCurrentValue();
				m_triIndexes->forwardIterator();

				//LOD: shall we display this triangle?
				if (n % decimStep)
					continue;

				if (visFiltering)
				{
					//we skip the triangle if at least one vertex is hidden
					if ((verticesVisibility->getValue(tsi->i1) != POINT_VISIBLE) ||
						(verticesVisibility->getValue(tsi->i2) != POINT_VISIBLE) ||
						(verticesVisibility->getValue(tsi->i3) != POINT_VISIBLE))
						continue;
				}

				if (glParams.showSF)
				{
					assert(colorScale);
					col1 = currentDisplayedScalarField->getValueColor(tsi->i1);
					if (!col1)
						continue;
					col2 = currentDisplayedScalarField->getValueColor(tsi->i2);
					if (!col2)
						continue;
					col3 = currentDisplayedScalarField->getValueColor(tsi->i3);
					if (!col3)
						continue;
				}
				else if (glParams.showColors)
				{
					col1 = rgbColorsTable->getValue(tsi->i1);
					col2 = rgbColorsTable->getValue(tsi->i2);
					col3 = rgbColorsTable->getValue(tsi->i3);
				}

				if (glParams.showNorms)
				{
					if (showTriNormals)
					{
						assert(m_triNormalIndexes);
						const int* idx = m_triNormalIndexes->getValue(n);
						assert(idx[0]<(int)m_triNormals->currentSize());
						assert(idx[1]<(int)m_triNormals->currentSize());
						assert(idx[2]<(int)m_triNormals->currentSize());
						N1 = (idx[0]>=0 ? ccNormalVectors::GetNormal(m_triNormals->getValue(idx[0])) : 0);
						N2 = (idx[0]==idx[1] ? N1 : idx[1]>=0 ? ccNormalVectors::GetNormal(m_triNormals->getValue(idx[1])) : 0);
						N3 = (idx[0]==idx[2] ? N1 : idx[2]>=0 ? ccNormalVectors::GetNormal(m_triNormals->getValue(idx[2])) : 0);

					}
					else
					{
						N1 = compressedNormals->getNormal(normalsIndexesTable->getValue(tsi->i1));
						N2 = compressedNormals->getNormal(normalsIndexesTable->getValue(tsi->i2));
						N3 = compressedNormals->getNormal(normalsIndexesTable->getValue(tsi->i3));
					}
				}

				if (applyMaterials || showTextures)
				{
					assert(m_materials);
					int newMatlIndex = m_triMtlIndexes->getValue(n);

					//do we need to change material?
					if (lasMtlIndex != newMatlIndex)
					{
						assert(newMatlIndex<(int)m_materials->size());
						glEnd();
						if (showTextures)
						{
							GLuint texID = (newMatlIndex>=0 ? (*m_materials)[newMatlIndex].texID : 0);
							if (texID>0)
								assert(glIsTexture(texID));
							glBindTexture(GL_TEXTURE_2D, texID);
						}

						//if we don't have any current material, we apply default one
						(newMatlIndex>=0 ? (*m_materials)[newMatlIndex] : context.defaultMat).applyGL(glParams.showNorms,false);
						glBegin(triangleDisplayType);
						lasMtlIndex=newMatlIndex;
					}

					if (showTextures)
					{
						assert(m_texCoords && m_texCoordIndexes);
						const int* txInd = m_texCoordIndexes->getValue(n);
						assert(txInd[0]<(int)m_texCoords->currentSize());
						assert(txInd[1]<(int)m_texCoords->currentSize());
						assert(txInd[2]<(int)m_texCoords->currentSize());
						Tx1 = (txInd[0]>=0 ? m_texCoords->getValue(txInd[0]) : 0);
						Tx2 = (txInd[1]>=0 ? m_texCoords->getValue(txInd[1]) : 0);
						Tx3 = (txInd[2]>=0 ? m_texCoords->getValue(txInd[2]) : 0);
					}
				}

				if (pushTriangleNames)
				{
					glEnd();
					glLoadName(n);
					glBegin(triangleDisplayType);
				}
				else if (showWired)
				{
					glEnd();
					glBegin(triangleDisplayType);
				}

				//vertex 1
				if (N1)
					glNormal3fv(N1);
				if (col1)
					glColor3ubv(col1);
				if (Tx1)
					glTexCoord2fv(Tx1);
				glVertex3fv(m_associatedCloud->getPoint(tsi->i1)->u);

				//vertex 2
				if (N2)
					glNormal3fv(N2);
				if (col2)
					glColor3ubv(col2);
				if (Tx2)
					glTexCoord2fv(Tx2);
				glVertex3fv(m_associatedCloud->getPoint(tsi->i2)->u);

				//vertex 3
				if (N3)
					glNormal3fv(N3);
				if (col3)
					glColor3ubv(col3);
				if (Tx3)
					glTexCoord2fv(Tx3);
				glVertex3fv(m_associatedCloud->getPoint(tsi->i3)->u);
			}

			glEnd();

			if (pushTriangleNames)
				glPopName();

			if (showTextures)
			{
#ifdef TEST_TEXTURED_BUNDLER_IMPORT
				glPopAttrib(); //GL_COLOR_BUFFER_BIT 
#endif
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
			}
		}
#ifndef __EMSCRIPTEN__
		if (m_stippling)
			glDisable(GL_POLYGON_STIPPLE);
#endif
		if (colorMaterial)
			glDisable(GL_COLOR_MATERIAL);

		if (glParams.showNorms)
		{
			glDisable(GL_LIGHTING);
			glDisable(GL_RESCALE_NORMAL);
		}

		if (pushName)
			glPopName();
	}
}

ccGenericMesh* ccMesh::createNewMeshFromSelection(bool removeSelectedVertices, CCLib::ReferenceCloud* selection/*=NULL*/, ccGenericPointCloud* vertices/*=NULL*/)
{
	assert(m_associatedCloud);

	ccGenericPointCloud::VisibilityTableType* verticesVisibility = m_associatedCloud->getTheVisibilityArray();
	if (!verticesVisibility || !verticesVisibility->isAllocated())
	{
		ccLog::Error(std::string("[Mesh ") + getName() + "] Internal error: vertex visibility table not instantiated!");
		return NULL;
	}

	ccMesh* newMesh = NULL;
	ccGenericPointCloud* newVertices = NULL;

	//if vertices were provided as input, we use them (otherwise we - try to - create them)
	if (vertices)
	{
		newVertices = vertices;
	}
	else
	{
		newVertices = m_associatedCloud->createNewCloudFromVisibilitySelection(false);
		if (!newVertices)
		{
			ccLog::Error(std::string("[Mesh ") + getName() + "] Failed to create sub-mesh vertices! (not enough memory?)");
			return NULL;
		}
	}
	assert(newVertices);

	//create a 'reference' cloud if none was provided
	CCLib::ReferenceCloud* rc = 0;
	if (selection)
	{
		assert(selection->getAssociatedCloud() == static_cast<CCLib::GenericIndexedCloud*>(m_associatedCloud));
		rc = selection;
	}
	else
	{
		//we create a temporary entity with the visible vertices only
		rc = new CCLib::ReferenceCloud(m_associatedCloud);

		for (unsigned i=0;i<m_associatedCloud->size();++i)
			if (verticesVisibility->getValue(i) == POINT_VISIBLE)
				if (!rc->addPointIndex(i))
				{
					ccLog::Error("[ccMesh::createNewMeshFromSelection] Not enough memory!");
					delete rc;
					return 0;
				}
	}

	//we create a new mesh with
	CCLib::GenericIndexedMesh* result = CCLib::ManualSegmentationTools::segmentMesh(this,rc,true,NULL,newVertices);
	if (!selection)
	{
		//don't use this anymore
		delete rc;
		rc=0;
	}

	if (result)
	{
		newMesh = new ccMesh(result,newVertices);
		if (!newMesh)
		{
			if (!vertices)
				delete newVertices;
			newVertices = NULL;
			ccLog::Error("An error occured: not enough memory?");
		}
		else
		{
			newMesh->setName(getName()+".part");

			//shall we add any advanced features?
			bool addFeatures = false;
			if (m_triNormals && m_triNormalIndexes)
				addFeatures |= newMesh->reservePerTriangleNormalIndexes();
			if (m_materials && m_triMtlIndexes)
				addFeatures |= newMesh->reservePerTriangleMtlIndexes();
			if (m_texCoords && m_texCoordIndexes)
				addFeatures |= newMesh->reservePerTriangleTexCoordIndexes();

			if (addFeatures)
			{
				//temporary structure for normal indexes mapping
				std::vector<int> newNormIndexes;
				NormsIndexesTableType* newTriNormals = 0;
				if (m_triNormals && m_triNormalIndexes)
				{
					assert(m_triNormalIndexes->currentSize()==m_triIndexes->currentSize());
					//create new 'minimal' subset
					newTriNormals = new NormsIndexesTableType();
					newTriNormals->link();
					try
					{
						newNormIndexes.resize(m_triNormals->currentSize(),-1);
					}
					catch(std::bad_alloc)
					{
						ccLog::Warning("Failed to create new normals subset! (not enough memory)");
						newMesh->removePerTriangleNormalIndexes();
						newTriNormals->release();
						newTriNormals = 0;
					}
				}

				//temporary structure for texture indexes mapping
				std::vector<int> newTexIndexes;
				TextureCoordsContainer* newTriTexIndexes = 0;
				if (m_texCoords && m_texCoordIndexes)
				{
					assert(m_texCoordIndexes->currentSize()==m_triIndexes->currentSize());
					//create new 'minimal' subset
					newTriTexIndexes = new TextureCoordsContainer();
					newTriTexIndexes->link();
					try
					{
						newTexIndexes.resize(m_texCoords->currentSize(),-1);
					}
					catch(std::bad_alloc)
					{
						ccLog::Warning("Failed to create new texture indexes subset! (not enough memory)");
						newMesh->removePerTriangleTexCoordIndexes();
						newTriTexIndexes->release();
						newTriTexIndexes = 0;
					}
				}

				//temporary structure for material indexes mapping
				std::vector<int> newMatIndexes;
				ccMaterialSet* newMaterials = 0;
				if (m_materials && m_triMtlIndexes)
				{
					assert(m_triMtlIndexes->currentSize()==m_triIndexes->currentSize());
					//create new 'minimal' subset
					newMaterials = new ccMaterialSet(m_materials->getName()+".subset");
					newMaterials->link();
					try
					{
						newMatIndexes.resize(m_materials->size(),-1);
					}
					catch(std::bad_alloc)
					{
						ccLog::Warning("Failed to create new material subset! (not enough memory)");
						newMesh->removePerTriangleMtlIndexes();
						newMaterials->release();
						newMaterials = 0;
						if (newTriTexIndexes) //we can release texture coordinates as well (as they depend on materials!)
						{
							newMesh->removePerTriangleTexCoordIndexes();
							newTriTexIndexes->release();
							newTriTexIndexes = 0;
							newTexIndexes.clear();
						}
					}
				}

				unsigned triNum=m_triIndexes->currentSize();
				m_triIndexes->placeIteratorAtBegining();
				for (unsigned i=0;i<triNum;++i)
				{
					const CCLib::TriangleSummitsIndexes* tsi = (CCLib::TriangleSummitsIndexes*)m_triIndexes->getCurrentValue();
					m_triIndexes->forwardIterator();

					//all vertices must be visible
					if (verticesVisibility->getValue(tsi->i1) == POINT_VISIBLE &&
						verticesVisibility->getValue(tsi->i2) == POINT_VISIBLE &&
						verticesVisibility->getValue(tsi->i3) == POINT_VISIBLE)
					{
						//import per-triangle normals?
						if (newTriNormals)
						{
							assert(m_triNormalIndexes);

							//current triangle (compressed) normal indexes
							const int* triNormIndexes = m_triNormalIndexes->getValue(i);

							//for each triangle of this mesh, try to determine if its normals are already in use
							//(otherwise add them to the new container and increase its index)
							for (unsigned j=0;j<3;++j)
							{
								if (triNormIndexes[j] >=0 && newNormIndexes[triNormIndexes[j]] < 0)
								{
									if (newTriNormals->currentSize() == newTriNormals->capacity() 
										&& !newTriNormals->reserve(newTriNormals->currentSize()+1000)) //auto expand
									{
										ccLog::Warning("Failed to create new normals subset! (not enough memory)");
										newMesh->removePerTriangleNormalIndexes();
										newTriNormals->release();
										newTriNormals = 0;
										break;
									}

									//import old normal to new subset (create new index)
									newNormIndexes[triNormIndexes[j]] = (int)newTriNormals->currentSize(); //new element index = new size - 1 = old size!
									newTriNormals->addElement(m_triNormals->getValue(triNormIndexes[j]));
								}
							}

							if (newTriNormals) //structure still exists?
							{
								newMesh->addTriangleNormalIndexes(triNormIndexes[0] < 0 ? -1 : newNormIndexes[triNormIndexes[0]],
									triNormIndexes[1] < 0 ? -1 : newNormIndexes[triNormIndexes[1]],
									triNormIndexes[2] < 0 ? -1 : newNormIndexes[triNormIndexes[2]]);
							}
						}

						//import texture coordinates?
						if (newTriTexIndexes)
						{
							assert(m_texCoordIndexes);

							//current triangle texture coordinates indexes
							const int* triTexIndexes = m_texCoordIndexes->getValue(i);

							//for each triangle of this mesh, try to determine if its textures coordinates are already in use
							//(otherwise add them to the new container and increase its index)
							for (unsigned j=0;j<3;++j)
							{
								if (triTexIndexes[j] >=0 && newTexIndexes[triTexIndexes[j]] < 0)
								{
									if (newTriTexIndexes->currentSize() == newTriTexIndexes->capacity() 
										&& !newTriTexIndexes->reserve(newTriTexIndexes->currentSize()+500)) //auto expand
									{
										ccLog::Error("Failed to create new texture coordinates subset! (not enough memory)");
										newMesh->removePerTriangleTexCoordIndexes();
										newTriTexIndexes->release();
										newTriTexIndexes = 0;
										break;
									}
									//import old texture coordinate to new subset (create new index)
									newTexIndexes[triTexIndexes[j]] = (int)newTriTexIndexes->currentSize(); //new element index = new size - 1 = old size!
									newTriTexIndexes->addElement(m_texCoords->getValue(triTexIndexes[j]));
								}
							}

							if (newTriTexIndexes) //structure still exists?
							{
								newMesh->addTriangleTexCoordIndexes(triTexIndexes[0] < 0 ? -1 : newTexIndexes[triTexIndexes[0]],
									triTexIndexes[1] < 0 ? -1 : newTexIndexes[triTexIndexes[1]],
									triTexIndexes[2] < 0 ? -1 : newTexIndexes[triTexIndexes[2]]);
							}
						}

						//import materials?
						if (newMaterials)
						{
							assert(m_triMtlIndexes);

							//current triangle material index
							const int triMatIndex = m_triMtlIndexes->getValue(i);

							//for each triangle of this mesh, try to determine if its material is already in use
							//(otherwise add it to the new container and increase its index)
							if (triMatIndex >=0 && newMatIndexes[triMatIndex] < 0)
							{
								//import old material to new subset (create new index)
								newMatIndexes[triMatIndex] = (int)newMaterials->size(); //new element index = new size - 1 = old size!
								try
								{
									newMaterials->push_back(m_materials->at(triMatIndex));
								}
								catch(std::bad_alloc)
								{
									ccLog::Warning("Failed to create new materials subset! (not enough memory)");
									newMesh->removePerTriangleMtlIndexes();
									newMaterials->release();
									newMaterials = 0;
								}
							}

							if (newMaterials) //structure still exists?
							{
								newMesh->addTriangleMtlIndex(triMatIndex < 0 ? -1 : newMatIndexes[triMatIndex]);
							}
						}

					}
				}

				if (newTriNormals)
				{
					newTriNormals->resize(newTriNormals->currentSize()); //smaller so it should always be ok!
					newMesh->setTriNormsTable(newTriNormals);
					newMesh->addChild(newTriNormals,true);
					newTriNormals->release();
					newTriNormals=0;
				}

				if (newTriTexIndexes)
				{
					newMesh->setTexCoordinatesTable(newTriTexIndexes);
					newMesh->addChild(newTriTexIndexes,true);
					newTriTexIndexes->release();
					newTriTexIndexes=0;
				}

				if (newMaterials)
				{
					newMesh->setMaterialSet(newMaterials);
					newMesh->addChild(newMaterials,true);
					newMaterials->release();
					newMaterials=0;
				}
			}

			if (!vertices)
			{
				newMesh->addChild(newVertices);
				newMesh->setDisplay_recursive(getDisplay());
				newMesh->showColors(colorsShown());
				newMesh->showNormals(normalsShown());
				newMesh->showMaterials(materialsShown());
				newMesh->showSF(sfShown());
				newVertices->setEnabled(false);
			}
		}

		delete result;
		result=0;
	}

	//shall we remove the selected vertices from this mesh?
	if (removeSelectedVertices)
	{
		//we remove all visible points
		unsigned lastTri=0,triNum=m_triIndexes->currentSize();
		m_triIndexes->placeIteratorAtBegining();
		for (unsigned i=0;i<triNum;++i)
		{
			const CCLib::TriangleSummitsIndexes* tsi = (CCLib::TriangleSummitsIndexes*)m_triIndexes->getCurrentValue();
			m_triIndexes->forwardIterator();

			//at least one hidden vertex
			if (verticesVisibility->getValue(tsi->i1) != POINT_VISIBLE ||
				verticesVisibility->getValue(tsi->i2) != POINT_VISIBLE ||
				verticesVisibility->getValue(tsi->i3) != POINT_VISIBLE)
			{
				if (i != lastTri)
				{
					m_triIndexes->setValue(lastTri, (unsigned*)tsi);

					if (m_triNormalIndexes)
						m_triNormalIndexes->setValue(lastTri, m_triNormalIndexes->getValue(i));
					if (m_triMtlIndexes)
						m_triMtlIndexes->setValue(lastTri, m_triMtlIndexes->getValue(i));
					if (m_texCoordIndexes)
						m_texCoordIndexes->setValue(lastTri, m_texCoordIndexes->getValue(i));
				}
				++lastTri;
			}
		}

		resize(lastTri);
		updateModificationTime();
	}

	return newMesh;
}

void ccMesh::shiftTriangleIndexes(unsigned shift)
{
	m_triIndexes->placeIteratorAtBegining();
	unsigned *ti,i=0;
	for (;i<m_triIndexes->currentSize();++i)
	{
		ti = m_triIndexes->getCurrentValue();
		ti[0]+=shift;
		ti[1]+=shift;
		ti[2]+=shift;
		m_triIndexes->forwardIterator();
	}
}

/*********************************************************/
/**************    PER-TRIANGLE NORMALS    ***************/
/*********************************************************/

bool ccMesh::arePerTriangleNormalsEnabled() const
{
	return m_triNormalIndexes && m_triNormalIndexes->isAllocated();
}

void ccMesh::removePerTriangleNormalIndexes()
{
	if (m_triNormalIndexes)
		m_triNormalIndexes->release();
	m_triNormalIndexes=0;
}

void ccMesh::setTriNormsTable(NormsIndexesTableType* triNormsTable, bool autoReleaseOldTable/*=true*/)
{
	ccGenericMesh::setTriNormsTable(triNormsTable,autoReleaseOldTable);

	if (!m_triNormals)
		removePerTriangleNormalIndexes();
}

bool ccMesh::reservePerTriangleNormalIndexes()
{
	assert(!m_triNormalIndexes); //try to avoid doing this twice!
	if (!m_triNormalIndexes)
	{
		m_triNormalIndexes = new triangleNormalsIndexesSet();
		m_triNormalIndexes->link();
	}

	assert(m_triIndexes && m_triIndexes->isAllocated());

	return m_triNormalIndexes->reserve(m_triIndexes->capacity());
}

void ccMesh::addTriangleNormalIndexes(int i1, int i2, int i3)
{
	assert(m_triNormalIndexes && m_triNormalIndexes->isAllocated());
	int indexes[3]={i1,i2,i3};
	m_triNormalIndexes->addElement(indexes);
}

void ccMesh::setTriangleNormalIndexes(unsigned triangleIndex, int i1, int i2, int i3)
{
	assert(m_triNormalIndexes && m_triNormalIndexes->currentSize() > triangleIndex);
	int indexes[3]={i1,i2,i3};
	m_triNormalIndexes->setValue(triangleIndex,indexes);
}

bool ccMesh::hasTriNormals() const
{
	return m_triNormals && m_triNormals->isAllocated() && m_triNormalIndexes && (m_triNormalIndexes->currentSize() == m_triIndexes->currentSize());
}

/*********************************************************/
/************    PER-TRIANGLE TEX COORDS    **************/
/*********************************************************/

void ccMesh::setTexCoordinatesTable(TextureCoordsContainer* texCoordsTable, bool autoReleaseOldTable/*=true*/)
{
	ccGenericMesh::setTexCoordinatesTable(texCoordsTable,autoReleaseOldTable);

	if (!m_texCoords)
		removePerTriangleTexCoordIndexes(); //auto-remove per-triangle indexes
}

void ccMesh::getTriangleTexCoordinates(unsigned triIndex, float* &tx1, float* &tx2, float* &tx3) const
{
	if (m_texCoords && m_texCoordIndexes)
	{
		const int* txInd = m_texCoordIndexes->getValue(triIndex);
		tx1 = (txInd[0]>=0 ? m_texCoords->getValue(txInd[0]) : 0);
		tx2 = (txInd[1]>=0 ? m_texCoords->getValue(txInd[1]) : 0);
		tx3 = (txInd[2]>=0 ? m_texCoords->getValue(txInd[2]) : 0);
	}
	else
	{
		tx1 = tx2 = tx3;
	}
}

bool ccMesh::reservePerTriangleTexCoordIndexes()
{
	assert(!m_texCoordIndexes); //try to avoid doing this twice!
	if (!m_texCoordIndexes)
	{
		m_texCoordIndexes = new triangleTexCoordIndexesSet();
		m_texCoordIndexes->link();
	}

	assert(m_triIndexes && m_triIndexes->isAllocated());

	return m_texCoordIndexes->reserve(m_triIndexes->capacity());
}

void ccMesh::removePerTriangleTexCoordIndexes()
{
	triangleTexCoordIndexesSet* texCoordIndexes = m_texCoordIndexes;
	m_texCoordIndexes = 0;

	if (texCoordIndexes)
		texCoordIndexes->release();
}

void ccMesh::addTriangleTexCoordIndexes(int i1, int i2, int i3)
{
	assert(m_texCoordIndexes && m_texCoordIndexes->isAllocated());
	int indexes[3]={i1,i2,i3};
	m_texCoordIndexes->addElement(indexes);
}

void ccMesh::setTriangleTexCoordIndexes(unsigned triangleIndex, int i1, int i2, int i3)
{
	assert(m_texCoordIndexes && m_texCoordIndexes->currentSize() > triangleIndex);
	int indexes[3]={i1,i2,i3};
	m_texCoordIndexes->setValue(triangleIndex,indexes);
}

bool ccMesh::hasTextures() const
{
	return hasMaterials() && m_texCoords && m_texCoords->isAllocated() && m_texCoordIndexes && (m_texCoordIndexes->currentSize() == m_triIndexes->currentSize());
}

/*********************************************************/
/**************    PER-TRIANGLE MATERIALS    *************/
/*********************************************************/

bool ccMesh::hasMaterials() const
{
	return m_materials && !m_materials->empty() && m_triMtlIndexes && (m_triMtlIndexes->currentSize() == m_triIndexes->currentSize());
}

bool ccMesh::reservePerTriangleMtlIndexes()
{
	assert(!m_triMtlIndexes); //try to avoid doing this twice!
	if (!m_triMtlIndexes)
	{
		m_triMtlIndexes = new triangleMaterialIndexesSet();
		m_triMtlIndexes->link();
	}

	assert(m_triIndexes && m_triIndexes->isAllocated());

	return m_triMtlIndexes->reserve(m_triIndexes->capacity());
}

void ccMesh::removePerTriangleMtlIndexes()
{
	if (m_triMtlIndexes)
		m_triMtlIndexes->release();
	m_triMtlIndexes=0;
}

void ccMesh::addTriangleMtlIndex(int mtlIndex)
{
	assert(m_triMtlIndexes && m_triMtlIndexes->isAllocated());
	m_triMtlIndexes->addElement(mtlIndex);
}

void ccMesh::setTriangleMtlIndex(unsigned triangleIndex, int mtlIndex)
{
	assert(m_triMtlIndexes && m_triMtlIndexes->currentSize() > triangleIndex);
	m_triMtlIndexes->setValue(triangleIndex,mtlIndex);
}

void ccMesh::setDisplay(ccGenericGLDisplay* win)
{
	if (m_materials && !m_materials->empty())
	{
		const ccGenericGLDisplay* currentDisplay = m_materials->getAssociatedDisplay();
		//if the material set is not associated to any display --> we associate it with input display!
		if (currentDisplay==0)
			m_materials->associateTo(win);
		/*else //else if it is associated with a different display
		{
		//we clone the material set
		ccMaterialSet* newMtlSet = new ccMaterialSet();
		newMtlSet->link();
		m_materials->clone(*newMtlSet);
		//and we link the clone to the new display
		newMtlSet->associateTo(win);
		setMaterialSet(newMtlSet);
		newMtlSet->release();
		}
		//*/
	}

	ccDrawableObject::setDisplay(win);
}

bool ccMesh::interpolateNormals(unsigned triIndex, const CCVector3& P, CCVector3& N)
{
	assert(triIndex<size());

	if (!hasNormals())
		return false;

	const unsigned* tri = m_triIndexes->getValue(triIndex);

	return interpolateNormals(tri[0],tri[1],tri[2],P,N, hasTriNormals() ? m_triNormalIndexes->getValue(triIndex) : 0);
}

bool ccMesh::interpolateNormals(unsigned i1, unsigned i2, unsigned i3, const CCVector3& P, CCVector3& N, const int* triNormIndexes/*=0*/)
{
	const CCVector3 *A = m_associatedCloud->getPointPersistentPtr(i1);
	const CCVector3 *B = m_associatedCloud->getPointPersistentPtr(i2);
	const CCVector3 *C = m_associatedCloud->getPointPersistentPtr(i3);

	//intepolation weights
	PointCoordinateType d1 = ((P-*B).cross(*C-*B)).norm()/*/2.0*/;
	PointCoordinateType d2 = ((P-*C).cross(*A-*C)).norm()/*/2.0*/;
	PointCoordinateType d3 = ((P-*A).cross(*B-*A)).norm()/*/2.0*/;

	CCVector3 N1,N2,N3;
	if (triNormIndexes) //per-triangle normals
	{
		if (triNormIndexes[0]>=0)
			N1 = ccNormalVectors::GetNormal(m_triNormals->getValue(triNormIndexes[0]));
		else
			d1 = 0;
		if (triNormIndexes[1]>=0)
			N2 = ccNormalVectors::GetNormal(m_triNormals->getValue(triNormIndexes[1]));
		else
			d2 = 0;
		if (triNormIndexes[2]>=0)
			N3 = ccNormalVectors::GetNormal(m_triNormals->getValue(triNormIndexes[2]));
		else
			d3 = 0;
	}
	else //per-vertex normals
	{
		N1 = CCVector3(m_associatedCloud->getPointNormal(i1));
		N2 = CCVector3(m_associatedCloud->getPointNormal(i2));
		N3 = CCVector3(m_associatedCloud->getPointNormal(i3));
	}

	N = N1*d1+N2*d2+N3*d3;

	N.normalize();

	return true;
}

bool ccMesh::interpolateColors(unsigned triIndex, const CCVector3& P, colorType rgb[])
{
	assert(triIndex<size());

	if (!hasColors())
		return false;

	const unsigned* tri = m_triIndexes->getValue(triIndex);

	return interpolateColors(tri[0],tri[1],tri[2],P,rgb);
}

bool ccMesh::interpolateColors(unsigned i1, unsigned i2, unsigned i3, const CCVector3& P, colorType rgb[])
{
	const CCVector3 *A = m_associatedCloud->getPointPersistentPtr(i1);
	const CCVector3 *B = m_associatedCloud->getPointPersistentPtr(i2);
	const CCVector3 *C = m_associatedCloud->getPointPersistentPtr(i3);

	//intepolation weights
	PointCoordinateType d1 = ((P-*B).cross(*C-*B)).norm()/*/2.0*/;
	PointCoordinateType d2 = ((P-*C).cross(*A-*C)).norm()/*/2.0*/;
	PointCoordinateType d3 = ((P-*A).cross(*B-*A)).norm()/*/2.0*/;
	//we must normalize weights
	PointCoordinateType dsum = d1+d2+d3;
	d1/=dsum;
	d2/=dsum;
	d3/=dsum;

	const colorType* C1 = m_associatedCloud->getPointColor(i1);
	const colorType* C2 = m_associatedCloud->getPointColor(i2);
	const colorType* C3 = m_associatedCloud->getPointColor(i3);

	rgb[0] = (colorType)floor((float)C1[0]*d1+(float)C2[0]*d2+(float)C3[0]*d3);
	rgb[1] = (colorType)floor((float)C1[1]*d1+(float)C2[1]*d2+(float)C3[1]*d3);
	rgb[2] = (colorType)floor((float)C1[2]*d1+(float)C2[2]*d2+(float)C3[2]*d3);

	return true;
}

bool ccMesh::getVertexColorFromMaterial(unsigned triIndex, unsigned char vertIndex, colorType rgb[], bool returnColorIfNoTexture)
{
	assert(triIndex < size());

	assert(vertIndex < 3);
	if (vertIndex > 2)
	{
		ccLog::Error("[ccMesh::getVertexColorFromMaterial] Internal error: invalid vertex index!");
		return false;
	}

	int matIndex = -1;

	if (hasMaterials())
	{
		assert(m_materials);
		matIndex = m_triMtlIndexes->getValue(triIndex);
		assert(matIndex < (int)m_materials->size());
	}

	const unsigned* tri = m_triIndexes->getValue(triIndex);
	assert(tri);

	//do we need to change material?
	bool foundMaterial = false;
	if (matIndex >= 0)
	{
		const ccMaterial& material = (*m_materials)[matIndex];
		if (material.texture.isNull())
		{
			rgb[0] = (colorType)(material.diffuseFront[0]*MAX_COLOR_COMP);
			rgb[1] = (colorType)(material.diffuseFront[1]*MAX_COLOR_COMP);
			rgb[2] = (colorType)(material.diffuseFront[2]*MAX_COLOR_COMP);

			foundMaterial = true;
		}
		else
		{
			assert(m_texCoords && m_texCoordIndexes);
			const int* txInd = m_texCoordIndexes->getValue(triIndex);
			const float* Tx = (txInd[vertIndex]>=0 ? m_texCoords->getValue(txInd[vertIndex]) : 0);
			if (Tx)
			{
				if (Tx[0] >= 0 && Tx[0] <= 1.0f && Tx[1] >= 0 && Tx[1] <= 1.0f)
				{
					//get color from texture image
					int xPix = std::min((int)floor(Tx[0]*(float)material.texture.width()),material.texture.width()-1);
					int yPix = std::min((int)floor(Tx[1]*(float)material.texture.height()),material.texture.height()-1);

					Rgb pixel = material.texture.pixel(xPix,yPix);

					rgb[0] = pixel.r;
					rgb[1] = pixel.g;
					rgb[2] = pixel.b;

					foundMaterial = true;
				}
			}
		}
	}

	if (!foundMaterial && returnColorIfNoTexture && hasColors())
	{
		const colorType* col = m_associatedCloud->getPointColor(tri[vertIndex]);

		rgb[0] = col[0];
		rgb[1] = col[1];
		rgb[2] = col[2];

		foundMaterial = true;
	}

	return foundMaterial;
}

bool ccMesh::getColorFromMaterial(unsigned triIndex, const CCVector3& P, colorType rgb[], bool interpolateColorIfNoTexture)
{
	assert(triIndex<size());

	int matIndex = -1;

	if (hasMaterials())
	{
		assert(m_materials);
		matIndex = m_triMtlIndexes->getValue(triIndex);
		assert(matIndex<(int)m_materials->size());
	}

	//do we need to change material?
	if (matIndex<0)
	{
		if (interpolateColorIfNoTexture)
			return interpolateColors(triIndex,P,rgb);
		return false;
	}

	const ccMaterial& material = (*m_materials)[matIndex];

	if (material.texture.isNull())
	{
		rgb[0] = (colorType)(material.diffuseFront[0]*MAX_COLOR_COMP);
		rgb[1] = (colorType)(material.diffuseFront[1]*MAX_COLOR_COMP);
		rgb[2] = (colorType)(material.diffuseFront[2]*MAX_COLOR_COMP);
		return true;
	}

	assert(m_texCoords && m_texCoordIndexes);
	const int* txInd = m_texCoordIndexes->getValue(triIndex);
	const float* Tx1 = (txInd[0]>=0 ? m_texCoords->getValue(txInd[0]) : 0);
	const float* Tx2 = (txInd[1]>=0 ? m_texCoords->getValue(txInd[1]) : 0);
	const float* Tx3 = (txInd[2]>=0 ? m_texCoords->getValue(txInd[2]) : 0);

	//interpolation weights
	const unsigned* tri = m_triIndexes->getValue(triIndex);
	const CCVector3 *A = m_associatedCloud->getPointPersistentPtr(tri[0]);
	const CCVector3 *B = m_associatedCloud->getPointPersistentPtr(tri[1]);
	const CCVector3 *C = m_associatedCloud->getPointPersistentPtr(tri[2]);

	PointCoordinateType d1 = ((P-*B).cross(*C-*B)).norm()/*/2.0*/;
	PointCoordinateType d2 = ((P-*C).cross(*A-*C)).norm()/*/2.0*/;
	PointCoordinateType d3 = ((P-*A).cross(*B-*A)).norm()/*/2.0*/;
	//we must normalize weights
	PointCoordinateType dsum = d1+d2+d3;
	d1/=dsum;
	d2/=dsum;
	d3/=dsum;

	float x = (Tx1 ? Tx1[0] : 0)*d1 + (Tx2 ? Tx2[0] : 0)*d2 + (Tx3 ? Tx3[0] : 0)*d3;
	float y = (Tx1 ? Tx1[1] : 0)*d1 + (Tx2 ? Tx2[1] : 0)*d2 + (Tx3 ? Tx3[1] : 0)*d3;

	if (x<0 || x>1.0f || y<0 || y>1.0f)
	{
		if (interpolateColorIfNoTexture)
			return interpolateColors(triIndex,P,rgb);
		return false;
	}

	//get color from texture image
	{
		int xPix = std::min((int)floor(x*(float)material.texture.width()),material.texture.width()-1);
		int yPix = std::min((int)floor(y*(float)material.texture.height()),material.texture.height()-1);

		Rgb pixel = material.texture.pixel(xPix,yPix);

		rgb[0] = pixel.r;
		rgb[1] = pixel.g;
		rgb[2] = pixel.b;
	}

	return true;
}

//we use as many static variables as we can to limit the size of the heap used by each recursion...
static const unsigned s_defaultSubdivideGrowRate = 50;
static float s_maxSubdivideArea = 1;
static std::map<int64_t,unsigned> s_alreadyCreatedVertices; //map to store already created edges middle points

static int64_t GenerateKey(unsigned edgeIndex1, unsigned edgeIndex2)
{
	if (edgeIndex1>edgeIndex2)
		std::swap(edgeIndex1,edgeIndex2);

	return ((((int64_t)edgeIndex1)<<32) | (int64_t)edgeIndex2);
}

bool ccMesh::pushSubdivide(/*PointCoordinateType maxArea, */unsigned indexA, unsigned indexB, unsigned indexC)
{
	if (s_maxSubdivideArea/*maxArea*/<=ZERO_TOLERANCE)
	{
		ccLog::Error("[ccMesh::pushSubdivide] Invalid input argument!");
		return false;
	}

	if (!getAssociatedCloud() || !getAssociatedCloud()->isA(CC_POINT_CLOUD))
	{
		ccLog::Error("[ccMesh::pushSubdivide] Vertices set must be a true point cloud!");
		return false;
	}
	ccPointCloud* vertices = static_cast<ccPointCloud*>(getAssociatedCloud());
	assert(vertices);
	const CCVector3* A = vertices->getPoint(indexA);
	const CCVector3* B = vertices->getPoint(indexB);
	const CCVector3* C = vertices->getPoint(indexC);

	//do we need to sudivide this triangle?
	PointCoordinateType area = ((*B-*A)*(*C-*A)).norm()/(PointCoordinateType)2.0;
	if (area > s_maxSubdivideArea/*maxArea*/)
	{
		//we will add 3 new vertices, so we must be sure to have enough memory
		if (vertices->size()+2 >= vertices->capacity())
		{
			assert(s_defaultSubdivideGrowRate>2);
			if (!vertices->reserve(vertices->size()+s_defaultSubdivideGrowRate))
			{
				ccLog::Error("[ccMesh::pushSubdivide] Not enough memory!");
				return false;
			}
			//We have to update pointers as they may have been wrangled by the 'reserve' call
			A = vertices->getPoint(indexA);
			B = vertices->getPoint(indexB);
			C = vertices->getPoint(indexC);
		}

		//add new vertices
		unsigned indexG1 = 0;
		{
			int64_t key = GenerateKey(indexA,indexB);
			auto it = s_alreadyCreatedVertices.find(key);
			if (it == s_alreadyCreatedVertices.end())
			{
				//generate new vertex
				indexG1 = vertices->size();
				CCVector3 G1 = (*A+*B)/(PointCoordinateType)2.0;
				vertices->addPoint(G1.u);
				//interpolate other features?
				/*if (vertices->hasNormals())
				{
				//vertices->reserveTheNormsTable();
				CCVector3 N(0.0,0.0,1.0);
				interpolateNormals(indexA,indexB,indexC,G1,N);
				vertices->addNorm(N.u);
				}
				//*/
				if (vertices->hasColors())
				{
					colorType C[3]={MAX_COLOR_COMP,MAX_COLOR_COMP,MAX_COLOR_COMP};
					interpolateColors(indexA,indexB,indexC,G1,C);
					vertices->addRGBColor(C);
				}
				//and add it to the map
				s_alreadyCreatedVertices[key] = indexG1;
			}
			else
			{
				indexG1 = it->second;
			}
		}
		unsigned indexG2 = 0;
		{
			int64_t key = GenerateKey(indexB,indexC);
			auto it = s_alreadyCreatedVertices.find(key);
			if (it == s_alreadyCreatedVertices.end())
			{
				//generate new vertex
				indexG2 = vertices->size();
				CCVector3 G2 = (*B+*C)/(PointCoordinateType)2.0;
				vertices->addPoint(G2.u);
				//interpolate other features?
				/*if (vertices->hasNormals())
				{
				//vertices->reserveTheNormsTable();
				CCVector3 N(0.0,0.0,1.0);
				interpolateNormals(indexA,indexB,indexC,G2,N);
				vertices->addNorm(N.u);
				}
				//*/
				if (vertices->hasColors())
				{
					colorType C[3]={MAX_COLOR_COMP,MAX_COLOR_COMP,MAX_COLOR_COMP};
					interpolateColors(indexA,indexB,indexC,G2,C);
					vertices->addRGBColor(C);
				}
				//and add it to the map
				s_alreadyCreatedVertices[key] = indexG2;
			}
			else
			{
				indexG2 = it->second;
			}
		}
		unsigned indexG3 = vertices->size();
		{
			int64_t key = GenerateKey(indexC,indexA);
			auto it = s_alreadyCreatedVertices.find(key);
			if (it == s_alreadyCreatedVertices.end())
			{
				//generate new vertex
				indexG3 = vertices->size();
				CCVector3 G3 = (*C+*A)/(PointCoordinateType)2.0;
				vertices->addPoint(G3.u);
				//interpolate other features?
				/*if (vertices->hasNormals())
				{
				//vertices->reserveTheNormsTable();
				CCVector3 N(0.0,0.0,1.0);
				interpolateNormals(indexA,indexB,indexC,G3,N);
				vertices->addNorm(N.u);
				}
				//*/
				if (vertices->hasColors())
				{
					colorType C[3]={MAX_COLOR_COMP,MAX_COLOR_COMP,MAX_COLOR_COMP};
					interpolateColors(indexA,indexB,indexC,G3,C);
					vertices->addRGBColor(C);
				}
				//and add it to the map
				s_alreadyCreatedVertices[key] = indexG3;
			}
			else
			{
				indexG3 = it->second;
			}
		}

		//add new triangles
		if (!pushSubdivide(/*maxArea, */indexA, indexG1, indexG3))
			return false;
		if (!pushSubdivide(/*maxArea, */indexB, indexG2, indexG1))
			return false;
		if (!pushSubdivide(/*maxArea, */indexC, indexG3, indexG2))
			return false;
		if (!pushSubdivide(/*maxArea, */indexG1, indexG2, indexG3))
			return false;
	}
	else
	{
		//we will add one triangle, so we must be sure to have enough memory
		if (size() == maxSize())
		{
			if (!reserve(size()+3*s_defaultSubdivideGrowRate))
			{
				ccLog::Error("[ccMesh::pushSubdivide] Not enough memory!");
				return false;
			}
		}

		//we keep this triangle as is
		addTriangle(indexA,indexB,indexC);
	}

	return true;
}

ccMesh* ccMesh::subdivide(float maxArea) const
{
	if (maxArea<=ZERO_TOLERANCE)
	{
		ccLog::Error("[ccMesh::subdivide] Invalid input argument!");
		return 0;
	}
	s_maxSubdivideArea = maxArea;

	unsigned triCount = size();
	ccGenericPointCloud* vertices = getAssociatedCloud();
	unsigned vertCount = (vertices ? vertices->size() : 0);
	if (!vertices || vertCount*triCount==0)
	{
		ccLog::Error("[ccMesh::subdivide] Invalid mesh: no face or no vertex!");
		return 0;
	}

	ccPointCloud* resultVertices = 0;
	if (vertices->isA(CC_POINT_CLOUD))
		resultVertices = static_cast<ccPointCloud*>(vertices)->cloneThis();
	else
		resultVertices = ccPointCloud::From(vertices);
	if (!resultVertices)
	{
		ccLog::Error("[ccMesh::subdivide] Not enough memory!");
		return 0;
	}

	ccMesh* resultMesh = new ccMesh(resultVertices);
	resultMesh->addChild(resultVertices);

	if (!resultMesh->reserve(triCount))
	{
		ccLog::Error("[ccMesh::subdivide] Not enough memory!");
		delete resultMesh;
		return 0;
	}

	s_alreadyCreatedVertices.clear();

	try
	{		
		for (unsigned i=0;i<triCount;++i)
		{
			const unsigned* tri = m_triIndexes->getValue(i);
			if (!resultMesh->pushSubdivide(/*maxArea,*/tri[0],tri[1],tri[2]))
			{
				ccLog::Error("[ccMesh::subdivide] Not enough memory!");
				delete resultMesh;
				return 0;
			}
		}
	}
	catch(...)
	{
		ccLog::Error("[ccMesh::subdivide] An error occured!");
		delete resultMesh;
		return 0;
	}

	//we must also 'fix' the triangles that share (at least) an edge with a subdivided triangle!
	try
	{
		unsigned newTriCount = resultMesh->size();
		for (unsigned i=0;i<newTriCount;++i)
		{
			unsigned* _face = resultMesh->m_triIndexes->getValue(i); //warning: array might change at each call to reallocate!
			unsigned indexA = _face[0];
			unsigned indexB = _face[1];
			unsigned indexC = _face[2];

			//test all edges
			int indexG1 = -1;
			{
				auto it = s_alreadyCreatedVertices.find(GenerateKey(indexA,indexB));
				if (it != s_alreadyCreatedVertices.end())
					indexG1 = (int)it->second;
			}
			int indexG2 = -1;
			{
				auto it = s_alreadyCreatedVertices.find(GenerateKey(indexB,indexC));
				if (it != s_alreadyCreatedVertices.end())
					indexG2 = (int)it->second;
			}
			int indexG3 = -1;
			{
				auto it = s_alreadyCreatedVertices.find(GenerateKey(indexC,indexA));
				if (it != s_alreadyCreatedVertices.end())
					indexG3 = (int)it->second;
			}

			//at least one edge is 'wrong'
			unsigned brokenEdges = (indexG1<0 ? 0:1)
				+ (indexG2<0 ? 0:1)
				+ (indexG3<0 ? 0:1);

			if (brokenEdges == 1)
			{
				int indexG = indexG1;
				unsigned char i1 = 2; //relative index facing the broken edge
				if (indexG2>=0)
				{
					indexG = indexG2;
					i1 = 0;
				}
				else if (indexG3>=0)
				{
					indexG = indexG3;
					i1 = 1;
				}
				assert(indexG>=0);
				assert(i1<3);

				unsigned indexes[3] = { indexA, indexB, indexC };

				//replace current triangle by one half
				_face[0] = indexes[i1];
				_face[1] = indexG;
				_face[2] = indexes[(i1+2)%3];
				//and add the other half (we can use pushSubdivide as the area should alredy be ok!)
				if (!resultMesh->pushSubdivide(/*maxArea,*/indexes[i1],indexes[(i1+1)%3],indexG))
				{
					ccLog::Error("[ccMesh::subdivide] Not enough memory!");
					delete resultMesh;
					return 0;
				}
			}
			else if (brokenEdges == 2)
			{
				if (indexG1<0) //broken edges: BC and CA
				{
					//replace current triangle by the 'pointy' part
					_face[0] = indexC;
					_face[1] = indexG3;
					_face[2] = indexG2;
					//split the remaining 'trapezoid' in 2
					if (!resultMesh->pushSubdivide(/*maxArea, */indexA, indexG2, indexG3) ||
						!resultMesh->pushSubdivide(/*maxArea, */indexA, indexB, indexG2))
					{
						ccLog::Error("[ccMesh::subdivide] Not enough memory!");
						delete resultMesh;
						return 0;
					}
				}
				else if (indexG2<0) //broken edges: AB and CA
				{
					//replace current triangle by the 'pointy' part
					_face[0] = indexA;
					_face[1] = indexG1;
					_face[2] = indexG3;
					//split the remaining 'trapezoid' in 2
					if (!resultMesh->pushSubdivide(/*maxArea, */indexB, indexG3, indexG1) ||
						!resultMesh->pushSubdivide(/*maxArea, */indexB, indexC, indexG3))
					{
						ccLog::Error("[ccMesh::subdivide] Not enough memory!");
						delete resultMesh;
						return 0;
					}
				}
				else /*if (indexG3<0)*/ //broken edges: AB and BC
				{
					//replace current triangle by the 'pointy' part
					_face[0] = indexB;
					_face[1] = indexG2;
					_face[2] = indexG1;
					//split the remaining 'trapezoid' in 2
					if (!resultMesh->pushSubdivide(/*maxArea, */indexC, indexG1, indexG2) ||
						!resultMesh->pushSubdivide(/*maxArea, */indexC, indexA, indexG1))
					{
						ccLog::Error("[ccMesh::subdivide] Not enough memory!");
						delete resultMesh;
						return 0;
					}
				}
			}
			else if (brokenEdges == 3) //works just as a standard subdivision in fact!
			{
				//replace current triangle by one quarter
				_face[0] = indexA;
				_face[1] = indexG1;
				_face[2] = indexG3;
				//and add the other 3 quarters (we can use pushSubdivide as the area should alredy be ok!)
				if (!resultMesh->pushSubdivide(/*maxArea, */indexB, indexG2, indexG1) ||
					!resultMesh->pushSubdivide(/*maxArea, */indexC, indexG3, indexG2) ||
					!resultMesh->pushSubdivide(/*maxArea, */indexG1, indexG2, indexG3))
				{
					ccLog::Error("[ccMesh::subdivide] Not enough memory!");
					delete resultMesh;
					return 0;
				}
			}
		}
	}
	catch(...)
	{
		ccLog::Error("[ccMesh::subdivide] An error occured!");
		delete resultMesh;
		return 0;
	}

	s_alreadyCreatedVertices.clear();

	if (resultMesh->size() < resultMesh->maxSize())
		resultMesh->resize(resultMesh->size());
	if (resultVertices->size()<resultVertices->capacity())
		resultVertices->resize(resultVertices->size());

	//we import from the original mesh... what we can
	if (hasNormals())
	{
		if (hasNormals()) //normals interpolation doesn't work well...
			resultMesh->computeNormals();
		resultMesh->showNormals(normalsShown());
	}
	if (hasColors())
	{
		resultMesh->showColors(colorsShown());
	}
	resultMesh->setVisible(isVisible());

	return resultMesh;
}

bool ccMesh::trianglePicking(const double clickPosX,
							const double clickPosY,
							const double MM[4],
							const double MP[4],
							const int VP[4],
							int& nearestTriIndex,
							double& nearestSquareDist)
{
	ccGLMatrix trans;
	bool noGLTrans = !getAbsoluteGLTransformation(trans);

	//back project the clicked point in 3D
	CCVector3d X(0,0,0);
	if (gluProject(clickPosX,clickPosY,0,MM,MP,VP,&X.x,&X.y,&X.z) != GLU_TRUE)
	{
		return false;
	}

	nearestTriIndex = -1;
	nearestSquareDist = -1.0;

	ccGenericPointCloud* vertices = getAssociatedCloud();
	assert(vertices);

	for (int i=0; i<static_cast<int>(m_triIndexes->currentSize()); ++i)
	{
		const unsigned *tri = m_triIndexes->getValue(i);
		const CCVector3* A3D = vertices->getPoint(tri[0]);
		const CCVector3* B3D = vertices->getPoint(tri[1]);
		const CCVector3* C3D = vertices->getPoint(tri[2]);

		CCVector3d A2D,B2D,C2D; 
		if (noGLTrans)
		{
			gluProject(A3D->x,A3D->y,A3D->z,MM,MP,VP,&A2D.x,&A2D.y,&A2D.z);
			gluProject(B3D->x,B3D->y,B3D->z,MM,MP,VP,&B2D.x,&B2D.y,&B2D.z);
			gluProject(C3D->x,C3D->y,C3D->z,MM,MP,VP,&C2D.x,&C2D.y,&C2D.z);
		}
		else
		{
			CCVector3 A3Dp(A3D->x,A3D->y,A3D->z);
			CCVector3 B3Dp(B3D->x,B3D->y,B3D->z);
			CCVector3 C3Dp(C3D->x,C3D->y,C3D->z);
			trans.apply(A3Dp);
			trans.apply(B3Dp);
			trans.apply(C3Dp);
			gluProject(A3Dp.x,A3Dp.y,A3Dp.z,MM,MP,VP,&A2D.x,&A2D.y,&A2D.z);
			gluProject(B3Dp.x,B3Dp.y,B3Dp.z,MM,MP,VP,&B2D.x,&B2D.y,&B2D.z);
			gluProject(C3Dp.x,C3Dp.y,C3Dp.z,MM,MP,VP,&C2D.x,&C2D.y,&C2D.z);
		}

		//barycentric coordinates
		GLdouble detT =  (B2D.y-C2D.y) *      (A2D.x-C2D.x) + (C2D.x-B2D.x) *      (A2D.y-C2D.y);
		GLdouble l1   = ((B2D.y-C2D.y) * (clickPosX-C2D.x) + (C2D.x-B2D.x) * (clickPosY-C2D.y)) / detT;
		GLdouble l2   = ((C2D.y-A2D.y) * (clickPosX-C2D.x) + (A2D.x-C2D.x) * (clickPosY-C2D.y)) / detT;

		//does the point falls inside the triangle?
		if (l1 >= 0 && l1 <= 1.0 && l2 >= 0.0 && l2 <= 1.0)
		{
			double l1l2 = l1+l2;
			assert(l1l2 >= 0);
			if (l1l2 > 1.0)
			{
				l1 /= l1l2;
				l2 /= l1l2;
			}
			GLdouble l3 = 1.0-l1-l2;
			assert(l3 >= -1.0e-12);

			//now deduce the 3D position
			CCVector3d P(	l1 * A3D->x + l2 * B3D->x + l3 * C3D->x,
							l1 * A3D->y + l2 * B3D->y + l3 * C3D->y,
							l1 * A3D->z + l2 * B3D->z + l3 * C3D->z);
			double squareDist = (X-P).norm2();
			if (nearestTriIndex < 0 || squareDist < nearestSquareDist)
			{
				nearestSquareDist = squareDist;
				nearestTriIndex = static_cast<int>(i);
			}
		}
	}

	return (nearestTriIndex >= 0);
}
