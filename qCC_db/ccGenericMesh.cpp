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

#include "ccGenericMesh.h"

//local
#include "ccGenericPointCloud.h"
#include "ccPointCloud.h"
#include "ccNormalVectors.h"
#include "ccMaterialSet.h"

//system
#include <assert.h>

ccGenericMesh::ccGenericMesh(ccGenericPointCloud* associatedCloud, std::string name/*=""*/)
	: GenericIndexedMesh()
	, ccHObject(name)
	, m_showWired(false)
	, m_associatedCloud(associatedCloud)
	, m_triNormals(0)
	, m_texCoords(0)
	, m_materials(0)
{
    setVisible(true);
    lockVisibility(false);
}

ccGenericMesh::~ccGenericMesh()
{
	clearTriNormals();
	setMaterialSet(0);
	setTexCoordinatesTable(0);
}

ccGenericPointCloud* ccGenericMesh::getAssociatedCloud() const
{
    return m_associatedCloud;
}

void ccGenericMesh::setAssociatedCloud(ccGenericPointCloud* cloud)
{
    m_associatedCloud=cloud;
}

void ccGenericMesh::showWired(bool state)
{
    m_showWired = state;
}

bool ccGenericMesh::isShownAsWire()const
{
    return m_showWired;
}

bool ccGenericMesh::hasColors() const
{
	return (m_associatedCloud ? m_associatedCloud->hasColors() : false);
}

bool ccGenericMesh::hasNormals() const
{
	return ((m_associatedCloud ? m_associatedCloud->hasNormals() : false) || hasTriNormals());
}

bool ccGenericMesh::hasDisplayedScalarField() const
{
	return (m_associatedCloud ? m_associatedCloud->hasDisplayedScalarField() : false);
}

bool ccGenericMesh::hasScalarFields() const
{
	return (m_associatedCloud ? m_associatedCloud->hasScalarFields() : false);
}

bool ccGenericMesh::computeNormals()
{
    if (!m_associatedCloud || !m_associatedCloud->isA(CC_POINT_CLOUD)) //TODO
        return false;
	
	unsigned triCount = size();
	if (triCount==0)
	{
		ccLog::Error("[ccGenericMesh::computeNormals] Empty mesh!");
        return false;
	}
	unsigned vertCount=m_associatedCloud->size();
	if (vertCount<3)
	{
		ccLog::Error("[ccGenericMesh::computeNormals] Not enough vertices! (<3)");
        return false;
	}

    ccPointCloud* cloud = static_cast<ccPointCloud*>(m_associatedCloud);

	//we instantiate a temporary structure to store each vertex normal (uncompressed)
	NormsTableType* theNorms = new NormsTableType;
	if (!theNorms->reserve(vertCount))
	{
		theNorms->release();
		return false;
	}
    theNorms->fill(0);

    //allocate compressed normals array on vertices cloud
    bool normalsWereAllocated = cloud->hasNormals();
    if (!normalsWereAllocated && !cloud->resizeTheNormsTable())
	{
		theNorms->release();
		return false;
	}

	//for each triangle
	placeIteratorAtBegining();
	{
		for (unsigned i=0;i<triCount;++i)
		{
			CCLib::TriangleSummitsIndexes* tsi = getNextTriangleIndexes();

			assert(tsi->i1<vertCount && tsi->i2<vertCount && tsi->i3<vertCount);
			const CCVector3 *A = cloud->getPoint(tsi->i1);
			const CCVector3 *B = cloud->getPoint(tsi->i2);
			const CCVector3 *C = cloud->getPoint(tsi->i3);

			//compute face normal (right hand rule)
			CCVector3 N = (*B-*A).cross(*C-*A);
			//N.normalize(); //DGM: no normalization = weighting by surface!

			//we add this normal to all triangle vertices
			PointCoordinateType* N1 = theNorms->getValue(tsi->i1);
			CCVector3::vadd(N1,N.u,N1);
			PointCoordinateType* N2 = theNorms->getValue(tsi->i2);
			CCVector3::vadd(N2,N.u,N2);
			PointCoordinateType* N3 = theNorms->getValue(tsi->i3);
			CCVector3::vadd(N3,N.u,N3);
		}
	}

	//for each vertex
	{
		for (unsigned i=0;i<vertCount;i++)
		{
			PointCoordinateType* N = theNorms->getValue(i);
			CCVector3::vnormalize(N);
			cloud->setPointNormal(i,N);
			theNorms->forwardIterator();
		}
	}

    showNormals(true);
	if (!normalsWereAllocated)
        cloud->showNormals(true);

	//theNorms->clear();
	theNorms->release();
	theNorms=0;

	return true;
}

