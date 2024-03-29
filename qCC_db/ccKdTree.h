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

#ifndef CC_KD_TREE_HEADER
#define CC_KD_TREE_HEADER

//CCLib
#include <TrueKdTree.h>

//Local
#include "ccHObject.h"

//System
#include <set>

class ccGenericPointCloud;

namespace ccLib {
class GenericProgressCallback;
};

//! KD-tree structure
/** Extends the CCLib::TrueKdTree class.
**/
#ifdef QCC_DB_USE_AS_DLL
#include "qCC_db_dll.h"
class QCC_DB_DLL_API ccKdTree : public CCLib::TrueKdTree, public ccHObject
#else
class ccKdTree : public CCLib::TrueKdTree, public ccHObject
#endif
{
public:

	//! Default constructor
	/** \param aCloud a point cloud
	**/
	ccKdTree(ccGenericPointCloud* aCloud);

	//! Multiplies the bounding-box of the tree
	/** If the cloud coordinates are simply multiplied by the same factor,
		there is no use to recompute the tree structure. It's sufficient
		to update its bounding-box.
		\param  multFactor multiplication factor
	**/
	void multiplyBoundingBox(const PointCoordinateType multFactor);

	//! Translates the bounding-box of the tree
	/** If the cloud has simply been translated, there is no use to recompute
		the tree structure. It's sufficient to update its bounding-box.
		\param T translation vector
	**/
	void translateBoundingBox(const CCVector3& T);

    //! Returns class ID
    virtual CC_CLASS_ENUM getClassID() const { return CC_POINT_KDTREE; }

    //Inherited from ccHObject
    virtual ccBBox getMyOwnBB();
    virtual ccBBox getDisplayBB();

	//! Flag points with cell index (as a scalar field)
	bool convertCellIndexToSF();

	//! Fuses cells
	/** Creates a new scalar fields with the groups indexes.
		\param maxRMS max RMS after fusion
		\param maxAngle_deg maximum angle between two sets to allow fusion (in degrees)
		\param overlapCoef maximum relative distance between two sets to accept fusion (1 = no distance, < 1 = overlap, > 1 = gap)
	**/
	bool fuseCells(double maxRMS, double maxAngle_deg, double overlapCoef = 1.0, bool closestFirst = true, CCLib::GenericProgressCallback* progressCb = 0);

	//! Returns the bounding-box of a given cell
	ccBBox getCellBBox(BaseNode* node) const;

	//! A set of leaves
	typedef std::set<Leaf*> LeafSet;

	//! Returns the neighbor leaves around a given cell
	bool getNeighborLeaves(BaseNode* cell, ccKdTree::LeafSet& neighbors, const int* userDataFilter = 0);

	//! Returns associated (generic) point cloud
	inline ccGenericPointCloud* associatedGenericCloud() const { return m_associatedGenericCloud; }

protected:

    //Inherited from ccHObject
    void drawMeOnly(CC_DRAW_CONTEXT& context);

    ccGenericPointCloud* m_associatedGenericCloud;

};

#endif //CC_KD_TREE_HEADER
