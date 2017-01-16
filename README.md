# BeScreenCapture

![Screenshot](https://raw.github.com/jackburton79/bescreencapture/master/BeScreenCapture.png)


BeScreenCapture lets you record what happens on your screen and save it to a clip in any media format supported by Haiku.

The recorded clip can be scaled to suit your needs.

Includes an "Incognito" mode, which hides the application window and the deskbar icon, to be able to record clips without leaving any 

trace. In Incognito mode you need to use a shortcut key combination to start/stop the recording process.

You can do that by installing the bescreencapture_inputfilter and using the ctrl-shift-alt-r shortcut, or by defining a shortcut key combo

using the "Shortcuts" preflet included in Haiku.

These are the commands you can configure in the "Shortcuts" preflet for the various actions:

* Start/Stop Recording: " SendMessage application/x-vnd.BeScreenCapture 'StoR' "

* Pause/Resume Recording: " SendMessage application/x-vnd.BeScreenCapture 'PauC' "

Note: The command constant could change between versions
 
## Authors:

Stefano Ceccherini ( stefano.ceccherini@gmail.com )


## Contributors

* Zumi, Pete Goodeve (icons)
* John Scipione (layout fixes)
* Jessica Hamilton (fixes)
* puckipedia ("incognito" capture mode)

## FAQ:

Q: BeScreenCapture crashed while using the codec \<insert your favourite codec name here\> !!!

A: Some codecs are buggy. Some could cause a crash, or fail to record anything. 


Q: I tried to record a fullscreen clip but the result was crappy!!!

A: Don't expect to record a full screen area on a slow computer and get an incredibly snappy video as a result. Capturing what happens on the screen is a really slow operation. If you get too few frames
per second, try reducing the capture area, or get a faster computer :)


Q: I switched resolution while recording and the resulting clip is crappy!!!

A: Switching resolution while recording isn't supported at the moment.


Q: This readme is lame!!!!

A: You're right. Sorry.


## Acknowledgments

* Marc Flerackers,
* Francois Revol,
* Andrew Bachmann for the help,
* Marcin Konicki for the web space and support,
* Zumi and Pete Goodeve for the icons,
* Every Haiku contributor
* and to anyone I forgot.
