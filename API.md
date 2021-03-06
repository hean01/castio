CAST.IO Rest API
================

# Changelog

This section will clearly specify the change between protocol version
for easy update of the client.


# Protocol

_CAST.IO_ web api tries to be as RESTful as it can. This section will
describe the constraints as simple as possible;

- There are only two verbs used, GET and PUT. This is because we try
  to constraint with a RESTfull api and we can only fetch a resource
  or update it.

- All request and responses between server and client should be using
  mime-type "application/json" and charset UTF-8 or a status code of
  **400** is returned.

- All requests are authenticated using standar HTTP digest
  authentication in the realm _CAST.IO_. If not authorized using the
  scheme a status code of **401** is retured.

- If a request method on a resource is not allowed, status code
  **405** is returned.


# Status Codes

The service will use a subset of all HTTP status codes to indicate
success or errors in the web api. Here follows a list of used status
codes for the service and when they are used;

- 200 Success
- 206 Partial Content
- 400 Bad Request
- 401 Unauthorized
- 404 Not Found
- 405 Method Not Allowed

- If a temporary resources such as search result is not finished,
  **206** is returned. This indicates that you should continue to
  request the temporary resource until you get a **200** return code.
  When you have received a **200** return code the temporary resource
  becomes  unavailable.

- A normal GET and PUT operation will return 200 if everything is ok.

- If a GET or PUT operation on a resource is performed with bad data
  such as attributes or malformed request body, **400** is returned.

- If requests are performed without a authorization, **401** is
  returned.

- If a resource is not available, **404** is returned.

- If a resource is read only and client tries to update it, **405** is
  returned.


# Definitions of data types

There are a few defined data types. This means there are object that
must include specificly named memebers to be accepted. This section
provides information for each defined object within the web API
protocol.

If and object is not valid in a PUT operation, HTTP status code
**400** will be returned.


## action

An action of an item. Actions are defined and differs from provider to
providers and can be anything such as; _like_, _buy_ etc.

The action uri is a combination of the item uri and the _action.id_
like `/providers/headweb/item/2935292/buy`. Then create GET request for
this action uri to instruct the provider to perform the specific
action.

| member      | type   | access      | description                            |
|-------------|--------|-------------|----------------------------------------|
| id          | string | read        | id of action                           |
| name        | string | read        | action name, keep it short like "buy"  |
| description | string | read        | description of the action              |


## metadata

Metadata is a part of an _item_ and includes metadata for the
item. This can be anything from a description to genre.

| member      | type   | access      | description                    |
|-------------|--------|-------------|--------------------------------|
| title       | string | read        | name                           |
| description | string | read        | description, plot              |
| keywords    | array  | read        | keywords                       |
| image       | string | read        | image representing the item    |


## item_type

A string constant which defines an item type. The _item.item_ref_ is
the action to perform on the item.

| value          | description              |
|----------------|--------------------------|
| "folder"       | Item is a folder         |
| "radiostation" | Item is a radio stations |
| "movie"        | Item is a movie          |
| "video"        | Item is a video          |
| "tvserie"      | Item is a tv serie       |
| "musictrack"   | Item is a music track    |


## item_ref

A json string with the _item_ resource uri. This string is used
in results of different _GET_ operations such as a search result.


## item

A json object which represents an item. _item.type_ should be any of
the defined _item_type_ constants.

| member       | type      | access      | description                          |
|--------------|-----------|-------------|--------------------------------------|
| type         | item_type | read        | item type                            |
| uri          | string    | read        | an uri to access the specific item   |
| metadata     | metadata  | read        | item metadata                        |
| actions      | list      | read        | list of _action_ for the item        |

An _uri_ have different meaning dependent of item _type_.


## setting

A definition of a setting, the service itself and the available
providers will expose a list of setting objects for configuration.

| member      | type   | access      | description                    |
|-------------|--------|-------------|--------------------------------|
| id          | string | read        | internal id for setting        |
| name        | string | read        | a display name for the setting |
| description | string | read        | a description of the setting   |
| value       | any    | read, write | the specific setting value     |


## provider

A json object which represents a provider.

| member      | type     | access      | description                    |
|-------------|----------|-------------|--------------------------------|
| id          | string   | read        | id of the provider             |
| name        | string   | read        | provider name                  |
| description | string   | read        | a description of the provider  |
| copyright   | string   | read        | copyright string               |
| version     | string   | read        | provider version               |
| homepage    | string   | read        | homepage uri                   |
| icon        | string   | read        | uri for the provider icon      |


## search_result

A json object with each providers search result as list of _item_ref_'s.

See the following example of what a result would look like;

    {
		"movies": [
			"/provider/movies/item/23011289"
			"/provider/movies/item/82727611"
		],

		"icecast": [
			"/provider/icecast/stream/1981287"
		]
    }