bool ccGenericMesh::normalsShown() const
{
	return (ccHObject::normalsShown() || triNormsShown());
}

void ccGenericMesh::showNormals(bool state)
{
	showTriNorms(state);
	ccHObject::showNormals(state);
}

bool ccGenericMesh::processScalarField(MESH_SCALAR_FIELD_PROCESS process)
{
	if (!m_associatedCloud || !m_associatedCloud->isScalarFieldEnabled())
        return false;

	unsigned nPts = m_associatedCloud->size();

	//instantiate memory for per-vertex mean SF
	ScalarType* meanSF = new ScalarType[nPts];
	if (!meanSF)
	{
		//Not enough memory!
		return false;
	}

	//per-vertex counters
	unsigned *count = new unsigned[nPts];
	if (!count)
	{
		//Not enough memory!
		delete[] meanSF;
		return false;
	}

	//init arrays
	unsigned i;
	for (i=0;i<nPts;++i)
	{
		meanSF[i] = m_associatedCloud->getPointScalarValue(i);
		count[i] = 1;
	}

	//for each triangle
	unsigned nTri = size();
	placeIteratorAtBegining();
	for (i=0;i<nTri;++i)
	{
		const CCLib::TriangleSummitsIndexes* tsi = getNextTriangleIndexes(); //DGM: getNextTriangleIndexes is faster for mesh groups!

		//compute the sum of all connected vertices SF values
		meanSF[tsi->i1] += m_associatedCloud->getPointScalarValue(tsi->i2);
		meanSF[tsi->i2] += m_associatedCloud->getPointScalarValue(tsi->i3);
		meanSF[tsi->i3] += m_associatedCloud->getPointScalarValue(tsi->i1);

		//TODO DGM: we could weight this by the vertices distance?
		++count[tsi->i1];
		++count[tsi->i2];
		++count[tsi->i3];
	}

	for (i=0;i<nPts;++i)
		meanSF[i] /= (ScalarType)count[i];

	switch (process)
	{
	case SMOOTH_MESH_SF:
		{
			//Smooth = mean value
			for (i=0;i<nPts;++i)
				m_associatedCloud->setPointScalarValue(i,meanSF[i]);
		}
		break;
	case ENHANCE_MESH_SF:
		{
			//Enhance = old value + (old value - mean value)
			for (i=0;i<nPts;++i)
			{
				ScalarType v = 2.0f*m_associatedCloud->getPointScalarValue(i) - meanSF[i];
				m_associatedCloud->setPointScalarValue(i,v > 0.0f ? v : 0.0f);
			}
		}
		break;
	}

	delete[] meanSF;
	delete[] count;

	return true;
}

void ccGenericMesh::drawMeOnly(CC_DRAW_CONTEXT& context)
{
    if (MACRO_Draw2D(context))
    {
        if (MACRO_Foreground(context) && !context.sfColorScaleToDisplay)
        {
            if (!m_associatedCloud || !m_associatedCloud->isA(CC_POINT_CLOUD))
                return;

            ccPointCloud* cloud = static_cast<ccPointCloud*>(m_associatedCloud);

            //we just want to display the current SF scale if the vertices cloud is hidden
            //(otherwise, it will take the SF display in charge)
            if (cloud->isEnabled() && cloud->isVisible() && cloud->sfColorScaleShown())
                return;

            //we must also check that the parent is not a mesh itself with the same vertices! (in
            //which case it will also take that in charge)
            ccHObject* parent = getParent();
            if (parent && parent->isKindOf(CC_MESH) && (static_cast<ccGenericMesh*>(parent)->getAssociatedCloud() == m_associatedCloud))
                return;

            cloud->addColorRampInfo(context);
            //cloud->drawScale(context);
        }
    }
	//*/
}

void ccGenericMesh::setTriNormsTable(NormsIndexesTableType* triNormsTable, bool autoReleaseOldTable/*=true*/)
{
	if (m_triNormals == triNormsTable)
		return;

	if (m_triNormals && autoReleaseOldTable)
	{
		int childIndex = getChildIndex(m_triNormals);
		m_triNormals->release();
		m_triNormals=0;
		if (childIndex>=0)
			removeChild(childIndex);
	}

	m_triNormals = triNormsTable;
	if (m_triNormals)
		m_triNormals->link();
}

