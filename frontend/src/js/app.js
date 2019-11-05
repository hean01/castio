import { render, linkEvent, Component } from 'inferno'; 
import { BrowserRouter, Route, Link } from 'inferno-router';
import * as md5 from 'md5';

if(typeof(String.prototype.strip) === "undefined")
{
    String.prototype.strip = function(ch)
    {
	switch (ch) {
	    case ' ':
	    return String(this).replace(/^\s+|\s+$/g, '');
	    case '"':
	    return String(this).replace(/^\"+|\"+$/g, '');
	}
    };
}

class DigestFetch {
    constructor(username, password, url_rewrite_func) {
	this.username = username;
	this.password = password;
	this.url_rewrite_func = url_rewrite_func
	this.have_challenge = false
	this.cnonce_counter = 0
    }

    _name_value_string_to_object(string) {
	let nv_pairs = string.split(',')
	nv_pairs = nv_pairs.map((item) => {
	    let nv_pair = item.split('=');
	    let pair={}
	    let name = nv_pair[0].strip(' ')
	    let value = nv_pair[1].strip('"')
	    pair[name] = value;
	    return pair;
	})

	return nv_pairs.reduce((result, item) => {
	    let res = {}
	    Object.assign(res, result)
	    Object.assign(res, item)
	    return res;
	})
    }

    authenticate(url) {
	return new Promise((resolve, reject) => {

	    // If we area already authenticated, just resolve
	    if (this.have_challenge == true) {
		return resolve()
	    }

	    // Perform a digest authenticate handshake
	    console.log('Perform digest challenge')
	    fetch(url, {
		credentials: 'omit',
	    })
		.then((response) => {

		    if (response.status != 401) {
			resolve()
		    }

		    let challenge = response.headers.get('www-authenticate')
		    if (challenge.slice(0,6).toLowerCase() != 'digest') {
			reject('unexpected www-authenticate: ' + auth_header)
		    }

		    let digest_challenge = challenge.slice(6)
		    let auth = this._name_value_string_to_object(digest_challenge)

		    if (auth.qop != 'auth') {
			reject('expected qop "auth" got ' + auth.qop)
		    }

		    let A1 = this.username + ":" + auth.realm + ':' + this.password;
		    this.HA1 = md5.default(A1)
		    this.auth = auth
		    this.cnonce = 'deadbeef'
		    this.have_challenge = true;

		    return resolve()

		}).catch((err) => {
		    return reject(err)
		})
	});
    }

    _authorization(url, method) {
	let authorization = 'Digest'
	if (method == undefined)
	    method = 'GET'

	url = this.url_rewrite_func(url)

	this.cnonce_counter++;
	let cnonce_counter = (''+this.cnonce_counter).padStart(8, '0')

	let A2 = method.toUpperCase() + ':' + url
	let HA2 = md5.default(A2)
	let resp = this.HA1 + ':' + this.auth.nonce + ':' + cnonce_counter + ':' + this.cnonce + ':' + this.auth.qop + ':' + HA2
	let response = md5.default(resp)

	authorization += ' username="' + this.username + '"'
	authorization += ', realm="' + this.auth.realm + '"'
	authorization += ', nonce="' + this.auth.nonce + '"'
	authorization += ', uri="' + url + '"'
	authorization += ', qop=' + this.auth.qop
	authorization += ', nc=' + cnonce_counter
	authorization += ', cnonce="' + this.cnonce + '"'
	authorization += ', response="' + response + '"'

	return authorization;
    }

    fetch(url, options) {
	return this.authenticate(url)
	    .then(() => {
		let internal_options = {
		    credentials: 'omit',
		}
		Object.assign(internal_options, options)

		if (internal_options.headers == undefined)
		    internal_options.headers = {}

		Object.assign(internal_options.headers, {
		    'Authorization': this._authorization(url, options.method)
		})

		return fetch(url, internal_options);
	    }).catch((err) => {
		console.log('Failed digest authentication: ' + err)
		throw err
	    })
    }
}

class Api extends DigestFetch {

    constructor(username, password) {
	super(username, password, (url) => {
	    return url.slice(this.API_BASE_URI.length)
	})
	this.API_BASE_URI = '/api/v1'
    }

    get(uri) {
	var url = this.API_BASE_URI + uri;
	return this.fetch(url, {
	    mode: 'same-origin'
	})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Failed to get entries: ', err);
		throw err
	    });
    }

    _response_to_data_uri(response) {

	return response.arrayBuffer()
	    .then((buffer) => {
		let datauri = 'data:'
		datauri += response.headers.get('content-type')
		datauri += ';base64,'
		datauri += btoa(String.fromCharCode(...new Uint8Array(buffer)));
		console.log(datauri)
		return datauri;
	    });

    }

    cache(resource) {
	var url = this.API_BASE_URI + '/cache?resource=' + resource;
	return this.fetch(url, {
	    mode: 'same-origin'
	}).then((response) => {

	    console.log(response)

	    if (response.status != 200) {
		throw 'expected status 200, got ' + response.status
	    }

	    return this._response_to_data_uri(response)

	}).catch((err) => {
	    console.log('get resource ' + resource + ' from cache failed with: ', err);
	    throw err
	});
    }

    search(providers, types, keywords) {
	var url = this.API_BASE_URI + '/search?'
        url += 'keywords=' + keywords.join('+')

	if (providers) {
	    url += '&providers=' + providers.join('+')
	}

	if (types) {
	    url += '&type=' + types.join('+')
	}

	return this.fetch(url, {
	    mode: 'same-origin',
	}) .then((response) => {

	    let location = response.url;
	    let more_data = (response.status == 206);

	    console.log(response)

	    return response.json()
		.then((entries) => {
		    return {
			location: location,
			have_more_data: more_data,
			entries: entries,
		    };
		})
	})
    }
}

