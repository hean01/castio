
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

class ProviderDetails extends Inferno.Component {
    constructor(props) {
	super(props);
    }

    render(props, state) {
	return Inferno.h('div', {style: 'padding: 1rem;'}, [
	    Inferno.h('strong', {}, props.provider.name),
	    Inferno.h('img', {style: 'padding: 1rem;', src: 'api/v1/cache/' + encodeURIComponent(props.provider.icon)}),
	    Inferno.h('div', {}, props.provider.description),
	    Inferno.h('small', {},
		      Inferno.h('a', {href: props.provider.homepage}, props.provider.homepage)),
	])
    }
}

class Application extends Inferno.Component {
    constructor(props) {
	super(props);

	this.api = new Api('admin', 'password')

	this.state = {
	    authenticated: false,
	    providers: ['dino'],
	}
    }

    componentDidMount() {
	console.log('Mount')
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
	//console.log('Render' + JSON.stringify(this.state))
        return Inferno.h('div', {class: 'container'},
			 Inferno.h('div', {},
				   state.providers.map(provider => {
				       return Inferno.h('div', {},
							Inferno.h(ProviderDetails, {provider: provider})
						       )
				   })
				  )
			)

    }
}

Inferno.render(Inferno.h(Application, {test: 'test'}), document.getElementById('app'));
