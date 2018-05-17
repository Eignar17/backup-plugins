/*
 * bullet.h
 *
 *  Created on: Mar 12, 2010
 *      Author: alex
 */

#ifndef BULLET_H_
#define BULLET_H_


#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>

#include <btBulletDynamicsCommon.h>
#include "BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h"
#include <tr1/memory>
#include <map>

#include "bullet_options.h"

/**
 * Bullet units are in meters
 */
/*Assuming openGL's right handed coordinate system, bullet units are seconds and meters*/
//useful classes: btClock, btHashMap<Key,Value>
//a good scaling factor would be 1/200 for a screensize of max 1980 and min 5 units i.e. pixels
class BulletMotionState : public btMotionState
{
public:
	BulletMotionState(CompWindow* w, const btTransform& startTrans = btTransform::getIdentity(),const btTransform& centerOfMassOffset = btTransform::getIdentity());
	~BulletMotionState();

	void getWorldTransform(btTransform& centerOfMassTransform) const;
	void setWorldTransform(const btTransform& centerOfMassTransform);
private:
	btTransform m_graphicsWorldTrans;
	btTransform m_lastGraphicsWorldTrans;
	btTransform	m_centerOfMassOffset;
	btTransform m_startWorldTrans;
	void*		m_userPointer;
	CompWindow* const window;
};

//ClosestRayResultCallback would not register a hit if collision filter is active
class ClosestRayResultCallbackCollideWithWindows : public btCollisionWorld::ClosestRayResultCallback
{
public:
	ClosestRayResultCallbackCollideWithWindows(const btVector3&	rayFromWorld,const btVector3&	rayToWorld);
};

class BulletScreen :
	public PluginClassHandler <BulletScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public BulletOptions
{
public:
	BulletScreen(CompScreen *);
	~BulletScreen();

	CompScreen *screen;
	GLScreen 	*gScreen;
	CompositeScreen *cScreen;

	// ScreenInterface
	void handleEvent (XEvent *);

//	// CompositeScreenInterface
	void preparePaint (int);
	void donePaint ();

	//GLScreenInterface
	bool glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix &, const CompRegion &,
		       CompOutput *, unsigned int);
	void glPaintTransformedOutput (const GLScreenPaintAttrib &,
		       		  const GLMatrix &, const CompRegion &,
		       		  CompOutput *, unsigned int);

	void
	setUpScreenBorders(float outputWidthScaled, float outputHeightScaled);
	void enableScreenBorders();
	void disableScreenBorders();
	void toggleScreenBorders();

	void invertTransformed(const GLMatrix&, CompOutput*);

	//accessors
	void addToWorld(const btRigidBody& body);
	void addToWorld(const btRigidBody& body, short group, short mask);
	void removeFromWorld(const btRigidBody& body);
	void removeFromWorld(const btTypedConstraint& constraint);
	//actions
	void enableDebugDraw();
	void disableDebugDraw();
	void toggleDebugDraw();

	bool
	toggleDebugDrawAction (CompAction *action,
				CompAction::State state,
				CompOption::Vector& options);
	bool
	toggleScreenBordersAction (CompAction *action,
				CompAction::State state,
				CompOption::Vector& options);
	bool
	initPhysicsForAllWindowsAction (CompAction *action,
				CompAction::State state,
				CompOption::Vector& options);

	void
	changeOption (CompOption* option,
			       BulletOptions::Options num);

	bool
	toggleSpringAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);

	bool
	attractInitiateAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);
	bool
	attractTerminateAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);
	bool
	repulseInitiateAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);
	bool
	repulseTerminateAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);
	bool
	moveInitiateAction (CompAction      *action,
		      CompAction::State state,
		      CompOption::Vector &options);
	bool
	moveTerminateAction (CompAction      *action,
		       CompAction::State state,
		       CompOption::Vector &options);

	void
	handleMotionEvent (
			       int	  xRoot,
			       int	  yRoot);
	//callback
	void checkAndHandleWindowVisibility();

	/**
	 * returns the CompWindow* the ray intersected with, NULL if no intersection
	 */
	CompWindow*
	checkRayIntersection();
	CompPoint getCurrentMousePos();
	float maxDivisor;
	bool isAttracted();
private:
	void initPhysics();
	btVector3 getRayTo(int x, int y);
	void updateMouse(const CompPoint&);
	void toggleMovePluginActions(bool enable);
