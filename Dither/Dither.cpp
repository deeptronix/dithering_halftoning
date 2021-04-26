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

#include <stdlib.h>
#include "Dither.h"

	// input image format MUST BE 256 shades of gray per pixel (monochrome). Use helper funtions (at the end of file) to up/downconvert the image if needed.
Dither::Dither(uint16_t width, uint16_t height, 	// image parameters, used to define image boundaries
							 bool invert_output){								// choose whether to use output for display (invert = 0; set by default) or printers (invert = 1)

	//	For some reason, operations in this constructor must be as few as possible, otherwise it does not run properly. So, try to keep very few lines of code here.
	
	// Load the given image parameters to private variables
	_img_width = width;
	_img_height = height;
	_invert_output = invert_output;
	
	// create a random buffer of values for temporal consistency used, if enabled, for random dithering
  for(uint16_t i = 0; i < _rnd_frame_width; i++){
  	_rnd_frame[i] = _Rnd();
  	delayMicroseconds(3);
  }
  
}



// ERROR-DIFFUSION Algorithms

// Standard Floyd-Steinberg dithering filter
int8_t Dither::FSDither(uint8_t *IMG_pixel, uint8_t quantization_bits){  // quantization_bits: number of bits between 1 and 7 used to represent the OUTPUT grayshades
	return _GPEDDither(IMG_pixel, quantization_bits, FSf);
}

// Jarvis, Judice, and Ninke filter
int8_t Dither::JJNDither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, JJNf);
}

// Stucki filter
int8_t Dither::StuckiDither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, STUf);
}

// Burkes filter
int8_t Dither::BurkesDither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, BURf);
}

// Sierra 3 filter
int8_t Dither::Sierra3Dither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, SIE3f);
}

// Sierra 2 filter
int8_t Dither::Sierra2Dither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, SIE2f);
}

// Sierra 2-4A filter
int8_t Dither::Sierra24ADither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, SIE24f);
}

// Atkinson filter
int8_t Dither::AtkinsonDither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, ATKf);
}

// Personal filter
int8_t Dither::PersonalFilterDither(uint8_t *IMG_pixel, uint8_t quantization_bits){
  return _GPEDDither(IMG_pixel, quantization_bits, PERf);
}


// General Purpose Error Distribution dithering structure
int8_t Dither::_GPEDDither(uint8_t *IMG_pixel, uint8_t quantization_bits, uint8_t filter_index){
	
  if(quantization_bits < 1  ||  quantization_bits > 7)  return -1;	// quantization bits not valid
  
  uint8_t r;	// , g, b;
  uint8_t newr, dummy; // , newg, newb;
  int16_t temp_r;	// , temp_g, temp_b;
  int16_t quant_err_r;	// , quant_err_g, quant_err_b;
  uint32_t ind;
  
  // Preamble: fill array with weights selected by the chosen algorithm
  int8_t weights[max_filter_entries];
  int8_t curr_weight;
  uint8_t len = 1, divisor = _filters[filter_index][0];
  uint8_t max_filt_height = 0, max_filt_width = 0;
  
  bool bitshift_avail = 0;
  uint8_t bitshift_div = 0;
  if(is_2s_pow(divisor)){
  	bitshift_avail = 1;
  	bitshift_div = _twos_power(divisor);
  }
  
	for(; _filters[filter_index][len] > END; len++){
		weights[len] = _filters[filter_index][len];
		if(weights[len] > 0){
			if(max_filt_height == 0)  max_filt_width++;	 // find the filter right-side length; assuming the longest row is the first one.
		}
		else  max_filt_height++;	// find the filter height
	}
	
	
  for(uint16_t row = 0; row < _img_height; row++){
    for(uint16_t col = 0; col < _img_width; col++){
  		
  		ind = index(col, row);
  		// Fetch pivot pixel, after conversion to RGB888
      colorGray256To888(IMG_pixel[ind], r, dummy, dummy);
      //colorGray256To888(IMG_pixel[index(col, row)], r, g, b);
      
      // Save value for later
      newr = r;
      // newg = g;
      // newb = b;

      // Quantize value
      quantize(quantization_bits, newr, dummy, dummy);
      // quantize(quantization_bits, newr, newg, newb);

			// Calculate error caused by quantization
      quant_err_r = r - newr;
      // quant_err_g = g - newg;
      // quant_err_b = b - newb;
      
      // If output is supposed to be inverted (see constructor)
      if(_invert_output){
				newr = 0xFF - newr;
				// newg = 0xFF - newg;
				// newb = 0xFF - newb;
			}
			
			// Set the quantized pixel
      IMG_pixel[ind] = color888ToGray256(newr, newr, newr);
      // IMG_pixel[index(col, row)] = color888ToGray256(newr, newg, newb);
      
      // Distribute error amongst neighbours
      int8_t row_offs = 0, col_offs = 1;
      for(uint8_t p = 1; p < len; p++){	//  fetch current filter weight
      	
    	  // adjacent pixels work starts here. First, check if away from edge cases:
				if(row < (_img_height - max_filt_height)  &&  col < (_img_width - max_filt_width)  &&  col > 0){
					
					// Fetch current dithering weight
					curr_weight = weights[p];
	      	
	      	if(curr_weight < 0){		// Negative values in the weights array indicates to go down to the next line, and by which amount to the left
						col_offs = curr_weight;
						row_offs++;			// linefeed
					}
					else{		// If not outside image boundaries
		      	
						ind = index(col + col_offs, row + row_offs);
		      	col_offs++;
		      	
		        // Fetch pixel and convert it to RGB888
		        colorGray256To888(IMG_pixel[ind], r, dummy, dummy);
		        // colorGray256To888(IMG_pixel[ind], r, g, b);
		        
		        if(!bitshift_avail){		// If divisor is not a power of 2, bitshift division is not possible.
		        	temp_r = r + ((quant_err_r * curr_weight) / divisor);
		        }
		        else{				// On most uCs, bitshift division is quite cycle-expensive. Whenever possible, use bitshift. On the other hand, multiplication is often single-cycle.
		        	temp_r = r + ((quant_err_r * curr_weight) >> bitshift_div);
		        }
		        
		        // Clamp pixel value if outside range [0-255]
						r = _clamp(temp_r, 0, 255);
						
						/*
		        temp_g = g + ((quant_err_g * weights[p]) / divisor);
		        g = _clamp(temp_g, 0, 255);
		        
		        temp_b = b + ((quant_err_b * weights[p]) / divisor);
		        b = _clamp(temp_b, 0, 255);
		        */
		        
		        // Assign new pixel value (each neighbour)
		        IMG_pixel[ind] = color888ToGray256(r, r, r);
		        // IMG_pixel[ind] = color888ToGray256(r, g, b);
		      }
	        
	      }
      }
    }
  }
  
  return 0;		// Everything ok
}

