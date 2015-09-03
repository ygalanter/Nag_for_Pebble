
Pebble.addEventListener("ready",
  function(e) {
    // console.log("PebbleKit JS ready!");
  }
);



Pebble.addEventListener("showConfiguration",
  function(e) {
       
    //Load the remote config page
    Pebble.openURL("http://codecorner.galanter.net/pebble/nag_config.html");
    
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    
    if (e.response !== '') {
      
      console.log('resonse: ' + decodeURIComponent(e.response));
      
      //Get JSON dictionary
      var settings = JSON.parse(decodeURIComponent(e.response));
      
      console.log(settings);
      
      //Send to Pebble, persist there
      Pebble.sendAppMessage(
          {
            "KEY_BUZZ_INTENSITY": settings.buzzIntensity,
            "KEY_BUZZ_INTERVAL": settings.buzzInterval,
            "KEY_BUZZ_START": settings.buzzStart
          },
        function(e) {
          console.log("Sending settings data...");
        },
        function(e) {
          console.log("Settings feedback failed!");
        }
      );
    }
  }
);