#!/bin/bash

# Update the files to scan
$data_dir="../data"
$base_dir=".."
$expose_dir="../expose"
# Least server friendly
#wget -qr -a wget_logfile --directory-prefix data -i image_url
# More server friendly
wget -r -w 5 -a wget_logfile --directory-prefix data -i $data_dir/image_url
# Most server friendly, avoids basic server detection attempts
#wget -qr -wait 10 --random-wait -a wget_logfile -- directory-prefix data -i image_url

# Scan the files
$base_dir/image.x

# Format the outputted XML, move it to where others can see it
sed -e "s/>/>\n/g" -e "s/City-only/\tCity-only/g" -e "s/<def /\t<def /g" -e "s/CP states/CP states\n/g" $base_dir/webtap.cpstates.citys.xml>$expose_dir/cpstates.citys.xml
