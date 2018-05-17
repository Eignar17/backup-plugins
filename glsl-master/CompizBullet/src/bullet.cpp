#include <iostream>
#include <cmath>
#include "boxDraw.h"
#include "bullet.h"
#include "debugDraw.h"
#include <GL/glu.h>
#include <X11/cursorfont.h>
#include "BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h"
#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "BulletCollision/CollisionShapes/btConvex2dShape.h"
#include <core/atoms.h>
#include <cmath>

#define USE_2D_SHAPES 1
#define VERBOSE_TIMESTEPPING_CONSOLEOUTPUT 1
#define DEBUG_CONSOLE_OUTPUT 0
COMPIZ_PLUGIN_20090315 (bullet, BulletPluginVTable);

//
//for updating kinetic obj e.g. object created by mouse ray: pos = getMotionState()->getPos; pos += incrPos; getMotionState()->setPos(pos);

//used to determine which objects are allowed to collide with each other, values can be OR'ed
//have to be set on both colliding parties, see also http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=3040
namespace Bullet {
	enum CollisionType {
		NOTHING = 0,
		GROUND = 1 << 1,
		WINDOW = 1 << 2,
		RAY = 1 << 3,
		MOUSE = 1 << 4
	};

	static const float SCALE_FACTOR = 200.0f;

	//This function is called by bullet after every tick
	void tickCallback(btDynamicsWorld *world, btScalar timeStep) {
		BulletScreen* bs = BulletScreen::get(::screen);
		CompPoint pos = bs->getCurrentMousePos();
		for (int i=0; i<world->getNumCollisionObjects(); i++)
		{
			btRigidBody* body = btRigidBody::upcast(world->getCollisionObjectArray()[i]);
			if (body && body->getUserPointer())
			{
				BulletWindow* bw = static_cast<BulletWindow*>(body->getUserPointer());
//				bw->updateDamping(pos, bs->maxDivisor);
				bw->applyForce(pos, bs->isAttracted());
			}
		}
	}

	struct
	RigidBodyDeleter
	{
		void
		operator()(btRigidBody* rb)
		{
			BulletScreen::get(::screen)->removeFromWorld(*rb);
			delete rb;
		}
	};

	struct
	ConstraintDeleter
	{
		void
		operator()(btTypedConstraint* constraint)
		{
			BulletScreen::get(::screen)->removeFromWorld(*constraint);
			delete constraint;
		}
	};

	static bool
	isMoveableWindow(CompWindow* w)
	{
		if (w->destroyed ())
			return false;

		if (!w->managed ())
		    return false;

		if (!(w->actions () & CompWindowActionMoveMask))
			return false;

		if (w->type () & (CompWindowTypeDesktopMask |
			       CompWindowTypeDockMask    |
			       CompWindowTypeFullscreenMask))
		    return false;

		if (w->overrideRedirect ())
		    return false;

		return true;
	}
}

ClosestRayResultCallbackCollideWithWindows::ClosestRayResultCallbackCollideWithWindows(const btVector3&	rayFromWorld,const btVector3&	rayToWorld)
: ClosestRayResultCallback(rayFromWorld, rayToWorld)
{
	m_collisionFilterGroup = Bullet::RAY;
	m_collisionFilterMask = Bullet::WINDOW;
}

BulletMotionState::BulletMotionState(	CompWindow* w,
										const btTransform& startTrans,
										const btTransform& centerOfMassOffset
										)
	: m_graphicsWorldTrans(startTrans),
	m_lastGraphicsWorldTrans(startTrans),
	m_centerOfMassOffset(centerOfMassOffset),
	m_startWorldTrans(startTrans),
	m_userPointer(NULL),
	window(w)
{
}

BulletMotionState::~BulletMotionState()
{
}

void
BulletMotionState::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
	centerOfMassWorldTrans = m_centerOfMassOffset.inverse() * m_graphicsWorldTrans;
}

void
BulletMotionState::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
	m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset;
	btVector3 currentPos = m_graphicsWorldTrans.getOrigin();

	CompRect input = window->inputRect();
	float dx = (currentPos.getX()*Bullet::SCALE_FACTOR) - input.centerX();
	float dy = (currentPos.getY()*Bullet::SCALE_FACTOR) - input.centerY();
	window->move(std::ceil(dx), std::ceil(dy));
	window->syncPosition();
	m_lastGraphicsWorldTrans = m_graphicsWorldTrans;
}

bool
BulletPluginVTable::init()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    	return false;

    return true;
}

BulletScreen::BulletScreen (CompScreen *screen) :
    PluginClassHandler <BulletScreen, CompScreen> (screen),
    screen(screen),
    gScreen (GLScreen::get (screen)),
    cScreen (CompositeScreen::get (screen)),
	debugDrawEnabled(false),
	screenBordersEnabled(false)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);
	mGrabWindow = NULL;

	maxDivisor = std::sqrt(::screen->height() * ::screen->height() + ::screen->width() * ::screen->width()); //the length of the screen diagonalie

    optionSetToggleDebugDrawKeyInitiate (boost::bind (&BulletScreen::toggleDebugDrawAction, this,
							_1, _2, _3));
    optionSetInitAllKeyInitiate (boost::bind (&BulletScreen::initPhysicsForAllWindowsAction, this,
							_1, _2, _3));
    optionSetToggleScreenBordersInitiate (boost::bind (&BulletScreen::toggleScreenBordersAction, this,
							_1, _2, _3));
    optionSetToggleSpringKeyInitiate (boost::bind (&BulletScreen::toggleSpringAction, this,
							_1, _2, _3));
    optionSetSelectMoveWindowInitiate (boost::bind (&BulletScreen::moveInitiateAction, this,
    						_1, _2, _3));
    optionSetSelectMoveWindowTerminate (boost::bind (&BulletScreen::moveTerminateAction, this,
    						_1, _2, _3));
    optionSetAttractInitiateKeyInitiate (boost::bind (&BulletScreen::attractInitiateAction, this,
							_1, _2, _3));
    optionSetAttractInitiateKeyTerminate (boost::bind (&BulletScreen::attractTerminateAction, this,
							_1, _2, _3));
    optionSetRepulseInitiateKeyInitiate (boost::bind (&BulletScreen::repulseInitiateAction, this,
							_1, _2, _3));
    optionSetRepulseInitiateKeyTerminate (boost::bind (&BulletScreen::repulseTerminateAction, this,
							_1, _2, _3));
    optionSetGravityNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));
    optionSetBorderRestitutionNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));
    optionSetWindowRestitutionNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));
    optionSetBorderFrictionNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));
    optionSetWindowFrictionNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));
    optionSetWorldDampingNotify(boost::bind(&BulletScreen::changeOption, this, _1, _2));

    toggleMovePluginActions(false); //disable move functionality from "move" plugin while "bullet" is active

    initPhysics();
    mMoveState = NONE;
    attract = false;
    springEnabled = false;
    mMoveCursor = XCreateFontCursor (screen->dpy (), XC_fleur);

    pollHandle.setCallback (boost::bind (
				&BulletScreen::updateMouse, this, _1));
    pollHandle.start ();
}

