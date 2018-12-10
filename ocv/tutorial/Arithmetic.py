#!/usr/bin/python
#https://docs.opencv.org/3.4/d0/d86/tutorial_py_image_arithmetics.html
import sys,os
sys.path.append('/home/ocv/installed/lib/python2.7/site-packages')
import cv2 as cv
import numpy as np

#Image Addition 
img1 = cv.imread('../opencv/samples/data/left01.jpg')
print img1.shape
img2 = cv.imread('../opencv/samples/data/left02.jpg')
print img2.shape
dst = cv.addWeighted(img1,0.7,img2,0.3,0)
cv.namedWindow('display',0)
cv.startWindowThread()
cv.imshow('display',dst)
cv.waitKey(0)
cv.destroyAllWindows()
