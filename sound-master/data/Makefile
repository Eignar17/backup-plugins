# Copyright (C) 2007 Andrew Riedi <andrewriedi@gmail.com>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

SOUNDS_DIR = ~/.compiz/sounds/default

ECHO = `which echo`

all:
	@if \
	! [ -s new.ogg ] || \
	! [ -s moveleft.ogg ] || \
	! [ -s moveright.ogg ] || \
	! [ -s moveup.ogg ] || \
	! [ -s movedown.ogg ]; \
	then \
		$(ECHO) -e -n "\033[0;32mPlease replace the empty Ogg Vorbis files with ones of your own.\n\033[0m"; \
		exit 0; \
	fi;

clean:

install: all
	@$(ECHO) -e -n "\033[0;34m";
	@mkdir -p $(SOUNDS_DIR)
	@cp -v *.ogg $(SOUNDS_DIR)
	@$(ECHO) -e -n "\033[0m";

uninstall:
	@$(ECHO) -e -n "\033[0;34m";
	@rm -rvf $(SOUNDS_DIR)/*.ogg
	@$(ECHO) -e -n "\033[0m";
