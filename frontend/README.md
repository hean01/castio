# CAST.io Frontend

This is a minimal web frontend to the _cast.io_ service.


## Development webserver

Install lighttpd and create the following configuration and then start
the webserver specifying the configuration file.

	server.modules = (
		"mod_proxy", "mod_rewrite"
	)
	server.port = 8000
	server.document-root = "/path/to/castio/frontend"

	index-file.names = ( "index.html" )


	$HTTP["url"] =~ "^/api/v1" {
		proxy.header = ("map-urlpath" => ( "/api/v1" => "" ))
		proxy.server = ( "" => ( ( "host" => "127.0.0.1", "port" => "1457" ) ) )
	}


	mimetype.assign = (
		".html" => "text/html",
		".js" => "application/javascript",
	)