BulletScreen::~BulletScreen()
{
	pollHandle.stop ();
    if (mMoveCursor)
    	XFreeCursor (screen->dpy (), mMoveCursor);
    toggleMovePluginActions(true);
}

void
BulletScreen::toggleMovePluginActions(bool enable)
{
	CompPlugin* plugin = CompPlugin::find ("move");
	if (plugin)
	{
		foreach (CompOption& option, plugin->vTable->getOptions ())
		{
			if (option.type () == CompOption::TypeAction ||
				option.type () == CompOption::TypeKey ||
				option.type () == CompOption::TypeButton)
			{
				if (option.name () == "initiate_button" || option.name() == "initiate_key")
				{
					if (enable)
						screen->addAction(&option.value().action());
					else
						screen->removeAction(&option.value().action());
				}
			}
		}
	}
}

void
BulletScreen::handleEvent(XEvent* event)
{
	switch(event->type)
	{
	case ClientMessage:
	    if (event->xclient.message_type == Atoms::wmMoveResize)
	    {
	    	//send by decorator, from move plugin
	    	CompWindow *w;
			unsigned   long type = (unsigned long) event->xclient.data.l[2];

			if (type == WmMoveResizeMove)
			{
				CompOption::Vector o;

				moveInitiateAction (&optionGetSelectMoveWindow (),
						  CompAction::StateInitButton, o);

			}
	    }
		break;
	}
	screen->handleEvent(event);
}

bool
BulletScreen::glPaintOutput(const GLScreenPaintAttrib& attrib,
							const GLMatrix		&transform,
							const CompRegion		&region,
							CompOutput 		*output,
							unsigned int		mask)
{
	bool status = gScreen->glPaintOutput(attrib, transform, region, output, mask);
	if (debugDrawEnabled && dynamicsWorld)
	{
		GLMatrix sTransform(transform);
	    gScreen->glApplyTransform (attrib, output, &sTransform);
	    sTransform.toScreenSpace (output, -attrib.zTranslate);
		glPushMatrix();
		glLoadMatrixf(sTransform.getMatrix());
		glScalef(Bullet::SCALE_FACTOR, Bullet::SCALE_FACTOR, 1); //scale up again since the rigidbodies are only 1/200 the size of the window
		dynamicsWorld->debugDrawWorld(); //TODO necessary? wiki says call to world->setDebugDrawer would suffice
//		const btTransform t(btTransform(btQuaternion(0,0,0,1), btVector3((screen->width()/2.0f)/Bullet::SCALE_FACTOR, (screen->height()/2.0f)/Bullet::SCALE_FACTOR,0)));
//		const btTransform t1(btTransform(btQuaternion(0,0,0,1), btVector3((screen->width()/2.0f)/Bullet::SCALE_FACTOR,0,0)));
//		const btTransform t2(btTransform(btQuaternion(0,0,0,1), btVector3((screen->width()/2.0f)/Bullet::SCALE_FACTOR,screen->height()/Bullet::SCALE_FACTOR,0)));
//		const btTransform t3(btTransform(btQuaternion(0,0,0,1), btVector3(0,(screen->height()/2.0f)/Bullet::SCALE_FACTOR,0)));
//		const btTransform t4(btTransform(btQuaternion(0,0,0,1), btVector3(screen->width()/Bullet::SCALE_FACTOR,(screen->height()/2.0f)/Bullet::SCALE_FACTOR,0)));
//
//		dynamicsWorld->debugDrawObject(t, groundShape.get(), btVector3(0,1,0));
//		dynamicsWorld->debugDrawObject(t1, upperBorderShape.get(), btVector3(1,0,0));
//		dynamicsWorld->debugDrawObject(t2, lowerBorderShape.get(), btVector3(0,0,1));
//		dynamicsWorld->debugDrawObject(t3, leftBorderShape.get(), btVector3(0,1,1));
//		dynamicsWorld->debugDrawObject(t4, rightBorderShape.get(), btVector3(1,1,0));
		glColor4usv(defaultColor);
		glPopMatrix();
	}
	if (dynamicsWorld && springEnabled)
	{
		GLMatrix sTransform(transform);
		gScreen->glApplyTransform (attrib, output, &sTransform);
		sTransform.toScreenSpace(output, -attrib.zTranslate);
		btTransform mouseHookTransform;
		mMouseHookRigidBody->getMotionState()->getWorldTransform(mouseHookTransform);
		btTransform mouseTransform;
		mMouseRigidBody->getMotionState()->getWorldTransform(mouseTransform);
		btVector3 mouseHookPos = mouseHookTransform.getOrigin();
		btVector3 mousePos = mouseTransform.getOrigin();
		btVector3 dist = mouseHookPos - mousePos;
		dist = dist.normalize();
		btVector3 z(0,0,1);
		btVector3 ortoOnDist = dist.cross(z);
		btVector3 triEndpointLeft = mouseHookPos + ortoOnDist * 0.2;
		btVector3 triEndpointRight = mouseHookPos + ortoOnDist * -0.2;
		glPushMatrix();
		glLoadMatrixf(sTransform.getMatrix());
		glScalef(Bullet::SCALE_FACTOR, Bullet::SCALE_FACTOR, 1);
		glBegin(GL_TRIANGLES);
		glColor3f(0.f, 1.f, 0.f);
		glVertex2f(mousePos.x(), mousePos.y());
		glColor3f(1.f, 0.f, 0.f);
		glVertex2f(triEndpointRight.x(), triEndpointRight.y());
		glVertex2f(triEndpointLeft.x(), triEndpointLeft.y());
		glEnd();
		glPopMatrix();

	}

	int outputNum = screen->outputDeviceForPoint(::pointerX, ::pointerY);
	CompOutput* o = &screen->outputDevs()[outputNum];
	if (mMoveState != NONE && (output == o))
	{
		invertTransformed(transform, output);
	}

	return status;
}

/**
 * @see Expo
 */
