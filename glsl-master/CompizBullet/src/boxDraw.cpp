/*
 * boxDraw.cpp
 *
 *  Created on: Mar 12, 2010
 *      Author: alex
 */

#include "boxDraw.h"
#include <GL/gl.h>

BoxDrawer::BoxDrawer(float posX, float posY, float posZ, float width,
		float height, float depth) :
	pos(posX, posY, posZ), width(width), height(height), depth(depth)

{
	height /= 2; //half extends
	width /= 2;
	depth /= 2;
	this->vertices[0].x = -width;
	this->vertices[0].y = height;
	this->vertices[0].z = depth;

	this->vertices[1].x = width;
	this->vertices[1].y = height;
	this->vertices[1].z = depth;

	this->vertices[2].x = width;
	this->vertices[2].y = height;
	this->vertices[2].z = -depth;

	this->vertices[3].x = -width;
	this->vertices[3].y = height;
	this->vertices[3].z = -depth;

	this->vertices[4].x = -width;
	this->vertices[4].y = -height;
	this->vertices[4].z = depth;

	this->vertices[5].x = width;
	this->vertices[5].y = -height;
	this->vertices[5].z = depth;

	this->vertices[6].x = width;
	this->vertices[6].y = -height;
	this->vertices[6].z = -depth;

	this->vertices[7].x = -width;
	this->vertices[7].y = -height;
	this->vertices[7].z = -depth;
}

BoxDrawer::~BoxDrawer()
{
}

unsigned int BoxDrawer::indices[6][4] =
{
{ 0, 1, 2, 3 }, //top
		{ 0, 3, 7, 4 }, //left
		{ 3, 2, 6, 7 }, //back
		{ 2, 1, 5, 6 }, //right
		{ 0, 4, 5, 1 }, //front
		{ 4, 7, 6, 5 }, //bottom
		};

void BoxDrawer::draw()
{

	float colors[6][3] =
	{
	{ 1.0, 0.0, 0.0 }, //red
			{ 1.0, 1.0, 0.0 }, //yellow
			{ 0.0, 1.0, 0.0 }, //green
			{ 0.0, 1.0, 1.0 }, //cyan
			{ 0.0, 0.0, 1.0 }, //blue
			{ 1.0, 0.0, 1.0 } //purple
	};
	//	int length = sizeof(BoxDrawer::indices) / sizeof (BoxDrawer::indices[0]);
	glFrontFace(GL_CW);
	for (int i = 0; i < 6; i++)
	{
		glColor3fv(colors[i]);
		glBegin(GL_QUADS);
		for (int j = 0; j < 4; j++)
		{
			glVertex3f(pos.x + vertices[BoxDrawer::indices[i][j]].x, pos.y
					+ vertices[BoxDrawer::indices[i][j]].y, pos.z
					+ vertices[BoxDrawer::indices[i][j]].z);
		}
		glEnd();
	}
	glFrontFace(GL_CCW);
}
