#!/bin/bash
# This script updates the data stored for each city, pulled from the html.
# This should be run once, to generate the file, and after that only if cities are added to the map, or their location is moved.
# Otherwise, this script will just be using bandwidth to generate the same data.

# The order is unimportant; make sure that the integers assigned correspond to the image_data indices, though.
grep -r 'option value' ./html_data/holland_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@1;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >./html_data/city_data
grep -r 'option value' ./html_data/channel_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@2;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >>./html_data/city_data
grep 'option value' ./html_data/belgium_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@3;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >>./html_data/city_data
grep -r 'option value' ./html_data/central_german_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@4;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >>./html_data/city_data
grep -r 'option value' ./html_data/ne_france_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@5;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >>./html_data/city_data
grep -r 'option value' ./html_data/maginot_supply.html | sed -e 's/^.*<option value=//g' -e 's@</option>@@g' -e 's@"@6;(@1' | sed -e 's@"@)@1' -e 's/>/;/g' >>./html_data/city_data
