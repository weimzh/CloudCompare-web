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

#ifndef CC_COLOR_SCALES_MANAGER_HEADER
#define CC_COLOR_SCALES_MANAGER_HEADER

//Local
#include "ccColorScale.h"

#include <string>
#include <map>

//! Color scales manager/container
#ifdef QCC_DB_USE_AS_DLL
#include "qCC_db_dll.h"
class QCC_DB_DLL_API ccColorScalesManager
#else
class ccColorScalesManager
#endif
{
public:
	//! Returns unique instance
	static ccColorScalesManager* GetUniqueInstance();

	//! Releases unique instance
	static void ReleaseUniqueInstance();

	//! Pre-defined color scales (all relative - i.e. expand to actual SF)
	enum DEFAULT_SCALE	{	BGYR			=	0,		/**< Blue-Green-Yellow-Red ramp (default for distances display) */
							GREY			=	1,		/**< Grey ramp (default for Global Illumination) */
							BWR				=	2,		/**< Blue-White-Red ramp (for signed SF)*/
							RY				=	3,		/**< Red-Yellow ramp */
							RW				=	4,		/**< Red-White ramp */
							ABS_NORM_GREY	=	5,		/**< Absolute normalized grey ramp (intensities between 0 and 1) */
	};

	//! Returns a pre-defined color scale UUID
	static std::string GetDefaultScaleUUID(int scale) { return std::to_string(scale); }

	//! Returns a pre-defined color scale (static shortcut)
	static ccColorScale::Shared GetDefaultScale(DEFAULT_SCALE scale = BGYR)
	{
		ccColorScalesManager* instance = GetUniqueInstance();
		return instance ? instance->getDefaultScale(scale) : ccColorScale::Shared(0);
	}

	//! Returns a pre-defined color scale
	ccColorScale::Shared getDefaultScale(DEFAULT_SCALE scale) { return getScale(GetDefaultScaleUUID(scale)); }

	//! Returns a color scale based on its UUID
	ccColorScale::Shared getScale(std::string UUID) const;

	//! Adds a new color scale
	void addScale(ccColorScale::Shared scale);

	//! Removes a color scale
	/** Warning: can't remove default scales!
	**/
	void removeScale(std::string UUID);

	//! Color scales map type
	typedef std::map< std::string, ccColorScale::Shared > ScalesMap;

	//! Access to the internal map
	ScalesMap& map() { return m_scales; }

	//! Access to the internal map (const)
	const ScalesMap& map() const { return m_scales; }

	//! Loads custom color scales from persistent settings
	void fromPersistentSettings();

	//! Save custom color scales to persistent settings
	void toPersistentSettings() const;

protected:

	//! Default constructor
	ccColorScalesManager();

	//! Destructor
	virtual ~ccColorScalesManager();

	//! Creates a pre-defined color scale
	static ccColorScale::Shared Create(DEFAULT_SCALE scaleType);

	//! Color scales
	ScalesMap m_scales;

};

#endif //CC_COLOR_SCALES_MANAGER_HEADER
