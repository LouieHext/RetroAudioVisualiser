# RetroAudioVisualiser
created by Louie Hext - 03/2021

Live Music Retro Visualiser

video documentation 
(uncompressed HEAVILY RECOMMENDED):

https://www.mediafire.com/file/92pn3fvf25msvnp/2022-04-10_18-00-22_Trim.mp4/file

live demo (cleaner than compressed):

https://vimeo.com/698143588

compressed (butchers the video):

https://vimeo.com/697729912
                      


this code extracts audio features which are then fed into a custom shader
this shader uses dithering and domain warping to create a retro styled visual

the "pitch centroid" of the sound is used to control the power of an FBM noise function in the shader
higher pitches should correspond to smooth curves and lower more noisey ones

the RMS value is used to control the scale of the visual. The louder the closer the visual will appear (as if youre "listening" to the visual)
a beat detection causes the dithering colour pallette to change
these values are averaged over time to give some more stability. 

to use adjust the min and max freq to the relevant domain that you wish to search for beats in
adjust the detection multiplier to define the threshold (the magnitude in the domain needs to be
greater than the previous timeaverage multiplied by the detection multiplier to count as a beat).

adjust the max RMS of the music such that you the visuals pulse in and out
adjust the mac pitch such that the visuals change from smooth -> noisey as the pitch changes

these may need to be adjusted per music genre/song but mainly need to be changed based on audio setup

based upon maxiFeature extraction code (Michael Zbyszyński)
domain warping from inigo Quilez:

https://www.iquilezles.org/www/articles/warp/warp.htm

beat detction algorithm from:

https://en.wikipedia.org/wiki/Beat_detection

dithering from :

http://alex-charlton.com/posts/Dithering_on_the_GPU/
