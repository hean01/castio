import { render, linkEvent, Component } from 'inferno'; 

class Api {

    constructor(username, password) {
    }


    getProviders() {
	var url = 'api/v1/providers'
	return fetch(url, {mode: 'same-origin'})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Failed to get providers: ', err);
		throw err
	    });
    }

    getEntries(uri) {
	var url = 'api/v1/' + uri;
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
	    <div class='card'>
  		<div class="section">
		<img src={'api/v1/cache?resource=' + encodeURIComponent(props.provider.icon)}></img>
		</div>

	      	<div class="section">
		<strong>
		<a href=''>{ props.provider.homepage }</a>
		</strong>
		{ props.provider.description }
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
		<div class='container'>
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
		<b>{ props.name }': '</b>
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
		<div class='row'>
		{props.metadata.image && 
		 <img class='logo' src={ props.metadata.image }></img>
		}	    
	    { props.metadata.artist &&
	      <NameValue name='Artist' value={props.metadata.artist} />
	    }
	    { props.metadata.on_air &&
	      <NameValue name='On-air' value={props.metadata.on_air} />
	    }
	    </div>
	)
    }
}

class EntryItem extends Component {
    constructor(props) {
	super(props);
	console.log(props);
    }

    on_click(props, event) {
	console.log('Onclick' + props.entry.metadata.title);
    }

    render(props, state) {
	console.log('render')
	return (
		<div class='entry' onclick={ linkEvent(props, this.on_click) }>
		<h3>{props.entry.metadata.title}</h3>
		<p>{props.entry.metadata.description}</p>
		<Metadata metadata={props.entry.metadata} />
		</div>
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
	this.api.getEntries(this.props.uri)
	    .then((entries) => {
		this.setState({
		    entries: entries
		})
	    }).catch((err) => {
	    })
    }

    render(props, state) {
	return (
		<div class='browser'>	
		{ state.entries.map(entry => (<EntryItem entry={entry} />) )}
		</div>
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
		<div>
		<Browser uri='/providers/rad.io/nearest' />
		<ProviderCollection />
		</div>
	)
    }
    
}

render(
    <Application />,
    document.getElementById('app')
);
