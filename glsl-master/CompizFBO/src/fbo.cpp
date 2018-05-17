#include "fbo.h"

#include <GL/glext.h>
#include <GL/glx.h>

COMPIZ_PLUGIN_20090315 (fbo, FboPluginVTable);

PFNGLCREATEPROGRAMPROC      m_glCreateProgram      = NULL;
PFNGLUSEPROGRAMPROC         m_glUseProgram         = NULL;
PFNGLCREATESHADERPROC       m_glCreateShader       = NULL;
PFNGLSHADERSOURCEPROC       m_glShaderSource       = NULL;
PFNGLCOMPILESHADERPROC      m_glCompileShader      = NULL;
PFNGLATTACHSHADERPROC       m_glAttachShader       = NULL;
PFNGLLINKPROGRAMPROC        m_glLinkProgram        = NULL;
PFNGLGETUNIFORMLOCATIONPROC m_glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC          m_glUniform1i          = NULL;
PFNGLGETSHADERIVPROC        m_glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC   m_glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMINFOLOGPROC  m_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC       m_glGetProgramiv = NULL;
PFNGLDELETESHADERPROC       m_glDeleteShader = NULL;
PFNGLDELETEPROGRAMPROC      m_glDeleteProgram = NULL;

bool
FboScreen::queryForShader()
{
    m_glCreateProgram         = (PFNGLCREATEPROGRAMPROC)      gScreen->getProcAddress ("glCreateProgram");
    m_glUseProgram            = (PFNGLUSEPROGRAMPROC)         gScreen->getProcAddress ("glUseProgram");
    m_glCreateShader          = (PFNGLCREATESHADERPROC)       gScreen->getProcAddress ("glCreateShader");
    m_glShaderSource          = (PFNGLSHADERSOURCEPROC)       gScreen->getProcAddress ("glShaderSource");
    m_glCompileShader         = (PFNGLCOMPILESHADERPROC)      gScreen->getProcAddress ("glCompileShader");
    m_glAttachShader          = (PFNGLATTACHSHADERPROC)       gScreen->getProcAddress ("glAttachShader");
    m_glLinkProgram           = (PFNGLLINKPROGRAMPROC)        gScreen->getProcAddress ("glLinkProgram");
    m_glGetUniformLocation    = (PFNGLGETUNIFORMLOCATIONPROC) gScreen->getProcAddress ("glGetUniformLocation");
    m_glUniform1i             = (PFNGLUNIFORM1IPROC)          gScreen->getProcAddress ("glUniform1i");
    m_glGetShaderiv           = (PFNGLGETSHADERIVPROC)        gScreen->getProcAddress ("glGetShaderiv");
    m_glGetShaderInfoLog      = (PFNGLGETSHADERINFOLOGPROC)   gScreen->getProcAddress ("glGetShaderInfoLog");
    m_glGetProgramInfoLog     = (PFNGLGETPROGRAMINFOLOGPROC)  gScreen->getProcAddress ("glGetProgramInfoLog");
    m_glGetProgramiv          = (PFNGLGETPROGRAMIVPROC)       gScreen->getProcAddress ("glGetProgramiv");
    m_glDeleteShader          = (PFNGLDELETESHADERPROC)       gScreen->getProcAddress ("glDeleteShader");
    m_glDeleteProgram         = (PFNGLDELETEPROGRAMPROC)      gScreen->getProcAddress ("glDeleteProgram");

	if (m_glCreateShader == NULL)
		std::cout << "glCreateShader not supported" << std::endl;
	if (m_glUseProgram == NULL)
		std::cout << "glUseProgram not supported" << std::endl;
	if (m_glShaderSource == NULL)
		std::cout << "glShaderSource not supported" << std::endl;
	if (m_glCompileShader == NULL)
		std::cout << "glCompileShader not supported" << std::endl;
	if (m_glGetShaderiv == NULL)
		std::cout << "glGetShaderiv not supported" << std::endl;
	if (m_glGetShaderInfoLog == NULL)
		std::cout << "glGetShaderInfoLog not supported" << std::endl;
	if (m_glDeleteShader == NULL)
		std::cout << "glDeleteShader not supported" << std::endl;
	if (m_glDeleteProgram == NULL)
		std::cout << "glDeleteProgram not supported" << std::endl;
	if (m_glCreateProgram == NULL)
		std::cout << "glCreateProgram not supported" << std::endl;
	if (m_glAttachShader == NULL)
		std::cout << "glAttachShader not supported" << std::endl;
	if (m_glLinkProgram == NULL)
		std::cout << "glLinkProgram not supported" << std::endl;
	if (m_glGetProgramiv == NULL)
		std::cout << "m_glGetProgramiv not supported" << std::endl;
	if (m_glGetProgramInfoLog == NULL)
		std::cout << "glGetProgramInfoLog not supported" << std::endl;
	if (m_glGetUniformLocation == NULL)
		std::cout << "glGetUniformLocation not supported" << std::endl;
	if (m_glUniform1i == NULL)
		std::cout << "glUniform1i not supported" << std::endl;

	if (!(m_glUseProgram        &&
			m_glCreateShader     &&
			m_glShaderSource        &&
			m_glCompileShader &&
			m_glGetShaderiv   &&
			m_glGetShaderInfoLog &&
			m_glDeleteShader &&
			m_glDeleteProgram &&
			m_glCreateProgram &&
			m_glAttachShader &&
			m_glLinkProgram &&
			m_glGetProgramiv &&
			m_glGetProgramInfoLog &&
			m_glGetUniformLocation &&
			m_glUniform1i))
	{
		return false;
	}
	return true;
}

