include (FindPkgConfig)

project (compositionkit)

function (compiz_configure_file _src _dst)
    foreach (_val ${ARGN})
        set (_${_val}_sav ${${_val}})
        set (${_val} "")
	foreach (_word ${_${_val}_sav})
	    set (${_val} "${${_val}}${_word} ")
	endforeach (_word ${_${_val}_sav})
    endforeach (_val ${ARGN})

    configure_file (${_src} ${_dst} @ONLY)

    foreach (_val ${ARGN})
	set (${_val} ${_${_val}_sav})
        set (_${_val}_sav "")
    endforeach (_val ${ARGN})
endfunction ()

pkg_check_modules (COMPOSITIONKIT REQUIRED wayland-client wayland-egl wayland-server egl glesv2 pixman-1 libpng)

option (BUILD_RSVG "Use librsvg conversion" OFF)
option (BUILD_X11_COMPOSITOR "Build X11 Composition Backend" OFF)
option (BUILD_DRM_COMPOSITOR "Build DRM Composition Backend" OFF)
option (BUILD_WAYLAND_COMPOSITOR "Build Wayland Composition Backend" OFF)
option (BUILD_OPENWFD_COMPOSITOR "Build OpenWF Composition Backend" OFF)
option (BUILD_COMPOSITIONKIT_SERVICE "Build CompositionKit Service" OFF)

cmake_minimum_required(VERSION 2.8)

set (MODULEDIR ${compositionkit_BINARY_DIR})
set (DATADIR ${compositionkit_SOURCE_DIR}/data)

list (APPEND COMPOSITIONKIT_LIBRARIES m pthread dl)
set (COMPOSITIONKIT_SRCS
     compositor/compositor.c
     compositor/image-loader.c
     compositor/matrix.c
     compositor/spring.c
     compositor/surface.c
     compositor/sprite.c
     compositor/pointer.c
     compositor/output.c
     compositor/inputdevice.c
     compositor/shader.c)

if (BUILD_RSVG)

	find_program (RSVG_CONVERT_EXECUTABLE rsvg-convert)
	mark_as_advanced (FORCE RSVG_CONVERT_EXECUTABLE)

	if (NOT RSVG_CONVERT_EXECUTABLE)
	    message ("rsvg-convert not found.")
	    set (BUILD_RSVG OFF)
	endif (NOT RSVG_CONVERT_EXECUTABLE)

endif ()

if (BUILD_X11_COMPOSITOR)

	pkg_check_modules (COMPOSITIONKIT_X11 x11 x11-xcb)

	if (COMPOSITIONKIT_X11_FOUND)

		include (CheckFunctionExists)

		include_directories (${COMPOSITIONKIT_X11_INCLUDE_DIRS} ${compositionkit_BINARY_DIR}/generated/)
		link_directories (${COMPOSITIONKIT_X11_LIBRARY_DIRS})

		check_function_exists (xcb_poll_for_queued_event HAVE_XCB_POLL_FOR_QUEUED_EVENT)

		add_library (x11-backend SHARED compositor/compositor-x11.c)
		target_link_libraries (x11-backend ${COMPOSITIONKIT_X11_LIBRARIES})

		if (NOT XCB_POLL_FOR_QUEUED_EVENT)

			message ("WARNING: libxcb does not have xcb_poll_for_queued_event")

			set (HAVE_XCB_POLL_FOR_QUEUED_EVENT 0)

		endif (NOT XCB_POLL_FOR_QUEUED_EVENT)

		list (APPEND COMPOSITIONKIT_BACKENDS "X11")
		
	else (COMPOSITIONKIT_X11_FOUND)

		set (BUILD_X11_COMPOSITOR OFF)

	endif (COMPOSITIONKIT_X11_FOUND)

endif (BUILD_X11_COMPOSITOR)

if (BUILD_DRM_COMPOSITOR)

	pkg_check_modules (COMPOSITIONKIT_DRM libudev>=136 libdrm>=2.4.23)

	if (COMPOSITIONKIT_DRM_FOUND)

		include_directories (${COMPOSITIONKIT_DRM_INCLUDE_DIRS} ${compositionkit_BINARY_DIR}/generated/)
		link_directories (${COMPOSITIONKIT_DRM_LIBRARY_DIRS})
		add_library (drm-backend SHARED compositor/compositor-drm.c compositor/tty.c compositor/evdev.c)
		target_link_libraries (drm-backend ${COMPOSITIONKIT_DRM_LIBRARIES})

		list (APPEND COMPOSITIONKIT_BACKENDS "DRM")

	else (COMPOSITIONKIT_DRM_FOUND)

		set (BUILD_DRM_COMPOSITOR OFF)

	endif (COMPOSITIONKIT_DRM_FOUND)

endif (BUILD_DRM_COMPOSITOR)

