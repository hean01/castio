(function() {
    var constants = {
	'base_uri': 'http://www.di.fm'
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


    function scrape_page(doc, limit) {
	first = true;
	result = [];

	while(1) {

	    var item = {};
	    item.type = "radiostation";
	    item.metadata = {};

	    var str = "<li data-channel-id=\"";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    doc = doc.slice(s);

	    /* skip first occurence */
	    if (first) {
		first = false;
		continue;
	    }

	    // get key and id
	    str = getValue(doc, "data-channel-key=\"", "\"");
	    if (str == null) continue;
	    item.uri = "http://listen.di.fm/public3/"+str+".pls";

	    // get station name
	    str = getValue(doc, "<span>", "</span>");
	    if (str == null) continue;
	    item.metadata.title = str;

	    result.push(item);
	}
	return result;
    }

    plugin.register("/", function(path, offest, limit) {
	res = http.get(constants.base_uri);
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

}) (this);
