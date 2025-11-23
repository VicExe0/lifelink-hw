#ifndef DEV_PAGE
#define DEV_PAGE

const char dashboard[] PROGMEM = R"rawliteral(
<html>
<head>
<title>LifeLink</title>
<style>
body { background-color: #212121; color: #ffffff }
</style>
</head>
<body>
<form action='/save'>
SSID: <input name='ssid' value='{ssid}' required><br>
PASSWORD: <input name='password' value='{password}' type='password' required><br>
<input type='submit' value='OK'>
</form>
<form action='/register'>
PESEL: <input name='pesel' placeholder='12345678901' required><br>
PASSWORD: <input name='password' type='password' required><br>
<input type='submit' value='REGISTER CARD'>
</form>
</body>
</html>
)rawliteral";

#endif