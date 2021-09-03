/********************************************************************************
This is a library for converting images already located in RAM to 
lower bit-depth versions with different kinds of dithering algorithms.

WARNING: These algorithms operate on the microcontroller's RAM buffer
			   in order to apply changes, so a fair amount of RAM (at least as big 
				 as the image buffer itself) is required to avoid crashes.


Copyright (c) 2021 Leopoldo Perizzolo - Deep Tronix
Find me @ https://rebrand.ly/deeptronix

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************************/

#if (ARDUINO >= 100)
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#define END (-32)
#define is_2s_pow(number)  !((number) & ((number) - 1))

class Dither {
 public:
  Dither(uint16_t width = 0, uint16_t height = 0, bool invert_output = false);
  
  void updateDimensions(uint16_t new_width, uint16_t new_height);
	uint16_t getWidth();
	uint16_t getHeight();
	void reRandomizeBuffer();
 	
  int8_t FSDither(uint8_t *IMG_pixel, uint8_t quant_bits = 1);
  int8_t JJNDither(uint8_t *IMG_pixel, uint8_t quant_bits = 1);
  int8_t StuckiDither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t BurkesDither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t Sierra3Dither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t Sierra2Dither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t Sierra24ADither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t AtkinsonDither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
	int8_t PersonalFilterDither(uint8_t *IMG_pixel, uint8_t quantization_bits = 1);
  
  void fastEDDither(uint8_t *IMG_pixel);				 	// Time complexity is O(3n), but also optimized for faster calculations and array accesses (especially on low-end uCs).
  #define fastEDDither_remove_artifacts  false		// making this true will make the above algorithm O(4n), but will reduce artifacts visible when images are bigger than roughly 8000 pixels (x*y).
  
  void buildClusteredPattern();		// Time complexity is O(1).
  void buildBayerPattern();		// Time complexity is O(1).
  int8_t patternDither(uint8_t *IMG_pixel, int8_t thresh = 0);		// Time complexity is O(n).
  
  int8_t randomDither(uint8_t *IMG_pixel, bool time_consistency = true, int8_t thresh = 0);	  // time-consistency enabled by default (faster speed); 	Time complexity is O(n).
  
  void thresholding(uint8_t *IMG_pixel, uint8_t thresh = 128);	// Time complexity is Theta(n).
  // void thresholding(uint8_t *IMG_pixel);										// Overloaded function - No longer implemented
  
  
  // Helping functions (public)
  uint32_t index(int x, int y);
  uint32_t index(int x, int y, uint8_t pix_len);
  uint8_t color888ToGray256(uint8_t r, uint8_t g, uint8_t b);
  void colorGray256To888(uint8_t color, uint8_t &r, uint8_t &g, uint8_t &b);
  bool colorGray256ToBool(uint8_t gs);
  inline void quantize(uint8_t quant_bits, uint8_t &r, uint8_t &g, uint8_t &b);
  inline void quantize_BW(uint8_t &c);
  
  void color565To888(uint16_t color565, uint8_t &r, uint8_t &g, uint8_t &b);
  uint16_t color888To565(uint8_t r, uint8_t g, uint8_t b);
  void color332To888(uint8_t color332, uint8_t &r, uint8_t &g, uint8_t &b);
  uint8_t color888To332(uint8_t r, uint8_t g, uint8_t b);
  void colorBoolTo888(bool color1, uint8_t &r, uint8_t &g, uint8_t &b);
  bool color888ToBool(uint8_t r, uint8_t g, uint8_t b);
  
  
private:
  uint16_t _img_width, _img_height;
  bool _invert_output;
  
  // For Error Distribution algorithms
  int8_t _GPEDDither(uint8_t *IMG_pixel, uint8_t quantization_bits, uint8_t filter_index);	// GPED (dithering) : General Purpose Error Distribution (dithering)
  #define max_filter_entries 16			// Max filter entries per line; this parameter is needed due to limitations in C++, that cannot recognize on its own when a line ends.
  #define filter_types 9
  const int8_t _filters[filter_types][max_filter_entries] = {		// when -n, that's the number of columns we have to go back from the current pixel, on the next row.
  	{16, 7, -1, 3, 5, 1, END},																	//  Floyd-Steinberg filter
  	{48, 7, 5, -2, 3, 5, 7, 5, 3, -2, 1, 3, 5, 3, 1, END},			//  Jarvis, Judice and Ninke filter
  	{42, 8, 4, -2, 2, 4, 8, 4, 2, -2, 1, 2, 4, 2, 1, END},			//  Stucki filter
  	{32, 8, 4, -2, 2, 4, 8, 4, 2, END},										 	 		//  Burkes filter
  	{32, 5, 3, -2, 2, 4, 5, 4, 2, -1, 2, 3, 2, END},		  			//  Sierra3 filter
  	{16, 4, 3, -2, 1, 2, 3, 2, 1, END},													//  Sierra2 filter
  	{4, 2, -1, 1, 1, END}, 																			//  Sierra-2-4A filter
  	{8, 1, 1, -1, 1, 1, 1, -1, 0, 1, END},											//  Atkinson filter
  	{8, 1, 1, -1, 0, 1, 1, END},																//  Personal filter
  };
  #define FSf			0
  #define JJNf		1
  #define STUf		2
  #define BURf		3
  #define SIE3f		4
  #define SIE2f		5
  #define SIE24f	6
  #define ATKf		7
  #define PERf		8
  
  
  
  // For Halftoning algorithms
  #define _size 2														// Halftoning pattern size (square convolutional matrix); minimum is 1. The number of output gray shades obtainable is [1 + (_size)^2]. Values of _size powers of 2 are recommended on non-math enhanced or slow processors.
  uint8_t _pattern[_size][_size] = {{0}};		// Halftoning pattern array
  
  // For Thresholding and Random dithering
  #define _rnd_frame_width  1024	// use ONLY powers of 2 ; recommended a value twice as big as the image width (see documentation)
  uint8_t _rnd_frame[_rnd_frame_width];
  #define _use_low_amplitude_noise  true		// Usually, low amplitude noise is best (resembles more Gaussian distribution). Only sometimes high amplitude noise will result in a more pleasing image.
  
  // Helping functions (private)
	uint8_t _Rnd(uint8_t seed = 150);
  inline uint8_t _twos_power(uint16_t number);
  inline uint8_t _clamp(int16_t v, uint8_t min, uint8_t max);

};

