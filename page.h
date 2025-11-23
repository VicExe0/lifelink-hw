#ifndef DEV_PAGE
#define DEV_PAGE

const char dashboard[] PROGMEM = R"rawliteral(
<html>
<head>
<style>
body { background-color: #212121; color: #ffffff }
</style>
</head>
<body>
<form action='/save'>
SSID: <input name='ssid' value='{ssid}'><br>
PASSWORD: <input name='pswd' value='{password}'><br>
<input type='submit' value='OK'>
</form>
<form action='/register'>
PESEL: <input name='pesel' placeholder='12345678901'><br>
<input type='submit' value='REGISTER CARD'>
</form>
</body>
</html>
)rawliteral";

#endif