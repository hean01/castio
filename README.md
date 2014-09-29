cast.io
=======

_cast.io_ is a service with transcoding possibility of video and audio from providers. A provider is a javascript plugin which hosts video / audio streams to be pushed through the transcoder to be casted to your TV.

What is _cast.io_?

 - a headless service with and RESTless webapi provided for client interaction.
 - developed using C and libevent for a asynchronous service.
 - service which hosts javascripts plugins named providers which sources
   media to be casted to your chromecast connected displays.


## Provider plugins

A provider plugin is written using javascript and implements the provider plugin API. The provider plugin API serves three functions, configuration, search and browse.

Configuration of a provider plugin are often specific to the service but the most commonly shared properties are login credentials.

Search API is used by the global search function, this provides the functionality to search all your provider plugins for content. The length of search result a provider plugin should return is a global _cast.io_ service configuration.

The browse function provides an interface to present folders with items for example categories, year's etc. anything that the service provides. Each item has properties of metadata and a launch uri which is the uri to the actual stream which is fed to the receiver for playback.


## Web API

The web api is used to configure the _cast.io_ service and the providers, search, browse and launch content for playback to a receiver.


## Android Client

The android client is the only _cast.io_ client this project will release. It will use the _cast.io_ web API to server as controller for playback of content from providers. It should be a simple task to create a new native client for another platform using the open and welldocumented _cast.io_ web API.


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

