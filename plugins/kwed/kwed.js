(function() {

    var constants = {
	'base_uri': 'http://remix.kwed.org',
	'items_per_page': 50
    };

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
	    var item_id = -1;
	    var item = {};

	    item.type = plugin.item.TYPE_MUSIC_TRACK;
	    item.metadata = {};

	    /* skip to next entry */
	    var s = doc.indexOf("<tr class=\"", 2);
	    if (s < 0) break;
	    doc = doc.slice(s);
	    cnt++;

	    if (offset >= cnt) continue;

	    // id
	    var str = getValue(doc, "?search_id=", "\"");
	    if (str == null) continue;

	    // skip to next info
	    var s = doc.indexOf("search_id=" + str, 2);
	    if (s < 0) continue;
	    doc = doc.slice(s);

	    // get uri
	    var str = getValue(doc, "href=\"download.php/", "\">");
	    if (str == null) continue;
	    item.uri = constants.base_uri + "/download.php/" + str;

	    // skip to title
	    var s = doc.indexOf("download.php/" + str, 2);
	    if (s < 0) continue;
	    doc = doc.slice(s);

	    // get title
	    var str = getValue(doc, "\">", "</a>");
	    if (str == null) continue;
	    item.metadata.title = str;

	    // get uri
	    var str = getValue(doc, "target=\"_self\">", "</a>");
	    if (str == null) continue;
	    item.metadata.artist = str;

	    result.push(item);
	    limit = limit - 1;
	}

	return result;
    };

    function get_items(uri, offset, limit) {
	result = [];
	start_page = 1 + Math.floor(offset / constants.items_per_page);
	end_page = 1 + Math.floor((offset + limit) / constants.items_per_page);
	page_offset = offset - ((start_page - 1) * constants.items_per_page);

	for (page = start_page; page <= end_page; page++) {

	    res = http.get(uri + "&page=" + page);
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

    plugin.search(function(keywords, limit) {
	res = http.get(constants.base_uri + "/index.php?search=" + keywords.join("+"));
	if (res.status != 200) return [];
	return scrape_page(res.body, 0, limit);
    });

    plugin.register("/", function(offset, limit) {
	result = [];

	result.push({
	    type: "musictrack",
	    metadata: {
		title: "Latest",
		description: "Latest submissions of remixes."
	    },
	    uri: plugin.URI_PREFIX + "/latest"
	});

	return result;
    });

    plugin.register("/latest", function(offset, limit) {
	uri = constants.base_uri + "/?view=date";
	return get_items(uri, offset, limit);
    });

}) (this);
