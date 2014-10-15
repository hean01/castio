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
	var item = {};
	var result = [];

	item.type = "radiostation";
	item.metadata = {};

	while(1 && limit != 0)
	{
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

	    result.push(item);
	    limit = limit - 1;
	}

	return result;
    };

    /* setup start page */
    plugin.register("/", function(path, offset, limit) {
	var res = [];
	var item = {};

	item.type = "folder";
	item.metadata = {};

	genres.forEach(function(entry) {
	    item.uri = "/genre/" + entry;
	    item.metadata.title = entry;
	    res.push(item);
	});

	return res;
    });

    /* add handlers for each genre */
    genres.forEach(function(entry) {
	/* register each entry point */
	plugin.register("/genre/" + entry, function(path, offset, limit) {

	    res = http.get(constants.base_uri + "/by_genre/" + entry);
	    if (res.status != 200) return [];

	    return scrape_page(res.body, limit);
	});
    });


    plugin.search(function(result, keywords, limit) {
	kw = keywords.join("+");
	res = http.get(constants.base_uri + "?search=" + kw);
	scrape_page(res.body, limit);
    });

}) (this);
