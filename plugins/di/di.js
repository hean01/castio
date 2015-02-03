(function() {
    var constants = {
	"front_uri": "http://www.di.fm",
	"api_uri" : "http://listen.di.fm/public3"
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
	var first = true;
	var result = [];

	while(1) {

	    var item = {};
	    item.type = plugin.item.TYPE_RADIO_STATION;
	    item.metadata = {};

	    var str = "<li data-channel-id=\"";
	    var s = doc.indexOf(str, 2);
	    if (s < 0) break;
	    var doc = doc.slice(s);

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

    function getChannels()
    {
	var res = http.get(constants.front_uri);
	if (res.status != 200) return [];

	// skip to channels json metadata within a <script>
	var s = res.body.indexOf("<!-- js application bootstrap -->", 0);
	if (s < 0) return null;
	s = res.body.indexOf("<script>", s);
	if (s < 0) return null;
	s = res.body.indexOf("  di.app.start(", s);
	s += "  di.app.start(".length;
	var data = res.body.slice(s);
	s = data.indexOf("</script>");
	data = data.slice(0, s-3);

	return JSON.parse(data);
    }

    plugin.register("/", function(offset, limit) {
	var result = [];
	var data = getChannels();
	var cnt = 0;

	service.debug("Offest "+ offset + ", Limit: " +limit);

	data.channels.forEach(function(channel) {

	    if (cnt < offset)
	    {
		cnt++;
		return;
	    }

	    if (cnt == offset + limit)
		return;

	    // lookup channel in cache
	    var data = cache.get(plugin.URI_PREFIX + "/channel/" + channel.key);
	    if (data != null)
	    {
		var item = JSON.parse(data);
		cnt++;
		result.push(item);
		return;
	    }

	    // channel not found, harvest data
	    var item = {};
	    item.metadata = {};

	    // Fetch uri for channel
	    var res = http.get(constants.api_uri + "/" + channel.key);
	    if (res.status != 200)
		return;

	    var streams = JSON.parse(res.body);

	    item.uri = streams[0];
	    item.type = plugin.item.TYPE_RADIO_STATION;
	    item.metadata.title = channel.name;
	    item.metadata.description = channel.description;

	    // use image uri for proper sized image
	    var hash = channel.asset_url.slice(
		channel.asset_url.lastIndexOf("/") + 1,
		channel.asset_url.lastIndexOf(".")
	    );
	    item.metadata.image = "http://api.audioaddict.com/v1/assets/image/" + hash + ".png?size=192";

	    cache.store(plugin.URI_PREFIX + "/channel/" + channel.key, JSON.stringify(item));

	    cnt++;
	    result.push(item);
	});
	return result;
    });

}) (this);
