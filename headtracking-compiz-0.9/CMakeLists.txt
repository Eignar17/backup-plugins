find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (headtracking PLUGINDEPS composite opengl mousepoll PKGDEPS opencv)