void
BulletScreen::invertTransformed(const GLMatrix& transform, CompOutput* output)
{
	GLMatrix sTransform(transform);
    GLdouble pFar[3], pNear[3], ray[3], alpha;
    GLdouble mvm[16], pm[16];
    GLint    viewport[4];
    int vertex[2] = { ::pointerX, ::pointerY };

    sTransform.toScreenSpace(output, -DEFAULT_Z_CAMERA);

    glGetIntegerv (GL_VIEWPORT, viewport);

    for (int i=0; i < 16; i++)
    {
    	mvm[i] = sTransform[i];
		pm[i]  = gScreen->projectionMatrix ()[i];
    }
    float yReal = viewport[1] + viewport[3] - vertex[1];
    if (output->y1() != 0)
    	yReal += viewport[3];
    gluUnProject (vertex[0], yReal, 1.0, mvm, pm,
		  viewport, &pFar[0], &pFar[1], &pFar[2]);
    gluUnProject (vertex[0], yReal, 0.0, mvm, pm,
		  viewport, &pNear[0], &pNear[1], &pNear[2]);

    for (int i = 0; i < 3; i++)
    	ray[i] = pFar[i] - pNear[i];

    //calculate world coords for constant z = 0
    alpha = (0 - pNear[2]) / ray[2];
    vertex[0] = ceil (pNear[0] + (alpha * ray[0]));
    vertex[1] = ceil (pNear[1] + (alpha * ray[1]));

    mRay.setX(vertex[0]);
    mRay.setY(vertex[1]);
    mRay.setZ(0.0);

    camForOutput[output] = btVector3(output->centerX(), output->centerY(), DEFAULT_Z_CAMERA);
    GLVector cameraInObjectCoords(camForOutput[output].x(), camForOutput[output].y(), camForOutput[output].z(), 1);
    currentOutput = output;
    GLVector cameraInEyeCoords = sTransform * cameraInObjectCoords;

#if DEBUG_CONSOLE_OUTPUT
    std::cout << "viewport height: "  << viewport[3] << ", viewport y: " << viewport[1] << std::endl;
    std::cout << "Current: " << ::pointerX << ", " << ::pointerY << std::endl;
    std::cout << "Calculated: " << vertex[0] << ", " << vertex[1] << std::endl;
    //Viewport == Current Output
    std::cout << "Camera in EyePos (should be 0,0,0): " << cameraInEyeCoords[0] << ", " << cameraInEyeCoords[1] << ", " << cameraInEyeCoords[2] << std::endl;
#endif

}

void
BulletScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
										const GLMatrix		&transform,
										const CompRegion		&region,
										CompOutput 		*output,
										unsigned int		mask)
{
    gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

void
BulletScreen::preparePaint(int msSinceLastRepaint)
{
	int maxSimSubSteps = 20;
	if (dynamicsWorld)
	{
#ifdef VERBOSE_TIMESTEPPING_CONSOLEOUTPUT
		int numSimSteps = 0;
		numSimSteps = dynamicsWorld->stepSimulation(msSinceLastRepaint/1000.0f, maxSimSubSteps);
//		if (numSimSteps == 0)
//			std::cout << "Interpolated transforms" << std::endl;
//		else
//		{
			if (numSimSteps > maxSimSubSteps)
				std::cout << "Dropped " << (numSimSteps - maxSimSubSteps) << " simulation steps out of " << numSimSteps << std::endl;
//			else
//				std::cout << "Simulated " << numSimSteps << " steps" << std::endl;
//		}
#else
		dynamicsWorld->stepSimulation(msSinceLastRepaint/1000.0f, maxSimSubSteps);
#endif
	}
	cScreen->preparePaint(msSinceLastRepaint);
}

/**
 * Inits Physics (set BroadPhase algorithm, constraint solver, etc.)
 * and create an (infinite) ground plane that lies perpendicular to the view direction (neg. Z-axis)
 */
void
BulletScreen::initPhysics()
{
	broadphase.reset(new btDbvtBroadphase());
	collisionConfiguration.reset(new btDefaultCollisionConfiguration());
	dispatcher.reset(new btCollisionDispatcher(collisionConfiguration.get()));
	solver.reset(new btSequentialImpulseConstraintSolver());

#if USE_2D_SHAPES
	m_simplexSolver.reset(new btVoronoiSimplexSolver());
	m_pdSolver.reset(new btMinkowskiPenetrationDepthSolver());

	btConvex2dConvex2dAlgorithm::CreateFunc* convexAlgo2d = new btConvex2dConvex2dAlgorithm::CreateFunc(m_simplexSolver.get(),m_pdSolver.get());

	dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE,CONVEX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
	dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE,CONVEX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
	dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE,BOX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
	dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE,BOX_2D_SHAPE_PROXYTYPE,new btBox2dBox2dCollisionAlgorithm::CreateFunc());
#endif

	dynamicsWorld.reset(new btDiscreteDynamicsWorld(dispatcher.get(), broadphase.get(), solver.get(), collisionConfiguration.get()));
	dynamicsWorld->setGravity(btVector3(0,5,0)); //gravity towards plane, along view direction

	//Plane is defined by normal and constant (the distance of the plane from the origin)
	groundShape.reset(new btStaticPlaneShape(btVector3(0,0,1), 0));
	groundMotionState.reset(new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3((screen->width()/2.0f)/Bullet::SCALE_FACTOR,(screen->height()/2.0f)/Bullet::SCALE_FACTOR,0))));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyConstructionInfo(0, groundMotionState.get(), groundShape.get()); //can also set friction/restitution/etc. here
	groundRigidBody.reset(new btRigidBody(groundRigidBodyConstructionInfo));
	dynamicsWorld->addRigidBody(groundRigidBody.get(), Bullet::GROUND, Bullet::NOTHING); //Ground does not receive collision

	setUpScreenBorders(screen->width()/Bullet::SCALE_FACTOR, screen->height()/Bullet::SCALE_FACTOR);
	std::cout << "init Physics" << std::endl;
}
void
BulletScreen::donePaint ()
{
//	cScreen->damagePending();
	cScreen->damageScreen(); //TODO: Always repaint?

    cScreen->donePaint ();

    switch (mMoveState)
    {
    CompWindow* w;
    case INIT:
		w = checkRayIntersection();
		if (w)
		{
			if (!mGrab)
				mGrab = screen->pushGrab (mMoveCursor, "bullet");

			if (mGrab)
			{
				mGrabWindow = w;

				mGrabWindow->grabNotify (::pointerX, ::pointerY, 0, //TODO:
				       CompWindowGrabMoveMask |
				       CompWindowGrabButtonMask);
				mGrabWindow->raise ();
				mGrabWindow->moveInputFocusTo ();
				mMoveState = MOVING;
				//maybe save some state, e.g. window position etc.
			}
		}
		break;
    case MOVING:
    	if (mGrab && mPickConstraint)
    	{
			//move the constraint pivot
			//keep it at the same picking distance

			btVector3 rayTarget = mRay / Bullet::SCALE_FACTOR; //z == 0

			mPickConstraint->setPivotB(rayTarget);
    	}
    	break;
    default:
    	break;
    }
}

