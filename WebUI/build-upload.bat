cd %~dp0
cmd.exe /c gulp package
curl -F "data=@index.html.gz" http://192.168.0.176/files