// Fast Error Diffusion Dithering algorithm
void Dither::fastEDDither(uint8_t *IMG_pixel){
  
  uint8_t c;
  uint8_t newc;
  int16_t temp_c;
  int8_t quant_err_c;
  uint32_t ind;

  for(uint16_t row = 0; row < _img_height; row++){
    for(uint16_t col = 0; col < _img_width; col++){
      
      ind = index(col, row);
    	
      c = IMG_pixel[ind];

      newc = c;

      quantize_BW(newc);

      quant_err_c = (c - newc) >> 1;
      
      if(_invert_output)	newc = 0xFF - newc;
      IMG_pixel[ind] = newc;
      
      // adjacent pixels work starts here:
      // distribute part of error at (x + 1, y)      
      if(col != (_img_width - 1)){
      	
      	ind = index(col + 1, row);
        c = IMG_pixel[ind];
        
        temp_c = c + quant_err_c;
        
        IMG_pixel[ind] = _clamp(temp_c, 0, 255);

      }

      // distribute part of error at (x, y + 1)
      if(row != (_img_height - 1)){
      	
        ind = index(col, row + 1);
        c = IMG_pixel[ind];
        
        #if fastEDDither_remove_artifacts
        	temp_c = c + (quant_err_c >> 1);	// distribute only half the quantization error to the pixel below
        #else
        	temp_c = c + quant_err_c;					// distribute the whole quantization error to the pixel below
        #endif

        IMG_pixel[ind] = _clamp(temp_c, 0, 255);
        
      }
      
      // ONLY if fastEDDither_remove_artifacts == true, distribute half of error also at (x + 1, y + 1)
      #if fastEDDither_remove_artifacts
      if(col != (_img_width - 1)  &&  row != (_img_height - 1)){
      	
        ind = index(col + 1, row + 1);
        c = IMG_pixel[ind];
        
        temp_c = c + (quant_err_c >> 1);

        IMG_pixel[ind] = _clamp(temp_c, 0, 255);
        
      }
	    #endif

    }
  }
}





// PATTERNING Classic Algorithms

