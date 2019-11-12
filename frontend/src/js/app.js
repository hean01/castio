import { render, linkEvent, Component, Fragment} from 'inferno';
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
		return datauri;
	    });

    }

    cache(resource) {
	var url = this.API_BASE_URI + '/cache?resource=' + resource;
	return this.fetch(url, {
	    mode: 'same-origin'
	}).then((response) => {

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
		<img class='media' style='width: 6rem;' alt="" src={ state.icon }></img>
		<div class="media">
		<h5 class='mr-3'>{ props.provider.name }</h5>
		<p>{ props.provider.description } </p>
		<ul class='list-unstyled'>
		{ props.provider.version &&
		  <NameValue name='Version' value={ props.provider.version[0] + '.' + props.provider.version[1] + '.' + props.provider.version[2]} />
		}
		{ props.provider.copyright &&
		  <NameValue name='Copyright' value={ props.provider.copyright } />
		}
	    { props.provider.homepage &&
	      <NameValue name='Homepage' value={ props.provider.homepage } />
	    }
	    </ul>
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
		<Fragment>
		<h1>Providers</h1>
		<ul class='list-unstyled'>
		{
		    state.providers.map(provider =>
					<ProviderItem provider={provider} />
				       )
		}
	    </ul>
		</Fragment>
	)
    }
}

class NameValue extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {

	return (
		<li>
		<b>{ props.name + ': '}</b>
		<span>{ props.value }</span>
		</li>
	)
    }
}

class Metadata extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return (
		<ul class='list-unstyled'>
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

    onPlayItem(props, event) {
	window.location = props.entry.uri
    }

    render(props, state) {

	const is_folder = (props.entry.type == 'folder')

	const item = (
		<li class='row' style='margin: 1rem;'>
		{ props.entry.metadata.image &&
		  <img class='media' style='width: 8rem;' alt="" src={ props.entry.metadata.image }></img>
		}
	    { !props.entry.metadata.image &&
	      <center class='icon material-icons'>{ this.material_icon(props.entry) }</center>
	    }
		<div class='column' style='padding-left: 0.5rem;'>
		<h2>{ props.entry.metadata.title }</h2>
		{ props.entry.metadata.description &&
		  <p>{props.entry.metadata.description}</p>
		}
	    { props.entry.metadata &&
	      <Metadata metadata={props.entry.metadata} />
	    }
	    { !is_folder &&
		  <span nowrap>
		  <button class="primary material-icons" onClick={ linkEvent(props, this.onPlayItem) }>play_circle_outline</button>
		  <button class="primary material-icons">queue</button>
		  <button class="primary material-icons">favorite_border</button>
		  </span>
		}

	    </div>
	    </li>
	);

	if (is_folder) {
	    return (
		    <a class="list-item" href={ props.entry.uri }>
		    { item }
		</a>
	    )
	} else {
	    return item;
	}
    }
}


class BrowserNavigation extends Component {
    constructor(props) {
	super(props);
    }

    render(props, stats) {
	return (
		<ol class='breadcrumb'>
		{ props.path.map((entry) => {
		    const idx = props.path.indexOf(entry);
		    const uri = '/providers/' + props.path.slice(0, idx + 1).join('/');
		    const is_active = (props.path.length == idx+1);
		    return is_active ? (
			    <li class='active'>{ entry }</li>
		    ) : (
			    <li>
			    <Link class='breadcrumb' to={uri}>{ entry }</Link>
			    </li>
		    );
		})}
		</ol>
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
		<Fragment>
	    	<BrowserNavigation path={ this.props.match.params[0].split('/') } />
		<ul style="width: 100%;" class='container list-unstyled'>
		{ state.entries.map((entry) => (
			<EntryItem entry={ entry } />
		))}
	        </ul>
	    </Fragment>
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
		<Fragment>
		<h1>Search</h1>
		<form class='form-inline' onSubmit={this.handleSubmit}>
		<input class='form-control mr-sm-2' type='text' value={this.state.value} onInput={this.handleChange} />
		<button class='primary' type='submit'>Search</button>
		</form>
		{ this.state.uri &&
		  <SearchResult result={this.state.uri} />
		}
		</Fragment>
	)
    }
}

class LogItem extends Component {
    constructor(props) {
	super(props);
    }

    timestampToDate(timestamp) {
	let ms = timestamp*1000;
	return new Date(ms).toLocaleString('en-US', {
	    hour12: false
	});
    }

    render(props, state) {
	const options = {
	    hour12: false
	};

	return (
		<tr>
		<td nowrap>{ this.timestampToDate(props.entry.timestamp) }</td>
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

    compareTimestamp(a, b) {
	if (a.timestamp < b.timestamp) {
	    return 1;
	} else if (a.timestamp > b.timestamp) {
	    return -1;
	}
	return 0;
    }

    componentDidMount() {
	api.get('/backlog')
	    .then((entries) => {
		entries.sort(this.compareTimestamp)
		this.setState({
		    entries: entries
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
	    <Fragment>
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
		</Fragment>
	)
    }
}

class SettingsItem extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return (
		<div class='form-group'>
		<label>{ props.entry.name }</label>
		<small class="form-text text-muted">{ props.entry.description }</small>
		<input class='form-control' value={ props.entry.value }></input>
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
		<Fragment>
		<h1>Settings</h1>
		<form>
		{ state.entries.map(entry => (		    
			<SettingsItem entry={entry} />
		))}
		</form>
		</Fragment>
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
		<header>
		<Link class="logo" to="#">CAST.IO</Link>
		<Link class='button' to='/'>Home</Link>
		<Link class='button' to='/providers'>Providers</Link>
		<Link class='button' to='/settings/service'>Settings</Link>
		<Link class='button' to='/backlog'>Backlog</Link>
		</header>

		<main class='container'>
		<Route exact path='/' component={SearchForm} />
		<Route exact path='/providers' component={ProviderCollection} />
		<Route path='/providers/*' component={Browser} />
		<Route path='/settings/:component' component={Settings} />
		<Route exact path='/backlog' component={Backlog} />
		</main>
		</BrowserRouter>
	)
    }
    
}

render(
    <Application />,
    document.getElementById('app')
);
