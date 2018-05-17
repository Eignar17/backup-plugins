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

all:
	@cd alsa && make all
	@if [ -e /usr/include/portaudio.h ]; then \
		cd portaudio && make all; \
	fi;
	@cd ogg && make all
	@cd sound && make all
	@cd soundeffects && make all
	@cd data && make all

clean:
	@cd alsa && make clean
	@cd portaudio && make clean
	@cd ogg && make clean
	@cd sound && make clean
	@cd soundeffects && make clean
	@cd data && make clean

install:
	@cd alsa && make install
	@cd portaudio && make install
	@cd ogg && make install
	@cd sound && make install
	@cd soundeffects && make install
	@cd data && make install

uninstall:
	@cd alsa && make uninstall
	@cd portaudio && make uninstall
	@cd ogg && make uninstall
	@cd sound && make uninstall
	@cd soundeffects && make uninstall
	@cd data && make uninstall
