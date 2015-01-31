(function() {
    var constants = {
	'base_uri': 'http://api.welovepublicservice.se/v1'
    };

    settings.define("childprotected", "Childprotect",
		    "Prevent showing content not suitable for children", false);


    function getCategories(offset, limit) {
	var result = [];
	var response = http.get(constants.base_uri
			   + "/category/?format=json"
			   + "&offset=" + offset
			   + "&limit=" + limit);

	if (response.status != 200)
	    return result;

	try {
	    var categories = JSON.parse(response.body);

	    categories.objects.forEach(function(category) {
		var item = {};
		item.type = plugin.item.TYPE_FOLDER;
		item.uri = plugin.URI_PREFIX + "/category/" + category.id;
		item.metadata = {};
		item.metadata.title = category.title;
		item.metadata.image = category.thumbnail_url;

		result.push(item);
	    });

	} catch (e) {
	    service.warning("Failed to parse categories: "  + e.message);
	}


	return result;
    }

    function entityToID(show) {
	// Return 1234 of string "/v1/show/1234/"
	var parts = show.split("\/");
	return parts[parts.length-2];
    }

    function getShowData(show_id) {

	var response = http.get(constants.base_uri
				+ "/show/" + show_id + "/"
				+ "?format=json");

	if (response.status != 200)
	{
	    service.warning("Failed to get info for show " + show_id + ".");
	    return null;
	}

	return response.body;
    }

    function getShows(category_id, offset, limit) {
	var result = [];
	var response = http.get(constants.base_uri
				+ "/category/" + category_id + "/?format=json"
				+ "&offset=" + offset
				+ "&limit=" + limit);

	if (response.status != 200)
	    return result;

	try {
	    var category = JSON.parse(response.body);
	    var cnt = 0;
	    for(var i=offset; cnt < limit; i++)
	    {
		if (i >= category.shows.length)
		    break;

		var item = {};
		item.metadata = {};
		item.type = plugin.item.TYPE_FOLDER;

		var show = category.shows[i];
		var show_id = entityToID(show);

		// Try get show item from cache, if not available
		// get one with getShowInfo()
		var data = cache.get(plugin.URI_PREFIX + "/show/" + show_id);
		if (!data)
		{
		    var response2 = http.get(constants.base_uri
					     + "/show/" + show_id + "/"
					     + "?format=json");

		    if (response2.status != 200)
		    {
			service.warning("Failed to get info for show " + show_id + ".");
			continue;
		    }
		    data = response2.body;
		    cache.store(plugin.URI_PREFIX + "/show/" + show_id, data);
		}

		// Create item
		try {
		    var show = JSON.parse(data);

		    item.uri = plugin.URI_PREFIX + "/show/" + show_id;
		    item.metadata.title = show.title;
		    item.metadata.image = show.thumbnail_url;

		} catch (e) {
		    service.warning("Failed to parse show: " + e.message);
		    continue;
		}

		result.push(item);
		cnt++;
	    }

	} catch (e) {
	    service.warning("Failed to parse shows: " + e.message);
	}

	return result;
    }

    function getEpisodes(show_id, offset, limit) {
	var result = [];
	var response = http.get(constants.base_uri
				+ "/show/" + show_id + "/?format=json"
				+ "&offset=" + offset
				+ "&limit=" + limit);

	if (response.status != 200)
	    return result;

	try {
	    var show = JSON.parse(response.body);
	    var cnt = 0;
	    for(var i=offset; cnt < limit; i++)
	    {
		if (i >= show.episodes.length)
		    break;

		var item = {};
		item.metadata = {};
		item.type = plugin.item.TYPE_VIDEO;

		var episode = show.episodes[i];
		var episode_id = entityToID(episode);

		// Try get episode item from cache
		var data = cache.get(plugin.URI_PREFIX + "/episode/" + episode_id);
		if (!data)
		{
		    var response2 = http.get(constants.base_uri
					     + "/episode/" + episode_id + "/"
					     + "?format=json");

		    if (response2.status != 200)
		    {
			service.warning("Failed to get info for episode " + show_id + ".");
			continue;
		    }
		    data = response2.body;
		    cache.store(plugin.URI_PREFIX + "/episode/" + show_id, data);
		}

		// Create item
		try {
		    var episode = JSON.parse(data);

		    // todo: get correct video uri
		    item.uri = plugin.URI_PREFIX + "/episode/" + show_id;
		    item.metadata.title = episode.title_slug;
		    item.metadata.length = episode.length;
		    item.metadata.image = episode.thumbnail_url;
		    item.metadata.air_date = episode.date_broadcasted;

		} catch (e) {
		    service.warning("Failed to parse show: " + e.message);
		    continue;
		}

		result.push(item);
		cnt++;
	    }

	} catch (e) {
	    service.warning("Failed to parse shows: " + e.message);
	}

	return result;
    }

    plugin.register("/", function(offset, limit) {
	return getCategories(offset, limit);
    });

    plugin.register("/category/*", function(offset, limit, category_id) {
	return getShows(category_id, offset, limit);
    });

    plugin.register("/show/*", function(offset, limit, show_id) {
	return getEpisodes(show_id, offset, limit);
    });

    plugin.search(function(keywords, limit) {
	return search(keywords, limit);
    });

}) (this);
