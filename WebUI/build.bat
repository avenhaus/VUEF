::
:: Execute with git bash
::
cd %~dp0
cmd.exe /c echo ==== Execute with Git Bash ====
cmd.exe /c npm install
cmd.exe /c npm audit fix
cmd.exe /c npm audit
cmd.exe /c gulp package
:: cmd.exe /c bin2c -o WebUI.h -m index.html.gz
cmd.exe /c xxd -i index.html.gz WebUI.h
cat header.txt > out.h
cat WebUI.h >> out.h
cat footer.txt >> out.h
sed -i "s/unsigned int index_html_gz_len/const size_t INDEX_HTML_GZ_SIZE/g" ./out.h
sed -i "s/unsigned char index_html_gz/const uint8_t INDEX_HTML_GZ/g" ./out.h
sed -i "s/] = {/] PROGMEM = {/g" ./out.h
mv ./out.h ../include/WebUi.h
rm WebUI.h
:: move /y index.html.gz ../data/index.html.gz

