(function() {

    var constants = {
	'base_uri': 'http://dir.xiph.org'
    };

    var genres = [
	"80s",
	"Alternative",
	"Ambient",
	"Breaks",
	"Chillout",
	"Dance",
	"Dubstep",
	"Drumnbass",
	"Electronic",
	"Funk",
	"Gothic",
	"Harcore",
	"Hardstyle",
	"Hardtrance",
	"Hip Hop",
	"House",
	"Jazz",
	"Jungle",
	"Lounge",
	"Metal",
	"Minimal",
	"Mixed",
	"Pop",
	"Progressive",
	"Punk",
	"Radio",
	"Rock",
	"Techno",
	"Top40",
	"Trance",
	"Tribal",
	"Various"
    ];

    function getValue(content, start, end) {
	var s = content.indexOf(start);
	if (s < 0) return null;
	s = s + start.length;
	if (end != null) {
	    var e = content.indexOf(end, s);
	    if (e < 0) return null;
	    return content.slice(s, e);
	}
	return content.slice(s);
    };

    function scrape_page(doc, limit)
    {
	var result = [];

	while(1 && limit != 0)
	{
	    var item = {};

	    item.type = "radiostation";
	    item.metadata = {};

	    var str = "<tr class=\"row";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    doc = doc.slice(s);

	    // title
	    var str = getValue(doc, "<span class=\"name\">", "</span>");
	    if (str == null) continue;
	    s = str.indexOf("<a href=");
	    if (s > -1) str = getValue(str, "');\">", "</a>");
	    item.metadata.title = str;

	    // description [optional]
	    str =  getValue(doc, "<p class=\"stream-description\">","</p>");
	    if (str) item.metadata.description = str;

	    // uri
	    str = getValue(doc, "<p>[ <a href=\"", "\" ");
	    item.uri = constants.base_uri + str;

	    result.push(item);
	    limit = limit - 1;
	}

	return result;
    };

    /* setup start page */
    plugin.register("/", function(path, offset, limit) {
	var result = [];

	genres.forEach(function(entry) {
	    var item = {};
	    item.type = "folder";
	    item.metadata = {};
	    item.uri = plugin.URI_PREFIX + "/" + entry;
	    item.metadata.title = entry;
	    result.push(item);
	});

	return result;
    });

    /* add handlers for each genre */
    genres.forEach(function(entry) {
	/* register each entry point */
	plugin.register("/" + entry, function(path, offset, limit) {

	    res = http.get(constants.base_uri + "/by_genre/" + entry);
	    if (res.status != 200) return [];

	    return scrape_page(res.body, limit);
	});
    });


    plugin.search(function(keywords, limit) {
	res = http.get(constants.base_uri + "/search?search=" + keywords.join("+"));
	return scrape_page(res.body, limit);
    });

}) (this);
