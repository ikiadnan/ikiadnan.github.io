var map = new ol.Map({
        target: 'map',
        layers: [
          new ol.layer.Tile({
            source: new ol.source.OSM()
          })
        ],
        view: new ol.View({
          center: ol.proj.fromLonLat([110.370500,-7.823900]),
          zoom: 15 //Initial Zoom Level
        })
      });
	  
