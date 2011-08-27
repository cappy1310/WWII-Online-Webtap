#!/bin/bash

# Update the files to scan
# Least server friendly
#wget -qr -a wget_logfile --directory-prefix data -i image_url
# More server friendly
wget -r -w 5 -a wget_logfile --directory-prefix data -i ./data/image_url
# Most server friendly, avoids basic server detection attempts
#wget -qr -wait 10 --random-wait -a wget_logfile -- directory-prefix data -i image_url

# Scan the files
./image.x

# Format the outputted XML, move it to where others can see it
sed -e "s/>/>\n/g" -e "s/City-only/\tCity-only/g" -e "s/<def /\t<def /g" -e "s/CP states/CP states\n/g" ./webtap.cpstates.citys.xml>./expose/cpstates.citys.xml
