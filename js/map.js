var MAP =
{
	MyMap : null,
	layerVector : null,
	sourceVector : null,
	iconGeometry : null, 
	styleMarker : null,
	lineLayer : null,
	sourceLineVector : null,
	previewLine : null,
	currentLat : null,
	currentLon : null,
	createMap : function()
	{
		this.sourceVector = new ol.source.Vector();
		this.layerVector = new ol.layer.Vector({
			source : this.sourceVector,
		});
		this.sourceLineVector = new ol.source.Vector();
		this.lineLayer = new ol.layer.Vector({
		  source: this.sourceLineVector,
		  style: new ol.style.Style({
			stroke: new ol.style.Stroke({
			  color: 'rgba(255, 0, 0, 0.5)',
			  width: 6
			})
		  })
		});
		this.MyMap = new ol.Map({
			target : 'map',
			layers : [
				new ol.layer.Tile({
					// source : new ol.source.OSM(),
					source : new ol.source.XYZ({
						attributions: '<a href="https://www.maptiler.com/copyright/" target="_blank">&copy; MapTiler</a> ' +'<a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
						url: 'https://api.maptiler.com/maps/hybrid/{z}/{x}/{y}.jpg?key=hhrw4qZsZwX7gNfZy8kI',
						tileSize: 512
					}),
				}),
				this.lineLayer,
				this.layerVector,
			],
			view : new ol.View({
				center: ol.proj.fromLonLat([115,-8]),
				zoom: 4.5 //Initial Zoom Level
			}),
		});
	},
	
	addMarker : function(markerName='Marker',lat=110.370500, lon=-7.823900)
	{
		console.log(markerName + " " + lat +" " + lon);
		var marker;
		iconGeometry = new ol.geom.Point(ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857'));
		marker = new ol.Feature({
			geometry : iconGeometry,
			name : markerName,
			
		});
		marker.setId(markerName);
		
		this.styleMarker = new ol.style.Style({
			// image : new ol.style.Icon({
				// scale : .05,
				// src : 'assets/location.png',
				// anchor : [.5, .5,],
			// }),
			image : new ol.style.Circle({
				radius: 8,
				fill: new ol.style.Fill({
					color: 'rgba(43, 166, 203, 0.8)',
				}),
				stroke: new ol.style.Stroke({
					color: 'white',
					width: 2
				}),
			}),
			text: new ol.style.Text({
				text: markerName ,//+ ': ' + lat + ' , ' + lon,
				offsetY: -25,
				fill: new ol.style.Fill({
					color: '#fff'
				}),
				stroke: new ol.style.Stroke({
					color: '#000',
					width: 4,
				}),
			}),
		});
		
		marker.setStyle(this.styleMarker);
		
		this.sourceVector.addFeatures([marker]);
		this.MyMap.setView(new ol.View({
				center: ol.proj.fromLonLat([lat,lon]),
				zoom: 15 //Initial Zoom Level
			}));
		this.previewLine = new ol.Feature({
			geometry: new ol.geom.LineString([]),
			name: 'TrackingLine'
		});
		this.previewLine.setId('TrackingLine');
		this.previewLine.getGeometry().appendCoordinate(new ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857'));
		this.sourceLineVector.addFeatures([this.previewLine]);
	},
	
	moveMarker : function(markerId='Marker',lat=110.370500, lon=-7.823900)
	{
		var marker = this.sourceVector.getFeatureById(markerId);
		if( marker == null )
		{
			this.addMarker(markerId,lat, lon);
			this.currentLat = lat;
			this.currentLon = lon;
		}
		else
		{
			console.log("Moving marker...");
			marker.getGeometry().setCoordinates(ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857'));
			var points = new Array(
				new ol.geom.Point(this.currentLat,this.currentLon),
				new ol.geom.Point(lat,lon)
			);
			var line = this.sourceLineVector.getFeatureById('TrackingLine');
			if( line == null )
			{
				console.log('line is not found');
				console.log('Features:');
				console.log(this.sourceLineVector.getFeatures());
			}
			else
			{
				console.log('line is found');
				line.getGeometry().appendCoordinate(new ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857'));
				console.log('coordinates:');
				console.log(line.getGeometry().getCoordinates());
			}
			this.currentLat = lat;
			this.currentLon = lon;
		}
	},
}

window.onload = MAP.createMap();