if (BUILD_OPENWFD_COMPOSITOR)

	pkg_check_modules (COMPOSITIONKIT_OPENWFD openwf)

	if (COMPOSITIONKIT_OPENWFD_FOUND)

		include_directories (${COMPOSITIONKIT_OPENWFD_INCLUDE_DIRS} ${compositionkit_BINARY_DIR}/generated/)
		link_directories (${COMPOSITIONKIT_OPENWFD_LIBRARY_DIRS})
		add_library (openwfd-backend SHARED compositor/compositor-openwfd.c compositor/tty.c compositor/evdev.c)
		target_link_libraries (openwfd-backend ${COMPOSITIONKIT_OPENWFD_LIBRARIES})

		list (APPEND COMPOSITIONKIT_BACKENDS "OpenWFD")

	else (COMPOSITIONKIT_OPENWFD_FOUND)

		set (BUILD_OPENWFD_COMPOSITOR OFF)

	endif (COMPOSITIONKIT_OPENWFD_FOUND)

endif (BUILD_OPENWFD_COMPOSITOR)

if (BUILD_WAYLAND_COMPOSITOR)

	pkg_check_modules (COMPOSITIONKIT_WAYLAND wayland-client wayland-egl)

	if (COMPOSITIONKIT_WAYLAND_FOUND)

		include_directories (${COMPOSITIONKIT_WAYLAND_INCLUDE_DIRS} ${compositionkit_BINARY_DIR}/generated/)
		link_directories (${COMPOSITIONKIT_WAYLAND_LIBRARY_DIRS})
		add_library (wayland-backend SHARED compositor/compositor-wayland.c)
		target_link_libraries (wayland-backend ${COMPOSITIONKIT_WAYLAND_LIBRARIES})

		list (APPEND COMPOSITIONKIT_BACKENDS "Wayland")

	else (COMPOSITIONKIT_WAYLAND_FOUND)

		set (BUILD_WAYLAND_COMPOSITOR OFF)

	endif (COMPOSITIONKIT_WAYLAND_FOUND)

endif (BUILD_WAYLAND_COMPOSITOR)

list (LENGTH COMPOSITIONKIT_BACKENDS LENGTH_COMPOSITIONKIT_BACKENDS)

if (${LENGTH_COMPOSITIONKIT_BACKENDS} EQUAL 0)

	message (FATAL_ERROR "At least one backend must be selected")

else (${LENGTH_COMPOSITIONKIT_BACKENDS} EQUAL 0)

	foreach (COMPOSITIONKIT_BACKEND ${COMPOSITIONKIT_BACKENDS})
		message ("Using backend: " ${COMPOSITIONKIT_BACKEND})
	endforeach ()

endif (${LENGTH_COMPOSITIONKIT_BACKENDS} EQUAL 0)

compiz_configure_file (${compositionkit_SOURCE_DIR}/config.h.in ${compositionkit_BINARY_DIR}/generated/config.h)

include_directories (${COMPOSITIONKIT_INCLUDE_DIRS} ${compositionkit_BINARY_DIR}/generated/)
link_directories (${COMPOSITIONKIT_LIBRARY_DIRS})
add_executable (compositionkit ${COMPOSITIONKIT_SRCS})
target_link_libraries (compositionkit ${COMPOSITIONKIT_LIBRARIES})

if (BUILD_COMPOSITIONKIT_SERVICE)

	pkg_check_modules (COMPOSITIONKIT_SERVICE glib-2.0 gobject-2.0 gio-2.0 gthread-2.0 x11)

	if (COMPOSITIONKIT_SERVICE_FOUND)

		include_directories (${CMAKE_BINARY_DIR}/generated/ckservice
				     ${COMPOSITIONKIT_SERVICE_INCLUDE_DIRS})
		link_directories (${COMPOSITIONKIT_SERVICE_LIBRARY_DIRS})
		file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated/ckservice)
		add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/generated/ckservice/compositionkit-service.c
				    COMMAND gdbus-codegen ARGS --c-namespace=CompositionKit
							       --interface-prefix=com.canonical.CompositionKit
							       ${CMAKE_SOURCE_DIR}/ckservice/ckservice.xml
							       --generate-c-code=compositionkit-service
							       --c-generate-object-manager
							       --generate-docbook=compositionkit-service 
				    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/generated/ckservice
				    COMMENT "Generating GDBus Boilerplate")

		add_executable (ckservice
				ckservice/ckservice-main.c
				${CMAKE_BINARY_DIR}/generated/ckservice/compositionkit-service.c)

		target_link_libraries (ckservice ${COMPOSITIONKIT_SERVICE_LIBRARIES})

	else (COMPOSITIONKIT_SERVICE_FOUND)

		set (BUILD_COMPOSITIONKIT_SERVICE OFF)

	endif (COMPOSITIONKIT_SERVICE_FOUND)

endif (BUILD_COMPOSITIONKIT_SERVICE)
