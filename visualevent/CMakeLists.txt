include (CompizFusion)

compiz_fusion_plugin (visualevent
    PKGDEPS xtst cairo cairo-xlib-xrender pango pangocairo
    LDFLAGSADD -lpthread)
