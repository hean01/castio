(function() {

    var constants = {
	'base_uri': 'http://dir.xiph.org',
	'items_per_page': 20
    };

    var genres = [];
    genres.push("80s");
    genres.push("Alternative");
    genres.push("Ambient");
    genres.push("Breaks");
    genres.push("Chillout");
    genres.push("Dance");
    genres.push("Dubstep");
    genres.push("Drumnbass");
    genres.push("Electronic");
    genres.push("Funk");
    genres.push("Gothic");
    genres.push("Harcore");
    genres.push("Hardstyle");
    genres.push("Hardtrance");
    genres.push("Hip Hop");
    genres.push("House");
    genres.push("Jazz");
    genres.push("Jungle");
    genres.push("Lounge");
    genres.push("Metal");
    genres.push("Minimal");
    genres.push("Mixed");
    genres.push("Pop");
    genres.push("Progressive");
    genres.push("Punk");
    genres.push("Radio");
    genres.push("Rock");
    genres.push("Techno");
    genres.push("Top40");
    genres.push("Trance");
    genres.push("Tribal");
    genres.push("Various");


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
	var cnt = 0;

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

	    // listeners
	    str = getValue(doc, "<span class=\"listeners\">[", "&nbsp;listeners]</span>");
	    if (str) item.metadata.listeners = parseInt(str);

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
	var result = [];
	var start_page = Math.floor(offset / constants.items_per_page);
	var end_page = Math.floor((offset + limit) / constants.items_per_page);
	var page_offset = offset - (start_page * constants.items_per_page);

	for (var page = start_page; page <= end_page; page++) {

	    var res = http.get(uri + "?page=" + page);
	    if (res.status != 200)
		return result;

	    if (page != start_page)
		page_offset = 0;

	    var r = scrape_page(res.body, page_offset, limit - result.length);

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

	return result.slice(offset, offset + limit);
    });

    /* add handler for genres */
    plugin.register("/*", function(offset, limit, genre) {
	var uri = constants.base_uri + "/by_genre/" + genre;
	return get_items(uri, offset, limit);
    });

    plugin.search(function(keywords, limit) {
	var res = http.get(constants.base_uri + "/search?search=" + keywords.join("+"));
	return scrape_page(res.body, 0, limit);
    });

}) (this);
