CAST.IO
=======

_CAST.IO_ is a service with transcoding possibility of video and audio
from providers. A provider is a javascript plugin which hosts video /
audio streams to be pushed through the transcoder to be casted to your
TV.

What is _CAST.IO_?

 - a headless service with a [RESTful][] Web API provided for client
   interaction.

 - developed using C and libevent for asynchronous usage.

 - service which hosts javascripts plugins named providers which
   sources media to be casted to your chromecast connected displays.


[RESTful]: http://en.wikipedia.org/w/index.php?title=Representational_state_transfer "REpresental State Transfer"


## Building CAST.IO

To build CAST.IO you need to fulfill a few dependencies, a c
compilator, `cmake` and the development packages for the following
libraries (names can differ between distributions);

- json-glib
- libarchive
- libsoup

If you are building from git repository you need to initialize a third
party library MuJS which is available as a git submodule. This is done
by issuing command `git submodule init` in your working copy. Then
follow the steps below to build and install CAST.IO.

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/opt/castio ..
	make install


## Provider plugins

CAST.IO provider plugins are written using JavaScript (ecma-262) were
each provider initializes and implements parts of the provider
[Plugin API][]. This is done using global accesor objects for
different purposes as you can read more about at [Plugin API][]
documentation.

[Plugin API]: https://github.com/hean01/castio/blob/master/PLUGINS.md "CAST.IO Plugin API"

## Web API

The [Web API][] is the only external interface to the _CAST.IO_
service. Used to configure the service and available providers,
search, browse and launch content for playback to a receiver.

[web api]: https://github.com/hean01/castio/blob/master/API.md "CAST.IO Rest API"


## Android Client

The android client is the only _CAST.IO_ client this project will
release. It will use the _CAST.IO_ web API to server as controller for
playback of content from providers. It should be a simple task to
create a new native client for another platform using the open and
welldocumented _CAST.IO_ web API.


## Version 1.0

Here follows a list of goals for stable release version 1.0

 - Android client application
 - Stable version of web api
 - Stable version javascript plugin api


## Version 2.0

Here follows a list of goals for stable release version 2.0

 - Revised stable version of web api
 - Revised stable version of javascript plugin api
 - Initial transcoding functionality