void
FboScreen::allocTexture (int index, char* data)
{
    glGenTextures (1, &texture[index]);
    glBindTexture (target, texture[index]);

    glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D (target, 0, GL_RGBA, width, height, 0, GL_BGRA,
#if IMAGE_BYTE_ORDER == MSBFirst
		  GL_UNSIGNED_INT_8_8_8_8_REV,
#else
		  GL_UNSIGNED_BYTE,
#endif
		  (data == NULL ? 0 : data));

    glBindTexture (target, 0);
}

void
FboScreen::loadMaps()
{
    void *image01 = NULL;
    void *image02 = NULL;
    CompSize size;
    CompString map01Path(optionGetFileDistProj1());
    CompString map02Path(optionGetFileDistProj2());

    if (map01Path.empty() || map02Path.empty())
    	return;

    if (!::screen->readImageFromFile (map01Path, size, image01) || !image01)
    {
    	compLogMessage ("fbo", CompLogLevelWarn,
    				"Failed to load map: %s",
    				map01Path.c_str ());
    	return;
    }
    if (!::screen->readImageFromFile (map02Path, size, image02) || !image02)
    {
    	compLogMessage ("fbo", CompLogLevelWarn,
    				"Failed to load map: %s",
    				map02Path.c_str ());
    	return;
    }
    allocTexture(1, (char*) image01);
    allocTexture(2, (char*) image02);
}

GLuint
FboScreen::loadShader(std::string& fileName, GLuint type)
{
	std::string strData;
	std::ifstream fileStream;
	fileStream.open(fileName.c_str());

	if(!fileStream)
	{
		std::cerr << "File " <<  fileName.c_str() << " could not be opened\n";
		return 0;
	}
	getline(fileStream, strData, fileStream.widen('\255'));
	fileStream.close();
	GLuint shader = 0;
	shader = (*m_glCreateShader)(type);
	if (shader == 0) //happens with FGLRX
	{
		std::cout << "Shader is invalid" << std::endl;
		return 0;
	}

	const char* src = strData.c_str();
	m_glShaderSource(shader, 1, &src, NULL);
	m_glCompileShader(shader);

	GLint compiled = 0;
	m_glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLsizei length;
		m_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char* log = new char[length];
		m_glGetShaderInfoLog(shader,length,&length,log);
		std::cerr << "Shader Compile Error : " << log << std::endl;
		delete log;
		m_glDeleteShader(shader);
		shader = 0;
	}
	return shader;
}

void
FboScreen::clearShader()
{
	if (shaderProgram != 0)
		m_glDeleteProgram(shaderProgram);
	if (vertexShader != 0)
		m_glDeleteShader(vertexShader);
	if (fragmentShader != 0)
		m_glDeleteShader(fragmentShader);

	shaderProgram = 0;
	vertexShader = 0;
	fragmentShader = 0;
}

