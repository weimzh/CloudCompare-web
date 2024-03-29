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

#ifndef CC_INTERACTOR_HEADER
#define CC_INTERACTOR_HEADER

//! Interactor interface (entity that can be dragged or clicked in a 3D view)
#ifdef QCC_DB_USE_AS_DLL
#include "qCC_db_dll.h"
class QCC_DB_DLL_API ccInteractor
#else
class ccInteractor
#endif
{
public:

	//! Called on mouse click
	virtual bool acceptClick(int x, int y, int button) { return false; }

	//! Called on mouse move (for 2D interactors)
	/** \return true if a movement occurs
	**/
	virtual bool move2D(int x, int y, int dx, int dy, int screenWidth, int screenHeight) { return false; }

	//! Called on mouse move (for 3D interactors)
	/** \return true if a movement occurs
	**/
	virtual bool move3D(const CCVector3& u) { return false; }

};

#endif //CC_INTERACTOR_HEADER
