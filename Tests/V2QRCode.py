import cv2
import numpy as np
from pyzbar.pyzbar import decode
from matplotlib import pyplot as plt
import random

#Questions:
#Order of QR corner readout
#Python finds substring to read part of QR code
#Look at barcode.polygon elements

#Sets counter for QR corners, to check to see if it found 16.
MIN_MATCH_COUNT = 0

xm = 0
ym = 0

#Screen information, in mm ("True Coordinates"), upper-left corner is origin. Order: UL, LL, UR, LR "Destination"
#upper edge of screen is horizontal, lower-left corner is negative because it's not perfectly rectangular
ScreenCornersmm = np.array([[0.0,0.0],[-2.8,616.5],[746.6,0.0],[745.8,615.8]],np.float32)

ScreenCornerspx = np.array([[0,0],[50,800],[1200,200],[1500,1200]],np.int32)

retval, mask = cv2.findHomography(ScreenCornerspx, ScreenCornersmm, cv2.RANSAC,5.0)


#QR Code Coordinates in mm in relation to screen UL, LL, UR, LR
QRCornersmm = np.array([[10.7,7.7],[6.9,536.6],[675.2,9.5],[675.9,541.3],
                     [12.7,68.6],[7.1,603.3],[673.6,68.8],[674.6,600.7],
                     [71.5,5.3],[66.1,543.7],[734.6,11.2],[735.2,542.6],
                     [73.6,66.7],[66.5,603.2],[732.7,70.7],[733.9,602.0]],np.float32)

#bogus entries

QRCornerspx = np.array([[10,7],[6,536],[675,9],[675,541],
                     [12,68],[7,603],[673,68],[674,600],
                     [71,5],[66,543],[734,11],[735,542],
                     [73,66],[66,603],[732,70],[733,602]],np.int32)
    
#QRCornerspx will be decoded QR corners, below.

#img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/QRCode1DISTORTED.png")
#img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/20180920_00_0_25_0001QR.png")
img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/0MDTAx4/20180920_00/20210127_133346.jpg")

#Gets x,y coordinates of QR code. Use print(code) later to print garbled paragraph of info.
code = decode(img)

#Tells us the position and the size of the QR code.
#   print(barcode.rect) gives left value,top value, width, and height of bounding box
#print(barcode.polygon) = gives coordinates of QR corners

for barcode in decode(img):
    #Decodes barcode and converts it into a string)
    myData = barcode.data.decode('utf-8')

  
    #Puts polygon coordinates of QR code in np.array named pts.
    pts = np.array([barcode.polygon],np.int32)
    print(pts.shape)
    print(QRCornerspx.shape)

    #Computer average x and y of QR Code coordinates. 
    xm = (pts[0][0][0] + pts[0][1][0]+pts[0][2][0]+pts[0][3][0])/4
    ym = (pts[0][0][1]+pts[0][1][1]+pts[0][2] [1]+pts[0][3][1])/4

    #Determine which QR coordinates are UL, LL, UR, and LR
    

    

    if myData.find('upper-left') > -1:
        x = pts[0][0][0]
        y = pts[0][0][1]

        QRCornerspx[0] = pts[0][0] #UL corner of UL QR Code
        QRCornerspx[1] = pts[0][1] #LL corner of UL QR Code
        QRCornerspx[2] = pts[0][2] #UR corner of UL QR Code
        QRCornerspx[3] = pts[0][3] #LR corner of UL QR Code
        
    if myData.find('Lower-left') > -1:
        QRCornerspx[4] = pts[0][0] #UL corner of LL QR Code
        QRCornerspx[5] = pts[0][1] #LL corner of LL QR Code
        QRCornerspx[6] = pts[0][2] #UR corner of LL QR Code
        QRCornerspx[7] = pts[0][3] #LR corner of LL QR Code
    
    if myData.find('Upper-right') > -1:
        QRCornerspx[8] = pts[0][0] #UL corner of UR QR Code
        QRCornerspx[9] = pts[0][1] #LL corner of UR QR Code
        QRCornerspx[10] = pts[0][2] #UR corner of UR QR Code
        QRCornerspx[11] = pts[0][3] #LR corner of UR QR Code
        
    if myData.find('Lower-right') > -1:
        QRCornerspx[12] = pts[0][0] #UL corner of LR QR Code
        QRCornerspx[13] = pts[0][1] #LL corner of LR QR Code
        QRCornerspx[14] = pts[0][2] #UR corner of LR QR Code
        QRCornerspx[15] = pts[0][3] #LR corner of LR QR Code

    MIN_MATCH_COUNT = MIN_MATCH_COUNT + 4

    #This reshapes the 4 QR corners into a perfect square. I'm not done experimenting with this,
    #but we probably don't want it anyway. But maybe it can act as a mini-homography?
    #pts = pts.reshape((-1,1,2))   


    #put QRcorners into

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

#MIN_MATCH_COUNT check, to see if program found 16 corners
print(MIN_MATCH_COUNT)
   


#Homography Transform
retval, mask = cv2.findHomography(QRCornerspx, QRCornersmm, cv2.RANSAC,5.0)
print(QRCornerspx)
print(retval)

print("Number of elements in pts:", len(pts))
print("Number of elements in QRCornerspx:", len(pts))
print("Code", code)


#Creates plot of image.
plt.imshow(img),plt.show()
cv2.imshow("Image", img)
cv2.waitKey(0)

#Good videos to watch.
#https://www.youtube.com/watch?v=SrZuwM705yE
#https://www.youtube.com/watch?v=-4MPtERPq2E
