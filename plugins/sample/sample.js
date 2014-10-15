(function() {

    var result = http.get("http://www.google.se");
    service.log("Status " + result.status);
    service.log("Content " + JSON.stringify(result.headers));

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
    plugin.search(function(keywords) {

	service.log("Keywords: " + JSON.stringify(keywords));

    });

}) (this);