void
FboScreen::loadProgram(std::string& vertexShaderPath, std::string& fragmentShaderPath)
{
	clearShader();

	vertexShader	= loadShader(vertexShaderPath, 		GL_VERTEX_SHADER);
	if (vertexShader == 0)
	{
		std::cerr << "ERROR creating Vertex shader" << std::endl;
		return;
	}
	fragmentShader	= loadShader(fragmentShaderPath, 	GL_FRAGMENT_SHADER);
	if (fragmentShader == 0)
	{
		std::cerr << "ERROR creating Fragment shader" << std::endl;
		return;
	}

	GLuint program = 0;
	program = m_glCreateProgram();
	m_glAttachShader(program, vertexShader);
	m_glAttachShader(program, fragmentShader);
	m_glLinkProgram(program);

	GLint linked;
	m_glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if(!linked)
	{
		GLsizei length;
		m_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char* log = new char[length];
		m_glGetProgramInfoLog(program, length, &length, log);
		std::cerr << "Program Link Error : " << log << "\n";
		delete log;
		m_glDeleteProgram(program);
		program = 0;
	}
	std::cout << "Shader creation complete" << std::endl;
	this->shaderProgram = program;
}

void
FboScreen::bindShader()
{
    if (shaderProgram != 0)
    {
    	m_glUseProgram(shaderProgram);

    	GLint colorMapSampler = m_glGetUniformLocation(shaderProgram,"colorMap");
    	m_glUniform1i(colorMapSampler,0);

		GLint distortionMapSampler = m_glGetUniformLocation(shaderProgram,"distortionMap");
		m_glUniform1i(distortionMapSampler,1);

		GLint blendMapSampler = m_glGetUniformLocation(shaderProgram,"blendMap");
		m_glUniform1i(blendMapSampler,2);
    }

}

void
FboScreen::unbindShader()
{
	m_glUseProgram(0);
}

bool
FboScreen::fboPrologue ()
{
	int tIndex = 0;
	
    if (fbo == 0)
    	return false;

    if (!texture[tIndex])
    	allocTexture (tIndex);

    GL::bindFramebuffer (GL_FRAMEBUFFER_EXT, fbo);

    GL::framebufferTexture2D (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			      target, texture[tIndex], 0);

    glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
    glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);

    if (!fboStatus)
    {
		fboStatus = GL::checkFramebufferStatus (GL_FRAMEBUFFER_EXT);
		if (fboStatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			compLogMessage ("fbo", CompLogLevelError,
					"framebuffer incomplete");

			GL::bindFramebuffer (GL_FRAMEBUFFER_EXT, 0);
			GL::deleteFramebuffers (1, &fbo);

			glDrawBuffer (GL_BACK);
			glReadBuffer (GL_BACK);

			fbo = 0;

			return false;
		}
    }

    return true;
}

