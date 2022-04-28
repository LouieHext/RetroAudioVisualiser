# RetroAudioVisualiser
created by Louie Hext - 03/2021


Description
============
A real-time reactive audio visualiser. Using the maximillian library the program extracts beats, volume and pitch
of the incoming audio. The beat detector works by defining a frequency domain and compraring the average magnitude in that domain 
against a previous time average, if it sufficiently large a beat is detected. The pitch is estimaed by taking the centroid
of the ofxMaxmim octave analyiser, and the volume by calculating the RMS of the audio signal. These values are then
fed into a shader as parameters. The shader is generated purely by manipulating simplex noise. 
Initially it is passed into a FBM function (combining higher freq with lower amps), before distroting this
FBM noise with a series of domain warps which can be thought of as using the operation f(p) -> f(p + f(p + dp)).
This create a series of distorted and undulating shapes that pulse and shift with the music. This is then fed into a 
dithering routine which gives the "retro-ness" to the visuals.

video links
===========

video documentation (uncompressed HEAVILY RECOMMENDED for visual clarity):

https://www.mediafire.com/file/92pn3fvf25msvnp/2022-04-10_18-00-22_Trim.mp4/file

live demo (cleaner than compressed):

https://vimeo.com/698143588

compressed (butchers the video):

https://vimeo.com/697729912


Reference links
===========
based upon maxiFeature extraction code (Michael Zbyszyński)

domain warping from inigo Quilez:

https://www.iquilezles.org/www/articles/warp/warp.htm

beat detction algorithm from:

https://en.wikipedia.org/wiki/Beat_detection

dithering from :

http://alex-charlton.com/posts/Dithering_on_the_GPU/
