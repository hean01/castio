import { render, linkEvent, Component } from 'inferno'; 
import {BrowserRouter, Route, Link} from 'inferno-router';

class Api {

    constructor(username, password) {
    }


    getProviders() {
	var url = '/api/v1/providers'
	return fetch(url, {mode: 'same-origin'})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Failed to get providers: ', err);
		throw err
	    });
    }

    search(providers, types, keywords) {
	var url = '/api/v1/search?'
        url += 'keywords=' + keywords.join('+')

	if (providers) {
	    url += '&providers=' + providers.join('+')
	}

	if (types) {
	    url += '&type=' + types.join('+')
	}

	return fetch(url, {mode: 'same-origin'})
	    .then((response) => {
		let location = response.url;
		let more_data = (response.status == 206);

		response.json()
		    .then((entries)=>{
			return {
			    location: location,
			    have_more_data: more_data,
			    entries: entries,
			};
		    })
	    })
    }

    get(uri) {
	var url = '/api/v1/' + uri;
	console.log('Fetching ' + url)
	return fetch(url, {mode: 'same-origin'})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Failed to get entries: ', err);
		throw err
	    });
    }
}

class ProviderCard extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {

	return (
	    <div class='card' style='margin: 2rem; width: 18rem;'>
		<img class='card-img-top' src={'api/v1/cache?resource=' + encodeURIComponent(props.provider.icon)}></img>
		<div class="card-body">
		<div class='card-title'>
		<strong>{ props.provider.name }</strong>
		</div>
		<div class='cart-text'>
		{ props.provider.description }
	    </div>
		<Link class='card-link' to={ '/providers/' + props.provider.id }>Browse</Link>
		<Link class='card-link' to={ '/settings/' + props.provider.id }>Settings</Link>
		</div>
		</div>
	)
    }
}

class ProviderCollection extends Component {
    constructor(props) {
	super(props)
	this.api = new Api('admin', 'password')
	this.state = {
	    providers: [],
	}
    }

    componentDidMount() {
	this.api.getProviders()
	    .then((providers) => {
		this.setState({
		    providers: providers
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
		<div class='row'>
		{
		    state.providers.map(provider =>
					<ProviderCard provider={provider} />
				       )
		}
		</div>
	)
    }
}

class NameValue extends Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {

	return (
		<div class='row'>
		<b>{ props.name + ':'}</b>
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
	this.api = new Api('admin', 'password');
	
	this.state = {
	    entries: []
	};
    }

    componentDidMount() {
	// fetch uri and set state
	const uri = 'providers/' + this.props.match.params[0];
	this.api.get(uri)
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

	this.api = new Api('admin', 'password');	
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
	this.api.search(data.providers, data.types, data.keywords)
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
	this.api = new Api('admin', 'password')
	this.state = {
	    entries: []
	}
    }

    componentDidMount() {
	this.api.get('/backlog')
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
	this.api = new Api('admin', 'password')
	this.state = {
	    entries: []
	}
    }

    componentDidMount() {
	const uri = 'settings/' + this.props.match.params.component;

	this.api.get(uri)
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
		<div class='collapse navbar-collapse'>
		<ul class='navbar-nav'>
		<li class='nav-item nav-link'><Link to='/'>Home</Link></li>
		<li class='nav-item nav-link'><Link to='/providers'>Providers</Link></li>
		<li class='nav-item nav-link'><Link to='/settings/service'>Settings</Link></li>
		<li class='nav-item nav-link'><Link to='/backlog'>Backlog</Link></li>
		</ul>
		</div>
		</nav>
		<div class='container' style='margin: 1rem;'>
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
