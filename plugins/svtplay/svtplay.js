(function() {

    settings.define("childprotected", "Childprotect",
		    "Prevent showing content not suitable for children", false);

    plugin.search(function(result, keywords, limit) {

	service.log("Keywords: " + JSON.stringify(keywords));

    });

}) (this);