const api = new Api('admin', 'password')

class ProviderItem extends Component {

    constructor(props) {
	super(props);
	this.state = {
	    icon: ''
	}
    }

    componentDidMount(dom) {
	api.cache(this.props.provider.icon).then((datauri) => {
	    this.setState({icon: datauri});
	})
    }

    render(props, state) {

	return (
	    <li class='media' style='margin: 1rem;'>
		<img class='media-image' src={ state.icon }></img>
		<div class="media-body">
		<h5 class='mr-3'>{ props.provider.name }</h5>
		<p>{ props.provider.description } </p>
		<p>
		{ props.provider.version &&
		  <NameValue name='Version' value={ props.provider.version[0] + '.' + props.provider.version[1] + '.' + props.provider.version[2]} />
		}
		{ props.provider.copyright &&
		  <NameValue name='Copyright' value={ props.provider.copyright } />
		}
	    { props.provider.homepage &&
	      <NameValue name='Homepage' value={ props.provider.homepage } />
	    }
		</p>
		<Link class='card-link' to={ '/providers/' + props.provider.id }>Browse</Link>
		<Link class='card-link' to={ '/settings/' + props.provider.id }>Settings</Link>
		</div>
		</li>
	)
    }
}

class ProviderCollection extends Component {
    constructor(props) {
	super(props)
	this.state = {
	    providers: [],
	}
    }

    componentDidMount() {
	api.get('/providers')
	    .then((providers) => {
		this.setState({
		    providers: providers
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
		<ul class='list-unstyled'>
		{
		    state.providers.map(provider =>
					<ProviderItem provider={provider} />
				       )
		}
		</ul>
	)
    }
}

class NameValue extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {

	return (
		<div>
		<b>{ props.name + ': '}</b>
		<span>{ props.value }</span>
		</div>
	)
    }
}

class Metadata extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return (
		<ul class='list-unstiled'>
		{ props.metadata.artist &&
		  <NameValue name='Artist' value={props.metadata.artist} />
		}
	    { props.metadata.on_air &&
	      <NameValue name='On-air' value={props.metadata.on_air} />
	    }
	    </ul>
	)
    }
}

class MediaPlayer extends Component {
    constructor(props) {
	super(props);
    }

