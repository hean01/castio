(function() {

    var constants = {
	'base_uri': 'https://api.soundcloud.com',
	'client_id': 'hijuflqxoOqzLdtr6W4NA'
    };

    function urlencode(parameters) {
	var arr = [];
	for (var key in parameters) {
	    arr.push(key + "=" + parameters[key]);
	}
	return arr.join("&");
    }

    function build_query_url(resource, parameters) {
	url = constants.base_uri + "/" + resource + ".json" + "?" + urlencode(parameters);
	return url;
    }

    function parse_tracks(buffer)
    {
	var result = [];

	try {
	    tracks = eval(res.body);

	    tracks.forEach(function(track) {
		var item = {};
		item.metadata = {};
		item.metadata.title = track.title;
		item.metadata.description = track.description;

		if (track.artwork_url)
		    item.metadata.image = track.artwork_url;
		else if (track.waveform_url)
		    item.metadata.image = track.waveform_url;
		else
		    item.metadata.image = track.user.avatar_url;

		item.uri = track.stream_url + "&consumer_key=" + constants.client_id;

		result.push(item);
	    });
	} catch(e) {
	    service.log("Failed to parse result: " + e.message);
	    return [];
	}


	return result;
    }

    plugin.register("/", function(offset, limit) {
	var query =
	    build_query_url("tracks", {
		"consumer_key": constants.client_id,
		"filter": "streamable",
		"offset": offset,
		"limit": limit,
		"order": "hotness"
	    });

	res = http.get(query);
	if (res.status != 200) return [];
	return parse_tracks(res.body);
    });

    plugin.search(function(keywords, limit) {
	var query =
	    build_query_url("tracks", {
		"consumer_key": constants.client_id,
		"filter": "streamable",
		"offset": 0,
		"limit": limit,
		"q": keywords.join("+")
	    });

	res = http.get(query);
	if (res.status != 200) return [];
	return parse_tracks(res.body);
    });

}) (this);