void ccGenericMesh::setTexCoordinatesTable(TextureCoordsContainer* texCoordsTable, bool autoReleaseOldTable/*=true*/)
{
	if (m_texCoords == texCoordsTable)
		return;

	if (m_texCoords && autoReleaseOldTable)
	{
		int childIndex = getChildIndex(m_texCoords);
		m_texCoords->release();
		m_texCoords=0;
		if (childIndex>=0)
			removeChild(childIndex);
	}

	m_texCoords = texCoordsTable;
	if (m_texCoords)
		m_texCoords->link();
}

void ccGenericMesh::setMaterialSet(ccMaterialSet* materialSet, bool autoReleaseOldMaterialSet/*=true*/)
{
	if (m_materials == materialSet)
		return;

	if (m_materials && autoReleaseOldMaterialSet)
	{
		int childIndex = getChildIndex(m_materials);
		m_materials->release();
		m_materials=0;
		if (childIndex>=0)
			removeChild(childIndex);
	}

	m_materials = materialSet;
	if (m_materials)
		m_materials->link();

	//update display (for textures!)
	setDisplay(m_currentDisplay);
}

void ccGenericMesh::applyGLTransformation(const ccGLMatrix& trans)
{
	ccHObject::applyGLTransformation(trans);

	//vertices should be handled another way!

    //we must take care of the triangle normals!
	if (m_triNormals && (!getParent() || !getParent()->isKindOf(CC_MESH)))
    {
        bool recoded = false;

        //if there is more triangle normals than the size of the compressed
		//normals array, we recompress the array instead of recompressing each normal
		unsigned i,numTriNormals = m_triNormals->currentSize();
        if (numTriNormals>ccNormalVectors::GetNumberOfVectors())
        {
            NormsIndexesTableType* newNorms = new NormsIndexesTableType;
            if (newNorms->reserve(ccNormalVectors::GetNumberOfVectors()))
            {
                for (i=0;i<ccNormalVectors::GetNumberOfVectors();i++)
                {
                    CCVector3 new_n(ccNormalVectors::GetNormal(i));
                    trans.applyRotation(new_n);
                    normsType newNormIndex = ccNormalVectors::GetNormIndex(new_n.u);
                    newNorms->addElement(newNormIndex);
                }

                m_triNormals->placeIteratorAtBegining();
                for (i=0;i<numTriNormals;i++)
                {
                    m_triNormals->setValue(i,newNorms->getValue(m_triNormals->getCurrentValue()));
                    m_triNormals->forwardIterator();
                }
                recoded=true;
            }
            newNorms->clear();
			newNorms->release();
			newNorms=0;
        }

        //if there is less triangle normals than the compressed normals array size
        //(or if there is not enough memory to instantiate the temporary array),
		//we recompress each normal ...
        if (!recoded)
        {
            //on recode direct chaque normale
            m_triNormals->placeIteratorAtBegining();
            for (i=0;i<numTriNormals;i++)
            {
                normsType* _theNormIndex = m_triNormals->getCurrentValuePtr();
                CCVector3 new_n(ccNormalVectors::GetNormal(*_theNormIndex));
                trans.applyRotation(new_n.u);
                *_theNormIndex = ccNormalVectors::GetNormIndex(new_n.u);
                m_triNormals->forwardIterator();
            }
        }
	}
	else
	{
		//TODO: process failed!
	}
}

