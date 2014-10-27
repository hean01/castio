(function() {

    var constants = {
	'base_uri': 'http://rad.io',
	'items_per_page': 50
    };

    function make_args(args) {
	var i=1, length = 0;
	var str="?";
	for(var dummy in args) length++;

	for(var key in args) {
	    str += key + "=" + args[key];
	    if(i < length) str += "&";
	    i++;
	}
	return str;
    }

    function get_data(path, args) {
	query = constants.base_uri + "/info" + path + make_args(args);

	res = http.get(query, {
	    'User-Agent':'radio.de 1.9.1 rv:37 (iPhone; iPhone OS 5.0; de_DE)'});

	if (res.status != 200)
	    return undefined;

	return res.body;
    }

    function parse_stations(data) {
	var result = [];

	if (data == undefined)
	    return result;

	list = JSON.parse(data);
	list.forEach(function(station) {
	    //service.info("Name: " + JSON.stringify(station));
	    var item = {
		type: plugin.item.TYPE_RADIO_STATION,
		metadata: {
		    title: station.name,
		    image: station.pictureBaseURL + station.picture1Name
		},
		uri: station.streamUrl
	    };


	    if (station.currentTrack != "")
		item.metadata.on_air =  station.currentTrack;

	    // lookup station info
	    res = get_data("/broadcast/getbroadcastembedded", {
		broadcast: station.id
	    });

	    if (res == undefined)
		return;

	    // parse station info
	    try {
		var bce = JSON.parse(res);

		item.metadata.description = bce.description;
		item.uri = bce.streamURL;

		result.push(item);

	    } catch(e) {
		service.warning("Failed to parse station info: " + e.message);
		service.info("Object: " + res);
	    }
	});

	return result;
    }

    plugin.search(function(keywords, limit) {
	var result = [];
	data = get_data("/index/searchembeddedbroadcast", {
	    'q': keywords.join("+"),
	    'start': 0,
	    'rows': limit
	});

	return parse_stations(data);
    });

    plugin.register("/", function(offset, limit) {
	var result = [];

	result.push({
	    type: plugin.item.TYPE_FOLDER,
	    metadata: {
		title: "Nearest",
		description: "Stations near me."
	    },
	    uri: plugin.URI_PREFIX + "/nearest"
	});

	return result;
    });

    plugin.register("/nearest", function(offset, limit) {
	data = get_data("/menu/broadcastsofcategory", {
	    category: "_country",
	    value: "Sweden",
	    start: offset,
	    rows: limit
	});
	return parse_stations(data);
    });

}) (this);