void
BulletScreen::setUpScreenBorders(float outputWidthScaled, float outputHeightScaled)
{
	if (outputWidthScaled < 0 || outputHeightScaled < 0)
		return;

	float outputHalfWidthScaled = outputWidthScaled / 2.0f;
	float outputHalfHeightScaled = outputHeightScaled / 2.0f;

	upperBorderShape.reset(new btStaticPlaneShape(btVector3(0,1,0),0));
	upperBorderMotionState.reset(new btDefaultMotionState(btTransform(
			btQuaternion(0,0,0,1), btVector3(outputHalfWidthScaled,0,0))));
	upperBorderRigidBody.reset(new btRigidBody(0, upperBorderMotionState.get(), upperBorderShape.get()), Bullet::RigidBodyDeleter());

	lowerBorderShape.reset(new btStaticPlaneShape(btVector3(0,-1,0),0));
	lowerBorderMotionState.reset(new btDefaultMotionState(btTransform(
			btQuaternion(0,0,0,1), btVector3(outputHalfWidthScaled, outputHeightScaled, 0))));
	lowerBorderRigidBody.reset(new btRigidBody(0, lowerBorderMotionState.get(), lowerBorderShape.get()), Bullet::RigidBodyDeleter());

	leftBorderShape.reset(new btStaticPlaneShape(btVector3(1,0,0),0));
	leftBorderMotionState.reset(new btDefaultMotionState(btTransform(
			btQuaternion(0,0,0,1), btVector3(0, outputHalfHeightScaled, 0))));
	leftBorderRigidBody.reset(new btRigidBody(0, leftBorderMotionState.get(), leftBorderShape.get()), Bullet::RigidBodyDeleter());

	rightBorderShape.reset(new btStaticPlaneShape(btVector3(-1,0,0),0));
	rightBorderMotionState.reset(new btDefaultMotionState(btTransform(
			btQuaternion(0,0,0,1), btVector3(outputWidthScaled, outputHalfHeightScaled,0))));
	rightBorderRigidBody.reset(new btRigidBody(0, rightBorderMotionState.get(), rightBorderShape.get()), Bullet::RigidBodyDeleter());

	enableScreenBorders();
}

void
BulletScreen::enableScreenBorders()
{
	if (dynamicsWorld &&
			upperBorderRigidBody && lowerBorderRigidBody && leftBorderRigidBody && rightBorderRigidBody)
	{
		short bordersCollideWith = Bullet::WINDOW | Bullet::MOUSE;
		dynamicsWorld->addRigidBody(upperBorderRigidBody.get(), Bullet::GROUND, bordersCollideWith);
		dynamicsWorld->addRigidBody(lowerBorderRigidBody.get(), Bullet::GROUND, bordersCollideWith);
		dynamicsWorld->addRigidBody(leftBorderRigidBody.get(),  Bullet::GROUND, bordersCollideWith);
		dynamicsWorld->addRigidBody(rightBorderRigidBody.get(), Bullet::GROUND, bordersCollideWith);
	}
	screenBordersEnabled = true;
}

void
BulletScreen::disableScreenBorders()
{
	if (dynamicsWorld &&
			upperBorderRigidBody && lowerBorderRigidBody && leftBorderRigidBody && rightBorderRigidBody)
	{
		dynamicsWorld->removeRigidBody(upperBorderRigidBody.get());
		dynamicsWorld->removeRigidBody(lowerBorderRigidBody.get());
		dynamicsWorld->removeRigidBody(leftBorderRigidBody.get());
		dynamicsWorld->removeRigidBody(rightBorderRigidBody.get());
	}
	screenBordersEnabled = false;
}

void
BulletScreen::toggleScreenBorders()
{
	if (screenBordersEnabled)
		disableScreenBorders();
	else
		enableScreenBorders();
	cScreen->damageScreen();
}

void
BulletScreen::addToWorld(const btRigidBody& body)
{
	if (dynamicsWorld)
		dynamicsWorld->addRigidBody(const_cast<btRigidBody*>(&body));
}

void
BulletScreen::addToWorld(const btRigidBody& body, short group, short mask)
{
	if (dynamicsWorld)
		dynamicsWorld->addRigidBody(const_cast<btRigidBody*>(&body), group, mask);
}

void
BulletScreen::removeFromWorld(const btRigidBody& body)
{
	if (dynamicsWorld)
		dynamicsWorld->removeRigidBody(const_cast<btRigidBody*>(&body));
}

void
BulletScreen::removeFromWorld(const btTypedConstraint& constraint)
{
	if (dynamicsWorld)
		dynamicsWorld->removeConstraint(&(const_cast<btTypedConstraint&>(constraint)));
}

void
BulletScreen::enableDebugDraw()
{
	if (!debugDrawer)
		debugDrawer.reset(new BulletDebugDrawer());

	debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe
//						| btIDebugDraw::DBG_DrawAabb
//                      | btIDebugDraw::DBG_DrawFeaturesText
						| btIDebugDraw::DBG_DrawConstraints
//                      | btIDebugDraw::DBG_DrawText
                        );
	if (dynamicsWorld)
		dynamicsWorld->setDebugDrawer(debugDrawer.get());
	debugDrawEnabled = true;
}

void
BulletScreen::disableDebugDraw()
{
	if (dynamicsWorld)
		dynamicsWorld->setDebugDrawer(NULL);
	debugDrawEnabled = false;
}

void
BulletScreen::toggleDebugDraw()
{
	if (debugDrawEnabled)
		disableDebugDraw();
	else
		enableDebugDraw();
	cScreen->damageScreen();
}

//Actions

void
BulletScreen::checkAndHandleWindowVisibility()
{
	foreach(CompWindow* w, ::screen->windows())
	{
		if (w->invisible()) {
			BulletWindow* bw = BulletWindow::get(w);
			bw->resetPhysics();
		}
	}
}