void
FboScreen::fboEpilogue ()
{
    GL::bindFramebuffer (GL_FRAMEBUFFER_EXT, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    glDrawBuffer (GL_BACK);
    glReadBuffer (GL_BACK);

}

/**
 * @see opengl/screen.cpp
 */
void
FboScreen::paint(CompOutput::ptrList &outputs, unsigned int mask)
{
	fboEpilogue ();

    if (!texture[0])
    	allocTexture (0);

	glEnable (target);
	GL::activeTexture (GL_TEXTURE0_ARB);
    glBindTexture (target, texture[0]);
    glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	XRectangle r;
	XRectangle lastViewport;
    int outputCount = 1;

    foreach (CompOutput *output, outputs)
    {
		r.x	 = output->x1 ();
		r.y	 = screen->height () - output->y2 ();
		r.width  = output->width ();
		r.height = output->height ();

        distWidth = r.width;
        distHeight = r.height;

		if (lastViewport.x      != r.x     ||
			lastViewport.y      != r.y     ||
			lastViewport.width  != r.width ||
			lastViewport.height != r.height)
		{
			glViewport (r.x, r.y, r.width, r.height);
			lastViewport = r;
		}

	    {
	    	glPushMatrix();
			GLMatrix transform;
		    transform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
		    glLoadMatrixf (transform.getMatrix ());

		    //Since coordinate system is mirrored at the x axis (by toScreenSpace()),
		    //(0,0) is at the upper left of the screen (=screen coordinates)
		    //however the texture matrix was not changed and thereby (0,0) in texture space is at (0,screen->height()) in screen space
		    float texX = (1.0f / screen->width()) * output->x1();
		    float texY = (1.0f / screen->height()) * (screen->height () - output->y2 ());
		    float texX2 = (1.0f / screen->width()) * output->x2(); //x2() = x + width
		    float texY2 = (1.0f / screen->height()) * (screen->height () - output->y2 () + output->height());

		    if (!texture[1] || !texture[2])
		        loadMaps();

			if (texture[1])
			{
				GL::activeTexture (GL_TEXTURE1_ARB);
				glBindTexture (target, texture[1]);
				glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			if (texture[2])
			{
				GL::activeTexture (GL_TEXTURE2_ARB);
				glBindTexture (target, texture[2]);
				glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			
		    if(shaderInitialized)
		    {
		    	if (mToggleShader)
		    		bindShader();
		    	else
		    		unbindShader();
		    }
			glBegin (GL_QUADS);
			glTexCoord2f (texX, texY2);
			glVertex2i   (output->x1(),		output->y1());
			glTexCoord2f (texX, texY);
			glVertex2i   (output->x1(),		output->y2());
			glTexCoord2f (texX2, texY);
			glVertex2i   (output->x2(), 	output->y2());
			glTexCoord2f (texX2, texY2);
			glVertex2i   (output->x2(), 	output->y1());
			glEnd ();

			if (shaderInitialized)
			{
				unbindShader();
			}

			if (texture[2])
			{
				glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glBindTexture (target, 0);
			}
			if (texture[1])
			{
				GL::activeTexture (GL_TEXTURE1_ARB);
				glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glBindTexture (target, 0);
			}
			glPopMatrix();
	    }
        outputCount++;
    }

    GL::activeTexture (GL_TEXTURE0_ARB);
    glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture (target, 0);

	glDisable(target);

	if (!fboPrologue ())
	{
		std::cout << "Error: Cannot use FBO" <<std::endl;
	}
    cScreen->paint (outputs, mask);
}

void
FboScreen::donePaint ()
{
	cScreen->damageScreen();
    cScreen->donePaint ();
}

bool
FboScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			   const GLMatrix	     &transform,
			   const CompRegion	     &region,
			   CompOutput		     *output,
			   unsigned int		     mask)
{
	bool status = gScreen->glPaintOutput(attrib, transform, region, output, mask);
//	if (toggle /*&& output == &screen->outputDevs().at(screen->outputDeviceForPoint(mouse.x(), mouse.y()))*/) //TODO: only draw cursor once
//	{
	if (optionGetTransformCursor ())
	{
		glPushMatrix();
		GLMatrix sTransform;
		sTransform.toScreenSpace(output, -DEFAULT_Z_CAMERA);
		glLoadMatrixf(sTransform.getMatrix());
		cursor.draw(); //draw cursor on top of all windows and let it be distorted
		glPopMatrix();
	}

	return status;
}

void
FboScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
			      		 const GLMatrix		&transform,
			      		 const CompRegion		&region,
			      		 CompOutput 		*output,
			      		 unsigned int		mask)
{
	//do nothing
    gScreen->glPaintOutput (attrib, transform, region, output, mask);
}


static bool
togglefbo (CompAction         *action,
		 CompAction::State  state,
		 CompOption::Vector &options)
{
	FboScreen* fs = FboScreen::get (screen);

	fs->toggleFBO();
    return false;
}

static bool
reloadShaderOption (CompAction         *action,
		 CompAction::State  state,
		 CompOption::Vector &options)
{
	FboScreen* fs = FboScreen::get (screen);
	fs->reloadShader();
    return false;
}

bool
FboScreen::toggleFBO ()
{
    if (!toggle)
	{
    	screen->handleEventSetEnabled(this, true);
		cScreen->paintSetEnabled (this, true);
		cScreen->donePaintSetEnabled (this, true);
		gScreen->glPaintOutputSetEnabled(this, true);

		if (optionGetTransformCursor ())
			cursorActivate();
	}
    else {
		fboEpilogue();

		screen->handleEventSetEnabled(this, false);
		cScreen->paintSetEnabled (this, false);
		cScreen->donePaintSetEnabled (this, false);
		gScreen->glPaintOutputSetEnabled(this, false);
		
		cursorDeactivate();

	}
	toggle = !toggle;
    cScreen->damageScreen ();

    return true;
}

void
FboScreen::reloadShader()
{
	if (!shaderInitialized)
		return;
	CompString vertShaderFilePath(optionGetFileVert());
	CompString fragShaderFilePath(optionGetFileFrag());
	if (!vertShaderFilePath.empty() && !fragShaderFilePath.empty())
		loadProgram(vertShaderFilePath, fragShaderFilePath);
}

bool
FboScreen::toggleShader (CompAction *action,
		 CompAction::State  state,
		 CompOption::Vector &options)
{
	if (!shaderInitialized)
	{
		if (!queryForShader())
		{
			std::cout << "ERROR: SHADER NOT SUPPORTED" << std::endl;
			return false;
		}
		else
		{
			shaderInitialized = true;
		}
	}
	if (!this->mToggleShader)
	{
		if (shaderProgram == 0)
			reloadShader();
	}
	mToggleShader = !mToggleShader;
	cScreen->damageScreen();
	return false;
}

void
FboScreen::handleEvent(XEvent* event)
{
	switch(event->type)
	{
	default:
	    if (event->type == fixesEventBase + XFixesCursorNotify)
	    {
		//XFixesCursorNotifyEvent *cev = (XFixesCursorNotifyEvent *)
		    //event;
//	    	std::cout << "Cursor changed!" << std::endl;
		    updateCursor (&cursor);
	    }
	}
	screen->handleEvent(event);
}


FboScreen::Cursor::Cursor()
: set(false),
  texture(0),
  screen(NULL)
{
}

void
FboScreen::Cursor::updateTextureData(unsigned char* data, int width, int height, int x, int y, int xHot, int yHot, CompScreen* screen)
{
    if (!set)
    {
    	set = true;
    	screen = screen;
    	glEnable (GL_TEXTURE_RECTANGLE_ARB);
    	glGenTextures (1, &texture);
    	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, texture);

    	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB,
    			 GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB,
    			 GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB,
    			 GL_TEXTURE_WRAP_S, GL_CLAMP);
    	glTexParameteri (GL_TEXTURE_RECTANGLE_ARB,
    			 GL_TEXTURE_WRAP_T, GL_CLAMP);

    	glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    mWidth = width;
    mHeight = height;
//    this->x = x;
//    this->y = y;
    mXHot = xHot;
    mYHot = yHot;

    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);

    glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width,
		  height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);

    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);
}

