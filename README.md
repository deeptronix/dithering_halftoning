# Dithering & Halftoning library
#### Developed by [Deep Tronix](https://www.rebrand.ly/deeptronix) 2018~2021

A (hopefully comprehensive) guide to this library

## Error diffusion dithering algorithms

The main goal of this library was to recreate the most popular error-diffusion algorithms (filters) developed and used back in the days where this technique was used the most.

To do so, I researched the web; back in 2018, when I started this project, a website (no longer alive today) explained in detail the different error-diffusion filters and their corresponding weights.
I managed to save a copy of the content; it can now be found [here](https://deeptronix.files.wordpress.com/2021/02/digital-halftoning-lee-daniel-crocker.pdf).

Following that document, I managed to develop the different algorithms, now collected in the presented library.
The core of the error diffusion algorithms is a function called “\_GPEDDither”, which stands for “General Purpose Error Diffusion Dithering” (the underscore highlights that it is under the private section).

As a way to keep in order the various algorithms, I decided to call it only from specific functions, which have the names of the filter applied.
The available error-diffusion dithering algorithms are:

- Floyd-Steinberg
- Jarvis, Judice, and Ninke
- Stucki
- Burkes
- Sierra3
- Sierra2
- Sierra-2-4A
- Atkinson (not found in the document, but available [here](http://verlagmartinkoch.at/software/dither/index.html#img:~:text=BILL%20ATKINSON%20ALGORITHM,1/8))
- a personal filter, of which the coefficients can be customized, for example by taking inspiration from other filters. By default, the values are set up so as to emulate a simplified and more “constrasty” version of the Atkinson filter.

Since the GPEDDither function is the same for all the algorithms used, there are two key points to notice:

- The function cannot easily be optimized any further, without knowing either the microcontroller's instruction set or other simplifications. For this reason, the function is clearly not as efficient as a filter-specific version of the same.
- The filter coefficients are stored in an array (actually, a matrix) in the “Dither.h” file; the arrangement can seem a little confusing at start, hence I decided to dedicate the next section to explain it, and also allow for editing.

**Please note**: all of the functions accept a second parameter, after the image array pointer, called “quantization\_bits”. This value IS NOT BEING USED in the functions; it's there to allow for further library expansions, when either me or other contributors will decide to deal with more than 1 bit gray shades (1 = monochrome) and even colors. On this note, you can also see that the function seems ready to also accept color inputs (on many lines, green and blue color variables have been commented out, but are there); however, I could not test the library with those parameters for a lack of time and hardware resources, so I decided to leave them disabled.

**Coefficient arrangement** for different error-diffusion algorithms (found in Dither.h):

Example:

const int8\_t \_filters[filter\_types][max\_filter\_entries] = {

`	`{16, 7, -1, 3, 5, 1, END},			//  Floyd-Steinberg filter

`	`{8, 1, 1, -1, 1, 1, 1, -1, 0, 1, END},		//  Atkinson filter

`	`…

};

teset
