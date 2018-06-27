find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (hoverfx PLUGINDEPS composite opengl)