void
FboScreen::updateCursor(Cursor* cursor)
{
    XFixesCursorImage *cursorImage = XFixesGetCursorImage(screen->dpy ()); //TODO: Hack still necessary (@see ezoom)?

    unsigned int numBytes = cursorImage->width * cursorImage->height * 4;
    unsigned char *pixels = (unsigned char*)malloc(numBytes); //allocation with new() crashes compiz

    if (!pixels)
    {
    	XFree(cursorImage);
    	return;
    }

    for (int i = 0; i < cursorImage->width * cursorImage->height; i++)
    {
		unsigned long pixel = cursorImage->pixels[i];
		pixels[i * 4] = pixel & 0xff;
		pixels[(i * 4) + 1] = (pixel >> 8) & 0xff;
		pixels[(i * 4) + 2] = (pixel >> 16) & 0xff;
		pixels[(i * 4) + 3] = (pixel >> 24) & 0xff;
    }

    cursor->updateTextureData(pixels,
    		cursorImage->width,
    		cursorImage->height,
    		cursorImage->x,
    		cursorImage->y,
    		cursorImage->xhot,
    		cursorImage->yhot,
    		screen);

    XFree (cursorImage);
    free (pixels);

}
void
FboScreen::Cursor::setPos(int x, int y)
{
    pos.setX (x);
    pos.setY (y);
}

void
FboScreen::Cursor::draw()
{
	glDisable(GL_LIGHTING);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
//	std::cout << "drawCursor: x: " << -cursor.xHot() << ", y: " << -cursor.yHot() << std::endl;

	int x = pos.x() - mXHot; //shouldn't that be cursor.x and .y?
	int y = pos.y() - mYHot;
	glEnable (GL_BLEND);
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, texture);
	glEnable (GL_TEXTURE_RECTANGLE_ARB);

	GLfloat vertices[] = {
		x, 			y,
		x, 			y + mHeight,
		x + mWidth, y + mHeight,
		x + mWidth, y};

	GLfloat texCoords[] = {
		0, 		0,
		0,		mHeight,
		mWidth, mHeight,
		mWidth, 0
	};
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopClientAttrib();

	glDisable (GL_BLEND);
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable (GL_TEXTURE_RECTANGLE_ARB);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_LIGHTING);
}

void
FboScreen::Cursor::free()
{
	if (!set)
		return;

    set = false;
	glDeleteTextures (1, &texture);
	texture = 0;
}