void Dither::buildClusteredPattern(){		// fills in the entries of _pattern[][] array using a clustered arrangement. The numbers are ordered "spirally".
	if(_size < 1)  return;
	
	int8_t m = _size, n = _size;
	int val = 1; 	// starting value
	uint8_t step = 255 / (_size*_size);

	int k = 0, l = 0; 
		while(k < m && l < n){ 
		for(int i = l; i < n; ++i)  _pattern[k][i] = step * val++;
		k++;
		
		for(int i = k; i < m; ++i)  _pattern[i][n-1] = step * val++;
		n--;
		
		if(k < m){ 
		  for(int i = n-1; i >= l; --i)  _pattern[m-1][i] = step * val++;
		  m--;
		}
		
		if (l < n){ 
		  for (int i = m-1; i >= k; --i)  _pattern[i][l] = step * val++;
		  l++;
		}
	}
}


void Dither::buildBayerPattern(){		// fills in the entries of _pattern[][] array using a dispersed arrangement. The method of choice is a pseudo-Bayer arrangement.
	if(_size < 1)  return;
	
	uint8_t offs = 0, increm = 1;
	if(!(_size & 0x01)) 	offs = 1;		// size is not a multiple of 2
	uint8_t step = 255 / (_size*_size);
	
	for(uint8_t passage = 0; passage < 2; passage++){
		uint8_t c = 0;
		c = c + passage;
		for(uint8_t r = 0; r < _size; r++){
			
			for(; c < _size; c += 2){
				_pattern[r][c] = step * increm++;
			}
			c += offs;
			c = (_size & 0x01)? (c % _size) : (c & 0x01);
		}
	}
	
}

int8_t Dither::patternDither(uint8_t *IMG_pixel, 
														 int8_t thresh){			// pixels will be compared to the pattern value offsetted by thresh (in the interval [-128 : +127]) ; by default it's set to 0
	
	if(_pattern[0][0] == 0)  return -1;	// No pattern has been built. You need to call one of the aforementioned functions, before proceeding.
	
	uint32_t ind;
	uint8_t pixel, compa, patt_row;

	for(uint16_t row = 0; row < _img_height; row++){
		#if (_size & (_size - 1)) == 0		// is _size a power of 2 ?
			patt_row = row & (_size - 1);
		#else
			patt_row = row % _size;
		#endif		
    for(uint16_t col = 0; col < _img_width; col++){
    	ind = index(col, row);
    	pixel = IMG_pixel[ind];
    	#if (_size & (_size - 1)) == 0		// is _size a power of 2 ?
    		compa = _pattern[patt_row][col & (_size - 1)];		// modulo operations are very slow on non-math-enhanced uCs, so bitwise operations are useful whenever pattern size is a power of 2
    	#else
				compa = _pattern[patt_row][col % _size];		// if pattern size is not a power of 2, bitwise modulo calculation cannot be implemented.
    	#endif
    	
    	pixel = (pixel > (compa + thresh))?  0xFF : 0x00;
      
      if(_invert_output)	pixel = 0xFF - pixel;
      IMG_pixel[ind] = pixel;
    }
  }
	return 0;
}




// Other dithering Algorithms

void Dither::randomDither(uint8_t *IMG_pixel, 
													bool time_consistency, 		// if time_consistency enabled, a frame of noise will be read from RAM to have all dithered frames time-consistent, and execute faster.
													int8_t thresh){					 	// pixels will be compared to the random value offsetted by thresh (in the interval [-128 : +127]) ; by default it's set to 0

  uint32_t ind;
  uint8_t pixel, rnd_val;
  
  for(uint16_t row = 0; row < _img_height; row++){
  	uint8_t noise_offs = _rnd_frame[row % _rnd_frame_width];
    for(uint16_t col = 0; col < _img_width; col++){
    	
			ind = index(col, row);
    	
    	if(time_consistency)
				rnd_val = _rnd_frame[noise_offs + (col & (_rnd_frame_width - 1))]; 	// This does the same as "col % _rnd_frame_width", as long as _frame_width is a power of 2
      else
			  rnd_val = _Rnd();
    	
      pixel = IMG_pixel[ind];
      
			pixel = (pixel >= (rnd_val + thresh))?  0xFF : 0x00;
      
      if(_invert_output)	pixel = 0xFF - pixel;
      IMG_pixel[ind] = pixel;
    }
  }
}




// NAIIVE Algorithms

void Dither::thresholding(uint8_t *IMG_pixel, uint8_t thresh){
	
	//if(thresh == 128)  return thresholding(IMG_pixel);		// use the faster overloaded implementation - No longer available: performance enhancement was too low.
	
	uint32_t ind;
	uint8_t pixel;
	
	for(uint16_t row = 0; row < _img_height; row++){
    for(uint16_t col = 0; col < _img_width; col++){
    	
      ind = index(col, row);
      pixel = IMG_pixel[ind];
      
			if(pixel >= thresh)  pixel = 0xFF;
			else pixel = 0x00;
      
      if(_invert_output)	pixel = 0xFF - pixel;
      IMG_pixel[ind] = pixel;
    }
  }
}

