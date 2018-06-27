find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (twist PLUGINDEPS composite opengl mousepoll)
