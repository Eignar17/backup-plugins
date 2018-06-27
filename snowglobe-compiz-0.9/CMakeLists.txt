find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (snowglobe PLUGINDEPS cube opengl composite)
