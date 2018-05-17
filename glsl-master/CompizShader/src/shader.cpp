
#include "shader.h"
#include <GL/glext.h>
#include <iostream>
#include <fstream>

COMPIZ_PLUGIN_20090315 (shader, ShaderPluginVTable);

PFNGLCREATEPROGRAMPROC      glCreateProgram      = NULL;
PFNGLUSEPROGRAMPROC         glUseProgram         = NULL;
PFNGLCREATESHADERPROC       glCreateShader       = NULL;
PFNGLSHADERSOURCEPROC       glShaderSource       = NULL;
PFNGLCOMPILESHADERPROC      glCompileShader      = NULL;
PFNGLATTACHSHADERPROC       glAttachShader       = NULL;
PFNGLLINKPROGRAMPROC        glLinkProgram        = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC          glUniform1i          = NULL;
PFNGLGETSHADERIVPROC        glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC       glGetProgramiv = NULL;
PFNGLDELETESHADERPROC       glDeleteShader = NULL;
PFNGLDELETEPROGRAMPROC      glDeleteProgram = NULL;

bool
ShaderScreen::queryForShader()
{
    glCreateProgram         = (PFNGLCREATEPROGRAMPROC)      gScreen->getProcAddress ("glCreateProgram");
    glUseProgram            = (PFNGLUSEPROGRAMPROC)         gScreen->getProcAddress ("glUseProgram");
    glCreateShader          = (PFNGLCREATESHADERPROC)       gScreen->getProcAddress ("glCreateShader");
    glShaderSource          = (PFNGLSHADERSOURCEPROC)       gScreen->getProcAddress ("glShaderSource");
    glCompileShader         = (PFNGLCOMPILESHADERPROC)      gScreen->getProcAddress ("glCompileShader");
    glAttachShader          = (PFNGLATTACHSHADERPROC)       gScreen->getProcAddress ("glAttachShader");
    glLinkProgram           = (PFNGLLINKPROGRAMPROC)        gScreen->getProcAddress ("glLinkProgram");
    glGetUniformLocation    = (PFNGLGETUNIFORMLOCATIONPROC) gScreen->getProcAddress ("glGetUniformLocation");
    glUniform1i             = (PFNGLUNIFORM1IPROC)          gScreen->getProcAddress ("glUniform1i");
    glGetShaderiv           = (PFNGLGETSHADERIVPROC)        gScreen->getProcAddress ("glGetShaderiv");
    glGetShaderInfoLog      = (PFNGLGETSHADERINFOLOGPROC)   gScreen->getProcAddress ("glGetShaderInfoLog");
    glGetProgramInfoLog     = (PFNGLGETPROGRAMINFOLOGPROC)  gScreen->getProcAddress ("glGetProgramInfoLog");
    glGetProgramiv          = (PFNGLGETPROGRAMIVPROC)       gScreen->getProcAddress ("glGetProgramiv");
    glDeleteShader          = (PFNGLDELETESHADERPROC)       gScreen->getProcAddress ("glDeleteShader");
    glDeleteProgram         = (PFNGLDELETEPROGRAMPROC)      gScreen->getProcAddress ("glDeleteProgram");

	if (!(glUseProgram        &&
			glCreateShader     &&
			glShaderSource        &&
			glCompileShader &&
			glGetShaderiv   &&
			glGetShaderInfoLog &&
			glDeleteShader &&
			glDeleteProgram &&
			glCreateProgram &&
			glAttachShader &&
			glLinkProgram &&
			glGetProgramiv &&
			glGetProgramInfoLog &&
			glGetUniformLocation &&
			glUniform1i))
	{
		return false;
	}
	return true;
}

GLuint
ShaderScreen::loadShader(std::string& fileName, GLuint type)
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

    GLuint shader;
	shader = glCreateShader(type);
	if (shader == 0) //works with nVidia, fails with fglrx
	{
		std::cerr << "Shader is: " << shader << std::endl;
		return 0;
	}

	GLenum error = glGetError();
	while (error != GL_NO_ERROR)
	{
		std::cout << "Error: " << error << std::endl;
		if (error == GL_INVALID_ENUM)
			std::cout << "GL_INVALID_ENUM" << std::endl;
		if (error == GL_INVALID_OPERATION)
			std::cout << "GL_INVALID_OPERATION" << std::endl;
		error = glGetError();
	}
	const char* src = strData.c_str();
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

