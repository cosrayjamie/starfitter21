#!/usr/bin/env python
import cv2
import numpy as np
from pyzbar.pyzbar import decode

#import png

#img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/QRCode1DISTORTED.png")
img = cv2.imread("C:/Users/JAZ/github/Starfitter/src/OpenCV/20180920_00_0_25_0001QR.png")

code = decode(img)

#tells us the position and the size of the QR code.
#(barcode.rect) = left value, top value, width, and height of bounding box
#(barcode.polygon) = gives coordinates of QR corners
for barcode in decode(img):
    print(barcode.polygon)
    #prints message of QR code
    print(barcode.data)
    myData = barcode.data.decode('utf-8')
    

print(code)

cv2.imshow("Image", img)
cv2.waitKey(0)

#https://www.youtube.com/watch?v=-4MPtERPq2E


if __name__ == '__main__' :

    # Read source image.
    im_src = cv2.imread('book2.jpg')
    # Four corners of the book in source image
    pts_src = np.array([[141, 131], [480, 159], [493, 630],[64, 601]])


    # Read destination image.
    im_dst = cv2.imread('book1.jpg')
    # Four corners of the book in destination image.
    pts_dst = np.array([[318, 256],[534, 372],[316, 670],[73, 473]])

    # Calculate Homography
    h, status = cv2.findHomography(pts_src, pts_dst)
    
    # Warp source image to destination based on homography
    im_out = cv2.warpPerspective(im_src, h, (im_dst.shape[1],im_dst.shape[0]))
    
    # Display images
    cv2.imshow("Source Image", im_src)
    cv2.imshow("Destination Image", im_dst)
    cv2.imshow("Warped Source Image", im_out)

    cv2.waitKey(0)

