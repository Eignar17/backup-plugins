/*
* twist.cpp

* Description:  

* The plugin allows to compress part of selected window to enlarge free space on the desktop.
* "Twisted" window thus has reduced height or width, while still readable (more or less). 
* The feature should be especially useful on netbooks for their weak screen resolution. 
* Please note that while being quiet usable, twist was created as an experiment 
* to find out ways of improving modern hardware-accelerated GUI.
* So don't panic if its experimental nature would show itself in some bad way 

* Using the plugin:
* To twist a window you should drag it to the direction you want, and continue dragging after 
* the screen edge is reached. Twisted window consists of two parts - normal (non-compressed) part,
* which is accessible by mouse as usually, and compressed one. 
* As for now, compressed part of the window can be accessed only by keyboard, but in many cases
* that is not a problem: e.g. you can compress bottom of the window, when most of its clickable
* elements (like menus or toolbars) are residing on top.

* also "screw" mode was added for testing purposes in addition to standart compressing 
* (try to chose it in config)

* Contributions:

* The concept and the code created by Dmitriy Kostiuk (d.k AT list.ru) and Alexander Nikoniuk (nikoniuk AT mail.ru).

* The code is licensed under GPL v.2. 
* The plugin was created in bounds of a research project in Micro and Midi-ergonomisc Laboratory of Brest State Technical University
*/


#include "twist.h"

COMPIZ_PLUGIN_20090315 (twist, TwistPluginVTable);

static void
toggleScreenFunctions (bool enabled) // add screen events handlers
{
    TWIST_SCREEN (screen);

    screen->handleEventSetEnabled (ts, enabled);
    ts->cScreen->donePaintSetEnabled (ts, enabled);
}

static void
toggleWindowFunctions (CompWindow *w, bool enabled) // add window event handlers
{
    TWIST_WINDOW (w);

    tw->gWindow->glPaintSetEnabled (tw, enabled);
    tw->gWindow->glAddGeometrySetEnabled (tw, enabled);
}

float f(float y)
{
    return 0.000007 * pow (y, 2);
}

bool
TwistWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      const CompRegion		&region,
		      unsigned int		mask)
{
    if (this->transform != NONE)   
    {
        int dx = 0, dy = 0;
        if (this->transform & TOGGLE_BOTTOM)
        {
	    dy = -ceil(30 * (1 - k_bottom));
        }
        if (this->transform & TOGGLE_TOP)
        {
	    dy = ceil(30 * (1 - k_top));
        }
        if (this->transform & TOGGLE_LEFT)
        {
	    dx = ceil(30 * (1 - k_left));
        }
        if (this->transform & TOGGLE_RIGHT)
        {
	    dx = -ceil(30 * (1 - k_right));
        }
		// exit if window isn't on the current wirtual desktop:
		if (screen->currentOutputDev ().workArea ().width() < window->x()
		 ||screen->currentOutputDev ().workArea ().x() > (window->x()+window->width())
		 ||screen->currentOutputDev ().workArea ().height() < window->y()
		 ||screen->currentOutputDev ().workArea ().y() > (window->y()+window->height()))
		  			return true;

        if (dx != 0 || dy != 0)
            window->moveToViewportPosition (window->x () + dx, window->y () + dy, true);
        
	const int GRID_SIZE = 50;
        float width = window->width () + 10;
        float y = screen->currentOutputDev ().workArea ().y () + screen->currentOutputDev ().workArea ().height () - BEND_SIZE;
        float height = BEND_SIZE;
        float x = window->x () - 5;
	if (x < 0)
	{
	    width += x;
	    x = 0;
	} 

        GLMatrix      mTransform = transform;
        float         xTranslate, yTranslate;
        float const   mScale = 0.99999; // chose 'almost 1:1 scale' for the window to work with it as with texture
        mask |= PAINT_WINDOW_TRANSFORMED_MASK; // setting a mask
		// making scaling transformations of a window:
        xTranslate = window->input ().left * (mScale - 1.0f);
        yTranslate = window->input ().top * (mScale - 1.0f);

        mTransform.translate (window->x (), window->y (), 0);
        mTransform.scale (mScale, mScale, 0);
        mTransform.translate (xTranslate / mScale - window->x (), yTranslate / mScale - window->y (), 0.0f);

        gWindow->glPaint (attrib, mTransform, region, mask); // call paint handler of the parent class
		// glAddGeometry should be automatically called at this moment
	TWIST_SCREEN(screen);	

	if (ts->optionGetTwistingType () == 0)
        {

            glBindTexture (GL_TEXTURE_2D, texture);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, screen->height () - (y + height), width, height, 0);

	    //glMatrixMode (GL_PROJECTION);
       
            //glPushMatrix ();

       //glLoadIdentity();
        //glOrtho(0, 0, screen->width (), screen->height (), -1, 1);   

            glEnable (GL_TEXTURE_2D);

	    float x1 = x;
	    float x2 = x + width;
	    float dy = float(height - 28) / GRID_SIZE;
	    glBegin (GL_QUADS);
	    for (int i = 0; i < GRID_SIZE; ++i)
	    {
	        float y1 = y + dy * i;
	        float y2 = y + dy * (i + 1);
	        float z1 = f(y1 - y);
	        float z2 = f(y2 - y);
	    
  	        float texY1 = 1 - float (i) / GRID_SIZE;
	        float texY2 = 1 - float (i + 1) / GRID_SIZE;
	        glTexCoord2f (0.0f, texY2);glVertex3f (x1, y2, z2); // bottom left
	        glTexCoord2f (1.0f, texY2);glVertex3f (x2, y2, z2); // bottom right
	        glTexCoord2f (1.0f, texY1);glVertex3f (x2, y1, z1); // top right
	        glTexCoord2f (0.0f, texY1);glVertex3f (x1, y1, z1); // top left
	    }
	    glEnd ();

	    glBindTexture (GL_TEXTURE_2D, 0);
	    glDisable (GL_TEXTURE_2D);
    
            //glPopMatrix ();

	}
    }
    else
    {
        //TWIST_WINDOW (window);
        //tw->gWindow->glAddGeometrySetEnabled (tw, false);
        gWindow->glPaint (attrib, transform, region, mask); 
        //tw->gWindow->glAddGeometrySetEnabled (tw, true);
    }

    return true;
}

