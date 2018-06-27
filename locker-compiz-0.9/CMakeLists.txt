find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (locker PLUGINDEPS composite opengl PKGDEPS xscrnsaver)
