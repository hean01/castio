(function() {

    var constants = {
	'base_uri': 'http://remix.kwed.org'
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

    function scrape_page(doc, limit)
    {
	var result = [];

	while(1 && limit != 0)
	{
	    var item_id = -1;
	    var item = {};

	    item.type = "musictrack";
	    item.metadata = {};

	    /* skip to next entry */
	    var s = doc.indexOf("<tr class=\"", 2);
	    if (s < 0) break;
	    doc = doc.slice(s);

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

    plugin.search(function(keywords, limit) {
	res = http.get(constants.base_uri + "/index.php?search=" + keywords.join("+"));
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

    plugin.register("/", function(offset, limit) {
	result = [];

	result.push({
	    type: "musictrack",
	    metadata: {
		title: "Latest",
		description: "Latest submissions of remixes."
	    },
	    uri: plugin.URL_PREFIX + "/latest"
	});

	return result;
    });

    plugin.register("/latest", function(offest, limit) {
	res = http.get(constants.base_uri + "/");
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

}) (this);
