## This manual applies to use with Handbrake, Ffmpeg and Imagemagick, for example, but of course any alternative can be used.

### For Linux

1. To compile the utility itself, you can use e.g.
```
$ gcc -o PicoPadVideo PicoPadVideo.cpp
```
or
```
$ clang -o PicoPadVideo PicoPadVideo.cpp
```

2. First extract the audio from the video using ffmpeg
```
$ ffmpeg -i input_video_file -vn -ar 22050 -acodec pcm_u8 -ac 1 SOUND.wav
```

3. Next, edit the video in a program such as Handbrake to a size of 160×240 and 10 FPS (frames per second)

4. Then we extract the individual fields from the video to a BMP folder
```
$ ffmpeg -i input_video_file -vf fps=10 BMP/%06d.bmp
```

5. And convert to RGB525 16-bit 
```
$ cd BMP; ls -1 ../BMP | xargs -I {} -n1 convert -colors 256 -define bmp:format=bmp3 -compress none ../BMP/"{}" "{}"; cd ..
```

6. Now we can use the PicoPadVideo utility itself
The utility assumes "Flip row oder" by default. So for our case we use the --bmpisnotbat argument
```
$ PicoPadVideo --bmpisnotbat
```

If everything went correctly, you should now have a VIDEO.VID file in your directory. This is your coveted video that
you can rename and upload to your SD card in the default /VIDEO folder and run with VIDEO.UF2 in Picopad
