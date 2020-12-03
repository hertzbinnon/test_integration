#!/bin/bash

for ((i=1;i<22;i++))
do
 name=`printf "%02d\n" $i`
 if [ ! -f $name.pdf ];then
  wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/lec$name.pdf
 fi

done
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/03_math_notes.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/04_notes.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/05_viewing_notes.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/06_splines_notes.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/14_p75_cohen.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/18_notesb.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/18_notesd.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/18_particles.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/Lesseter.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/Subdivision.pdf
wget https://www.cs.cmu.edu/afs/cs/academic/class/15462-f11/www/lec_slides/a1-jensen.pdf
