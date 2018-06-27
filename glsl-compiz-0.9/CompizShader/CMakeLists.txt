find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (shader PLUGINDEPS composite opengl LIBRARIES ${OPENGL_gl_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
