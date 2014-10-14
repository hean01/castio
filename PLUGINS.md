CAST.IO Plugins
===============

CAST.IO uses JavaScript (ECMA-262) as scripting language for
plugins. For now, CAST.IO only supports one type of plugins which is
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

The global scope of a plugin contains three objects to help the plugin
developer to implement a plugin. These helper objects are;

- **_service_** provides core functionalities such as logging, http
  client etc.

- **_settings_** provides an interface to define and use plugin specific
  settings which is stored in the service configuration file.

- **_plugin_** provides is the interface between CAST.IO and the
  provide plugin.

Here is a sample plugin script which does nothing more than gets
initialize and logs "Hello World!" using the _service_ object.


	(function() {

		service.log("Hello World!");

	}) (this)