bool
BulletScreen::toggleSpringAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (screen->otherGrabExist ("bullet", NULL))
		return false;

	if (!springEnabled)
	{
		btScalar springRange(1920.f / Bullet::SCALE_FACTOR); //oscillation Range: max. one screenlength

		btTransform tr1(btTransform::getIdentity());
		tr1.setOrigin(btVector3(mRay / Bullet::SCALE_FACTOR));

		btScalar mass(1.f);
		btVector3 localInertia(0,0,0);
		mMouseHookCollisionShape.reset(new btBoxShape(btVector3(0.2, 0.2, 0.2)));
		mMouseHookCollisionShape->calculateLocalInertia(mass, localInertia);
		mMouseHookMotionState.reset(new btDefaultMotionState(tr1));
		btRigidBody::btRigidBodyConstructionInfo rbcinfo(mass, mMouseHookMotionState.get(), mMouseHookCollisionShape.get(), localInertia);
		mMouseHookRigidBody.reset(new btRigidBody(rbcinfo), Bullet::RigidBodyDeleter());
		mMouseHookRigidBody->setActivationState(DISABLE_DEACTIVATION);
		dynamicsWorld->addRigidBody(mMouseHookRigidBody.get());

		btTransform tr2(btTransform::getIdentity());
		tr2.setOrigin(btVector3 (mRay / Bullet::SCALE_FACTOR));
		mMouseCollisionShape.reset(new btBoxShape(btVector3(0.2, 0.2, 0.2)));
		mMouseCollisionShape->calculateLocalInertia(mass, localInertia);
		mMouseMotionState.reset(new btDefaultMotionState(tr2));
		btRigidBody::btRigidBodyConstructionInfo rbci2(mass, mMouseMotionState.get(), mMouseCollisionShape.get(), localInertia);
		mMouseRigidBody.reset(new btRigidBody(rbci2), Bullet::RigidBodyDeleter());
		mMouseRigidBody->setActivationState(DISABLE_DEACTIVATION);
		mMouseRigidBody->setLinearFactor(btVector3(1,1,0)); //limit motion to X-Y-plane
		mMouseRigidBody->setAngularFactor(btVector3(0,0,0)); //no rotation allowed
		dynamicsWorld->addRigidBody(mMouseRigidBody.get(), Bullet::MOUSE, Bullet::GROUND);

		btTransform frameInA(btTransform::getIdentity());
		frameInA.setOrigin(btVector3(btScalar(0.), btScalar(0.), btScalar(0.))); //in local coordinate system of A

		btTransform frameInB(btTransform::getIdentity());
		frameInB.setOrigin(btVector3(btScalar(0.), btScalar(0.), btScalar(0.))); //in local coordinate system of B

		mSpringConstraint.reset(new btGeneric6DofSpringConstraint(*mMouseRigidBody.get(), *mMouseHookRigidBody.get(), frameInA, frameInB, true),
				Bullet::ConstraintDeleter());
		mSpringConstraint->setLinearLowerLimit(btVector3(-springRange, -springRange, 0.f));
		mSpringConstraint->setLinearUpperLimit(btVector3(springRange, springRange, 0.f));
		mSpringConstraint->setAngularLowerLimit(btVector3(0.f, 0.f, 0.f));
		mSpringConstraint->setAngularUpperLimit(btVector3(0.f, 0.f, 0.f));
		dynamicsWorld->addConstraint(mSpringConstraint.get(), true);
		mSpringConstraint->setDbgDrawSize(btScalar(3.f));

		mSpringConstraint->enableSpring(0, true); //x movement
		mSpringConstraint->enableSpring(1, true); //y movement

		mSpringConstraint->setStiffness(0, 50.f);
		mSpringConstraint->setStiffness(1, 50.f);
		mSpringConstraint->setDamping(0, 0.08f); //small number -> high dampening?
		mSpringConstraint->setDamping(1, 0.08f);
		mSpringConstraint->setEquilibriumPoint(); //current Position is Equilibrium

		btVector3 localPivot = btVector3(0, 0, 0);
		mMouseMoveConstraint.reset(new btPoint2PointConstraint(*mMouseRigidBody.get(),localPivot), Bullet::ConstraintDeleter());
		mMouseMoveConstraint->m_setting.m_impulseClamp = 111130.f;
	} else
	{
		mSpringConstraint.reset();
		mMouseMoveConstraint.reset();
		mMouseCollisionShape.reset();
		mMouseMotionState.reset();
		mMouseRigidBody.reset();
		mMouseHookCollisionShape.reset();
		mMouseHookMotionState.reset();
		mMouseHookRigidBody.reset();
	}

	springEnabled = !springEnabled;
	return false;
}

bool
BulletScreen::attractInitiateAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (screen->otherGrabExist ("bullet", NULL))
		return false;

	if (!mGrab)
		mGrab = screen->pushGrab (0, "bullet");

	if (mGrab)
	{
		attract = true;
		dynamicsWorld->setInternalTickCallback(Bullet::tickCallback, NULL, true);
	}

    if (state & CompAction::StateInitButton)
    	action->setState (action->state () | CompAction::StateTermButton);
    if (state & CompAction::StateInitKey)
    	action->setState (action->state () | CompAction::StateTermKey);

	return true;
}

bool
BulletScreen::attractTerminateAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (mGrab)
	{
		screen->removeGrab (mGrab, NULL);
		mGrab = NULL;

		dynamicsWorld->setInternalTickCallback(NULL, NULL, true);
	}

    action->setState (action->state () &
						~(CompAction::StateTermKey | CompAction::StateTermButton));
	return true;
}

bool
BulletScreen::repulseInitiateAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (screen->otherGrabExist ("bullet", NULL))
		return false;

	if (!mGrab)
		mGrab = screen->pushGrab (0, "bullet");

	if (mGrab)
	{
		attract = false;
		dynamicsWorld->setInternalTickCallback(Bullet::tickCallback, NULL, true);
	}

    if (state & CompAction::StateInitButton)
    	action->setState (action->state () | CompAction::StateTermButton);
    if (state & CompAction::StateInitKey)
    	action->setState (action->state () | CompAction::StateTermKey);

	return true;
}

bool
BulletScreen::repulseTerminateAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (mGrab)
	{
		screen->removeGrab (mGrab, NULL);
		mGrab = NULL;

		dynamicsWorld->setInternalTickCallback(NULL, NULL, true);
	}

    action->setState (action->state () &
						~(CompAction::StateTermKey | CompAction::StateTermButton));
	return true;
}

//move window by forces
//see Move plugin, move.cpp, Expo plugin
bool
BulletScreen::moveInitiateAction (CompAction      *action,
	      CompAction::State state,
	      CompOption::Vector &options)
{
	if (screen->otherGrabExist ("bullet", NULL))
	{
		std::cout << "Other grab outside bullet plugin exists. Abort action" << std::endl;
		return false;
	}

	if (mGrabWindow != NULL)
	{
		std::cout << "mGrabWindow is not null. Abort action" << std::endl;
		return false;
	}

	if (mMoveState != NONE)
	{
		std::cout << "Window movement already initiated. Abort action" << std::endl;
		return false;
	}

    if (state & CompAction::StateInitButton)
    	action->setState (action->state () | CompAction::StateTermButton);
    if (state & CompAction::StateInitKey)
    	action->setState (action->state () | CompAction::StateTermKey);

	mMoveState = INIT;

	if (mMouseMoveConstraint)
		dynamicsWorld->addConstraint(mMouseMoveConstraint.get());

//	int x = CompOption::getIntOptionNamed (options, "x", ::pointerX);
//	int y = CompOption::getIntOptionNamed (options, "y", ::pointerY);
//	mMousePos.setX(x);
//	mMousePos.setY(y);

	cScreen->damageScreen();  //need call to invertTransform

    return true;
}

