(function() {

    settings.define("childprotected", "Childprotect",
		    "Prevent showing content not suitable for children", false);

    plugin.search(function(keywords) {

	service.log("Keywords: " + JSON.stringify(keywords));

    });

}) (this);
