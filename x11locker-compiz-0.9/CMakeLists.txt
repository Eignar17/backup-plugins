find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (xlocker PLUGINDEPS composite opengl locker)
