CAST.IO Plugins
===============

CAST.IO uses JavaScript (ECMA-262) as scripting language for
plugins. Currently, CAST.IO only supports one type of plugins which
are providers.

Plugins are bundled into a zip file with a _manifest_ which describes
the plugin. The _manifest_ is a json object and a sample is provided
below.

	{
		"id":"sample",
		"name": "Sample Provider",
		"description": "A sample provider",
		"copyright": "2014 - CAST.IO Development Team",
		"version": [0, 0, 1],
		"url": "http://local.domain/sample",
		"icon": "sample.png",
		"plugin": "sample.js"
	}

The global scope of a script contains objects to help the plugin
developer to implement a plugin.

Here is an example plugin script which does nothing more than fetches
a web page and logs the heades as json string.

	(function() {

	    result = http.get("http://www.google.com");

	    service.info("Headers: ", JSON.stringify(result.headers));

	}) (this)


A provider plugin provides items to the service either by specified
paths or by a search. A provider needs to register a serach function
and at least one path "/".


## service

This object provides access to core functionalities.

The following properties and methods can be used with service object

| Property / Method       | Description                          |
|-------------------------|--------------------------------------|
| service.info(string)    | Adds an info entry to service log    |
| service.warning(string) | Adds a warning  entry to service log |

**Example of usage:**

	service.info("Hello World!");


## plugin

This object provides the actual plugin api for a provider. A provider
should register a search function and a path function for path `/`.

| Property / Method               | Description                           |
|---------------------------------|---------------------------------------|
| plugin.URI_PREFIX               | The prefix for internal plugin uris   |
| plugin.register(path, function) | Registers a function with a path      |
| plugin.search(function)         | Registers a function with search      |


The prototype for register function is `function(offset, limit, [arg])
{}`. The _offset_ and _limit_ are used for pagination and _offset_
specifies a offset to request items from and were limit is how many
items the plugin needs to request.

The path specified in register call supports simple case of
wildcard. A wildcard can _ONLY_ exists at the end of a path. As an
example of usage, suppose you register a handler for path
`"/genres/*"`, this handler will be used for both `/genres/80s` and
`genres/Trance`. The arg argument to handler function will be string
`80s` or `Trance` for the example described.

The prototype for search function is `function(keywords, limit)
{}`. The _keywords_ argument is a list of keywords to search on and
_limit_ is the amount of items that is requested.

**Example of usage:**

	(function() {

	    plugin.register("/", function(offset, limit) {
			return [
				{
					type: plugin.item.TYPE_FOLDER,
					metadata: {
						name: "Latest",
							description: "Latest additions",
	                },
					uri: plugin.URI_PREFIX + "/latest"
				}
			];
		});

		plugin.register("/latest", function(offset, limit) {
			return [
				{
					type: plugin.item.TYPE_RADIO_STATION,
					metadata: {
						name: "Endless drone",
							description: "FM3 Buddag Machine simulation example"
	                },
					uri: "http://dir.xiph.org/listen/10799/listen.m3u"
				}
			];
		});

        plugin.register("/genre/*", function(offset, limit, genre) {
		    service.info("Items for genre: " + genre);
		});

	}) (this)


## plugin.item

This object provides constants for item types used for creating item
in response to a query.

| Property / Method       | Description             |
|-------------------------|-------------------------|
| item.TYPE_FOLDER        | Item is a folder        |
| item.TYPE_RADIO_STATION | Item is a radio station |
| item.TYPE_MOVIE         | Item is a movie         |
| item.TYPE_VIDEO         | Item is a video clip    |
| item.TYPE_TVSERIE       | Item is a tv serie      |
| item.TYPE_MUSIC_TRACK   | Item is a music track   |

**Example of usage:**

    item.type = plugin.item.TYPE_MUSIC_STATION;


## settings

Providers which needs to handle settings should use this object, this
object provides access to define a setting and its default value and
get / set it.

Supported data type of setting values can be any of: list, string,
number or boolean

The following properties and methods can be used on settings object.

| Property / Method                             | Description                                     |
|-----------------------------------------------|-------------------------------------------------|
| settings.define(id, name, description, value) | Defines a plugin settings and its default value |
| settings.get(id)                              | Returns current setting value for id            |


**Example of usage:**

	settings.define("option", "Sample option",
	                "A sample option to show how this works", false);

	if (settings.get("option"))
		service.info("Option value is: false");
	else
		service.info("Option value is: true");


## http

Most providers needs to get it's data from somewere and therefor
CAST.IO provides a HTTP request object to be used to fetch data from
services.

All content fetched through this object are cached accordingly to
standards.

Methods throws exception upon failures.

The following properties and methods can be used on http object and
it's result.

| Property / Method             | Description                                                  |
|-------------------------------|--------------------------------------------------------------|
| http.get(uri, headers)        | HTTP Get request of the current uri, returns a result object |
| http.port(uri, headers, body) | HTTP Post request with body, returns a result object         |
| http.unescapeHTML(buffer)     | Unescapes HTML entities in buffer and returns the result     |
| result.status                 | HTTP status code of the request                              |
| result.headers                | Object with headers from response                            |
| result.body                   | Response body                                                |


**Example of usage:**

	uri = "http://www.google.se";
	headers = {"Accept-Content": "gzip"};
	try {
		res = http.get(uri, headers);
		if (res.status != 200) {
			service.warning("Failed to get uri '" + uri + "',status " + res.status);
			return;
	    }
		service.info("Content: " + res.body);
	} catch(e) {
		service.info("Failed to fetch data: " + e.message);
	}
