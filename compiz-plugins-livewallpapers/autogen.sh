#!/bin/bash

autoreconf -f -i -Wno-portability \
&& libtoolize \
&& autoheader \
&& aclocal \
&& autoconf \
&& touch AUTHORS NEWS README ChangeLog \
&& automake --add-missing -a -Wno-portability