    player(entry) {
	switch(entry.type) {
	case 'radiostation':
	case 'musictrack':
	    return (
		    <audio src={ entry.uri } controls/>
	    )

	case 'movie':
	case 'video':
	case 'tvserie':
	    return (
		    <video src={ entry.uri } controls />
	    )

	default:
	    return undefined
	}
    }

    render(props, state) {
	return (
		this.player(props.entry)
	)
    }
}

class EntryItem extends Component {
    constructor(props) {
	super(props);
    }

    material_icon(entry) {
	switch(entry.type) {
	case 'folder':
	    return 'folder';
	case 'radiostation':
	    return 'radio';
	case 'movie':
	    return 'movie';
	case 'video':
	    return 'ondemand_video';
	case 'tvserie':
	    return 'tv';
	case 'musictrack':
	    return 'audiotrack';
	default:
	    return 'error';
	}
    }

    render(props, state) {

	return (
		<li class='media' style='margin: 1rem;'>
		{ props.entry.metadata.image &&
		  <img class='media-image align-self-start mr-3' src={ props.entry.metadata.image }></img>
		}
	    { !props.entry.metadata.image &&
	      <i style='font-size:3.0rem;' class='align-self-start mr-3 material-icons'>{ this.material_icon(props.entry) }</i>
	    }
		<div class='media-body'>
		<h5 class='mt-0'>{ props.entry.metadata.title }</h5>
		{ props.entry.metadata.description &&
		  <p>{props.entry.metadata.description}</p>
		}
	        { props.entry.metadata &&
		  <Metadata metadata={props.entry.metadata} />
		}
		<MediaPlayer entry={ props.entry } />
		</div>
		</li>
	)
    }
}

class BrowserNavigation extends Component {
    constructor(props) {
	super(props);
    }

    render(props, stats) {
	return (
	    	<nav aria-label='breadcrumb'>
		<ol class='breadcrumb'>
		{ props.path.map((entry) => {
		    const idx = props.path.indexOf(entry);
		    const uri = '/providers/' + props.path.slice(0, idx + 1).join('/');
		    const is_active = (props.path.length == idx+1);
		    return is_active ? (	
		    	    <li class='breadcrumb-item active'>{ entry }</li>
		    ) : (
			    <li class='breadcrumb-item'>
			    <Link to={uri}>{ entry }</Link>
			    </li>
		    );
		})}
		</ol>
		</nav>
	)
    }
}

class Browser extends Component {
    constructor(props) {
	super(props);
		
	this.state = {
	    entries: []
	};
    }

    componentDidMount() {
	// fetch uri and set state
	const uri = '/providers/' + this.props.match.params[0];
	api.get(uri)
	    .then((entries) => {
		this.setState({
		    entries: entries
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
	    <div>
	    	<BrowserNavigation path={ this.props.match.params[0].split('/') } />
		<ul class='list-unstyled'>	
		{ state.entries.map(entry => (
			<a href={entry.uri}>
			<EntryItem entry={entry} />
			</a>
		))}
	    </ul>
		</div>

	)
    }
}

class SearchResult extends Component {
    constructor(props) {
	super(props);
	this.state = {
	    entries: []
	}
    }

    render(props, state) {
	return (
		<h1>Search result: {this.props.result}</h1>
	)
    }
}

class SearchForm extends Component {
    constructor(props) {
	super(props);

	this.handleChange= this.handleChange.bind(this);
	this.handleSubmit = this.handleSubmit.bind(this);
	this.state = {
	    value: '',
	    uri: undefined,
	};
    }

    processQuery(query) {
	// Parse search query, for :providers, #type and keywords
	let fields = query.split(' ')
	let providers = fields.filter((field) => field[0] == ':')
	let types = fields.filter((field) => field[0] == '#')
	let keywords = fields.filter((field) => field[0] != '#' && field[0] != ':')
	return {
	    providers: providers.length ? providers : undefined,
	    types: types.length ? types : undefined,
	    keywords: keywords
	}
    }

    handleSubmit(event) {
	let data = this.processQuery(this.state.value)
	api.search(data.providers, data.types, data.keywords)
	    .then((result) => {
		console.log(result)
		
		this.setState({
		    uri: result,
		})
	    });

	event.preventDefault();
    }

    handleChange(event) {
	this.setState({
	    value: event.target.value
	})
    }

    render(props, state) {
	return (
		<div>
		<form class='form-inline' onSubmit={this.handleSubmit}>
		<input class='form-control mr-sm-2' type='text' value={this.state.value} onInput={this.handleChange} />
		<button class='btn btn-outline-success my-2 my-sm-0' type='submit'>Search</button>
		</form>
		{ this.state.uri &&
		  <SearchResult result={this.state.uri} />
		}
		</div>
	)
    }
}

class LogItem extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return (
		<tr>
		<td>{ props.entry.timestamp }</td>
		<td>{ props.entry.domain }</td>
		<td>{ props.entry.level }</td>
		<td>{ props.entry.message }</td>
		</tr>
	)
    }
}

class Backlog extends Component {
    constructor(props) {
	super(props);
	this.state = {
	    entries: []
	};
    }

