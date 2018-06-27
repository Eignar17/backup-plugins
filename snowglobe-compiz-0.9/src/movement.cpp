#include <cmath>
#include "snowglobe.h"


void
SnowflakeTransform (snowflakeRec * snow)
{
    glTranslatef (snow->y, snow->z, snow->x);
    glRotatef (snow->theta, 1.0, 0.0, 0.0);
}

void
SnowglobeScreen::newSnowflakePosition (int i)
{
    SnowglobeScreen *as = this;
  
    int sector = NRAND(as->mHsize);
    float ang = randf(as->mArcAngle*toRadians)-0.5*as->mArcAngle*toRadians;
    float r = (as->mRadius-0.01*as->mSnow[i].size/2);
    float factor = sinf(0.5*(PI-as->mArcAngle*toRadians))/
    		   sinf(0.5*(PI-as->mArcAngle*toRadians)+fabsf(ang));
    ang += (0.5+((float) sector))*as->mArcAngle*toRadians;
    ang = fmodf(ang,2*PI);
	
    float d = randf(1);
    d=(1-d*d)*r*factor;

    as->mSnow[i].x = d*cosf(ang);
    as->mSnow[i].y = d*sinf(ang);
    as->mSnow[i].z = 0.5;
}

void
SnowglobeScreen::SnowflakeDrift (int index)
{
    float progress;
    
    csScreen->cubeGetRotation (mXRotate, mVRotate, progress);

    mXRotate = fmodf (mXRotate - csScreen->invert () * (360.0f / screen->vpSize ().width ()) *
           		 ((screen->vp ().x () * csScreen->nOutput ())), 360 );
    
    snowflakeRec * snow = &(mSnow[index]);

    float speed = snow->speed * mSpeedFactor;
    speed /= 1000;
    float x = snow->x;
    float y = snow->y;
    float z = snow->z;

    float sideways = 2 * (randf (2 * speed) - speed);
    float vertical = -speed;

    if (optionGetShakeCube ())
    {
	x+= sideways * cosf (mXRotate * toRadians) * cosf (mVRotate * toRadians)
	   -vertical * cosf (mXRotate * toRadians) * sinf (mVRotate * toRadians);
	
    	y+= sideways * sinf(mXRotate * toRadians) * cosf (mVRotate * toRadians)
    	   + vertical * sinf(mXRotate * toRadians) * sinf (mVRotate * toRadians);
    	
	z+= sideways * sinf (mVRotate * toRadians)
	   + vertical * cosf (mVRotate * toRadians);
    }
    else
    {
	x += sideways;
    	y += sideways;
	z += vertical;
    }
    

    float bottom = (optionGetShowGround () ? getHeight (mGround, x, y) : -0.5) + 0.01 * snow->size / 2;

    if (z < bottom)
    {
	z = 0.5;
	newSnowflakePosition (index);
	    
	x = snow->x;
	y = snow->y;
    }

    float top = 0.5-0.01 * snow->size / 2;
    if (z > top)
    {
	z = top;
    }

    
    float ang = atan2f(y, x);

    int i;
    for (i = 0; i < mHsize; i++)
    {
	float cosAng = cosf (fmodf (i * mArcAngle * toRadians - ang, 2 * PI));
	if (cosAng <= 0)
	    continue;

	float r = hypotf (x, y);
	float d = r * cosAng - (mDistance - 0.01 * snow->size / 2);
	
	if (d>0)
	{
	    x -= d * cosf (ang) * fabsf (cosf (i * mArcAngle * toRadians));
	    y -= d * sinf (ang) * fabsf (sinf (i * mArcAngle * toRadians));
	}
    }

    snow->x = x;
    snow->y = y;
    snow->z = z;

    snow->psi = fmodf (snow->psi + snow->dpsi * mSpeedFactor, 360);
    snow->theta= fmodf (snow->theta + snow->dtheta * mSpeedFactor, 360);
}
