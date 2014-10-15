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

	    service.log("Headers: ", JSON.stringify(result.headers));

	}) (this)


A provider plugin provides items to the service either by specified
paths or by a search. A provider needs to register a serach function
and at least one path "/".


## service

This object provides access to core functionalities.

The following properties and methods can be used with service object

| Property / Method   | Description                 |
|---------------------|-----------------------------|
| service.log(string) | Adds a entry to service log |

**Example of usage:**

	service.log("Hello World!");


## plugin

This object provides the actual plugin api for a provider. A provider
should register a search function and a path function for path `/`.

| Property / Method   | Description                                       |
|---------------------|---------------------------------------------------|
| plugin.register(path, function) | Registers a function with a path |
| plugin.search(function)         | Registers a function with search |

The prototype for register function is `function(results, offset,
limit) {}`. The _offset_ and _limit_ are used for pagination and
_offset_ specifies a offset to request items from and were limit is
how many items the plugin needs to request.

The prototype for search function is `function(results,
keywords) {}`. The _keywords_ argument is a list of keywords to search
the service for.

**Example of usage:**

	(function() {

	    plugin.register("/", function(result, offset, limit) {
		    result.append(JSON.stringify({
				type: "folder",
				metadata: {
					name: "Latest",
					description: "Latest additions",
				},
				uri: "/latest"
			}));
		});

		plugin.register("/latest", function(result, offset, limit) {
			result.append(JSON.stringify({
				type: "radiostation",
				metadata: {
					name: "Endless drone",
					description: "FM3 Buddag Machine simulation example"
				},
				uri: "http://dir.xiph.org/listen/10799/listen.m3u"
			}));
		});

	}) (this)


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
		service.log("Option value is: false");
	else
		service.log("Option value is: true");


## http

Most providers needs to get it's data from somewere and therefor
CAST.IO provides a HTTP request object to be used to fetch data from
services.

All content fetched through this object are cached accordingly to
standards.

The following properties and methods can be used on http object and
it's result.

| Property / Method | Description                                                  |
|-------------------|--------------------------------------------------------------|
| http.get(uri)     | HTTP Get request of the current uri, returns a result object |
|                   |                                                              |
| result.status     | HTTP status code of the request                              |
| result.headers    | Object with headers from response                            |
| result.body       | Response body                                                |


**Example of usage:**

	uri = "http://www.google.se"
    res = http.get(uri);
	if (res.status != 200) {
		service.log("Failed to get uri '" + uri + "',status " + res.status);
		return;
	}

	service.log("Content: " + res.body);