/*
// This overloading function was ment to speed up thresholding when thresh was 128, but unless the cost of branching on your uC is high (quite rarely), this may not help at all (quite the contrary, actually!)
// I left it here as a scholastic approach; some techniques are good to know just in case.
void Dither::thresholding(uint8_t *IMG_pixel){	// default: assumes thresholding value of >= 128 and uses bitwise operations to speed things up

	uint32_t ind;
	uint8_t pixel;
	
	for(uint16_t row = 0; row < _img_height; row++){
    for(uint16_t col = 0; col < _img_width; col++){
    	
    	ind = index(col, row);
    	
    	if(_invert_output)	pixel = (int8_t)((IMG_pixel[ind] >> 7) - 1);	// use bitwise operators and 2's complement tricks to speed things up. Performance enhancement is quite low, though.
    	else  pixel = ~((int8_t)((IMG_pixel[ind] >> 7) - 1));
      IMG_pixel[ind] = pixel;
    }
  }
}
*/

uint32_t Dither::index(int x, int y){		// ONLY for byte-aligned pixels (so monochrome or, generally speaking, single-byte color such as RGB332 format)
  return (x) + (y) * _img_width;
}

uint32_t Dither::index(int x, int y, uint8_t pix_len){			// Generic indexing function; pix_len is in range [1, 2, 3] , which correspond to [8, 16, 24] color bitdepths.
																														// The returned value will be the index of the first pixel byte at location (x, y) of the image
  return (((x) * pix_len) + ((y) * _img_width * pix_len));
}


uint8_t Dither::_Rnd(uint8_t seed) {
  static uint8_t y = 1;
  y += y * micros() * millis() + seed;
  #if _use_low_amplitude_noise
  	return 64 + (y >> 1);		// low influence noise
	#else
		return y;		// high influence noise
	#endif
}

uint8_t Dither::_twos_power(uint16_t number){		// returns the position of the highest (MS) '1' in a power of 2 number
	uint8_t l2 = 0;
	while(number >>= 1){
	  l2++;
	}
	return l2;
}

uint8_t Dither::_clamp(int16_t v, uint8_t min, uint8_t max){	// Clamps values between a range inside [0 : 255] ; substitute "uint8_t min, uint8_t max" with "uint16_t ..." to extend this range.
  uint16_t t = (v) < min ? min : (v);
  return t > max ? max : t;
}



uint8_t Dither::color888ToGray256(uint8_t r, uint8_t g, uint8_t b) {
  return ((r + g + b) / 3);
}

void Dither::colorGray256To888(uint8_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = color;
  g = color;
  b = color;
}

bool Dither::colorGray256ToBool(uint8_t gs) {
  return gs >> 7;
}

void Dither::quantize(uint8_t quant_bits, uint8_t &r, uint8_t &g, uint8_t &b){
  uint8_t quant_step = 255 / ((1 << quant_bits) - 1);
  uint8_t shifter = 8 - quant_bits;
  r = ((uint16_t)r >> shifter) * quant_step;
  g = ((uint16_t)g >> shifter) * quant_step;
  b = ((uint16_t)b >> shifter) * quant_step;
}

void Dither::quantize_BW(uint8_t &c){
  c = (int8_t)c >> 7;
}


	// Other useful helping functions

void Dither::color565To888(uint16_t color565, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = ((color565 >> 11) & 0x1F) << 3;
  if(r == 248)  r = 255;
  g = ((color565 >> 5) & 0x3F) << 2;
  if(g == 252)  g = 255;
  b = ((color565) & 0x1F) << 3;
  if(b == 248)  b = 255;
}

uint16_t Dither::color888To565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

void Dither::color332To888(uint8_t color332, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = ((color332 >> 5) & 0x07) * 36;
  if(r == 252)  r = 255;
  g = ((color332 >> 2) & 0x07) * 36;
  if(g == 252)  g = 255;
  b = (color332 & 0x03) * 85;
}

uint8_t Dither::color888To332(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t val = ((r & 0xE0) | ((g & 0xE0) >> 3) | ((b & 0xC0) >> 6));
  return val;
}

void Dither::colorBoolTo888(bool color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = ~((int8_t)(color - 1));
  g = r;
  b = r;
}

bool Dither::color888ToBool(uint8_t r, uint8_t g, uint8_t b) {
  return ((r + g + b) / 3) >> 7;
}

