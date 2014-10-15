(function() {

    plugin.search(function(results, keywords, limit) {

	service.log("Keywords: " + JSON.stringify(keywords));

    });

}) (this);
