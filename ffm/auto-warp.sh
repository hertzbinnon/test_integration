#!/bin/bash
#https://stackoverflow.com/questions/43413445/how-to-add-a-2d-texture-to-an-equirectangular-image
#----------- Extract image section ------------
# command  test-base.png .\ 10 10
echo Centering image on desired point...
./ffmpeg -hide_banner  -i $2$1 -vf v360=e:e:yaw=-$3:pitch=-$4 -y $2test_equi_rotated-$1
echo --

echo Extracing region to edit...
./ffmpeg -hide_banner  -i $2test_equi_rotated-$1 -vf v360=e:flat -y $2test_crop_flat-$1

echo Please write your text onto "test_crop_flat-$1"
#set /p=Hit ENTER to continue...
./ffmpeg -hide_banner  -i $2test_crop_flat-$1 -vf "drawtext=text='Test Text':fontcolor=white:fontsize=75:x=1002:y=100:"  tmp.png
mv tmp.png $2test_crop_flat-$1
echo --

#-------- Convert edited extracted section back to equirectangular ---------------
echo Converting crop+text back to equirectangular...
./ffmpeg  -hide_banner  -i $2test_crop_flat-$1 -vf v360=flat:e -y $2test_crop_equi-$1
echo --

echo Please save "$2test_crop_equi-$1" with transparency
#set /p=Hit ENTER to continue...
./ffmpeg -y -f lavfi -i "color=color=white@0.0:size=3840x1920,format=rgba" -q:v 2 -vframes 1 -r 1  -c:v png  -update 1 alpha.png
./ffmpeg -i $2test_crop_equi-$1 -i alpha.png  -filter_complex overlay tmp1.png
mv tmp1.png  $2test_crop_equi-$1
echo --
#--------- Overlay edited extracted section on original image --------------

echo Merging edited crop into rotated image...
./ffmpeg  -hide_banner   -i $2test_equi_rotated-$1 -i  $2test_crop_equi-$1 -filter_complex "overlay" -y $2test_merge_rotated-$1
echo --

echo Rotating the image back to original direction:
./ffmpeg  -hide_banner   -i $2test_merge_rotated-$1 -vf v360=e:e:yaw=$3:pitch=$4:roll=2.1  -y $2test_merge-$1
echo --
#@del $2test_merge_rotated-$1

echo Output saved in "test_merge-$1"
