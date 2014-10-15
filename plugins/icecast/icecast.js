(function() {

    var constants = {
	'base_uri': 'http://dir.xiph.org'
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

    function scrape_page(result, doc, limit)
    {
	var item = {};

	item.type = "radiostation";
	item.metadata = {};

	while(1)
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

	    service.log("Station title: " + JSON.stringify(item));
	}
    };

    plugin.search(function(result, keywords, limit) {
	kw = keywords.join("+");
	res = http.get(constants.base_uri + "?search=" + kw);
	scrape_page(result, res.body, limit);
    });

}) (this);
