(function() {

    // SoundcloudClient javascript object
    function SoundcloudClient() {
	this.constants = {
	    'api_uri': 'https://api.soundcloud.com',
	    'client_id': 'b0479634e56c0feaa933fffa85c7b33b',
	    'client_secret': '2e6dcc195af704f07daf6889a734ab96'
	};

	// private helper functions, prefixed with _
	this._paramsToQuery = function(parameters) {
	    var arr = [];

	    if (parameters == null)
		return "";

	    for (var key in parameters) {
		arr.push(key + "=" + parameters[key]);
	    }

	    if (arr.length == 0)
		return "";

	    return "?" + arr.join("&");
	}

	this._pathForUser = function(user, path) {
	    if (user == "me")
		return "/me" + path;
	    else
		return "/users/" + user + path;
	}

	this._performRequest = function(path, headers, params) {
	    params.client_id = this.constants.client_id;

	    var url = this.constants.api_uri + path
		+ this._paramsToQuery(params);

	    var response = http.get(url, headers);
	    if (response.status != 200) {
		service.warning("Failed to get resource, status "
				+ response.status);
		return null;
	    }

	    return JSON.parse(response.body);
	}

	// Get tracks from stream
	this.getStream = function(user_id, offset, limit) {
	    var params = {};
	    var headers = {};

	    params.offset = offset;
	    params.limit = limit;

	    headers.Accept = "application/json";

	    return this._performRequest(_pathForUser(user_id, "/activities/tracks/affiliated"),
					headers, params);
	}

	this.getCategories = function(offset, limit) {
	    var params = {'offset': offset, 'limit': limit};
	    var headers = {'Accept': "application/json"};

	    return this._performRequest("/app/mobileapps/suggestions/tracks/categories",
					headers, params);
	}

	this.getTracksForCategory = function(category, offset, limit) {
	    var params = {'offset': offset, 'limit': limit};
	    var headers = {'Accept': "application/json"};

	    return this._performRequest("/app/mobileapps/suggestions/tracks/categories/" + category,
					headers, params);
	}

	this.searchTracks = function(keywords, offset, limit) {
	    var params = {};
	    var headers = {'Accept': "application/json"};
	    params.offset = offset;
	    params.limit = limit;
	    params.q = keywords.join("+");

	    service.debug("Keywords " + keywords.length);

	    service.debug(JSON.stringify(params));

	    return this._performRequest("/search/sounds", headers, params);
	}
    };


    // Create a soundcloud client instance
    var sc = new SoundcloudClient();

    plugin.register("/", function(offset, limit) {
	var result = [];

	if (offset != 0)
	    return result;

	// Create categories entry
	var item = {};
	item.metadata = {};
	item.type = plugin.item.TYPE_FOLDER;
	item.uri = plugin.URI_PREFIX + "/categories";
	item.metadata.title = "Categories";
	item.metadata.description = "Browse tracks suggestions by category";

	result.push(item);

	return result;
    });

    plugin.register("/categories", function(offset, limit) {
	var result = [];
	var cnt = 0;
	var categories = sc.getCategories(offset, limit);

	categories.music.forEach(function(category) {

	    if (cnt < offset) {
		cnt++;
		return;
	    }

	    if (cnt == offset + limit)
		return;

	    var item = {};
	    item.metadata = {};

	    item.type = plugin.item.TYPE_FOLDER;
	    item.uri = plugin.URI_PREFIX + "/categories/" + category.title;
	    item.metadata.title = category.title;
	    result.push(item);
	});

	return result;
    });

    function durationToString(duration)
    {
	var s = Math.round(duration / 1000);
	var m = Math.round(s / 60);
	var h = Math.round(m / 60);
	m -= h*60;
	s -= m*60;

	var ss = (s + "0").slice(0, 2);
	var sm = m;

	var result = "";
	if (h != 0) {
	    sm = (m + "0").slice(0, 2);
	    result += h + ":";
	}
	result += sm + ":" + ss;

	return result;
    }

    plugin.register("/categories/*", function(offset, limit, category) {
	var result = [];
	var cnt = 0;
	var tracks = sc.getTracksForCategory(category, offset, limit);

	tracks.collection.forEach(function(track) {

	    if (cnt < offset) {
		cnt++;
		return;
	    }

	    if (cnt == offset + limit)
		return;

	    var item = {};
	    item.metadata = {};
	    item.uri = track.stream_url;
	    item.type = plugin.item.TYPE_MUSIC_TRACK;
	    item.metadata.title = track.title;
	    item.metadata.length = durationToString(track.duration);
	    item.metadata.image = track.artwork_url;
	    item.metadata.artist = track._embedded.user.username;
	    item.metadata.likes = track._embedded.stats.likes_count;
	    item.metadata.plays = track._embedded.stats.playback_count;

	    result.push(item);

	});

	return result;
    });

    plugin.search(function(keywords, limit) {
	var result = [];
	var tracks = sc.searchTracks(keywords, 0, limit);

	tracks.collection.forEach(function(track) {

	    var item = {};
	    item.metadata = {};

	    item.uri = track.stream_url;
	    item.type = plugin.item.TYPE_MUSIC_TRACK;
	    item.metadata.title = track.title;
	    item.metadata.description = track.description;
	    item.metadata.length = durationToString(track.duration);
	    item.metadata.image = track.artwork_url;
	    item.metadata.artist = track.user.username;
	    item.metadata.likes = track.likes_count;
	    item.metadata.plays = track.playback_count;

	    result.push(item);
	});

	return result;
    });

}) (this);
