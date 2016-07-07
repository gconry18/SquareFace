console.log('JS File Start!');
var configuration = {};

Pebble.addEventListener('ready', function(e) {
	console.log('PebbleKit JS ready!');
	loadData();
});

Pebble.addEventListener('showConfiguration', function(e) {
	console.log('PebbleKit Show Config!');
	Pebble.openURL("http://gconry18.github.io/squareface_test.html");
});
	
Pebble.addEventListener('webviewclosed',
	function(e) {
		
		if (e.response) {
			var config = JSON.parse(e.response);
			saveData(config);

			//Send to Pebble, persist there
			Pebble.sendAppMessage(
				{
					"KEY_INVERT": configuration.invert,
					"KEY_BACKGROUND": configuration.back,
					"KEY_DATE": configuration.date,
					"KEY_GRAPHPERIOD": configuration.graphPeriod
				},
				function(e) {
					console.log('Sending settings data...');
				},
				function(e) {
					console.log('Settings feedback failed!');
				}
			);
		}		
	});

function loadData() {
	console.log('Loading Data!');
	
	configuration.invert = localStorage.getItem("invert");
	configuration.back = localStorage.getItem("back");
	configuration.date = localStorage.getItem("date");
	configuration.graphPeriod = localStorage.getItem("graphPeriod");
}

function saveData(config) {
	console.log('Configuration window returned: ' + JSON.stringify(config));
	
	localStorage.setItem("invert", config.invert);
	localStorage.setItem("back", config.back);
	localStorage.setItem("date", config.date);
	localStorage.setItem("graphPeriod", config.graphPeriod);
	
	loadData();
}