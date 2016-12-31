#! /bin/bash

echo "PWD: " + $PWD

if [ ! -d "Blackmagic DeckLink SDK" ] ; then
    wget http://slackware.uk/sbosrcarch/by-name/multimedia/vlc/Blackmagic_DeckLink_SDK_10.1.4.zip
    unzip Blackmagic_DeckLink_SDK_10.1.4.zip
    mv "Blackmagic DeckLink SDK 10.1.4" "Blackmagic DeckLink SDK"
fi