//    GLsizei length_;
//		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length_);
//		GLchar* log_ = new GLchar[length_];
//		glGetShaderInfoLog(shader,length,&length_,log);
//		std::cerr << "Shader Info : " << log_ << "\n";
	if(!compiled)
	{
		GLsizei length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		GLchar* log = new GLchar[length];
		glGetShaderInfoLog(shader,length,&length,log);
		std::cerr << "Shader Compile Error : " << log << "\n";
		delete log;
		glDeleteShader(shader);
		shader = 0;
	}
	return shader;
}

void
ShaderScreen::clearShader()
{
	if (vertexShader != 0)
		glDeleteShader(vertexShader);
	if (fragmentShader != 0)
		glDeleteShader(fragmentShader);
	if (shaderProgram != 0)
		glDeleteProgram(shaderProgram);
	shaderProgram = 0;
	vertexShader = 0;
	fragmentShader = 0;
}

void
ShaderScreen::loadProgram(std::string& vertexShaderPath, std::string& fragmentShaderPath)
{
	clearShader();

	vertexShader	= loadShader(vertexShaderPath, 		GL_VERTEX_SHADER);
	fragmentShader	= loadShader(fragmentShaderPath, 	GL_FRAGMENT_SHADER);

	if (vertexShader == 0 || fragmentShader == 0)
	{
		std::cerr << "Cannot create shader" << std::endl;
		return;
	}
	GLuint program = 0;
	program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if(!linked)
	{
		GLsizei length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char* log = new char[length];
		glGetProgramInfoLog(program, length, &length, log);
		std::cerr << "Program Link Error : " << log << "\n";
		delete log;
		glDeleteProgram(program);
		program = 0;
	}
	shaderProgram = program;
}

void
ShaderScreen::bindShader()
{
    if (shaderProgram != 0)
    {
    	glUseProgram(shaderProgram);
    }

}

void
ShaderScreen::shaderInit()
{
  	if (!queryForShader()) {
		std::cout << "ERROR: Shader not supported" << std::endl;
		return;
  	}
	char* homedir = getenv("HOME");
	if (!homedir)
	{
		std::cerr << "Error: Cannot find HOME" << std::endl;
		return;
	}
	std::string fragmentPath(homedir);
	fragmentPath += "/";
	std::string vertexPath(fragmentPath);
	vertexPath += "shader_vertex";
	fragmentPath += "shader_fragment_simple";
	loadProgram(vertexPath, fragmentPath);
	initialized = true;
}


void
ShaderScreen::unbindShader()
{
	glUseProgram(0);
}

bool
ShaderScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			      const GLMatrix		&transform,
			      const CompRegion		&region,
			      CompOutput 		*output,
			      unsigned int		mask )
{
	int rectWidth = 200;
    int rectHeight = 100;

    bool ret;
    ret = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    GLMatrix sTransform(transform);
    sTransform.toScreenSpace(output, -DEFAULT_Z_CAMERA);
    glPushMatrix;
    glLoadMatrixf(sTransform.getMatrix());

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1,0,0);

    glBegin(GL_TRIANGLES);
    glVertex2i(0,0);
    glVertex2i(0, 10);
    glVertex2i(10, 0);
    glEnd();

	glColor4usv(defaultColor);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

    if (!initialized)
    	shaderInit();
    bindShader();
    glBegin(GL_QUADS);
    glTexCoord2f(0,1);
    glVertex2i(output->centerX() - rectWidth/2.0f, output->centerY() - rectHeight/2.0f);
    glTexCoord2f(0,0);
    glVertex2i(output->centerX() - rectWidth/2.0f, output->centerY() + rectHeight/2.0f);
    glTexCoord2f(1,0);
	glVertex2i(output->centerX() + rectWidth/2.0f, output->centerY() + rectHeight/2.0f);
	glTexCoord2f(1,1);
	glVertex2i(output->centerX() + rectWidth/2.0f, output->centerY() - rectHeight/2.0f);
	glEnd();
	unbindShader();

	glPopMatrix();
    return ret;
}

void
ShaderScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
			      		 const GLMatrix		&transform,
			      		 const CompRegion		&region,
			      		 CompOutput 		*output,
			      		 unsigned int		mask )
{
    gScreen->glPaintTransformedOutput (attrib, transform, region, output, mask);
}

ShaderScreen::ShaderScreen (CompScreen *screen) :
    PluginClassHandler <ShaderScreen, CompScreen> (screen),
    screen (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    initialized(false)
{
    ScreenInterface::setHandler (screen, true);
    CompositeScreenInterface::setHandler (cScreen, true);
    GLScreenInterface::setHandler (gScreen, true);
}

ShaderScreen::~ShaderScreen ()
{
}

bool
ShaderPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	 return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}
