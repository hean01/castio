cast.io RESTful Web API
=======================

# Changelog

This section will clearly specify the change between protocol version
for easy update of the client.


# Result codes

The service will return the following HTTP error codes for web api
requests.

	102 Processing
	200 Success
    400 Bad Request
	401 Unauthorized
    404 Not Found
	405 Method Not Allowed

- If a temporary resources such as search result is not finished,
  **102** is returned. This indicates that you should continue to
  request the temporary resource until you get a **200** return code.
  When you have received a **200** return code the temporary resource
  becomes  unavailable.

- A normal GET and PUT operation will return 200 if everything is ok.

- If a GET or PUT operation on a resource is performed with bad data
  such as URI attributes or request body, **400** is returned.

- If requests are performed without a authorization, **401** is
  returned.

- If a resource is not available, **404** is returned.

- If a resource is read only and one tries to update it, **405** is
  returned.


# Authentication

All request are done using a standard http digest authentication, a
user + password within the CAST.IO realm. The service only provides
authentication for one user.


# Definitions of data types

There are a few defined data types. This means there are object that
must include specificly named memebers to be accepted. This section
provides information for each defined object within the web API
protocol.

If and object is not valid in a PUT operation, result code 400 will be
raised.


## metadata

Metadata is a part of an _item_ and includes metadata for the
item. This can be anything from a description to genre.

| member      | type   | access      | description                    |
|-------------|--------|-------------|--------------------------------|
| title       | string | read        | name                           |
| description | string | read        | description, plot              |
| keywords    | array  | read        | keywords                       |
| image       | string | read        | image representing the item    |
|             |        |             |                                |


## item_type

A string constant which defines an item type. The _item.action_uri_ is the action
to perform on the item.

| value    | description            |
|----------|------------------------|
| "folder" | Item is a folder       |
| "stream" | Item is a media stream |
|          |                        |


## item_ref

A json object which defines an item resource uri. This object is used
in results of different _GET_ operations such as a search result.

| member       | type     | access      | description                    |
|--------------|----------|-------------|--------------------------------|
| resource_uri | string   | read        | resource URI for the item      |
|              |          |             |                                |


## item

A json object which represents an item. _item.type_ should be any of
the defined _item_type_ constants.

| member       | type      | access      | description                    |
|--------------|-----------|-------------|--------------------------------|
| type         | item_type | read        | item type                      |
| metadata     | metadata  | read        | item metadata                  |
| stream_uri   | string    | read        | an uri to access the specific  |
|              |           |             |                                |


## setting

A definition of a setting, the service itself and the available
providers will expose a list of setting objects for configuration.

| member      | type   | access      | description                    |
|-------------|--------|-------------|--------------------------------|
| id          | string | read        | internal id for setting        |
| name        | string | read        | a display name for the setting |
| description | string | read        | a description of the setting   |
| value       | any    | read, write | the specific setting value     |
|             |        |             |                                |


## provider

A json object which represents a provider.

| member      | type     | access      | description                    |
|-------------|----------|-------------|--------------------------------|
| id          | string   | read        | id of the provider             |
| metadata    | metadata | read        | item metadata                  |


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
server then modify the values and PUT the same object back, this way
errors will be minimized. However, it is possible to just PUT an
object with a single _setting_ to update the specific value.

**resource:** _"service"_ or a provider id string

**accepted verbs:** GET, PUT

**returns:** A list of _setting_ objects for specified resource.


## /providers

Retreives a list of avilable providers registered with the service.

**accepted verbs:** GET

**returns:** A list of _provider_ref_ objects.


## /providers/[resource]

Get a _provider_ object for specified resource.

**accepted_verbs:** GET

**returns:** A _provider_ object.
