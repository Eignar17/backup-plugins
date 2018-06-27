find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (drunken PLUGINDEPS composite opengl)
