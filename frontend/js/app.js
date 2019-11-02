
class Api {
    constructor(username, password) {
    }


    getProviders() {
	var url = 'api/v1/providers'
	return fetch(url, {mode: 'same-origin'})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Fetch Error :-S', err);
	    });
    }

    getEntries(uri) {
	var url = 'api/v1/' + uri;
	return fetch(url, {mode: 'same-origin'})
	    .then((response) => response.json())
	    .catch((err) => {
		console.log('Fetch Error :-S', err);
	    });
    }
}

class ProviderCard extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {
	    class: 'card',
	}, [

	    Inferno.h('div', {
		class: 'section',
	    }, [
		Inferno.h('img', {
		    src: 'api/v1/cache?resource=' + encodeURIComponent(props.provider.icon)
		})
	    ]),

	    Inferno.h('div', {
		class: 'section',
	    }, [
		Inferno.h('strong', {}, [
		    Inferno.h('a', {href: props.provider.homepage}, props.provider.name),
		    ", "
		]),
		props.provider.description,
	    ]),
	])
    }
}

class ProviderCollection extends Inferno.Component {
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
		console.log('Failed: ' + err)
	    })
    }

    render(props, state) {
        return Inferno.h('div', {
	    class: 'container'
	}, [
	    state.providers.map(provider => {
		return Inferno.h('div', {},
				 Inferno.h(ProviderCard, {provider: provider})
				)
	    })
	])
    }
}

class NameValue extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {class: 'row'}, [
	    Inferno.h('b', {}, props.name + ': '),
	    Inferno.h('span', {}, props.value),
	])
    }
}

class Metadata extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {class: 'row'}, [
	    props.image ? Inferno.h('img', {class: 'logo', src: props.image}) : undefined,
	    props.artist ? Inferno.h(NameValue, {
		name: 'Artist', value: props.artist
	    }) : undefined,

	    props.on_air ? Inferno.h(NameValue, {
		name: 'On air', value: props.on_air
	    }) : undefined,

	])
    }
}

class EntryItem extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {
	    class: 'entry'
	}, [
	    Inferno.h('h3', {}, props.entry.metadata.title),
	    Inferno.h('p', {}, props.entry.metadata.description),
	    Inferno.h(Metadata, props.entry.metadata)
	])
    }
}

class Browser extends Inferno.Component {
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
		console.log('Failed: ' + err)
	    })
    }

    render(props, state) {
	return Inferno.h('div', {
	    class: 'browser',
	}, state.entries.map(entry => {
	    return Inferno.h(EntryItem, {entry: entry})
	}))
    }
}

class Application extends Inferno.Component {
    constructor(props) {
	super(props);

	this.state = {
	    authenticated: false,
	}
    }

    render(props, state) {
	return Inferno.h('div', {}, [
	    Inferno.h(Browser, {uri: '/providers/rad.io/nearest'}),
	    Inferno.h(ProviderCollection, {})
	]);
    }
    
}

Inferno.render(Inferno.h(Application, {}), document.getElementById('app'));
