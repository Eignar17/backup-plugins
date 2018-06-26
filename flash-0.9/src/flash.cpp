/**
*
* Compiz plugin Flash
*
* Copyright : (C) 2007 by Jean-Fran�is Souville & Charles Jeremy
* E-mail    : souville@ecole.ensicaen.fr , charles@ecole.ensicaen.fr
*
* Ported to 0.9.x and maintained by:
* Copyright (c) 2009 Sam Spilsbury <smspillaz@gmail.com>
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

/**
 *    FLASH - macro names
 *    Flash - struct names
 *    flash - function names
 *    fd  - FlashDisplay variable name
 *    fs  - FlashScreen variable name
 */

#include "flash.h"

#define OPTION_DISTANCE_MAX 1024

#define POS_TOP 0
#define POS_BOTTOM 1
#define POS_LEFT 2
#define POS_RIGHT 3

COMPIZ_PLUGIN_20090315 (flash, FlashPluginVTable);

/*
 * flashInitiate
 *
 */
bool
FlashScreen::flashInitiate (CompAction *action,
			    CompAction::State state,
			    CompOption::Vector options)
{
    /* flashInitiate is static, so we need to get vars directly */
    FLASH_SCREEN (screen);
    CompositeScreen *coScreen = CompositeScreen::get (screen);

    fs->active = !fs->active;

    /* TODO: Enable / Disable paint bits */

    for(int i=0 ; i<5 ; i++)
    {
	fs->remainTime[i] = fs->optionGetTime () + fs->interval[i];
	fs->lightning[i] = FALSE;
    }

    if(!fs->active)    
	coScreen->damageScreen();
    return FALSE;
}

static bool
pointerIsInWindow (FlashScreen *fs)
{
    if(!fs->existWindow)
	return false;

    if( (fs->pointX < fs->winX + fs->winW) && (fs->pointX > fs->winX) && 
	(fs->pointY < fs->winY + fs->winH) && (fs->pointY > fs->winY) )
	return True;

    return false;
}

/*
 * getDigression : deplacer la destination
 *
 */
static int
getDigression (FlashScreen *fs, int coord, int min, int max, int *alea)
{
        int ecart = fs->optionGetEcartMax ();

	int interval = max - min;

	int tmp=alea[0] % interval;

	if(coord-min >= ecart)
	    min = coord - ecart;

	if(max-coord >= ecart)
	    max=coord + ecart;
	
	tmp = tmp * (max - min) / interval;
	
	return (max - tmp) - coord;
}

/*
 * getCoordTarget
 *
 */
static void
getWindowTarget (FlashScreen *fs, int* distance, int* Tx, int* Ty, int Px, int Py, int Wx, int Wy, int Ww, int Wh, int *alea)
{
        int distanceMax = fs->optionGetDistanceMax ();

	if(Px <= Wx) *Tx=Wx;
	else if(Px >= Wx+Ww) *Tx=Wx+Ww;
	else *Tx=Px;
	
	if(Py <= Wy) *Ty=Wy;
	else if(Py >= Wy+Wh) *Ty=Wy+Wh;
	else *Ty=Py;
	
	*distance = sqrt( (Px - *Tx)*(Px - *Tx) + (Py - *Ty)*(Py - *Ty) );
	
	if(*distance > 0)
		if(*Tx == Px)
			*Tx += getDigression(fs,Px,Wx,Wx+Ww, alea) * (*distance) / distanceMax + 1;
		if(*Ty == Py)
			*Ty += getDigression(fs,Py,Wy,Wy+Wh, alea) * (*distance) / distanceMax + 1;
}



/*
 * getScreenTarget
 *
 */
