(function() {
    var constants = {
	'base_uri': 'http://www.svtplay.se'
    };

    settings.define("childprotected", "Childprotect",
		    "Prevent showing content not suitable for children", false);


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
	result = [];

	while(1) {

	    var item = {};
	    item.type = "video";
	    item.metadata = {};

	    var str = "<article class=\"";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    doc = doc.slice(s);

	    // get title
	    str = getValue(doc, "data-title=\"", "\"");
	    if (str == null) continue;
	    item.metadata.title = str;

	    // get station name
	    str = getValue(doc, "data-description=\"", "\"");
	    if (str)
		item.metadata.description = decodeURI(str);

	    str = getValue(doc, "data-length=\"", "\"");
	    if (str)
		item.metadata.duration = str;

	    str = getValue(doc, "data-imagename=\"", "\"");
	    if (str)
		item.metadata.image = str;

	    result.push(item);
	}
	return result;
    }

    plugin.register("/", function(path, offest, limit) {
	return [
	    {
		type: "folder",
		metadata: {
		    title: "Poplärast just nu"
		},
		uri: plugin.URI_PREFIX + "/populara"
	    },

	    {
		type: "folder",
		metadata: {
		    title: "Senaste program"
		},
		uri: plugin.URI_PREFIX + "/senaste"
	    },

	    {
		type: "folder",
		metadata: {
		    title: "Sista chansen"
		},
		uri: plugin.URI_PREFIX + "/sista-chansen"
	    },

	    {
		type: "folder",
		metadata: {
		    title: "Live"
		},
		uri: plugin.URI_PREFIX + "/live"
	    },

	];
    });

    plugin.register("/populara", function(path, offest, limit) {
	res = http.get(constants.base_uri + "/populara");
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

    plugin.register("/senaste", function(path, offest, limit) {
	res = http.get(constants.base_uri + "/senaste");
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

    plugin.register("/sista-chansen", function(path, offest, limit) {
	res = http.get(constants.base_uri + "/sista-chansen");
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

    plugin.register("/live", function(path, offest, limit) {
	res = http.get(constants.base_uri + "/live");
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });


    plugin.search(function(keywords, limit) {
	res = http.get(constants.base_uri + "/sok?q=" + keywords.join("+"));
	if (res.status != 200) return [];
	return scrape_page(res.body, limit);
    });

}) (this);
