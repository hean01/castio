(function() {

    var constants = {
	'base_uri': 'http://dir.xiph.org',
	'items_per_page': 20
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

    function scrape_page(doc, offset, limit)
    {
	var result = [];
	cnt = 0;

	while(1 && limit != 0)
	{
	    var item = {};

	    item.type = plugin.item.TYPE_RADIO_STATION;
	    item.metadata = {};

	    var str = "<tr class=\"row";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    doc = doc.slice(s);
	    cnt++;

	    if (offset >= cnt) continue;

	    // title
	    var str = getValue(doc, "<span class=\"name\">", "</span>");
	    if (str == null) continue;
	    s = str.indexOf("<a href=");
	    if (s > -1) str = getValue(str, "');\">", "</a>");
	    item.metadata.title = http.unescapeHTML(str);

	    // description [optional]
	    str =  getValue(doc, "<p class=\"stream-description\">","</p>");
	    if (str) item.metadata.description = http.unescapeHTML(str);

	    // currently on air
	    str = getValue(doc, "<p class=\"stream-onair\"><strong>On Air:</strong> ", "</p>");
	    if (str) item.metadata.on_air = http.unescapeHTML(str);

	    // uri
	    str = getValue(doc, "<p>[ <a href=\"", "\" ");
	    item.uri = constants.base_uri + str;

	    result.push(item);
	    limit = limit - 1;
	}

	return result;
    };

    function get_items(uri, offset, limit) {
	result = [];
	start_page = Math.floor(offset / constants.items_per_page);
	end_page = Math.floor((offset + limit) / constants.items_per_page);
	page_offset = offset - (start_page * constants.items_per_page);

	for (page = start_page; page <= end_page; page++) {

	    res = http.get(uri + "?page=" + page);
	    if (res.status != 200)
		return result;

	    if (page != start_page)
		page_offset = 0;

	    r = scrape_page(res.body, page_offset, limit - result.length);

	    Array.prototype.push.apply(result, r);

	    if (limit == result.length)
		break;
	}
	return result;
    }

    /* fill start page with genres */
    plugin.register("/", function(offset, limit) {
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

    /* add handler for genres */
    plugin.register("/*", function(offset, limit, genre) {
	uri = constants.base_uri + "/by_genre/" + genre;
	return get_items(uri, offset, limit);
    });

    plugin.search(function(keywords, limit) {
	res = http.get(constants.base_uri + "/search?search=" + keywords.join("+"));
	return scrape_page(res.body, 0, limit);
    });

}) (this);