static void
getScreenTarget (CompScreen *s, int* distance, int* Tx, int* Ty, int Sx, int Sy, int Sw, int Sh, int *alea, int location)
{
	FLASH_SCREEN (s);
	
	int min=0;
	int max=0;
	int distanceMax = fs->optionGetDistanceMax ();
	
	switch(location)
	{
	case POS_TOP:
		*Tx=fs->pointX;
		*Ty=Sy;
		min=Sx;
		max=Sx+Sw;
		if(!fs->existWindow)
		    break;

		if(fs->pointX < fs->winX && fs->pointY > fs->winY)
		    max=fs->winX;

		else if(fs->pointX > fs->winX + fs->winW && fs->pointY > fs->winY)
		    min=fs->winX+fs->winW;

		else if(fs->pointY >= fs->winY + fs->winH) 
		    *Ty=fs->pointY;
		break;
		
	case POS_BOTTOM:
		*Tx=fs->pointX;
		*Ty=Sy+Sh;
		min=Sx;
		max=Sx+Sw;
		if(!fs->existWindow)
		    break;
		
		if(fs->pointX < fs->winX && fs->pointY < fs->winY+fs->winH)
			max=fs->winX;

		else if(fs->pointX > fs->winX+fs->winW && fs->pointY < fs->winY+fs->winH)
			min=fs->winX+fs->winW;

		else if(fs->pointY <= fs->winY)
		    *Ty=fs->pointY;
		break;
		
	case POS_LEFT:
		*Ty=fs->pointY;
		*Tx=Sx;
		min=Sy;
		max=Sy+Sh;
		if(!fs->existWindow)
		    break;
		
		if(fs->pointY < fs->winY && fs->pointX > fs->winX)
			max=fs->winY;

		else if(fs->pointY > fs->winY+fs->winH && fs->pointX > fs->winX)
			min = fs->winY + fs->winH;

		else if(fs->pointX >= fs->winX + fs->winW)
		    *Tx=fs->pointX;
		break;
		
	case POS_RIGHT:
		*Ty=fs->pointY;
		*Tx=Sx+Sw;
		min=Sy;
		max=Sy+Sh;
		if(!fs->existWindow)
		    break;

		if(fs->pointY < fs->winY && fs->pointX < fs->winX+fs->winW)
			max=fs->winY;

		else if(fs->pointY > fs->winY+fs->winH && fs->pointX < fs->winX+fs->winW)
			min=fs->winY+fs->winH;

		else if(fs->pointX <= fs->winX)
		    *Tx=fs->pointX;
		break;
		
	default:
		break;
	}
	
	*distance = sqrt( (fs->pointX - *Tx)*(fs->pointX - *Tx) + (fs->pointY - *Ty)*(fs->pointY - *Ty) );
	
	if(*distance > 0)
		if(*Tx == fs->pointX)
		    *Tx += getDigression(fs,fs->pointX,min,max, alea) * (*distance) / distanceMax + 1;
		if(*Ty == fs->pointY)
		    *Ty += getDigression(fs,fs->pointY,min,max, alea) * (*distance) / distanceMax + 1;
}


/*
 * getWay : fonction du chemin semi-aleatoire
 *
 */
static int
getWay(int xy, int distance, int *alea)
{

	double pi = 4 * atan (1.0);
	int resultat = 0;
	int amplitude = distance * xy / (distance + xy);
	int i;
	int ms = alea[0] % 100;

	if(ms <= 50)
	    resultat += ms * amplitude * cos( xy * pi / (2*distance));
	else
	    resultat += -(ms - 50) * amplitude * cos( xy * pi / (2*distance));
	
	for(i=1;i<10;i++)
	{
	    resultat += (alea[i] % 100) * amplitude * cos( pow(2,i) * xy * pi * (alea[i] % 100) / (20 * distance) ) * (distance - xy) / (distance * 5 * (i + 1));
	}
	
	return (resultat / 100);
}


static void
getCoordinateArray (double** CoordinateArray ,int distance, int Tx, int Ty, int Px, int Py, int* tailleTab, int* alea, int numero)
{
    	int i;
	double Mx,My;
	int ecart;

	*tailleTab=distance;
		//courbe demarre au pointeur de la souris
	CoordinateArray[0][0]=0;
	CoordinateArray[0][1]=0;

	for(i=1;i<distance;i++)
	    {
		Mx=((double)i / (double)distance)*(Tx-Px);
		My=((double)i / (double)distance)*(Ty-Py);
		   			    
		ecart = getWay(i, distance,alea) + numero;

		CoordinateArray[i][0] = Mx + (double) ecart * (Ty-Py) / distance;
		CoordinateArray[i][1] = My + (double) ecart * (Tx-Px) / distance;
	    }
}

/*
 * flashPreparePaintOutput
 *
 */
