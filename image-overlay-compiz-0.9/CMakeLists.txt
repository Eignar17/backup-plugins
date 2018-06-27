find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (imageoverlay PLUGINDEPS composite opengl)
