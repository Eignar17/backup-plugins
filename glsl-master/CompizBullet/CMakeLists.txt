find_package (Compiz REQUIRED)
include (FindOpenGL)
include (CompizPlugin)

if (OPENGL_GLU_FOUND)
	compiz_plugin (bullet PLUGINDEPS composite opengl mousepoll LIBRARIES BulletDynamics BulletCollision LinearMath ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
endif (OPENGL_GLU_FOUND)