bool ccGenericMesh::laplacianSmooth(unsigned nbIteration, float factor, CCLib::GenericProgressCallback* progressCb/*=0*/)
{
	if (!m_associatedCloud)
		return false;

	//vertices
	unsigned vertCount = m_associatedCloud->size();
	//triangles
	unsigned faceCount = size();
	if (!vertCount || !faceCount)
		return false;

	GenericChunkedArray<3,PointCoordinateType>* verticesDisplacement = new GenericChunkedArray<3,PointCoordinateType>;
	if (!verticesDisplacement->resize(vertCount))
	{
		//not enough memory
		verticesDisplacement->release();
		return false;
	}

	//compute the number of edges to which belong each vertex
	unsigned* edgesCount = new unsigned[vertCount];
	if (!edgesCount)
	{
		//not enough memory
		verticesDisplacement->release();
		return false;
	}
	memset(edgesCount, 0, sizeof(unsigned)*vertCount);
	placeIteratorAtBegining();
	for(unsigned j=0; j<faceCount; j++)
	{
		const CCLib::TriangleSummitsIndexes* tri = getNextTriangleIndexes();
		edgesCount[tri->i1]+=2;
		edgesCount[tri->i2]+=2;
		edgesCount[tri->i3]+=2;
	}

	//progress dialog
	CCLib::NormalizedProgress* nProgress = 0;
	if (progressCb)
	{
		unsigned totalSteps = nbIteration;
		nProgress = new CCLib::NormalizedProgress(progressCb,totalSteps);
		progressCb->setMethodTitle("Laplacian smooth");
		char buf[256];
		sprintf(buf, "Iterations: %u\nVertices: %u\nFaces: %u", nbIteration, vertCount, faceCount);
		progressCb->setInfo(buf);
		progressCb->start();
	}

	//repeat Laplacian smoothing iterations
	for(unsigned iter = 0; iter < nbIteration; iter++)
	{
		verticesDisplacement->fill(0);

		//for each triangle
		placeIteratorAtBegining();
		for(unsigned j=0; j<faceCount; j++)
		{
			const CCLib::TriangleSummitsIndexes* tri = getNextTriangleIndexes();

			const CCVector3* A = m_associatedCloud->getPoint(tri->i1);
			const CCVector3* B = m_associatedCloud->getPoint(tri->i2);
			const CCVector3* C = m_associatedCloud->getPoint(tri->i3);

			CCVector3 dAB = (*B-*A);
			CCVector3 dAC = (*C-*A);
			CCVector3 dBC = (*C-*B);

			CCVector3* dA = (CCVector3*)verticesDisplacement->getValue(tri->i1);
			(*dA) += dAB+dAC;
			CCVector3* dB = (CCVector3*)verticesDisplacement->getValue(tri->i2);
			(*dB) += dBC-dAB;
			CCVector3* dC = (CCVector3*)verticesDisplacement->getValue(tri->i3);
			(*dC) -= dAC+dBC;
		}

		if (nProgress && !nProgress->oneStep())
		{
			//cancelled by user
			break;
		}

		//apply displacement
		verticesDisplacement->placeIteratorAtBegining();
		for (unsigned i=0; i<vertCount; i++)
		{
			//this is a "persistent" pointer and we know what type of cloud is behind ;)
			CCVector3* P = const_cast<CCVector3*>(m_associatedCloud->getPointPersistentPtr(i));
			const CCVector3* d = (const CCVector3*)verticesDisplacement->getValue(i);
			(*P) += (*d)*(factor/(PointCoordinateType)edgesCount[i]);
		}
	}

	m_associatedCloud->updateModificationTime();

	if (hasNormals())
		computeNormals();

	if (verticesDisplacement)
		verticesDisplacement->release();
	verticesDisplacement=0;

	if (edgesCount)
		delete[] edgesCount;
	edgesCount=0;

	if (nProgress)
		delete nProgress;
	nProgress=0;

	return true;
}

bool ccGenericMesh::convertMaterialsToVertexColors()
{
	if (!hasMaterials())
	{
		ccLog::Warning("[ccGenericMesh::convertMaterialsToVertexColors] Mesh has no material!");
		return false;
	}

	if (!m_associatedCloud->isA(CC_POINT_CLOUD))
	{
		ccLog::Warning("[ccGenericMesh::convertMaterialsToVertexColors] Need a true point cloud as vertices!");
		return false;
	}

	ccPointCloud* cloud = static_cast<ccPointCloud*>(m_associatedCloud);
	if (!cloud->resizeTheRGBTable(true))
	{
		ccLog::Warning("[ccGenericMesh::convertMaterialsToVertexColors] Failed to resize vertices color table! (not enough memory?)");
		return false;
	}

	//now scan all faces and get the vertex color each time
	unsigned faceCount = size();

	placeIteratorAtBegining();
	for (unsigned i=0; i<faceCount; ++i)
	{
		const CCLib::TriangleSummitsIndexes* tsi = getNextTriangleIndexes();
		for (unsigned char j=0; j<3; ++j)
		{
			colorType rgb[3];
			if (getVertexColorFromMaterial(i,j,rgb,true))
			{
				//FIXME: could we be smarter? (we process each point several times! And we assume the color is always the same...)
				cloud->setPointColor(tsi->i[j],rgb);
			}
		}
	}

	return true;
}
