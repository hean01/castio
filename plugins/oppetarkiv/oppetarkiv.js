(function() {

    var constants = {
	'base_uri': 'https://origin-www.svt.se/oppet-arkiv-api',
    };

    function make_args(args) {
	var i = 1, length = 0;
	var str="?";
	for(var dummy in args) length++;
	for(var key in args) {
	    str += key + "=" + args[key];
	    if(i < length) str += "&";
	    i++;
	}
	return str;
    }

    /* query api */
    function query(uri, args) {
	var query = constants.base_uri + uri + make_args(args);
	var response = http.get(query, {
	    'User-Agent':'radio.de 1.9.1 rv:37 (iPhone; iPhone OS 5.0; de_DE)'
	});

	if (response.status != 200)
	    return undefined;

	return JSON.parse(response.body);
    }

    function entryToItem(entry) {
	var item = {
	    type: plugin.item.TYPE_VIDEO,
	    metadata: {
		title: (entry.programTitle ? entry.programTitle + " - " : "") + entry.title,
		description: entry.description,
		image: entry.thumbnailXL,
		year: entry.decade,
		duration: entry.materialLength,
	    },
	    uri: entry.videoReferences[0].url
	};

	if (entry.seasonNumber != undefined)
	{
	    item.type = plugin.item.TYPE_TVSERIE;
	    item.metadata.season = entry.seasonNumber;
	    item.metadata.episode = entry.episodeNumber;
	}

	return item;
    }
    

    /*
     * Implementation of search function
     */
    plugin.search(function(keywords, limit) {

	var result = query("/search/videos", {
	    'q': keywords.join("+"),
	    'pretty': true
	});
	// TODO: process search result into list
	return []
    });

    plugin.register('/recommended', function(offset, limit) {
	var result = [];

	var data = query('/frontpage/', {
	    'page': offset % limit,
	    'count': limit,
	    'pretty': true
	});

	if (data == undefined)
	    return result;

	data.forEach(function(list) {
	    if (list.label != "Rekommenderat")
		return;

	    list.teaserlist.forEach(function(entry) {
		var item = entryToItem(entry);
		result.push(item);
	    })
	});

	return result;
    });

    plugin.register('/humor', function(offset, limit) {
	var result = [];

	var data = query('/frontpage/', {
	    'page': offset % limit,
	    'count': limit,
	    'pretty': true
	});

	if (data == undefined)
	    return result;

	data.forEach(function(list) {
	    if (list.label != "Humor")
		return;

	    list.teaserlist.forEach(function(entry) {
		var item = entryToItem(entry);
		result.push(item);
	    })
	});

	return result;
    });

    plugin.register('/latest', function(offset, limit) {
	var result = [];

	var data = query('/latest', {
	    'page': offset % limit,
	    'count': limit,
	    'pretty': true
	});

	if (data == undefined)
	    return result;

	data.entries.forEach(function(entry) {
	    var item = entryToItem(entry);
	    result.push(item);
	});

	return result;
    });

    plugin.register('/', function(offset, limit) {
	var result = [];

	result.push({
	    type: plugin.item.TYPE_FOLDER,
	    metadata: {
		title: 'Recommended',
		description: 'Recommended',
	    },
	    uri: plugin.URI_PREFIX + '/recommended'
	});

	result.push({
	    type: plugin.item.TYPE_FOLDER,
	    metadata: {
		title: 'Humor',
		description: 'Humor',
	    },
	    uri: plugin.URI_PREFIX + '/humor'
	});

	result.push({
	    type: plugin.item.TYPE_FOLDER,
	    metadata: {
		title: 'Latest',
		description: 'Latest additions',
	    },
	    uri: plugin.URI_PREFIX + '/latest'
	});

	return result.slice(offset, offset + limit);
    });

}) (this);
