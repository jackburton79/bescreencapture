[![Codacy Badge](https://api.codacy.com/project/badge/Grade/63f373e0c5c04abfa329e6d505d1f014)](https://app.codacy.com/gh/jackburton79/bescreencapture?utm_source=github.com&utm_medium=referral&utm_content=jackburton79/bescreencapture&utm_campaign=Badge_Grade_Settings)
[![CodeFactor](https://www.codefactor.io/repository/github/jackburton79/bescreencapture/badge)](https://www.codefactor.io/repository/github/jackburton79/bescreencapture)

# BeScreenCapture

![Screenshot](https://raw.github.com/jackburton79/bescreencapture/master/BeScreenCapture.png) ![Screenshot](https://raw.github.com/jackburton79/bescreencapture/master/BeScreenCapture-options.png)

BeScreenCapture lets you record what happens on your screen and save it to a clip in any media format supported by Haiku.

The recorded clip can be scaled to suit your needs.

Includes an "Incognito" mode, which hides the application window and the deskbar icon, to be able to record clips by using a keyboard combo and without leaving any trace.
This functionality requires installing the bescreencapture_inputfilter and using the CTRL-ALT-SHIFT-r shortcut, which launches BeScreenCapture and start recording directly. Hit CTRL-ALT-SHIFT-r again to stop.

BeScreenCapture is also scriptable with hey:

Start recording

 hey BeScreenCapture DO Record

Stop recording

 hey BeScreenCapture DO Stop
 
Get capture rect 

 hey BeScreenCapture GET CaptureRect

Set capture rect

 hey BeScreenCapture SET CaptureRect to "BRect(0,0, 200,300)"

Get scale

 hey BeScreenCapture GET Scale

Set scale to 50%

 hey BeScreenCapture SET Scale to 50

Set recording time to 5 seconds

 hey BeScreenCapture SET RecordingTime to 5

You can also define your own shortcuts in the "Shortcuts" preflet:

* Start/Stop Recording: " SendMessage application/x-vnd.BeScreenCapture 'StoR' "
* Pause/Resume Recording: " SendMessage application/x-vnd.BeScreenCapture 'PauC' "

## Notes

* Command constants could change in the next releases.
* It seems that the most reliable codec / format to use for encoding, in current Haiku nightlies, is MPEG/Mpeg4. Other codec/format combos may cause various kinds of problems.
* Beware: The GIF export option is very slow and uses a lot of memory.

## Authors

Stefano Ceccherini

## Contributors

* Zumi, Pete Goodeve (icons)
* John Scipione (layout fixes)
* humdinger (layout fixes, other)
* Jessica Hamilton (fixes)
* puckipedia ("incognito" capture mode)

## Acknowledgements

* Marc Flerackers,
* Francois Revol,
* Andrew Bachmann for the help,
* Marcin Konicki for the web space and support,
* Zumi and Pete Goodeve for the icons,
* Every Haiku contributor
* and to anyone I forgot
