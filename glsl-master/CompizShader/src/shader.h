/**
 * author: Alexander Lang
 */
#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include "shader_options.h"

class ShaderScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PluginClassHandler <ShaderScreen, CompScreen>,
    public ShaderOptions
{
    public:

    ShaderScreen (CompScreen *s);
	~ShaderScreen ();

	CompScreen      *screen;
	CompositeScreen *cScreen;
    GLScreen        *gScreen;

    bool initialized;
    GLuint shaderProgram;
    GLuint vertexShader;
    GLuint fragmentShader;

    bool
    queryForShader();
    void
    bindShader();
    void
    unbindShader();
    void
    loadProgram(std::string& vertexShaderPath, std::string& fragmentShaderPath);
    void
    clearShader();
    GLuint
    loadShader(std::string& fileName, GLuint type);
    void
    shaderInit();

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix &, const CompRegion &,
		       CompOutput *, unsigned int);

	void
	glPaintTransformedOutput (const GLScreenPaintAttrib &,
		       		  const GLMatrix &, const CompRegion &,
		       		  CompOutput *, unsigned int);

};

class ShaderPluginVTable :
    public CompPlugin::VTableForScreen<ShaderScreen>
{
    public:

	bool init ();
};
