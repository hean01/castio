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

- 102 Processing
- 200 Success
- 400 Bad Request
- 401 Unauthorized
- 404 Not Found
- 405 Method Not Allowed

- If a temporary resources such as search result is not finished,
  **102** is returned. This indicates that you should continue to
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


# action

An action of an item. Actions are defined and differs from provider to
providers and can be anything such as; _like_, _buy_ etc.

The action uri is a combination of the item uri and the _action.id_
like `/providers/headweb/item2935292/buy`. Then create GET request for
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

A string constant which defines an item type. The _item.action_uri_ is the action
to perform on the item.

| value    | description            |
|----------|------------------------|
| "folder" | Item is a folder       |
| "stream" | Item is a media stream |


## item_ref

A json object which defines an item resource uri. This object is used
in results of different _GET_ operations such as a search result.

| member       | type     | access      | description                    |
|--------------|----------|-------------|--------------------------------|
| resource_uri | string   | read        | resource URI for the item      |


## item

A json object which represents an item. _item.type_ should be any of
the defined _item_type_ constants.

| member       | type      | access      | description                         |
|--------------|-----------|-------------|-------------------------------------|
| type         | item_type | read        | item type                           |
| item_uri     | string    | read        | an uri to access the specific item  |
| metadata     | metadata  | read        | item metadata                       |
| actions      | list      | read        | list of _action_ for the item       |


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


## searchResult

A json object with each providers search result as list of _item_'s.

    {
		"movies": [
			{<<_item_>>}
			.
			.
			.
		],

		"icecast": [
			{<<_item_>>}
			.
			.
			.
		]
    }

# Resource URI's

This section will provide a list of available resource URI and how to
use them.


## /

This resource returns a list of items to be shown as a front view of
the server. It will include folders such as _Latest_, _Top rated_,
_Favorites_, _Watch later_ etc.

**accepted verbs:** GET

**returns:** A list of _item_ref_ objects.


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
			"metadata": <<{_metadata_}>>
		},

		{
			"type": "folder",
			"item_uri: "/New",
			"metadata": <<{_metadata_}>>
		}
	]

You would travers the browsable tree folder `/Top Rated` by joining
the base path with the item_uri as; `/providers/youtube/Top%20Rated`
and then request a new item list from that uri.

**resource:** provider id string

**accepted_verbs:** GET

**returns:** A list of _item_ objects.

## /search

Performs a search of items in all available providers.

To narrow down the search you can use the attributes _providers_ in
combination with _media_types_.

See the following example usage of the search functionality and how to
handle the status code on the client side:

1. Create a GET request for `/search?query="A movie"`

2. Server responds with a status code **302** and set's a "Location:"
   header were the temporary result is available to be fetched.

3. GET the temporary search result location URI until you get a HTTP
   status code **200**. This URI will return a _searchResult_ object
   of current search result.

   - You will get a http status **102** and partial results of the
     search for interactive update of the query.

   - Do not hammer the service and use a sleep of 2-5 seconds between
	 the requests.

4. When status code **200** is recived the search is finished, content
   in response is the full result and the temporary search result
   location URI is removed.

**Attributes:**

| attribute | description                                               |
|-----------|-----------------------------------------------------------|
| query     | A comma separated string with keywords                    |
| providers | A comma separated string with provider id's               |
| type      | A comma separated string with type constants              |

**accepted_verbs:** GET

**returns:** A location for search result. The temporary search result
location returns a _searchResult_ object.
