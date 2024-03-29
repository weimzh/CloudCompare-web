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

#ifndef CC_RENDERING_TOOLS_HEADER
#define CC_RENDERING_TOOLS_HEADER

//qCC_db
#include <ccDrawableObject.h>

class ccGBLSensor;

class ccRenderingTools
{
public:

    //! Displays the colored scale corresponding to the currently activated context scalar field
	/** Its appearance depends on the scalar fields min and max displayed
        values, min and max saturation values, and also the selected
		color ramp.
		\param context OpenGL context description
    **/
	static void DrawColorRamp(const CC_DRAW_CONTEXT& context);

};

#endif //CC_RENDERING_TOOLS_HEADER
