(function() {
    settings.define("test", "test", "test settings.", true); 
    plugin.search(function(keywords) {
	service.log("Keywords: " + JSON.stringify(keywords));
    });
}) (this);