void
FlashScreen::preparePaint (int msSinceLastPaint)
{

	CompWindow *w;
	Window root, child;
	int winx, winy;
	int i,j;
	unsigned int pmask;


	if(active)
	{

	    //Get informations about active window
		w = screen->findWindow (screen->activeWindow ());		
		if(w)
		{

		    if(optionGetWindowTypes ().evaluate (w))
				existWindow=TRUE;
		    else
				existWindow=FALSE;
		
		    winW=w->width () + w->input ().left + w->input ().right;
		    winH=w->height () + w->input ().top + w->input ().bottom;
		    winX=w->x () - w->input ().left;
		    winY=w->y () - w->input ().top;
		}
		else
		{
		    existWindow=FALSE;
		    winW=0;
		    winH=0;
		    winX=0;
		    winY=0;	
		    isAnimated = FALSE;
		}
		

		//test if an other plugin is used at this time.
		if(screen->otherGrabExist (0))
		    anotherPluginIsUsing = TRUE;
		else
		    anotherPluginIsUsing = FALSE;
	
		//get position of pointer
		XQueryPointer(screen->dpy (), screen->root (), &root, &child, &(pointX), &(pointY), &winx, &winy, &pmask);

		int time = optionGetTime ();
	
		for(i = 0 ; i < 5 ; i++)
		{
		    remainTime[i] -= msSinceLastPaint;

		    if(remainTime[i] < 0)
		    {
			lightning[i] = FALSE;
			remainTime[i] = interval[i] + time;
			
			srand(alea[i][1] * (pointX+1) * (pointY+1) / (interval[i] + 1) );
			for(j=0;j<10;j++)
			    alea[i][j]=(int) rand();
		    }
		    else
		    {
			if( (remainTime[i]) < time )
			    lightning[i] = TRUE;
		    }
		}
	}

	cScreen->preparePaint (msSinceLastPaint);
}

/*
 * flashDonePaintScreen
 *
 */
void
FlashScreen::donePaint ()
{
    if(active)
		cScreen->damageScreen();

    cScreen->donePaint ();
}

/*
 * flashPaintOutput
 *
 */
