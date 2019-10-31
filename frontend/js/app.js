
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
	return Inferno.h('div', {class: 'container', style: 'background-color: #e0e0e0; margin: 1rem; padding: 1rem;'}, [
	    Inferno.h('div', {class: 'row'}, [
		Inferno.h('div', {class: 'column column-10'}, [
		    Inferno.h('img', {
			style: 'padding: 1rem;', src: 'api/v1/cache?resource=' + encodeURIComponent(props.provider.icon)
		    })
		]),

		Inferno.h('div', {class: 'column'}, [
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
							Inferno.h(ProviderCard, {provider: provider})
						       )
				   })
				  )
			)

    }
}

Inferno.render(Inferno.h(Application, {test: 'test'}), document.getElementById('app'));
