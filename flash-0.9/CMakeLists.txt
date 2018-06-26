find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (flash PLUGINDEPS composite opengl)