bool
FlashScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
		       	    const GLMatrix            &transform,
		            const CompRegion          &region,
		            CompOutput                *output,
		            unsigned int              mask)
{
    bool status;

    gScreen->glPaintOutput (attrib, transform, region, output, mask);

    int x1, y1;
    x1 = pointX;
    y1 = pointY;

    int x2, y2;
    int distanceToWindow;
    int i,j;

    if(!active || pointerIsInWindow(this) || anotherPluginIsUsing)
	return status;

    glPushMatrix();  //sauve la matrice courante
    glLoadIdentity();//remplace la matrice courante par la matrice identite
    glTranslatef (-0.5f, -0.5f, DEFAULT_Z_CAMERA);
    glScalef (1.0f  / output->width (),
	      -1.0f / output->height (),
	      1.0f);
    glTranslatef (-output->x1 (),
		  -output->y2 (),
		  0.0f);

    glEnable(GL_BLEND);//active le melange des couleurs (glBlendFunc)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//permet les effets de transparence

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    glTranslatef(x1, y1, 0);

    glLineWidth(2);
	
    //lightning between pointer and active window
    if(lightning[0])
	{
	    getWindowTarget(this, &distanceToWindow, &x2, &y2, x1, y1, winX, winY, winW, winH, alea[0]);

	    if(existWindow && distanceToWindow < optionGetDistanceMax () && !isAnimated)
		{
		    //changement de la frequence d'apparition en fonction de la distance
		    interval[0] = (optionGetInterval () + distanceToWindow) * (alea[0][1]%400) / 200;// * 2 / fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;
		
		    //affichage du trait du haut ou a gauche
		    glBegin(GL_LINE_STRIP);
		    glColor4f(
			      optionGetFlashColorUpLeftRed () / 0xffff,
			      optionGetFlashColorUpLeftGreen () / 0xffff,
			      optionGetFlashColorUpLeftBlue () / 0xffff,
			      optionGetFlashColorUpLeftAlpha () / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[0], -2);
		    for(i=0;i<tailleTab;i++)
			glVertex2dv(CoordinateArray[i]);
		    glEnd();
		
		    //affichage du trait central
		    glBegin(GL_LINE_STRIP);
		    glColor4f(
			      optionGetFlashColorCenterRed () / 0xffff,
			      optionGetFlashColorCenterGreen () / 0xffff,
			      optionGetFlashColorCenterBlue () / 0xffff,
			      optionGetFlashColorCenterAlpha () / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[0], 0);
		    for(i=0;i<tailleTab;i++)
			glVertex2dv(CoordinateArray[i]);
		    glEnd();
		
		    //affichage du trait du bas ou a droite
		    glBegin(GL_LINE_STRIP);	    
		    glColor4f(
			      optionGetFlashColorDownRightRed () / 0xffff,
			      optionGetFlashColorDownRightGreen () / 0xffff,
			      optionGetFlashColorDownRightBlue () / 0xffff,
			      optionGetFlashColorDownRightAlpha () / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[0], 2);
		    for(i=0;i<tailleTab;i++)
			glVertex2dv(CoordinateArray[i]);	
		    glEnd();
		}
	}

    for(i=1;i<5;i++)
	{
	    if(lightning[i])
		{
		    CompPoint p (screen->vp ());
		     
		    getScreenTarget(screen, &distanceToWindow, &x2, &y2, p.x (), p.y (), screen->width (), screen->height (), alea[i], i-1);

		    if(distanceToWindow < optionGetDistanceMax () )
			{
			    //changement de la frequence d'apparition en fonction de la distance
			    interval[i] = optionGetInterval () + distanceToWindow * (alea[i][1]%600) / 200; // / fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;
		
			    //affichage du trait du haut ou a gauche
			    glBegin(GL_LINE_STRIP);
		    	    glColor4f(
			      optionGetFlashColorUpLeftRed () / 0xffff,
			      optionGetFlashColorUpLeftGreen () / 0xffff,
			      optionGetFlashColorUpLeftBlue () / 0xffff,
			      optionGetFlashColorUpLeftAlpha () / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[i], -2);
			    for(j=0;j<tailleTab;j++)
				glVertex2dv(CoordinateArray[j]);
			    glEnd();
		
			    //affichage du trait central
			    glBegin(GL_LINE_STRIP);
		    	    glColor4f(
			      optionGetFlashColorCenterRed () / 0xffff,
			      optionGetFlashColorCenterGreen () / 0xffff,
			      optionGetFlashColorCenterBlue () / 0xffff,
			      optionGetFlashColorCenterAlpha () / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[i], 0);
			    for(j=0;j<tailleTab;j++)
				glVertex2dv(CoordinateArray[j]);
			    glEnd();
		
			    //affichage du trait du bas ou a droite
			    glBegin(GL_LINE_STRIP);	    
		    	    glColor4f(
			      optionGetFlashColorDownRightRed () / 0xffff,
			      optionGetFlashColorDownRightGreen () / 0xffff,
			      optionGetFlashColorDownRightBlue () / 0xffff,
			      optionGetFlashColorDownRightAlpha () / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(tailleTab), alea[i], 2);
			    for(j=0;j<tailleTab;j++)
				glVertex2dv(CoordinateArray[j]);	
			    glEnd();
			}
		}
	}
    glTranslatef(-x1, -y1, 0);

    glColor4usv(defaultColor);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glPopMatrix();

    return status;
}


FlashScreen::FlashScreen (CompScreen *screen) :
    PluginClassHandler <FlashScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    CoordinateArray ((double **) malloc (OPTION_DISTANCE_MAX * sizeof (double *))),
    remainTime ((int *) malloc (5 * sizeof (int))),
    interval ((int *) malloc (5 * sizeof (int))),
    alea ((int **) malloc (5 * sizeof (int *))),
    active (false),
    lightning ((bool *) malloc (5 * sizeof (bool)))
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    for(int i=0; i<OPTION_DISTANCE_MAX; i++)
	CoordinateArray[i]=(double*)malloc(2*sizeof(double));

    for(int i=0;i<5;i++)
    {
	lightning[i] = FALSE;
	
	alea[i]=(int*) malloc(10 * sizeof(int));
	
	interval[i] = optionGetInterval ();
	remainTime[i] = optionGetTime () + interval[i];
	
	srand(1);
	for(int j=0;j<10;j++)
	    alea[i][j]=(int) rand();

    }

    optionSetInitiateButtonInitiate (flashInitiate);
    optionSetInitiateKeyInitiate (flashInitiate);

}

bool
FlashPluginVTable::init ()
{

    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