## log_level

A string constant which defines the log severity level for a _log_entry_.

| value       | description              |
|-------------|--------------------------|
| "undefined" | |
| "error"     | |
| "critical"  | |
| "warning"   | |
| "message"   | |
| "info"      | |
| "debug"     | |


## log_entry

A log entry for the CAST.IO service.

| member    | type        | access | description                 |
|-----------|-------------|--------|-----------------------------|
| timestamp | string      | read   | unix timestamp sec.usec     |
| domain    | string      | read   | log entry domain            |
| level     | _log_level_ | read   | severity of the log message |
| message   | string      | read   | the log message             |


# Resource URI's

This section will provide specification for each resource uri
available and how to operate with them.


## /settings/[resource]

This resource provides a interface to settings for resources such as
the service it self and available providers registered with the
service.

While updating settings you should first get the settings object from
server then modify the values and PUT the same object back. Doing it
this way errors will be minimized.

The web api does not support creating resources on the server, this
means that we can simplify the api and use an object with stripped
down _setting_ objects containing just the value to be updated:

	{
		"a" : { "value": "Value A" },
		"b" : { "value": "Value B" }
	}

**resource:** _"service"_ or a provider id string

**accepted verbs:** GET, PUT

**returns:** A list of _setting_ objects for specified resource.


## /providers

Retreives a list of available providers registered with the service.

**accepted verbs:** GET

**returns:** A list of _provider_ objects.


## /providers/[resource]

The base path for items of a specific provider which returns a list of
_item_'s for the provider. This list provides items to build up a
browsable tree of the provider plugin.


As an example, suppose that a provider with id `youtube` returns the
following list of items at its base path:

    [
		{
			"type": "folder",
			"item_uri: "/Top Rated",
			"metadata": <<_metadata_>>,
			"actions": [<<_action_>>]
		},

		{
			"type": "folder",
			"item_uri: "/New",
			"metadata": <<_metadata_>>
			"actions": [<<_action_>>]
		}
	]

You would travers the browsable tree folder `/Top Rated` by joining
the base path with the item_uri as; `/providers/youtube/Top%20Rated`
and then request a new item list from that uri.

**resource:** provider id string

**Attributes:**

| attribute | default | description                                      |
|-----------|---------|--------------------------------------------------|
| offset    |       0 | Offset used for iteration over parts of a result |
| limit     |      10 | Limit result set to specific count               |

The use of the attributes are optional and if not specified default
values will be used.

**accepted_verbs:** GET

**returns:** A list of _item_ objects.

## /search

Performs a search of items, use query attributes _providers_ and
_type_ to narrow down the search.

A search is an asynchronous operation and when intiated the request
will be redirected usin **302** to a temporary location where the
client can gather the partial results of an ongoing search.

The temporary location for the result can be fetched on timer basis
and while the search continues, status code **206** will be returned.
When the search is finished **200** will be returned and the temporary
resource URI will be unavailable.

See the following example usage of the search functionality and how to
handle the status code on the client side:

1. Create a GET request for `/search?keywords=the+movie`

2. Server responds with a status code **302** and set's a "Location:"
   header with the temporary resource URI which serves the result for
   the initiated search.

3. GET the temporary search result location URI until you get status
   code **200**. This URI will return a _search_result_ object of
   current search result.

   - You will get a http status **206** and partial results of the
     search for interactive update of the query.

   - Do not hammer the service and use a sleep of 2-5 seconds between
	 the requests.

4. When status code **200** is recived the search is finished, content
   in response is the full result and the temporary search result
   location URI is removed.

**Attributes:**

| attribute | description                                                  |
|-----------|--------------------------------------------------------------|
| keywords  | A list of keywords separated using _+_ sign                  |
| providers | A list of provider id's to search separated using _+_ sign   |
| type      | A list of _type_ constants to search separated using _+_sign |

Attributes _providers_ and _type_ are optional.

**accepted_verbs:** GET

**returns:** The temporary search result as a _search_result_ object.

# /backlog

Retreives a list of last _N_ _log_entry_ objects from the service backlog.

**accepted_verbs:** GET

**returns:** A json array with _log_entry_ objects.

# /cache

Retreives a resource through the internal service cache.

The service will get the resource request from it's internal blob
cache. If the requested resource is not found in cache it will
download and add it for future requests.

There are also a special case were urls of provider icons are handle
as well. These special urls uses scheme named as provider id, here
follows an example of a provider icon uri; _di://di.png_.

**Attributes:**

| attribute | description                                                  |
|-----------|--------------------------------------------------------------|
| uri       | An escaped url of the resource to retreive                   |

**accepted_verbs:** GET

**returns:** The requested resource
