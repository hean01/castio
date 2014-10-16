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
    plugin.search(function(keywords, limit) {
	return ["test1", "test2"]
    });

    plugin.register("/items", function(offset, limit) {
	return [[true, 232.1, ["string", false]], ["item 1", "item 2"]];
    });

}) (this);