    componentDidMount() {
	api.get('/backlog')
	    .then((entries) => {
		this.setState({
		    entries: entries
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
	    <div>
		<h1>Backlog</h1>
		<table class='table'>
		<thead>
		<tr>
		<th scope='col'>Timestamp</th>
		<th scope='col'>Domain</th>
		<th scope='col'>Level</th>
		<th scope='col'>Message</th>
		</tr>
		</thead>
		<tbody>
		{
		    state.entries.map(entry => (		    
			    <LogItem entry={entry} />
		    ))
		}
	    </tbody>
		</table>
		</div>
	)
    }
}

class SettingsItem extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return (
		<div class='setting'>
		<strong>{ props.entry.name }</strong>
		<p>{ props.entry.description }</p>
		<input value={ props.entry.value }></input>
		</div>
	)
    }
}

class Settings extends Component {
    constructor(props) {
	super(props);
	this.state = {
	    entries: []
	}
    }

    componentDidMount() {
	const uri = '/settings/' + this.props.match.params.component;

	api.get(uri)
	    .then((entries) => {
		let entries_list = []

		Object.keys(entries).map((key) => {
		    entries_list.push({
			id: key,
			name: entries[key].name,
			description: entries[key].description,
			value: entries[key].value
		    })
		})
		
		this.setState({
		    entries: entries_list
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
		<form>
		<h1>Settings</h1>
		<div class='column'>
		{ state.entries.map(entry => (		    
			<SettingsItem entry={entry} />
		))}
	    </div>
		</form>
	)
    }
}

class Application extends Component {
    constructor(props) {
	super(props);

	this.state = {
	    authenticated: false,
	}
    }

    render(props, state) {
	return (
		<BrowserRouter>
		<nav class='navbar navbar-expand-lg navbar-light bg-light'>
		<a class="navbar-brand mb-0 h1" href="#">CAST.IO</a>
		<button class="navbar-toggler" type="button" data-toggle="collapse"
	    data-target="#navbar" aria-controls="navbar"
	    aria-expanded="false" aria-label="Toggle navigation">
		<span class="navbar-toggler-icon"></span>
		</button>

		<div id='navbar' class="collapse navbar-collapse">
		<ul class='navbar-nav mr-auto'>
		<li class='nav-item'><Link class='nav-link' to='/'>Home</Link></li>
		<li class='nav-item'><Link class='nav-link' to='/providers'>Providers</Link></li>
		<li class='nav-item'><Link class='nav-link' to='/settings/service'>Settings</Link></li>
		<li class='nav-item'><Link class='nav-link' to='/backlog'>Backlog</Link></li>
		</ul>
		</div>
		</nav>
		<div class='container' style='padding: 1rem;'>
		<Route exact path='/' component={SearchForm} />
		<Route exact path='/providers' component={ProviderCollection} />
		<Route path='/providers/*' component={Browser} />
		<Route path='/settings/:component' component={Settings} />
		<Route exact path='/backlog' component={Backlog} />
		</div>
		</BrowserRouter>
	)
    }
    
}

render(
    <Application />,
    document.getElementById('app')
);
