#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<math.h>
#include<unistd.h>
#include<memory.h>

/*#include "perlin.h"*/

/*#define LINEAR */
#define SMOOTHSTEP 
/*#define SMOOTHERSTEP */
float interpolate (float a0, float a1, float w) {
	if(0.0f > w) return a0;
	if(1.0f < w) return a1;
#ifdef LINEAR
	return(a1 - a0) * w + a0;
#elif defined SMOOTHSTEP
	return(a1 - a0) * (3.0f - w * 2.0f) * w * w + a0; 
#elif defined SMOOTHERSTEP
	return (a1 - a0) * ((w * (w * 6.0f - 15.0f) + 10.0) * w * w * w) + a0;
#endif
}

typedef struct {
	float x;
	float y;
} v2;

static v2 gradients[255*255];
size_t gradlen = 255*255;

#define IX(x, y) (scrw*y + x)

const float sqrt2 = 1.4142135f;
v2 random_gradient() {
	v2 v;
	float r = (float) rand() / (float) RAND_MAX;
	float angle = 2*M_PI * r;
	//we want gradients of length sqrt2 so that our final output will
	//be in range [-1, 1]
	v.x = cos(angle) * sqrt2;
	v.y = sin(angle) * sqrt2;
	return v;

}

v2 rg2(int ix, int iy) {
    // No precomputed gradients mean this works for any number of grid coordinates
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2; // rotation width
    unsigned a = ix, b = iy;
    a *= 3284157443; b ^= a << s | a >> (w-s);
    b *= 1911520717; a ^= b << s | b >> (w-s);
    a *= 2048419325;
    float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
    v2 v;
    v.x = cos(random); v.y = sin(random);
    return v;
}

float sample_grid(int gridx, int gridy, float samplex, float sampley) {
	v2 samplegradient = gradients[255*gridy+gridx];
	float dx = samplex - (float)gridx;
	float dy = sampley - (float)gridy;

	float ret = dx*samplegradient.x + dy*samplegradient.y;
	return ret;
}

float perlin(float x, float y) {
	//find surrounding corners
	//
	/*x = fmod(x, 253.0);*/
	/*x = fmod(y, 253.0);*/
	x = fmod(x, 254.0f);
	y = fmod(y, 254.0f);
	int xlft = ((int)floor(x)) % 255;
	int xrgt = xlft + 1;
	int ytop = ((int)floor(y)) % 255;
	int ybot = ytop + 1;

	//get interpolation weights
	float sx = x - (float)xlft;
	float sy = y - (float)ytop;

	//interpolate between all 4 corners
	float tl, bl, tr, br;

	tl = sample_grid(xlft, ytop, x, y);
	bl = sample_grid(xrgt, ytop, x, y);
	float xl = interpolate(tl, bl, sx);

	tr = sample_grid(xlft, ybot, x, y);
	br = sample_grid(xrgt, ybot, x, y);
	float xr = interpolate(tr, br, sx);

	float p;
	p = interpolate(xl, xr, sy);
	return p;
}

void perlinit(int seed) {
	srand(seed);
	for(int i = 0; i < 255*255; i++){
		gradients[i] = random_gradient();
	}
}

