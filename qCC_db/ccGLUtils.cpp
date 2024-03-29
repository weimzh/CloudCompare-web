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

#include "ccGLUtils.h"

//Local
#include "ccLog.h"

//CCLib
#include <CCConst.h>

//*********** OPENGL TEXTURES ***********//

void ccGLUtils::DisplayTexture2DPosition(GLuint tex, int x, int y, int w, int h, unsigned char alpha/*=255*/)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glColor4ub(255, 255, 255, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,0.0);
    glVertex2i(x, y+h);
    glTexCoord2f(0.0,1.0);
    glVertex2i(x, y);
    glTexCoord2f(1.0,1.0);
    glVertex2i(x+w, y);
    glTexCoord2f(1.0,0.0);
    glVertex2i(x+w, y+h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);
}

void ccGLUtils::DisplayTexture2D(GLuint tex, int w, int h, unsigned char alpha/*=255*/)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    float halfW = float(w)*0.5;
    float halfH = float(h)*0.5;

    glColor4ub(255, 255, 255, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-halfW, -halfH);
    glTexCoord2f(1.0, 0.0);
    glVertex2f( halfW, -halfH);
    glTexCoord2f(1.0, 1.0);
    glVertex2f( halfW,  halfH);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(-halfW,  halfH);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void ccGLUtils::DisplayTexture2DCorner(GLuint tex, int w, int h, unsigned char alpha/*=255*/)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glColor4ub(255, 255, 255, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(0, 0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f( w, 0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f( w,  h);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(0,  h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#define READ_PIXELS_PER_LINE
void ccGLUtils::SaveTextureToArray(unsigned char* data, GLuint texID, unsigned w, unsigned h)
{
	assert(data);

#ifdef READ_PIXELS_PER_LINE
	//to avoid memory issues, we read line by line
	for (int i=0;i<(int)h;++i)
	{
		glReadPixels(0,i,w,1,GL_BGRA,GL_UNSIGNED_BYTE,data+((int)h-1-i)*(int)w*4);
		ccGLUtils::CatchGLError("ccGLUtils::SaveTextureToArray");
	}
#else
	//GLint width,height,format,align;
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	//glGetIntegerv(GL_PACK_ALIGNMENT,&align);

    //we grab texture data
    glBindTexture(GL_TEXTURE_2D, texID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

//*********** OPENGL MATRICES ***********//

void ccGLUtils::MultGLMatrices(const float* A, const float* B, float* dest)
{
    //we backup actual matrix...
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glLoadMatrixf(A);
    glMultMatrixf(B);
    glGetFloatv(GL_MODELVIEW_MATRIX, dest);

    //... and restore it
    glPopMatrix();
}

void ccGLUtils::TransposeGLMatrix(const float* A, float* dest)
{
    unsigned char i,j;

    for (i=0;i<4;i++)
        for (j=0;j<4;j++)
            dest[(i<<2)+j] = A[(j<<2)+i];
}

ccGLMatrix ccGLUtils::GenerateGLRotationMatrixFromVectors(const float* sourceVec, const float* destVec)
{
    //we compute scalar prod between the two vectors
    float ps = Vector3Tpl<float>::vdot(sourceVec,destVec);

    //we bound result (in case vecors are not exactly unit)
    if (ps>1.0)
        ps=1.0;
    else if (ps<-1.0)
        ps=-1.0;

    //we deduce angle from scalar prod
    float angle_deg = acos(ps)*CC_RAD_TO_DEG;

    //we compute rotation axis with scalar prod
    float axis[3];
    CCVector3::vcross(sourceVec,destVec,axis);

    //we eventually compute the rotation matrix with axis and angle
    return GenerateGLRotationMatrixFromAxisAndAngle(axis, angle_deg);
}

ccGLMatrix ccGLUtils::GenerateGLRotationMatrixFromAxisAndAngle(const float* axis, float angle_deg)
{
    //we backup actual matrix...
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glLoadIdentity();
    glRotatef(angle_deg, axis[0], axis[1], axis[2]);
	ccGLMatrix mat;
    glGetFloatv(GL_MODELVIEW_MATRIX, mat.data());

    //... and restore it
    glPopMatrix();

	return mat;
}

ccGLMatrix ccGLUtils::GenerateViewMat(CC_VIEW_ORIENTATION orientation)
{
	GLdouble eye[3] = {0.0, 0.0, 0.0};
	GLdouble top[3] = {0.0, 0.0, 0.0};

	//we look at (0,0,0) by default
    switch (orientation)
    {
    case CC_TOP_VIEW:
        eye[2] = 1.0;
        top[1] = 1.0;
        break;
    case CC_BOTTOM_VIEW:
        eye[2] = -1.0;
        top[1] = -1.0;
        break;
    case CC_FRONT_VIEW:
        eye[1] = -1.0;
        top[2] = 1.0;
        break;
    case CC_BACK_VIEW:
        eye[1] = 1.0;
        top[2] = 1.0;
        break;
    case CC_LEFT_VIEW:
        eye[0] = -1.0;
        top[2] = 1.0;
        break;
    case CC_RIGHT_VIEW:
        eye[0] = 1.0;
        top[2] = 1.0;
        break;
    case CC_ISO_VIEW_1:
        eye[0] = -1.0;
        eye[1] = -1.0;
        eye[2] = 1.0;
		top[0] = 1.0;
		top[1] = 1.0;
        top[2] = 1.0;
        break;
    case CC_ISO_VIEW_2:
        eye[0] = 1.0;
        eye[1] = 1.0;
        eye[2] = 1.0;
        top[0] = -1.0;
        top[1] = -1.0;
        top[2] = 1.0;
        break;
    }

    ccGLMatrix result;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(eye[0],eye[1],eye[2],0.0,0.0,0.0,top[0],top[1],top[2]);
    glGetFloatv(GL_MODELVIEW_MATRIX, result.data());
    result.data()[14] = 0.0; //annoying value (?!)
    glPopMatrix();

    return result;
}

bool ccGLUtils::CatchGLError(const char* context)
{
	GLenum err = glGetError();

	//see http://www.opengl.org/sdk/docs/man/xhtml/glGetError.xml
	switch(err)
	{
	case GL_NO_ERROR:
		return false;
		break;
	case GL_INVALID_ENUM:
		ccLog::Warning("[%s] OpenGL error: invalid enumerator",context);
		break;
	case GL_INVALID_VALUE:
		ccLog::Warning("[%s] OpenGL error: invalid value",context);
		break;
	case GL_INVALID_OPERATION:
		ccLog::Warning("[%s] OpenGL error: invalid operation",context);
		break;
	case GL_STACK_OVERFLOW:
		ccLog::Error("[%s] OpenGL error: stack overflow",context);
		break;
	case GL_STACK_UNDERFLOW:
		ccLog::Error("[%s] OpenGL error: stack underflow",context);
		break;
	case GL_OUT_OF_MEMORY:
		ccLog::Error("[%s] OpenGL error: out of memory",context);
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		ccLog::Warning("[%s] OpenGL error: invalid framebuffer operation",context);
		break;
	}

	return true;
}

void ccGLUtils::MakeLightsNeutral()
{
	GLint maxLightCount;
	glGetIntegerv(GL_MAX_LIGHTS,&maxLightCount);
	for (int i=0;i<maxLightCount;++i)
	{
		if (glIsEnabled(GL_LIGHT0+i))
		{
			float diffuse[4];
			float ambiant[4];
			float specular[4];

			glGetLightfv(GL_LIGHT0+i,GL_DIFFUSE,diffuse);
			glGetLightfv(GL_LIGHT0+i,GL_AMBIENT,ambiant);
			glGetLightfv(GL_LIGHT0+i,GL_SPECULAR,specular);

			diffuse[0]=diffuse[1]=diffuse[2]=(diffuse[0]+diffuse[1]+diffuse[2])/3.0f;		//gray 'mean' value
			ambiant[0]=ambiant[1]=ambiant[2]=(ambiant[0]+ambiant[1]+ambiant[2])/3.0f;		//gray 'mean' value
			specular[0]=specular[1]=specular[2]=(specular[0]+specular[1]+specular[2])/3.0f;	//gray 'mean' value

			glLightfv(GL_LIGHT0+i,GL_DIFFUSE,diffuse);
			glLightfv(GL_LIGHT0+i,GL_AMBIENT,ambiant);
			glLightfv(GL_LIGHT0+i,GL_SPECULAR,specular);
		}
	}
}
