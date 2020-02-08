var MAP =
{
	MyMap : null,
	layerVector : null,
	sourceVector : null,
	iconGeometry : null, 
	isFirstTime : true,
	styleMarker : null,
	createMap : function()
	{
		sourceVector = new ol.source.Vector();
		this.layerVector = new ol.layer.Vector({
			source : sourceVector,
		});
		//this.addMarker();
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
		
		//this.moveMarkerTo('Marker',lat=110.380500, lon=-7.833900);
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
			image : new ol.style.Icon({
				scale : .05,
				src : 'assets/location.png',
				anchor : [.5, .5,],
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
		
		sourceVector.addFeatures([marker]);
	},
	
	moveMarkerTo : function(markerId='Marker',lat=110.370500, lon=-7.823900)
	{
		var px = this.MyMap.getPixelFromLonLat(new ol.proj.fromLonLat(lat,lon));
		var marker = sourceVector.getFeatureById(markerId);
		marker.moveTo(px);
	},
	
	moveMarker : function(markerId='Marker',lat=110.370500, lon=-7.823900)
	{
		//console.log(markerId + lat +" " + lon);
		//if( this.iconGeometry == null)
		if( this.isFirstTime)
		{
			this.addMarker(markerId,lat, lon);
			this.isFirstTime = false;
		}
		else
		{
			console.log("Moving marker...");
			var marker = sourceVector.getFeatureById(markerId);
			marker.getGeometry().setCoordinates(ol.proj.transform([lat,lon], 'EPSG:4326','EPSG:3857'));
			//var lonlat = new OpenLayers.LonLat(lon, lat);
			//this.MyMap.panTo(ol.proj.fromLonLat(lon,lat));
			//this.MyMap.getView().setCenter(ol.proj.fromLonLat(lon,lat));
			//this.moveMarkerTo(markerId,lat,lon);
			//this.MyMap.render();
		}
	},
}
// export function MapMoveMarker(markerId='Marker',lat=110.370500, lon=-7.823900)
// {
	// MAP.moveMarker(markerId,lat, lon)
// }
MAP.createMap();