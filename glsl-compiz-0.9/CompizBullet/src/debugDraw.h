/*
 * debugDrawer.h
 *
 *  Created on: Mar 12, 2010
 *      Author: alex
 */

#ifndef DEBUGDRAWER_H_
#define DEBUGDRAWER_H_

#include "LinearMath/btIDebugDraw.h"


/**
 * @see GLDebugDrawer in Bullet Demo Archive
 */
class BulletDebugDrawer : public btIDebugDraw
{
private:
	int m_debugMode;

public:

	BulletDebugDrawer();


	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& fromColor, const btVector3& toColor);

	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color);

	virtual void	drawSphere (const btVector3& p, btScalar radius, const btVector3& color);
	virtual void	drawBox (const btVector3& boxMin, const btVector3& boxMax, const btVector3& color, btScalar alpha);

	virtual void	drawTriangle(const btVector3& a,const btVector3& b,const btVector3& c,const btVector3& color,btScalar alpha);

	virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color);

	virtual void	reportErrorWarning(const char* warningString);

	virtual void	draw3dText(const btVector3& location,const char* textString);

	virtual void	setDebugMode(int debugMode);

	virtual int		getDebugMode() const;

};

inline void
BulletDebugDrawer::setDebugMode(int debugMode)
{
	m_debugMode = debugMode;
}

inline int
BulletDebugDrawer::getDebugMode() const
{
	return m_debugMode;
}
#endif /* DEBUGDRAWER_H_ */
