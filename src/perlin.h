#ifndef PERLIN_H
#define PERLIN_H
void perlinit(int seed);
float perlin(float x, float y);
int perlin_sample_grid(char* buffer, size_t buflen, int width, int height, float startx, float starty, float zoom );
#endif
