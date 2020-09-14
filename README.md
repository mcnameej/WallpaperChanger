# Wallpaper Changer

This is an application to change the Windows desktop wallpaper at regular
intervals, switching randomly between a set of images.

### Why?

Windows has had a wallpaper changing feature since at least Windows 7.
So why did I go to the trouble of writing this application?

The problem is that the Windows feature is somewhat limited, and has
actually gotten worse over time.  Windows 7 could use images from
multiple directories, but Windows 10 requires all the images to be
in a single directory.  This isn't how my images are organized.

When I first installed Windows 10 and discovered the functionality
downgrade, I went looking for a third-party solution.  I didn't
find any suitable open source projects, but did find an inexpensive
commercial application, which I purchased.  It worked well, but
every year it decided that my "trial period" had ended, and no
longer accepted my license key.  The vendor provided new keys
several times, but I didn't want to keep doing that.

So I decided to write my own.  I thought "How hard could it be?",
and figured I could bang something out over a weekend.  As you
probably guessed, I was wrong about that!  I ended up hacking on
this for over two weeks.  Ordinarily that wouldn't have been a great
use of my time, but I was [stuck at home with nothing better to do](https://www.google.com/search?q=2020+sucks&tbm=isch).

Nothing about this application was horribly difficult (changing the
wallpaper isn't exactly *rocket surgury*), but I underestimated the
time/effort is required to build a reasonable user interface with
native Win32.  I knew better, but the memory of how tedious Win32
can be had faded.  I've been using WPF or HTML for UI's in recent
years, but they weren't an option for this project.  Low system
overhead was a hard requirement, which meant native Win32.

### Key features

* Play lists.
  * Include files from multiple directories.
  * Add files via UI, or by drag-and-drop from Windows Explorer.
  * Use different play lists for different moods/circumstances.
  * Can designate one play list as "Safe For Work".
* Global hotkeys to change wallpaper at any time.
  * Switch to next/previous image.
  * Switch to "Safe For Work" play list.
    * For those times when you need to share your desktop during a
      conference call, and your current wallpaper may not be suitable
      for all audiences.
* Low system overhead.
  * Under 4 MB private working set.
  * No legacy baggage -- Only supports Windows 10 X64.

### Future plans

* Add "watched directories" to playlists.
  * Automagically keep playlist in sync with directory contents.
  * Need to come up with a good way to handle exceptions, so you
    can remove a file from a playlist and it won't be re-added.
* Avoid creature feep!
  * This is intended to be small focused application.

### Code

* Available on [github](https://github.com/mcnameej/WallpaperChanger).
* Written in C++ 17.
* Built using Visual Studio 2019.
  * VS 2017 should probably work too, with some tweaking of the project file.
* No external dependencies needed to build the application.
  * All dependencies are included with the source code.
  * The [WiX Toolset](https://wixtoolset.org/) is required to build
    the MSI installer.
* Uses WTL (Windows Template Library) for the user interface.
  * Very lightweight compared to MFC, wxWindows, or Qt.
  * Fun fact: Google Chrome also uses WTL.
* BSD-3-Clause [license](LICENSE.txt).