private:
	enum MoveState
	{
		NONE,
		INIT,
		MOVING
	} mMoveState;

	bool attract;
	CompPoint mMousePollerPos;
	CompOutput* currentOutput;
	std::map<CompOutput*, btVector3> camForOutput;
	MousePoller		 pollHandle;
	btVector3 mRay;
	GLMatrix mModelViewMatrix;
	CompWindow* mGrabWindow;
	CompScreen::GrabHandle mGrab;
	Cursor mMoveCursor;
	int mReleaseButton;
	btPoint2PointConstraint* mPickConstraint;
	btRigidBody* pickedBody;
	btVector3 mHitPos;
	btVector3 mOldPickingPos;
	btScalar mOldPickingDist;
	CompPoint mMousePos;
	std::tr1::shared_ptr<btBroadphaseInterface> broadphase;
	std::tr1::shared_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
	std::tr1::shared_ptr<btCollisionDispatcher> dispatcher;
	std::tr1::shared_ptr<btSequentialImpulseConstraintSolver> solver;
	std::tr1::shared_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

	//For 2D Algorithms
	std::tr1::shared_ptr<btVoronoiSimplexSolver> m_simplexSolver;
	std::tr1::shared_ptr<btMinkowskiPenetrationDepthSolver> m_pdSolver;

	std::tr1::shared_ptr<btCollisionShape> groundShape;
	std::tr1::shared_ptr<btCollisionShape> upperBorderShape;
	std::tr1::shared_ptr<btCollisionShape> lowerBorderShape;
	std::tr1::shared_ptr<btCollisionShape> leftBorderShape;
	std::tr1::shared_ptr<btCollisionShape> rightBorderShape;
	std::tr1::shared_ptr<btDefaultMotionState> groundMotionState;
	std::tr1::shared_ptr<btDefaultMotionState> upperBorderMotionState;
	std::tr1::shared_ptr<btDefaultMotionState> lowerBorderMotionState;
	std::tr1::shared_ptr<btDefaultMotionState> leftBorderMotionState;
	std::tr1::shared_ptr<btDefaultMotionState> rightBorderMotionState;
	std::tr1::shared_ptr<btRigidBody> groundRigidBody;
	std::tr1::shared_ptr<btRigidBody> upperBorderRigidBody;
	std::tr1::shared_ptr<btRigidBody> lowerBorderRigidBody;
	std::tr1::shared_ptr<btRigidBody> leftBorderRigidBody;
	std::tr1::shared_ptr<btRigidBody> rightBorderRigidBody;

	std::tr1::shared_ptr<btCollisionShape> mMouseCollisionShape;
	std::tr1::shared_ptr<btMotionState> mMouseMotionState;
	std::tr1::shared_ptr<btRigidBody> mMouseRigidBody;
	std::tr1::shared_ptr<btCollisionShape> mMouseHookCollisionShape;
	std::tr1::shared_ptr<btMotionState> mMouseHookMotionState;
	std::tr1::shared_ptr<btRigidBody> mMouseHookRigidBody;

	std::tr1::shared_ptr<btGeneric6DofSpringConstraint> mSpringConstraint;
	std::tr1::shared_ptr<btPoint2PointConstraint> mMouseMoveConstraint;

	std::tr1::shared_ptr<btIDebugDraw> debugDrawer;
	bool debugDrawEnabled;
	bool screenBordersEnabled;
	bool springEnabled;
};

class BulletWindow :
	public PluginClassHandler <BulletWindow, CompWindow>,
	public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
public:

	BulletWindow (CompWindow *);
	~BulletWindow();

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow        *gWindow;

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		 const CompRegion &, unsigned int);

	bool damageRect (bool, const CompRect &);

	void activate ();

	void moveNotify (int, int, bool);

	void resizeNotify (int, int, int, int);

	void grabNotify (int, int, unsigned int, unsigned int);

	void ungrabNotify ();

	void windowNotify(CompWindowNotify);

	//enablePhysics();		//maybe for each methods a method in Screen is needed to call these methods on the active/focused window
	//disablePhysics();
	//togglePhysics();

	void initPhysics();
	void resetPhysics();
	void applyForce (const CompPoint& mousePos, bool attract);
	void updateDamping(const CompPoint& mousePos, float maxDivisor);
	void setForceMultiplier(int multiplier);
private:
	float myWidth();
	float myHeight();
	float myDepth();
	float centerX();
	float centerY();
	float centerZ();
	float myMass();
private:
	std::tr1::shared_ptr<btCollisionShape> windowShape;
	std::tr1::shared_ptr<btMotionState> windowMotionState;
	std::tr1::shared_ptr<btRigidBody> windowRigidBody;
	int forceMultiplier; //TODO:
};

class BulletPluginVTable :
	public CompPlugin::VTableForScreenAndWindow <BulletScreen, BulletWindow>
{
public:
	bool init ();
};


#endif /* BULLET_H_ */
