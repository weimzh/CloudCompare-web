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

#include "ccIncludeGL.h"

//Local
#include "cc2DViewportLabel.h"

//CCLib
#include <CCConst.h>

//system
#include <string.h>

cc2DViewportLabel::cc2DViewportLabel(const char* name/*=0*/)
	: cc2DViewportObject(name ? name : "")
{
	//label rectangle
	memset(m_roi,0,sizeof(float)*4);
	setVisible(false);
}

void cc2DViewportLabel::setRoi(const float* roi)
{
	memcpy(m_roi,roi,sizeof(float)*4);
}

void cc2DViewportLabel::drawMeOnly(CC_DRAW_CONTEXT& context)
{
	//2D foreground only
	if (!MACRO_Foreground(context) || !MACRO_Draw2D(context))
		return;

	//test viewport parameters
	const ccViewportParameters& params = context._win->getViewportParameters();

	//general parameters
	if (params.perspectiveView != m_params.perspectiveView
		|| params.objectCenteredView != m_params.objectCenteredView
		|| params.pixelSize != m_params.pixelSize)
			return;

	//test base view matrix
	for (unsigned i=0;i<12;++i)
		if (fabs(params.viewMat.data()[i] - m_params.viewMat.data()[i]) > ZERO_TOLERANCE)
			return;

	if (m_params.perspectiveView)
	{
		if (params.fov != m_params.fov || params.aspectRatio != m_params.aspectRatio)
			return;

		if (   (params.pivotPoint - m_params.pivotPoint).norm() > ZERO_TOLERANCE
			|| (params.cameraCenter - m_params.cameraCenter).norm() > ZERO_TOLERANCE)
			return;
	}

	glPushAttrib(GL_LINE_BIT);

	float relativeZoom = 1.0f;
	float dx = 0.0f;
	float dy = 0.0f;
	if (!m_params.perspectiveView) //ortho mode
	{
		//Screen pan & pivot compensation
		float totalZoom = m_params.zoom/m_params.pixelSize;
		float winTotalZoom = params.zoom/params.pixelSize;
		relativeZoom = winTotalZoom/totalZoom;

		CCVector3 dC = m_params.cameraCenter - params.cameraCenter;

		CCVector3 P = m_params.pivotPoint - params.pivotPoint;
		m_params.viewMat.apply(P);

		dx = dC.x + P.x;
		dy = dC.y + P.y;

		dx *= winTotalZoom;
		dy *= winTotalZoom;
	}

	//thick dotted line
	glLineWidth(2);
	glLineStipple(1, 0xAAAA);
	glEnable(GL_LINE_STIPPLE);

	const colorType* defaultColor = m_selected ? ccColor::red : context.textDefaultCol;
	glColor3ubv(defaultColor); 

	glBegin(GL_LINE_LOOP);
	glVertex2f(dx+m_roi[0]*relativeZoom,dy+m_roi[1]*relativeZoom);
	glVertex2f(dx+m_roi[2]*relativeZoom,dy+m_roi[1]*relativeZoom);
	glVertex2f(dx+m_roi[2]*relativeZoom,dy+m_roi[3]*relativeZoom);
	glVertex2f(dx+m_roi[0]*relativeZoom,dy+m_roi[3]*relativeZoom);
	glEnd();

	glPopAttrib();

	//title
	std::string title(getName());
	if (title.length() > 0)
	{
		int titleHeight = 16;

		int xStart = (int)(dx+0.5f*(float)context.glW+std::min<float>(m_roi[0],m_roi[2])*relativeZoom);
		int yStart = (int)(dy+0.5f*(float)context.glH+std::min<float>(m_roi[1],m_roi[3])*relativeZoom);

		context._win->displayText(title,xStart,yStart-5-titleHeight,ccGenericGLDisplay::ALIGN_DEFAULT,0,defaultColor);
	}
}
