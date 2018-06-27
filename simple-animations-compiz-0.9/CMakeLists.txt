find_package (Compiz REQUIRED)

include (FindOpenGL)
include (CompizPlugin)

if (OPENGL_GLU_FOUND)
    compiz_plugin (animationsim PLUGINDEPS composite opengl animation LIBRARIES ${OPENGL_glu_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
endif (OPENGL_GLU_FOUND)
