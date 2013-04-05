# BeScreenCapture
by Stefano Ceccherini ( stefano.ceccherini@gmail.com )

![Screenshot](https://raw.github.com/jackburton79/doctor-who/master/BeScreenCapture.png)


BeScreenCapture lets you record what happens on your screen and save it to a clip in any media format you want, as long as it's supported by Haiku.


## FAQ:


Q: I have the codec <insert your favourite codec name here> but it's not listed in BeScreenCapture!!!

A: Not every codec supports every clip configuration: some codecs can only support some particular
color depth, or certain particular sizes (i.e. 320x200): try changing the "Clip color depth" option in the
"Advanced" settings. The default (32 bit), though, should work for most codecs.


Q: BeScreenCapture crashed while using the codec <insert your favourite codec name here> !!!

A: There are many buggy codecs around. I've seen many codecs which simply refuse to record anything,
and in some cases, codecs which simply crash (3ivx, for example). 


Q: I tried to record a fullscreen clip but the result was crappy!!!

A: Don't expect to record an area like 1024x768 with a 32 bit depth and get an incredibly snappy video
as a result. Capturing what happens on the screen is a really slow operation. If you get too few frames
per second, try reducing the capture area or the screen depth. Or get a faster computer :)


Q: I switched resolution while recording and the resulting clip is crappy!!!

A: Switching resolution while recording isn't supported (at least at the moment).


Q: This readme is lame!!!

A: You're right. Sorry.


Thanks to:
* Marc Flerackers,
* Francois Revol,
* Andrew Bachmann for the help,
* Marcin Konicki for the web space and support,
* Zumi for the icons,
* and to anyone I forgot.
