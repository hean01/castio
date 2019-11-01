
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
}

class ProviderCard extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {
	}, [
	    Inferno.h('div', {
	    }, [
		Inferno.h('div', {
		}, [
		    Inferno.h('img', {
			src: 'api/v1/cache?resource=' + encodeURIComponent(props.provider.icon)
		    })
		]),

		Inferno.h('div', {
		}, [
		    Inferno.h('strong', {}, [
			Inferno.h('a', {href: props.provider.homepage}, props.provider.name),
			", "
		    ]),
		    props.provider.description,
		]),
	    ]),
	])
    }
}

class Application extends Inferno.Component {
    constructor(props) {
	super(props);

	this.api = new Api('admin', 'password')

	this.state = {
	    authenticated: false,
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
	},
			 Inferno.h('div', {},
				   state.providers.map(provider => {
				       return Inferno.h('div', {},
							Inferno.h(ProviderCard, {provider: provider})
						       )
				   })
				  )
			)

    }
}

Inferno.render(Inferno.h(Application, {}), document.getElementById('app'));