CompWindow*
BulletScreen::checkRayIntersection()
{
	if (!dynamicsWorld)
		return NULL;

	btVector3 rayOrigin = camForOutput[currentOutput];
	btVector3 rayTarget = mRay;

	rayOrigin.setX(rayOrigin.x() / Bullet::SCALE_FACTOR);
	rayOrigin.setY(rayOrigin.y() / Bullet::SCALE_FACTOR);

	rayTarget 	/= Bullet::SCALE_FACTOR;

	//handle mouseRigidBody movement
	if (mMouseRigidBody && mMouseMoveConstraint)
	{
		mMouseMoveConstraint->setPivotB(rayTarget);
	}

	ClosestRayResultCallbackCollideWithWindows rayCallback (rayOrigin, rayTarget);
	dynamicsWorld->rayTest (rayOrigin, rayTarget, rayCallback);

#if DEBUG_CONSOLE_OUTPUT
	if (currentOutput->x1() == 0)
		std::cout << "Output 1: " << std::endl;
	else
		std::cout << "Output 2: " << std::endl;
	std::cout << "origin: " 	<< camForOutput[currentOutput].x()<< ", " << camForOutput[currentOutput].y()<< ", "	<< camForOutput[currentOutput].z()<< std::endl;
	std::cout << "target: " 	<< mRay.x()<< ", " << mRay.y()<< ", " << mRay.z()<< std::endl;
#endif
	if (!rayCallback.hasHit())
	{
#if DEBUG_CONSOLE_OUTPUT
		std::cout << "No Hit" << std::endl;
#endif
		return NULL;
	}
#if DEBUG_CONSOLE_OUTPUT
	std::cout << "Ray intersects with collision object" << std::endl;
#endif
	btRigidBody* body = btRigidBody::upcast (rayCallback.m_collisionObject);

	if (!body)
		return NULL;

	if (body->isStaticObject() || body->isKinematicObject())
		return NULL;

	BulletWindow* bw = (BulletWindow*)body->getUserPointer();

	if (!Bullet::isMoveableWindow(bw->window))
	{
		std::cout << "Window cannot be moved" << std::endl;
		return NULL;
	}

	body->forceActivationState(DISABLE_DEACTIVATION);
	pickedBody = body;

	btVector3 pickPos = rayCallback.m_hitPointWorld;

	btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
	btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body,localPivot);
	p2p->m_setting.m_impulseClamp = 111130.f;

	dynamicsWorld->addConstraint(p2p);
	mPickConstraint = p2p;
	//very weak constraint for picking
	p2p->m_setting.m_tau = 0.1f;

	return bw->window;
}

bool
BulletScreen::moveTerminateAction (CompAction      *action,
	       CompAction::State state,
	       CompOption::Vector &options)
{
	if (mMouseMoveConstraint)
		dynamicsWorld->removeConstraint(mMouseMoveConstraint.get());

    if (mGrabWindow && mMoveState == MOVING)
    {
//		if (state & CompAction::StateCancel)
//			ms->w->move (ms->savedX - ms->w->geometry ().x (),
//				 ms->savedY - ms->w->geometry ().y (), false); //maybe later
//
    	if (mPickConstraint && dynamicsWorld)
    	{
    		dynamicsWorld->removeConstraint(mPickConstraint);
			delete mPickConstraint;
			mPickConstraint = NULL;
			if (pickedBody)
				pickedBody->activate(true);
			pickedBody = NULL;
    	}
//
//    	/* update window attributes as window constraints may have
//		   changed - needed e.g. if a maximized window was moved
//		   to another output device */
		mGrabWindow->updateAttributes (CompStackingUpdateModeNone);
		mGrabWindow->ungrabNotify ();
    }

	if (mGrab)
	{
		screen->removeGrab (mGrab, NULL);
		mGrab = NULL;
	}

	mGrabWindow = NULL;

	mMoveState = NONE;

    action->setState (action->state () &
						~(CompAction::StateTermKey | CompAction::StateTermButton));
	cScreen->damageScreen();

    return true;
}

void
BulletScreen::changeOption (CompOption* option,
			       BulletOptions::Options num)
{
//	if (!dynamicsWorld)
//		return;

	switch(num)
	{
	case BulletOptions::Gravity:
		dynamicsWorld->setGravity(btVector3(0,optionGetGravity(), 0));
		break;
	case BulletOptions::BorderRestitution:
		upperBorderRigidBody->setRestitution(optionGetBorderRestitution());
		lowerBorderRigidBody->setRestitution(optionGetBorderRestitution());
		leftBorderRigidBody->setRestitution(optionGetBorderRestitution());
		rightBorderRigidBody->setRestitution(optionGetBorderRestitution());
		groundRigidBody->setRestitution(optionGetBorderRestitution());
		break;
	case BulletOptions::WindowRestitution:
	{
		int numObjects = dynamicsWorld->getNumCollisionObjects();

		for (int i=0; i<numObjects; i++)
		{
			btCollisionObject* colObj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(colObj);
			if (body)
			{
				if (body->getUserPointer() != NULL)
					body->setRestitution(optionGetWindowRestitution());
			}
		}
		break;
	}
	case BulletOptions::BorderFriction:
		upperBorderRigidBody->setFriction(optionGetBorderFriction());
		lowerBorderRigidBody->setFriction(optionGetBorderFriction());
		leftBorderRigidBody->setFriction(optionGetBorderFriction());
		rightBorderRigidBody->setFriction(optionGetBorderFriction());
		groundRigidBody->setFriction(optionGetBorderFriction());
		break;
	case BulletOptions::WindowFriction:
	{
		int numObjects = dynamicsWorld->getNumCollisionObjects();

		for (int i=0; i<numObjects; i++)
		{
			btCollisionObject* colObj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(colObj);
			if (body)
			{
				if (body->getUserPointer() != NULL)
					body->setFriction(optionGetWindowFriction());
			}
		}
		break;
	}
	case BulletOptions::WorldDamping:
	{
		int numObjects = dynamicsWorld->getNumCollisionObjects();

		for (int i=0; i<numObjects; i++)
		{
			btCollisionObject* colObj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(colObj);
			if (body)
			{
				if (body->getUserPointer() != NULL)
					body->setDamping(optionGetWorldDamping(), 0.);
			}
		}
		break;
	}
	case BulletOptions::ForceMultiplier:
	{
		int numObjects = dynamicsWorld->getNumCollisionObjects();

		for (int i=0; i<numObjects; i++)
		{
			btCollisionObject* colObj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(colObj);
			if (body && body->getUserPointer() != NULL)
			{
				BulletWindow* bw = static_cast<BulletWindow*>(body->getUserPointer());
				bw->setForceMultiplier(optionGetForceMultiplier());
			}
		}
		break;
	}
	default:
		break;
	}
}

bool
BulletScreen::toggleDebugDrawAction (CompAction *action,
			CompAction::State state,
			CompOption::Vector& options)
{
	toggleDebugDraw();
	return false;
}

bool
BulletScreen::toggleScreenBordersAction (CompAction *action,
			CompAction::State state,
			CompOption::Vector& options)
{
	toggleScreenBorders();
	return false;
}

bool
BulletScreen::initPhysicsForAllWindowsAction(CompAction* action,
			CompAction::State state,
			CompOption::Vector& options)
{
	foreach(CompWindow* w, ::screen->windows())
	{
		BulletWindow* bw = BulletWindow::get(w);
		bw->initPhysics();
	}
	return false;
}

bool
BulletScreen::isAttracted()
{
	return attract;
}

