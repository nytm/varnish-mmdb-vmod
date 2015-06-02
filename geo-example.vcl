# This is a basic VCL configuration file for varnish.  See the vcl(7)
# man page for details on VCL syntax and semantics.
# 
# Default backend definition.  Set this to point to your content
# server.
# 
backend default {
    .host = "www.varnish-cache.org";
    .port = "80";
}
import geo;
import std;

sub vcl_recv{
    if (req.restarts == 0) {
      if (req.http.x-forwarded-for) {
          set req.http.X-Forwarded-For =
              req.http.x-real-ip  + ", " + req.http.X-Forwarded-For + ", " + client.ip;
      } else {
        set req.http.X-Forwarded-For = req.http.x-real-ip + ", " + client.ip;
      }
    }
    if (req.url ~ "/svc/location/v1/current.json") {
        set req.http.IP = regsuball(req.http.X-Forwarded-For, ",.*", "");
        return (error);
    }
    // logic to do weather based on IP or NYT-W2 cookie
    if ((req.url ~ "/svc/weather/v2/current.json") || (req.url ~ "/svc/weather/v2/current-and-five-day-forecast.json") || (req.url ~ "/svc/weather/v2/current-and-seven-day-forecast.json")) {
        if (req.restarts == 0) {
            if (req.http.Cookie ~ "NYT_W2=[a-zA-Z0-9%:\-|]+") {
                set req.http.X-Weather = geo.get_weather_cookie(req.http.Cookie, "NYT_W2");
                set req.url = regsub(req.url, ".json", "/" + req.http.X-Weather + ".json");
                set req.url = regsub(req.url, " ", "%20");
            } else {
                // i think we want the first forwarded for ip.
                if (req.http.X-Forwarded-For ~ ",") {
                    if (!req.http.IP) {
                        set req.http.IP = regsuball(req.http.X-Forwarded-For, ",.*", "");
                    }
                } else {
                    if (!req.http.IP) {
                        set req.http.IP = req.http.x-real-ip;
                    }
                }
                set req.http.X-Weather = geo.weather_code(req.http.IP);
                set req.http.X-Weather = regsuball(req.http.X-Weather, " ", "%20");
                set req.http.X-Timezone = geo.timezone(req.http.IP);
                set req.url = regsub(req.url, ".json", "/" + req.http.X-Weather + ".json");
            }
        }
    }
}

sub vcl_error {
  if (req.url ~ "/svc/location/v1/current.json") {
    set obj.http.Content-Type = "text/json; charset=utf-8";
    synthetic 
      geo.location(req.http.IP)
    ; 
    return (deliver);
  }   
}