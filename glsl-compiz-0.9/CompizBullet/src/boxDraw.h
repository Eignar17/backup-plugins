/*
 * boxDrawer.h
 *
 *  Created on: Mar 12, 2010
 *      Author: alex
 */

#ifndef BOXDRAWER_H_
#define BOXDRAWER_H_

struct Vertex
{
	Vertex() : x(0.0f), y(0.0f), z(0.0f) {}
	Vertex(float x, float y, float z) : x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
};

class BoxDrawer
{
public:
	BoxDrawer(float posX, float posY, float posZ, float width, float height, float depth);
	~BoxDrawer();
	void draw();
private:
	Vertex pos;
	float width;
	float height;
	float depth;
	Vertex vertices[8];
	static unsigned int indices[6][4]; //6 faces, each 4 vertices
};

#endif /* BOXDRAWER_H_ */
