(function() {
    var constants = {
	'base_uri': 'http://api.shoutcast.com',
	'dev_id' : "sh1t7hyn3Kh0jhlV"
    };


    function getGenres(parent_id, offset, limit) {
	var result = [];

	// build request uri
	var req = constants.base_uri;
	if (parent_id == 0)
	    req += "/genre/primary";
	else
	    req += "/genre/secondary";

	req += "?f=json"
	    + "&k=" + constants.dev_id
	    + "&limit=" + offset + "," + limit;

	if (parent_id != 0)
	    req += "&parentid=" + parent_id;

	// carry out get request
	var response = http.get(req);
	if (response.status != 200)
	    return result;

	try {
	    var cnt = 0;
	    var genres = JSON.parse(response.body);

	    genres.response.data.genrelist.genre.forEach(function(genre) {

		if (cnt < offset)
		{
		    cnt++;
		    return;
		}

		if (cnt == offset + limit)
		{
		    return;
		}

		var item = {};
		item.type = plugin.item.TYPE_FOLDER;

		if (parent_id == 0)
		    item.uri = plugin.URI_PREFIX + "/genre/" + genre.id;
		else
		    item.uri = plugin.URI_PREFIX + "/stations/" + genre.id;

		item.metadata = {};
		item.metadata.title = genre.name;

		result.push(item);
	    });

	} catch (e) {
	    service.warning("Failed to parse genres: "  + e.message);
	}

	return result;
    }

    function getStations(genre_id, offset, limit) {
	var result = [];

	var response = http.get(constants.base_uri
				+ "/station/advancedsearch"
				+ "?f=json"
				+ "&k=" + constants.dev_id
				+ "&genre_id=" + genre_id
				+ "&limit=" + offset + "," + limit);

	if (response.status != 200)
	    return result;

	try {
	    var stations = JSON.parse(response.body);

	    var tunein = stations.response.data.stationlist.tunein;

	    stations.response.data.stationlist.station.forEach(function(station) {
		var item = {};
		item.type = plugin.item.TYPE_RADIO_STATION;
		item.uri = "http://yp.shoutcast.com" + tunein.base + "?id=" + station.id
		item.metadata = {};
		item.metadata.image = station.logo;
		item.metadata.title = station.name;
		item.metadata.listeners = station.lc;
		item.metadata.bitrate = station.br;
		item.metadata.on_air = station.ct;

		result.push(item);
	    });

	} catch(e) {
	    service.warning("Failed to parse stations: "  + e.message);
	}

	return result;
    }

    plugin.register("/", function(offset, limit) {
	return getGenres(0, offset, limit);
    });

    plugin.register("/genre/*", function(offset, limit, id) {
	return getGenres(id, offset, limit);
    });

    plugin.register("/stations/*", function(offset, limit, id) {
	return getStations(id, offset, limit);
    });


}) (this);
