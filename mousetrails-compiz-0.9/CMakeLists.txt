find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (mousetrails PLUGINDEPS composite opengl mousepoll)
