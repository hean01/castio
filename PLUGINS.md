CAST.IO Plugins
===============

CAST.IO uses JavaScript (ECMA-263) as a script language for
plugins. For now CAST.IO only supports one type of plugins which is
providers.

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


# Provider plugin

The global scope of a provider plugin contains three objects to help
the plugin developer to implement a provider plugin. These helper
objects is named as following sections.

Here is a sample plugin script which does nothing more than gets
initialize and logs "Hello World!" using the _service_ object.


	(function() {

		service.log("Hello World!");

	}) (this)

