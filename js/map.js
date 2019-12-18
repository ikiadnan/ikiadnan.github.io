var MAP =
{
	MyMap : null,
	layerVector : null,
	sourceVector : null,
	
	createMap : function()
	{
		sourceVector = new ol.source.Vector();
		this.layerVector = new ol.layer.Vector({
			source : sourceVector,
		});
		this.addMarker();
		this.MyMap = new ol.Map({
			target : 'map',
			layers : [
				new ol.layer.Tile({
					source : new ol.source.OSM(),
				}),
				this.layerVector,
			],
			
			
			view : new ol.View({
				center: ol.proj.fromLonLat([110.370500,-7.823900]),
				zoom: 15 //Initial Zoom Level
			}),
		});
		
	},
	
	addMarker : function(markerName='Marker',lat=110.370500, lon=-7.823900)
	{
		var marker, styleMarker;
		marker = new ol.Feature({
			geometry : new ol.geom.Point(ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857')),
			name : markerName,
			
		});
		
		styleMarker = new ol.style.Style({
			image : new ol.style.Icon({
				scale : .05,
				src : 'assets/location.png',
				anchor : [.5, .5,],
			}),
			text: new ol.style.Text({
				text: markerName + ': ' + lat + ' , ' + lon,
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
		
		marker.setStyle(styleMarker);
		
		sourceVector.addFeatures([marker]);
	},
}

MAP.createMap();