void
TwistWindow::glAddGeometry (const GLTexture::MatrixList& matrices,
			   const CompRegion&            region,
			   const CompRegion&            clip,
			   unsigned int                 maxGridWidth,
			   unsigned int                 maxGridHeight)
{ // transforming part of the window
  // oldVCount - old amount of vertices 
  // workArea().y - height of an apper panel
  // workArea().height - height of the working area
  // bendingBorderY - upper border of the window compressed area in screen coordinates
  // k is a scale factor for the concrete window
	int         i, oldVCount = gWindow->geometry ().vCount;
	GLfloat     *v;

	bendingBorderBottom = screen->currentOutputDev ().workArea ().y () + screen->currentOutputDev ().workArea ().height () - BEND_SIZE;  
	k_bottom = BEND_SIZE / (window->y () + window->height () + 5 - bendingBorderBottom);

        bendingBorderTop = screen->currentOutputDev ().workArea ().y () + BEND_SIZE;
        k_top = (BEND_SIZE) / (bendingBorderTop - window->y () + 20);

        bendingBorderLeft = screen->currentOutputDev ().workArea ().x () + BEND_SIZE;
        k_left = BEND_SIZE / (bendingBorderLeft - window->x () + 5);

        bendingBorderRight = screen->currentOutputDev ().workArea ().x () + screen->currentOutputDev ().workArea ().width () - BEND_SIZE; 
        k_right = BEND_SIZE / (window->x () + window->width () + 5 - bendingBorderRight);

	gWindow->glAddGeometry (matrices, region, clip,   // parent function
				MIN(maxGridWidth , 50),	  // here window is painted
				MIN(maxGridHeight , 50)); // v will be array of vertices coordinates
	v  = gWindow->geometry ().vertices; 		  // v points to window vertices array
	v += gWindow->geometry ().vertexStride - 3;	  // going 1 vertex (3 coordinates) back 
	v += gWindow->geometry ().vertexStride * oldVCount;	// passing old vertices

        int toggle = transform & (TOGGLE_BOTTOM | TOGGLE_TOP | TOGGLE_LEFT | TOGGLE_RIGHT);
	transform = NONE;

        if (k_bottom < 1 && k_bottom > 0)
	{
	    transform |= BOTTOM | (toggle & TOGGLE_BOTTOM);
        }
	if (k_top < 1 && k_top > 0)
	{
	    transform |= TOP | (toggle & TOGGLE_TOP);
        }
	if (k_left < 1 && k_left > 0)
	{
	    transform |= LEFT | (toggle & TOGGLE_LEFT);
        }
	if (k_right < 1 && k_right > 0)
	{
	    transform |= RIGHT | (toggle & TOGGLE_RIGHT);
        }

	for (i = oldVCount; i < gWindow->geometry ().vCount; i++)  // for all new vertices
	{
	    if (transform & BOTTOM && v[1] > bendingBorderBottom) // if vertex is in bottom scaled area
	    {
                v[1] = bendingBorderBottom + (v[1] - bendingBorderBottom) * k_bottom ; // changing its y coord
	    }
	    if (transform & TOP && v[1] < bendingBorderTop) 	// if vertex is in top scaled area
	    {
                v[1] = (screen->currentOutputDev ().workArea ().y () + BEND_SIZE) * (1 - k_top) + v[1] * k_top;
	    }
	    if (transform & RIGHT && v[0] > bendingBorderRight) // if vertex is in right scaled area
	    {
                v[0] = bendingBorderRight + (v[0] - bendingBorderRight) * k_right;
	    }
	    if (transform & LEFT && v[0] < bendingBorderLeft)	// if vertex is in left scaled area
	    {
                v[0] = (screen->currentOutputDev ().workArea ().x () + BEND_SIZE) * (1 - k_left) + v[0] * k_left;
	    }

	    v += gWindow->geometry ().vertexStride;	// passing one vertex
        } 
   
}

