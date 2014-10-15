(function() {

    /*
     * Define plugin settings
     */
    settings.define("sample1", "Sample setting #1",
		    "A sample setting used in the sample plugin", 1234);
    settings.define("sample2", "Sample setting #2",
		    "A sample setting used in the sample plugin", "value");

    /*
     * Implementation of search function
     */
    plugin.search(function(result, keywords, limit) {

	service.log("Keywords: " + JSON.stringify(keywords));

    });

    plugin.register("/items", function(offset, limit) {
	return ["item 1", "item 2"];
    });

}) (this);