/* Timeout handler to poll the mouse. Returns false (and thereby does not
 * get re-added to the queue) when zoom is not active. */
void
FboScreen::updateMouseInterval (const CompPoint &p)
{
    cursor.setPos(p.x(), p.y());

    cScreen->damageScreen ();
}

void
FboScreen::cursorHide ()
{
	if (canHideCursor && optionGetHideOriginalMouse () && optionGetTransformCursor ()) //only allow hiding if cursor is drawn otherwise
    {
    	XFixesHideCursor (screen->dpy (), screen->root ());
    }
}

void
FboScreen::cursorShow ()
{
	if (canHideCursor)
    {
    	XFixesShowCursor (screen->dpy (), screen->root ());
    }
}

void
FboScreen::cursorActivate ()
{	
    if (!fixesSupported)
    	return;

    if (!cursorInfoSelected)
    {
    	cursorInfoSelected = true;
        XFixesSelectCursorInput (screen->dpy (), screen->root (),
				 XFixesDisplayCursorNotifyMask);
        updateCursor (&cursor);
    }

	if (!pollHandle.active ())
		pollHandle.start ();

	cursorHide (); //in case transform was unchecked and hide was checked
}

void
FboScreen::cursorDeactivate ()
{
    if (!fixesSupported)
    	return;

    if (cursorInfoSelected)
    {
    	cursorInfoSelected = false;
    	XFixesSelectCursorInput (screen->dpy (), screen->root (), 0);
    }

	if (pollHandle.active ())
		pollHandle.stop ();
	
    cursor.free();

	cursorShow ();
}

void
FboScreen::optionChanged (CompOption	       *option,
			       FboOptions::Options num)
{
	switch(num)
	{
	case FboOptions::HideOriginalMouse:
		if (optionGetHideOriginalMouse () && toggle)//do not allow to hide cursor if fbo mode not active!
		{
			cursorHide();
		}
		else
		{
			cursorShow();
		}
		break;
	case FboOptions::TransformCursor:
		if (optionGetTransformCursor () && toggle) //do not allow to hide cursor if fbo mode not active!
		{
			cursorActivate ();
		}
		else
		{
			cursorDeactivate ();
		}
		break;
	default:
		break;
	}
	cScreen->damageScreen ();
}

FboScreen::FboScreen (CompScreen *screen) :
    PluginClassHandler<FboScreen,CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    grabIndex (0),
    width (0),
    height (0),
    tIndex (0),
    target (0),
    tx (0),
    ty (0),
    count (0),
    fbo (0),
    fboStatus (0),
	shaderProgram(0),
	vertexShader(0),
	fragmentShader(0),
	toggle(false),
	mToggleShader(false),
	shaderInitialized(false),
	cursorInfoSelected (false)
{

    memset (texture, 0, sizeof (GLuint) * TEXTURE_NUM);

	height = screen->height();
	width  = screen->width ();
	target = GL_TEXTURE_2D;

	if (GL::fbo)
	{
		GL::genFramebuffers (1, &fbo);
	}

	//Query X if current XFixes extension allows Hiding the cursor
    int major, minor;
    fixesSupported = XFixesQueryExtension(screen->dpy (),
			     &fixesEventBase,
			     &fixesErrorBase);

    XFixesQueryVersion (screen->dpy (), &major, &minor);

    if (major >= 4)
    	canHideCursor = true;
    else
    	canHideCursor = false;

    pollHandle.setCallback (boost::bind (
				&FboScreen::updateMouseInterval, this, _1));

    optionSetToggleFboKeyInitiate (togglefbo);
    optionSetReloadShaderKeyInitiate (reloadShaderOption);
    optionSetToggleShaderKeyInitiate (boost::bind (&FboScreen::toggleShader, this,
							_1, _2, _3));

    optionSetHideOriginalMouseNotify(boost::bind(&FboScreen::optionChanged, this, _1, _2));
	optionSetTransformCursorNotify(boost::bind(&FboScreen::optionChanged, this, _1, _2));

    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler(gScreen, false);
}

FboScreen::~FboScreen ()
{
    cursorDeactivate ();

    if (fbo)
    	GL::deleteFramebuffers (1, &fbo);

    for (unsigned int i = 0; i < TEXTURE_NUM; i++)
    {
		if (texture[i]) {
			glDeleteTextures (1, &texture[i]);
			texture[i] = 0;
		}
    }
    clearShader();
}

bool
FboPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) |
        !CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	 return false;

    return true;
}

