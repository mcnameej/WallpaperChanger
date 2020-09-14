//////////////////////////////////////////////////////////////////////////////
//
//  Resize.cpp
//
//  Wallpaper bitmap resize implementation.
//
//----------------------------------------------------------------------------
//
//  Copyright 2020 by State University of Catatonia and other Contributors
//
//  This file is provided under a "BSD 3-Clause" open source license.
//  The full text of the license is provided in the "LICENSE.txt" file.
//
//  SPDX-License-Identifier: BSD-3-Clause
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "Resize.h"

// Resize wallpaper bitmap.
bool
ResizeWallpaperBitmap(
    HBITMAP* phBitmap,              // IN: Original bitmap, OUT: Resized bitmap.
    SIZE dstSize,                   // New bitmap size.
    WallpaperResizeMode resizeMode, // Resize mode.
    COLORREF backgroundColor        // Background color.
)
{
    //
    // Get source bitmap information.
    //

    BITMAP srcInfo;
    if (::GetObject(*phBitmap, sizeof(BITMAP), &srcInfo) != sizeof(BITMAP))
    {
        ATLASSERT(false);
        return false;
    }

    SIZE srcSize = { srcInfo.bmWidth, srcInfo.bmHeight };

    //
    // Don't do anything if bitmap is already the right size.
    //

    if (srcSize.cx == dstSize.cx && srcSize.cy == dstSize.cy)
        return false;

    //
    // Create DC for source bitmap.
    //

    CDCHandle srcDC;
    srcDC.CreateCompatibleDC();
    srcDC.SelectBitmap(*phBitmap);

    //
    // Create DC and BITMAP for resized bitmap.
    //

    CBitmapHandle dstBitmap;
    dstBitmap.CreateCompatibleBitmap(srcDC, dstSize.cx, dstSize.cy);

    CDCHandle dstDC;
    dstDC.CreateCompatibleDC();
    dstDC.SelectBitmap(dstBitmap);
    dstDC.SetStretchBltMode(HALFTONE);
    dstDC.SetBrushOrg(0, 0);

    //
    // Initialize the resized bitmap background (if needed).
    //

    if (resizeMode == RESIZE_Center || resizeMode == RESIZE_Fit)
    {
        CBrush backgroundBrush = ::CreateSolidBrush(backgroundColor);
        RECT dstRect = { 0, 0, dstSize.cx, dstSize.cy };
        dstDC.FillRect(&dstRect, backgroundBrush);
    }

    //
    // Resize the bitmap.
    //

    int srcX = 0,
        srcY = 0,
        srcWidth = srcSize.cx,
        srcHeight = srcSize.cy;

    int dstX = 0,
        dstY = 0,
        dstWidth = dstSize.cx,
        dstHeight = dstSize.cy;

    int diffWidth = dstWidth - srcWidth;
    int diffHeight = dstHeight - srcHeight;

    switch (resizeMode)
    {
        case RESIZE_Center:
        {
            // Center the wallpaper image in its original size, filling the remaining
            // area with a solid background color if image is smaller than screen or
            // cropping image if image is larger.

            if (diffWidth < 0)
            {
                // Crop left/right
                diffWidth = abs(diffWidth);
                srcX += diffWidth / 2;
                srcWidth -= diffWidth;
            }
            else if (diffWidth > 0)
            {
                // Center left/right
                diffWidth = abs(diffWidth);
                dstX += diffWidth / 2;
            }

            if (diffHeight < 0)
            {
                // Crop top/bottom
                diffHeight = abs(diffHeight);
                srcY += diffHeight / 2;
                srcHeight -= diffHeight;
            }
            else if (diffHeight > 0)
            {
                // Center top/bottom
                diffHeight = abs(diffHeight);
                dstY += diffHeight / 2;
            }

            ::BitBlt(dstDC,
                     dstX, dstY,
                     srcWidth, srcHeight,
                     srcDC,
                     srcX, srcY,
                     SRCCOPY);

            break;
        }

        case RESIZE_Tile:
        {
            // Tile the wallpaper image, starting in the upper left corner of the screen.
            // This uses the image in its original size. If image is larger than screen,
            // it will be cropped on the right and bottom.

            if (srcWidth > dstWidth && srcHeight > dstHeight)
            {
                // Image is larger than screen -- just crop it.
                ::BitBlt(dstDC,
                         0, 0,
                         dstWidth, dstHeight,
                         srcDC,
                         0, 0,
                         SRCCOPY);
                break;
            }

            // Tile the image.
            for (;;)
            {
                if (dstX >= dstWidth)
                {
                    // Next row
                    dstX = 0;
                    dstY += srcHeight;
                }

                if (dstY >= dstHeight)
                    break;  // We're done!

                int drawWidth = std::min(srcWidth, dstWidth - dstX);
                int drawHeight = std::min(srcHeight, dstHeight - dstY);

                ATLASSERT(drawWidth > 0);
                ATLASSERT(drawHeight > 0);

                ::BitBlt(dstDC,
                         dstX, dstY,
                         drawWidth, drawHeight,
                         srcDC,
                         0, 0,
                         SRCCOPY);

                dstX += srcWidth;
            }

            break;
        }

        case RESIZE_Stretch:
        {
            // Stretch the image to cover the full screen. This can result in distortion
            // of the image as the image's aspect ratio is not retained.

            ::StretchBlt(dstDC,
                         0, 0,
                         dstWidth, dstHeight,
                         srcDC,
                         0, 0,
                         srcWidth, srcHeight,
                         SRCCOPY);
            break;
        }

        default:
        case RESIZE_Fit:
        case RESIZE_Fill:
        case RESIZE_Span:
        {
            double srcAspect = (double) srcWidth  / (double) srcHeight;
            double dstAspect = (double) dstWidth  / (double) dstHeight;

            if (srcAspect == dstAspect)
            {
                // Source and destination have the same aspect ratio.
                // Nothing to adjust.
            }
            else
            {
                double widthRatio  = (double) dstWidth  / (double) srcWidth;
                double heightRatio = (double) dstHeight / (double) srcHeight;

                if (resizeMode == RESIZE_Fit)
                {
                    //
                    // RESIZE_Fit
                    //
                    // Enlarge or shrink the image to fill the screen, retaining the aspect
                    // ratio of the original image. If necessary, the image is padded either
                    // on the top and bottom or on the right and left with the background
                    // color to fill any screen area not covered by the image.
                    //

                    double resizeRatio = std::min(widthRatio, heightRatio);

                    if (resizeRatio == widthRatio)
                    {
                        // Stretch full width.
                        // Borders on top and bottom.

                        dstY = (int) std::round((dstHeight - (srcHeight * resizeRatio)) / 2.0);
                        dstHeight = (int) std::round(srcHeight * resizeRatio);
                    }
                    else if (resizeRatio == heightRatio)
                    {
                        // Stretch full height.
                        // Borders on left and right.

                        dstX = (int) std::round((dstWidth - (srcWidth * resizeRatio)) / 2.0);
                        dstWidth = (int) std::round(srcWidth * resizeRatio);
                    }
                }
                else
                {
                    //
                    // RESIZE_Fill
                    //
                    // Enlarge or shrink the image to fill the screen, retaining the aspect
                    // ratio of the original image. If necessary, the image is cropped either
                    // on the top and bottom or on the left and right to fit the screen.
                    //

                    if (widthRatio < heightRatio)
                    {
                        // Stretch full height.
                        // Crop on left and right.

                        double resizeRatio = heightRatio;
                        int drawWidth = (int) std::round(srcWidth * resizeRatio);
                        int excessWidth = (int) ((drawWidth - dstWidth) / resizeRatio);
                        srcX += excessWidth / 2;
                        srcWidth -= excessWidth;
                    }
                    else if (widthRatio > heightRatio)
                    {
                        // Stretch full width.
                        // Crop on top and bottom.

                        double resizeRatio = widthRatio;
                        int drawHeight = (int) std::round(srcHeight * resizeRatio);
                        int excessHeight = (int) ((drawHeight - dstHeight) / resizeRatio);
                        srcY += excessHeight / 2;
                        srcHeight -= excessHeight;
                    }
                }
            }

            ::StretchBlt(dstDC,
                         dstX, dstY,
                         dstWidth, dstHeight,
                         srcDC,
                         srcX, srcY,
                         srcWidth, srcHeight,
                         SRCCOPY);
            break;
        }
    }

    // Delete DC's
    srcDC.DeleteDC();
    dstDC.DeleteDC();

    // Delete original bitmap
    ::DeleteObject(*phBitmap);

    // Return the resized bitmap.
    *phBitmap = dstBitmap.Detach();

    // Indicate that bitmap was resized.
    return true;
}
