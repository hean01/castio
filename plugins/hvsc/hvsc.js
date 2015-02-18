(function() {

    var constants = {
	'base_uri': 'http://www.hvsc.c64.org'
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

	    item.type = plugin.item.TYPE_MUSIC_TRACK;
	    item.metadata = {};

	    var str = "<tr onclick=\"sidAction(";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    doc = doc.slice(s);

	    // sid id
	    var str = getValue(doc, "(", ")");
	    if (str == null) break;
	    item.uri = constants.base_uri + "/siddownload.htm?id=" + str;

	    // title
	    var str = getValue(doc, "<td>", "</td>");
	    if (str == null) continue;
	    item.metadata.title = str;
	    s = doc.indexOf(str);
	    doc = doc.slice(s);

	    // artist
	    var str = getValue(doc, "<td>", "</td>");
	    if (str == null) continue;
	    if (str != "&lt;?&gt;") item.metadata.artist = str;
	    s = doc.indexOf(str);
	    doc = doc.slice(s);

	    // copyright
	    var str = getValue(doc, "<td>", "</td>");
	    if (str == null) continue;
	    if (str != "&lt;?&gt;") item.metadata.copyright = str;

	    result.push(item);
	    limit = limit - 1;
	}

	return result;
    };

    plugin.search(function(keywords, limit) {
	var res = http.get(constants.base_uri + "/dosearch.htm?searchQuery=" + keywords.join("+"));
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

}) (this);
