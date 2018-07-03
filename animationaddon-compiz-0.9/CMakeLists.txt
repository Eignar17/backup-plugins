find_package (Compiz REQUIRED)
include (CompizPlugin)
include (FindOpenGL)

if (OPENGL_GLU_FOUND)
    compiz_plugin (animationaddon PLUGINDEPS animation LIBRARIES ${OPENGL_glu_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
endif (OPENGL_GLU_FOUND)
