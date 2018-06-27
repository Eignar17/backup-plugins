#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <fstream>
#include <X11/cursorfont.h>
#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>

#include "fbo_options.h"

#define STEP_SIZE_X             4   //Number of quads in x  output->width() mod STEP_SIZE_X should be 0!!!
#define STEP_SIZE_Y             512//540 //Number of quads in y, output->height() mod STEP_SIZE_Y should be 0!!!

//typedef void ( *GLUseProgramProc) (GLuint program);
//typedef GLuint ( *GLCreateShaderProc) (GLuint type);
//typedef void ( *GLShaderSourceProc) (GLuint shader, GLsizei count, const GLchar** strings, const GLint* lengths);
//typedef void ( *GLCompileShaderProc) (GLuint shader);
//typedef void ( *GLGetShaderIvProc) (GLuint shader, GLenum pname, GLint* param);
//typedef void ( *GLGetShaderInfoLogProc) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
//typedef void ( *GLDeleteShaderProc) (GLuint shader);
//typedef void ( *GLDeleteProgramProc) (GLuint program);
//typedef GLuint ( *GLCreateProgramProc) (void);
//typedef void (*GLAttachShaderProc) (GLuint program, GLuint shader);
//typedef void (*GLLinkProgramProc) (GLuint program);
//typedef void (*GLGetProgramIvProc) (GLuint program, GLenum pname, GLint* param);
//typedef void (*GLGetProgramInfoLogProc) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
//typedef GLint (*GLGetUniformLocationProc) (GLuint program, const GLchar* name);
//typedef void (*GLUniform1iProc) (GLint location, GLint v0);
//
//extern GLUseProgramProc m_glUseProgram;
//extern GLCreateShaderProc m_glCreateShader;
//extern GLShaderSourceProc m_glShaderSource;
//extern GLCompileShaderProc m_glCompileShader;
//extern GLGetShaderIvProc m_glGetShaderiv;
//extern GLGetShaderInfoLogProc m_glGetShaderInfoLog;
//extern GLDeleteShaderProc m_glDeleteShader;
//extern GLDeleteProgramProc m_glDeleteProgram;
//extern GLCreateProgramProc m_m_glCreateProgram;
//extern GLAttachShaderProc m_glAttachShader;
//extern GLLinkProgramProc m_glLinkProgram;
//extern GLGetProgramIvProc m_glGetProgramiv;
//extern GLGetProgramInfoLogProc m_glGetProgramInfoLog;
//extern GLGetUniformLocationProc m_glGetUniformLocation;
//extern GLUniform1iProc m_glUniform1i;
extern PFNGLCREATEPROGRAMPROC      m_glCreateProgram;
extern PFNGLUSEPROGRAMPROC         m_glUseProgram;
extern PFNGLCREATESHADERPROC       m_glCreateShader;
extern PFNGLSHADERSOURCEPROC       m_glShaderSource;
extern PFNGLCOMPILESHADERPROC      m_glCompileShader;
extern PFNGLATTACHSHADERPROC       m_glAttachShader;
extern PFNGLLINKPROGRAMPROC        m_glLinkProgram;
extern PFNGLGETUNIFORMLOCATIONPROC m_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC          m_glUniform1i;
extern PFNGLGETSHADERIVPROC        m_glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC   m_glGetShaderInfoLog;
extern PFNGLGETPROGRAMINFOLOGPROC  m_glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC       m_glGetProgramiv;
extern PFNGLDELETESHADERPROC       m_glDeleteShader;
extern PFNGLDELETEPROGRAMPROC      m_glDeleteProgram;

#define TEXTURE_NUM 3 //color, distortion, blend

class FboScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PluginClassHandler<FboScreen,CompScreen>,
    public FboOptions
{
    public:

    CompositeScreen *cScreen;
	GLScreen        *gScreen;

	FboScreen (CompScreen *screen);
	~FboScreen ();

	//Screen Interface
	void handleEvent(XEvent* event);

	void paint(CompOutput::ptrList &outputs, unsigned int mask);
	void donePaint ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix &, const CompRegion &,
		       CompOutput *, unsigned int);
	void glPaintTransformedOutput (const GLScreenPaintAttrib &,
		       		  const GLMatrix &, const CompRegion &,
		       		  CompOutput *, unsigned int);

	void allocTexture (int index, char* data=NULL);
	void loadMaps();
	bool fboPrologue ();
	void fboEpilogue ();
 
	bool toggleFBO();
	bool toggleShader();
	bool queryForShader();

	void clearShader();
	GLuint loadShader(std::string& fileName, GLuint type);
	void reloadShader();
	void loadProgram(std::string& vertexShaderPath, std::string& fragmentShaderPath);
	void bindShader();
	void unbindShader();

	bool
	toggleShader (CompAction *action,
			 CompAction::State  state,
			 CompOption::Vector &options);

    //Cursor related code, partly taken from ezoom plugin
	public:
    	class Cursor
    	{
    	    private:
			bool set;
    		GLuint     texture;
    		CompScreen *screen;
    		int        mXHot;
    		int        mYHot;
    		int        mWidth;
    		int        mHeight;
    		CompPoint pos;
			

    	    public:
				Cursor ();
				void
				setPos(int x, int y);
				void
				updateTextureData(unsigned char* data, int width, int height, int x, int y, int xHot, int yHot, CompScreen* screen);
				void
				draw();
				void
				free();
    	};
    void
    cursorActivate();
    void
    cursorDeactivate();
	void
	cursorHide();
	void
	cursorShow();
    void
    updateCursor (Cursor * cursor);
    void
    updateMouseInterval (const CompPoint &p);
    void
    optionChanged (CompOption	       *option,
    			       FboOptions::Options num);

private:
	CompScreen::GrabHandle grabIndex;

	int width, height;

	GLuint program;
	GLuint texture[TEXTURE_NUM];

	int     tIndex;
	unsigned int mCurrentTextureIndex;
	GLenum  target;
	GLfloat tx, ty;

	int count;

	GLuint fbo;
	GLint  fboStatus;
	GLuint shaderProgram;
	GLuint vertexShader;
	GLuint fragmentShader;

	bool toggle, mToggleShader;
	bool shaderInitialized;
	float rotate;
	int counter;
    int distWidth, distHeight;

	MousePoller		 pollHandle;

	bool fixesSupported;
	int fixesEventBase;
	int fixesErrorBase;
	bool canHideCursor;
	bool cursorInfoSelected;
	Cursor cursor;
};

class FboPluginVTable :
    public CompPlugin::VTableForScreen<FboScreen>
{
    public:

	bool init ();
};
