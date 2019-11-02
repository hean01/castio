const Bundler = require('parcel-bundler');
const express = require('express');
const proxy = require('http-proxy-middleware');

const app = express();

// Setup backend proxy
const apiProxy = proxy({
    target: 'http://localhost:1457',
    logLevel: 'debug',
    pathRewrite: {
	'^/api/v1': ''
    }
});

app.use('/api/v1', apiProxy);

// Setup pacel bundler
const bundler = new Bundler('src/index.html');
app.use(bundler.middleware());

app.listen(Number(process.env.PORT || 8000));
