import cv2
import numpy as np
from pyzbar.pyzbar import decode
from matplotlib import pyplot as plt
import random

xm = 0
ym = 0

img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/QRCode1DISTORTED.png")
#img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/0MDTAx4/20180920_00/20210127_133346.jpg")

code = decode(img)
whichQR = "unidentified"

for barcode in decode(img):
    #Decodes barcode and converts it into a string)
    myData = barcode.data.decode('utf-8')

  
    #Puts polygon coordinates of QR code in np.array named pts. **No particular order.**
    pts = np.array([barcode.polygon],np.int32)

    #Determine which corners of the barcode are UL, LL, UR, LR.
    x = pts[0][0][0]
    y = pts[0][0][1]

    print(x)
    print(y)
       
    print(pts[0][0][0])
    print(pts[0][0][1])
    print(pts[0][1][0])
    print(pts[0][1][1])
    print(pts[0][2][0])
    print(pts[0][2][1])
    print(pts[0][3][0])
    print(pts[0][3][1]) 
    print("\n")

    #prints message of QR code and coordinate array of each.
    print(myData)   
    print(pts)
    print("\n")

  
 

        #Draws lines around polygon. True means it's a closed polygon, color, thickness.
    #"myData" in cv2.putText means that the QR message of that QR code is printed directly on the image.
    cv2.polylines(img,[pts],True,(255,0,0),3)
    pts2 = np.array([barcode.polygon],np.int32)
    cv2.putText(img, myData,(pts2[0][0][0],pts2[0][0][1]),cv2.FONT_HERSHEY_SIMPLEX, 0.5,(0,0,255),2)
    #cv2.putText(img, myData,(pts2[0][0][0],pts2[0][0][1]),cv2.FONT_HERSHEY_SIMPLEX, 0.9,(0,0,255),3)


#Creates plot of image.
plt.imshow(img),plt.show()
cv2.imshow("Image", img)
cv2.waitKey(0)