btVector3
BulletScreen::getRayTo(int x, int y)
{
#if 0
	btVector3 cameraPosition(0,0, DEFAULT_Z_CAMERA);
	btVector3 cameraTargetPosition(0,0,0);
	btVector3 cameraUp(0, 1, 0);

	float far    = 100.f;
	float fov    = 60;
	float tanfov = tanf(0.5f*fov);

	btVector3 rayFrom = cameraPosition;
	btVector3 rayForward = (cameraTargetPosition-cameraPosition);
	rayForward.normalize();
	rayForward *= far;

	btVector3 rightOffset;
	btVector3 up = cameraUp;

	btVector3 right;
	right = rayForward.cross(up);
	right.normalize();
	up = right.cross(rayForward);
	up.normalize();

	right   *= 2.f * far * tanfov;
	up 		*= 2.f * far * tanfov;

	btScalar aspect;
	if (screen->width () > screen->height ())
	{
		aspect = screen->width () / screen->height ();
		right *= aspect;
	} else
	{
		aspect = screen->width () / screen->height ();
		up *= aspect;
	}

	btVector3 rayToCenter = rayFrom + rayForward;
	btVector3 dHor = right / screen->width ();
	btVector3 dVert = up / screen->height ();


	btVector3 rayTo = rayToCenter - 0.5f * right + 0.5f * up;
	rayTo += btScalar(x) * dHor;
	rayTo -= btScalar(y) * dVert;
#endif
#if 0
	//Do a manual unproject, see http://bookofhook.com/mousepick.pdf

	float w = screen->width ();
	float h = screen->height ();
	GLVector clipCoord(2*x / (float)w - 1, 2*y/(float)h, 0, 1); //use viewport z coord = 0

	const float* projMat = gScreen->projectionMatrix();
	float invProjMat[16];
	memset(invProjMat, 0, 16 * sizeof(float));

	invProjMat[0] = 1.f / projMat[0];
	invProjMat[5] = 1.f / projMat[5];
	invProjMat[12] = 1.f / projMat[14];
	invProjMat[14] = 1.f / projMat[11];
	invProjMat[15] = - projMat[10] / (projMat[11] * projMat[14]);

	GLMatrix invProjMatrix(invProjMat);
	GLVector modelviewCoord = invProjMatrix * clipCoord;
	modelviewCoord /= modelviewCoord.w;

	float invModelViewMat[16];
	memset(invModelViewMat, 0, 16 * sizeof(float));

	float modelViewMat[16];
	glGetFloatv( GL_MODELVIEW_MATRIX, modelViewMat);

	invModelViewMat[1] = modelViewMat[4]; //inv. modelview matrix is created by transposing rotation part and negating translation part
	invModelViewMat[2] = modelViewMat[5];
	invModelViewMat[4] = modelViewMat[1];
	invModelViewMat[5] = modelViewMat[2];
	invModelViewMat[6] = modelViewMat[9];
	invModelViewMat[9] = modelViewMat[6];
	invModelViewMat[12] = - modelViewMat[12];
	invModelViewMat[13] = - modelViewMat[13];
	invModelViewMat[14] = - modelViewMat[13];
	invModelViewMat[15] = 1;

	GLMatrix invModelViewMatrix(invModelViewMat);
	modelviewCoord[3] = 0;
	GLVector worldCoord = invModelViewMatrix * modelviewCoord;

	btVector3 rayTo(worldCoord[0], worldCoord[1], worldCoord[2]);
#endif

	//generate pick ray in eye coordinates
	float w = screen->width (); //output->x();
	float h = screen->height (); //output->y();
	const float PI = 3.14159265;
	float modelview[16];
	glGetFloatv(GL_MODELVIEW, modelview);
	for (int i=0; i<16; i++) {
		std::cout << modelview[i] << ", ";
	}
	std::cout << std::endl;

	float clipX = (2 * x / w) - 1; //x is between -1 and 1
	float clipY = (2 * (h-y) / h) - 1; //y is between -1 and 1
	float nearPlaneWidth = 0.1 * tan (60.0 * PI / 360); //see screen.cpp opengl plugin
	float nearPlaneHeight = nearPlaneWidth * 1.0f; //should be aspect ratio
	std::cout << nearPlaneWidth << std::endl;
	clipX *= nearPlaneWidth;
	clipY *= nearPlaneHeight;
	std::cout << "clipX: " << clipX << ", clipY: " << clipY << std::endl;
	btVector3 mouseCoordsInEyeCoords(clipX, clipY, 0.1f); //z = near //see opengl plugin
	mouseCoordsInEyeCoords.setW(0.0); //denotes a vector, not a point

	btVector3 originInEyeCoords(0,0,0);

	GLMatrix tempMat;
	tempMat.translate(0, 0, -DEFAULT_Z_CAMERA);

	btTransform inverseModelViewMatrix;
	inverseModelViewMatrix.setFromOpenGLMatrix(tempMat.getMatrix());
	inverseModelViewMatrix = inverseModelViewMatrix.inverse();

	btVector3 mouseInWorldCoords = inverseModelViewMatrix * mouseCoordsInEyeCoords;
	btVector3 originInWorldCoords = inverseModelViewMatrix * originInEyeCoords;

	btVector3 rayTo = mouseInWorldCoords;
	return rayTo;
}

CompPoint
BulletScreen::getCurrentMousePos()
{
	return mMousePollerPos;
}

void
BulletScreen::updateMouse (const CompPoint &p)
{
	mMousePollerPos = p;
//	if (!dynamicsWorld)
//		return;
//	//Recalculate Forces (Attraction/Repulsion)
//	for (int i=0; i<dynamicsWorld->getNumCollisionObjects(); i++)
//	{
//		btRigidBody* body = btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[i]);
//		if (body && body->getUserPointer())
//		{
//			BulletWindow* win = static_cast<BulletWindow*>(body->getUserPointer());
//			win->applyForce(p, true);
//		}
//	}
//	mRay = getRayTo(p.x (), p.y ());
//	CompOption::Vector options;
//	CompOption o1("x", CompOption::TypeInt);
//	o1.value().set(p.x());
//	CompOption o2("y", CompOption::TypeInt);
//	o2.value().set(p.y());
//
//	options.push_back(o1);
//	options.push_back(o2);
//	moveInitiateAction(NULL, CompAction::StateInitButton, options);
//	std::cout << mRay.getX() << ", " << mRay.getY() << ", " << mRay.getZ() << std::endl;
//	cScreen->damageScreen();
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//Note that rigid body uses centerOfMassTransform as 0,0 whereas 0,0 is at upperleft of the window

BulletWindow::BulletWindow(CompWindow* window)
:   PluginClassHandler <BulletWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window))
{
    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);

//    initPhysics();
}

BulletWindow::~BulletWindow()
{
}

