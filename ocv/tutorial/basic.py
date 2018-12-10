#!/usr/bin/python
# refers to https://docs.opencv.org/3.4/d3/df2/tutorial_py_basic_ops.html
#
#  RGB 			= [  [ [b,g,r],[b,g,r],[b,g,r],...   ],[],[], ... , []  ] ==> 3d list
#  grayscale    = [  [ intensity,intensity,    ...   ],[],[], ... , []  ] ==> 2d list
#
import sys,os,numpy
sys.path.append('/home/ocv/installed/lib/python2.7/site-packages')
import cv2
#### Process ####
#lists=[[[1,2,3],[4,5,6]],[[4,5,6],[1,2,3]]]
img = cv2.imread('../opencv/samples/data/messi5.jpg')
print type(img)
print img.shape
print img.size
print img.dtype
#for px in img[0]: 
	#print px
ball = img[280:340,330:390]
img[273:333,100:160] = ball
#costly operation
#b,g,r = cv2.split(img)  
# Numpy indexing is faster
b = img[:,:,0] # 
print b.shape
print b[0][2] # == b[0,2]
g = img[:,:,1]
r = img[:,:,2]
img = cv2.merge((b,g,r))
img[:,:,2] = 0
img[:,:,1] = 0
#img[:,:,0] = 2
# modifying pixel RED Value 
img.item(10,10,2)
img.itemset((10,10,2),200)
img.item(10,10,2)


#### display #### 
### way2: can close the window 
##       can change the shape of the window
cv2.namedWindow('display',0) 
cv2.startWindowThread()
### way2: can close the window 
##       can not change the shape of the window
cv2.imshow('display',img)
cv2.waitKey(0)
cv2.destroyAllWindows()