void
TwistScreen::donePaint ()			
{
    foreach (CompWindow *w, screen->windows ())
    {
	TWIST_WINDOW(w);

        if (tw->transform != NONE)
  	{
	    tw->cWindow->addDamage ();
        }
    }
    cScreen->donePaint ();
 		
}

void
TwistScreen::handleEvent (XEvent *event)
{
    /*switch (event->type)
    {
	case ButtonPrets:
        if (pointerY > 650 && optionGetTwistingType () == 1)
        {
	    CompWindow *window = screen->findWindow (screen->activeWindow ());
	    const float bendingBorderY = 650.0;
	    const float k = (screen->currentOutputDev ().workArea ().y () + screen->currentOutputDev ().workArea ().height () - bendingBorderY) / (window->y () + window->height () - bendingBorderY);
            float y = bendingBorderY + (pointerY - bendingBorderY) / k;
	    pointerY = y;
	    event->type = ButtonRelease;
	    screen->handleEvent (event);
	    event->type = ButtonPrets;
	    screen->warpPointer (0 , y - pointerY);
	    if (XSendEvent(XOpenDisplay(NULL), PointerWindow, True, ButtonReleaseMask, event) == 0)
    		std::cout << "Error to send the event!";
        }	    
	    break; 
    }*/

    screen->handleEvent (event);
}


TwistScreen::TwistScreen (CompScreen *screen) :
    PluginClassHandler <TwistScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen))
{
    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    pollHandle.setCallback (boost::bind (&TwistScreen::positionUpdate, this, _1));
    pollHandle.start ();

    toggleScreenFunctions(true);
}

TwistScreen::~TwistScreen ()
{
    pollHandle.stop ();
}

TwistWindow::TwistWindow (CompWindow *window) :
    PluginClassHandler <TwistWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    transform(NONE),
    bendingBorderTop(0),
    bendingBorderBottom(0),
    bendingBorderLeft(0),
    bendingBorderRight(0)
{
    WindowInterface::setHandler (window, false);
    CompositeWindowInterface::setHandler (cWindow, false);
    GLWindowInterface::setHandler (gWindow, false);


    glGenTextures (1, &texture);

    glEnable (GL_TEXTURE_2D);

    //Bind the texture 
    glBindTexture (GL_TEXTURE_2D, texture);

    // Load the parameters
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable (GL_TEXTURE_2D);

    if (window->wmType () & 65283)
      //not (CompWindowTypeDesktopMask | CompWindowTypeDockMask) and etc
	return;

    toggleWindowFunctions (window, true);
}

void
TwistScreen::positionUpdate (const CompPoint & pos)
{
    CompWindow *w = screen->findWindow (screen->activeWindow ());
    TWIST_WINDOW (w);
  
    int posX = pos.x ();
    int posY = pos.y ();

    if (tw->transform & BOTTOM && posY >= tw->bendingBorderBottom && posX >= w->x () && posX <= w->x () + w->width ())
    {
	tw->transform |= TOGGLE_BOTTOM;
    }
    if (tw->transform & TOP && posY <= tw->bendingBorderTop && posX >= w->x () && posX <= w->x () + w->width ())
    {
	tw->transform |= TOGGLE_TOP;
    }
    if (tw->transform & LEFT && posX <= tw->bendingBorderLeft && posY >= w->y () - 20 && posY <= w->y () + w->height ())
    {
	tw->transform |= TOGGLE_LEFT;
    }
    if (tw->transform & RIGHT && posX >= tw->bendingBorderRight && posY >= w->y () - 20 && posY <= w->y () + w->height ())
    {
	tw->transform |= TOGGLE_RIGHT;
    } 
}

TwistWindow::~TwistWindow () { }

bool
TwistPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    if (!screen->XShape ())
    {
	compLogMessage ("twist", CompLogLevelError,
			"No Shape extension found. Twisting not potsible \n");
	return false;
    }
	compLogMessage ("twist", CompLogLevelWarn, "Hello from twist!\n"); 
    return true;
}
