//////////////////////////////////////////////////////////////////////////////
//
//  Resize.h
//
//  Wallpaper bitmap resize declarations.
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

#pragma once

// Wallpaper bitmap resize mode.
enum WallpaperResizeMode
{
    // Center the wallpaper image in its original size, filling the remaining
    // area with a solid background color if image is smaller than screen or
    // cropping image if image is larger. (Equivalent to DWPOS_CENTER and
    // WPSTYLE_CENTER.)
    RESIZE_Center = 0,

    // Tile the wallpaper image, starting in the upper left corner of the screen.
    // This uses the image in its original size. If image is larger than screen,
    // it will be cropped on the right and bottom. (Equivalent to DWPOS_TILE and
    // WPSTYLE_TILE.)
    RESIZE_Tile = 1,

    // Stretch the image to cover the full screen. This can result in distortion
    // of the image as the image's aspect ratio is not retained. (Equivalent to
    // DWPOS_STRETCH and WPSTYLE_STRETCH.)
    RESIZE_Stretch = 2,

    // Enlarge or shrink the image to fill the screen, retaining the aspect
    // ratio of the original image. If necessary, the image is padded either
    // on the top and bottom or on the right and left with the background
    // color to fill any screen area not covered by the image. (Equivalent
    // to DWPOS_FIT and WPSTYLE_KEEPASPECT.)
    RESIZE_Fit = 3,

    // Enlarge or shrink the image to fill the screen, retaining the aspect
    // ratio of the original image. If necessary, the image is cropped either
    // on the top and bottom or on the left and right as necessary to fit
    // the screen. (Equivalent to DWPOS_FILL and WPSTYLE_CROPTOFIT.)
    RESIZE_Fill = 4,

    // Span the image across all monitors. ResizeWallpaperBitmap() treats
    // this the same way as RESIZE_Fill, but Windows does the spanning
    // when it displays the wallpaper. (Equivalent to DWPOS_SPAN.)
    RESIZE_Span = 5
};

// Resize wallpaper bitmap.
bool
ResizeWallpaperBitmap(
    HBITMAP* phBitmap,                              // IN: Original bitmap, OUT: Resized bitmap.
    SIZE dstSize,                                   // New bitmap size.
    WallpaperResizeMode resizeMode = RESIZE_Fill,   // Resize mode.
    COLORREF backgroundColor = 0                    // Background color.
);