void
BulletWindow::initPhysics()
{
//	if ((!(window->wmType ()
//			& (CompWindowTypeDockMask | CompWindowTypeDesktopMask))))
	if (window->wmType() & CompWindowTypeNormalMask)
	{
		btVector3 shapeExtents(myWidth() / 2.0f, myHeight() / 2.0f, myDepth() / 2.0f);
#if USE_2D_SHAPES
		windowShape.reset(new btBox2dShape(shapeExtents)); //box initialized with half extents, compensate for margin?
#else
		windowShape.reset(new btBoxShape(shapeExtents));
#endif
		windowMotionState.reset(new BulletMotionState(window, btTransform(btQuaternion(0,0,0,1), btVector3(centerX(), centerY(), 0))));
		btScalar mass = myMass();
		btVector3 inertia(0,0,0);
		windowShape->calculateLocalInertia(mass, inertia);

		btRigidBody::btRigidBodyConstructionInfo rbci(mass, windowMotionState.get(), windowShape.get(), inertia);
		windowRigidBody.reset(new btRigidBody(rbci), Bullet::RigidBodyDeleter());
		windowRigidBody->setLinearFactor(btVector3(1,1,0)); //limit motion to X-Y-plane
		windowRigidBody->setAngularFactor(btVector3(0,0,0)); //no rotation allowed
		windowRigidBody->setUserPointer(this);
		BulletScreen::get(::screen)->addToWorld(*windowRigidBody.get(), Bullet::WINDOW, Bullet::GROUND | Bullet::WINDOW | Bullet::RAY); //Window receives collision


		// Only do CCD if  motion in one timestep (1.f/60.f) exceeds CUBE_HALF_EXTENTS
		float halfExtents = (myWidth() > myHeight() ? myWidth() : myHeight()) / 2.0;
		windowRigidBody->setCcdMotionThreshold(halfExtents);

		//Experimental: better estimation of CCD Time of Impact:
//		windowRigidBody->setCcdSweptSphereRadius( 0.2*CUBE_HALF_EXTENTS );
		std::cout << "window init" << std::endl;
	}
}

void
BulletWindow::windowNotify(CompWindowNotify notify)
{
	switch (notify)
	{
	case CompWindowNotifyMap:
		if (windowRigidBody)
		{
			if (!windowRigidBody->isInWorld())
			{
				BulletScreen* bs = BulletScreen::get(::screen);
				bs->addToWorld(*windowRigidBody.get(), Bullet::WINDOW, Bullet::GROUND | Bullet::WINDOW | Bullet::RAY);
			}
		}
		break;
	case CompWindowNotifyUnmap:
		if (windowRigidBody)
		{
			if (windowRigidBody->isInWorld())
			{
				BulletScreen* bs = BulletScreen::get(::screen);
				bs->removeFromWorld(*windowRigidBody.get());
			}
		}
		break;
	default:
		break;
	}
	window->windowNotify(notify);
}

bool
BulletWindow::damageRect(bool b, const CompRect& rect)
{
	return cWindow->damageRect(b, rect);
}

void
BulletWindow::activate ()
{
    window->activate ();
}

void
BulletWindow::moveNotify (int dx,
	    		   int dy,
	    		   bool immediate)
{
    window->moveNotify (dx, dy, immediate);
}

void
BulletWindow::resizeNotify (int dx,
	      		     int dy,
	      		     int dwidth,
	      		     int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);
	initPhysics();
}

void
BulletWindow::grabNotify (int x,
	    		   int y,
	    		   unsigned int state,
	    		   unsigned int mask)
{
//	BulletScreen::get(::screen)->removeFromWorld(*windowRigidBody.get());
    window->grabNotify (x, y, state, mask);
}

void
BulletWindow::ungrabNotify ()
{
//	BulletScreen::get(::screen)->addToWorld(*windowRigidBody.get(), Bullet::WINDOW, Bullet::GROUND);
    window->ungrabNotify ();
}

bool
BulletWindow::glPaint (const GLWindowPaintAttrib &attrib,
		        const GLMatrix &transform,
		        const CompRegion &region,
		        unsigned int mask)
{
//	Maybe use rotation
//	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	return gWindow->glPaint(attrib, transform, region, mask);
}

void
BulletWindow::applyForce(const CompPoint& mousePos, bool attract)
{
	CompRect win = window->inputRect();
	btVector3 distance(win.centerX() - mousePos.x(), win.centerY() - mousePos.y(), 0);
	float factor = distance.length2() / (Bullet::SCALE_FACTOR * 1000); //cannot be set dynamically (e.g. to forceMultiplier)?

	btVector3 dir = attract ? -distance : distance;
	btVector3 forceVector = (attract ? factor : 1.0/factor) * dir;
	windowRigidBody->applyCentralForce(forceVector);

	//Set damping depending on distance
//	float centerX = win.centerX() % ::screen->width();
//	float centerY = win.centerY() & ::screen->height();
//	btVector3 dist(centerX - mousePos.x(), centerY - mousePos.y(), 0);
//	dist /= BulletScreen::get(::screen)->maxDivisor; //distance.length is now in the range [0,1]
//	windowRigidBody->setDamping(1.0 - dist.length(), 0.);
//	windowRigidBody->setDamping(0.95, 0.);
}

void
BulletWindow::updateDamping(const CompPoint& mousePos, float maxDivisor)
{
	CompRect win = window->inputRect();
	float centerX = win.centerX() % ::screen->width();
	float centerY = win.centerY() & ::screen->height();
	btVector3 distance(centerX - mousePos.x(), centerY - mousePos.y(), 0);

	distance /= maxDivisor; //distance.length is now in the range [0,1]
	windowRigidBody->setDamping(1.0 - distance.length(), 0.);
}

void
BulletWindow::setForceMultiplier(int multiplier)
{
	//sanity check to prevent overflows:
//	if (sizeof(int) == 4 && multiplier <= 4294967296 / 1000.0)
		this->forceMultiplier = multiplier * Bullet::SCALE_FACTOR;
}

void
BulletWindow::resetPhysics()
{
	BulletScreen* bs = BulletScreen::get(::screen);
	windowRigidBody->clearForces(); //maybe delete rb instead?
	bs->removeFromWorld(*windowRigidBody.get());
}

float
BulletWindow::myWidth()
{
	return (window->serverWidth() +
			2 * window->serverGeometry().border() + window->input().left + window->input().right)
			/ Bullet::SCALE_FACTOR;
}

float
BulletWindow::myHeight()
{
	return (window->serverHeight() +
			2 * window->serverGeometry().border() + window->input().top + window->input().bottom)
			/ Bullet::SCALE_FACTOR;
}

float
BulletWindow::myDepth()
{
	return 0.4;
}

float
BulletWindow::centerZ()
{
	return myDepth() / 2.0f;
}


/**
 * Incl. decorations
 * X + WIDTH/2
 */
float
BulletWindow::centerX()
{
	return ((window->serverX() - window->input().left) +
			(window->serverWidth() + 2 * window->serverGeometry().border() + window->input().left + window->input().right) / 2.0f)
			/ Bullet::SCALE_FACTOR; //TODO: serverX() vs. x() etc.
}

/**
 * Incl. decorations
 * Y + HEIGHT/2
 */
float
BulletWindow::centerY()
{
	return ((window->serverY() - window->input().top) +
			(window->serverHeight() + 2 * window->serverGeometry().border() + window->input().top + window->input().bottom) / 2.0f)
			/ Bullet::SCALE_FACTOR; //TODO: see centerX()
}
float
BulletWindow::myMass()
{
//	return (myWidth() * myHeight()) / 1000.0f;
	return 10;
}
