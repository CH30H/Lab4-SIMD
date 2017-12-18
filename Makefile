CC = gcc
PROC = yuv_MMX yuv_noSIMD yuv_SSE2_AVX
C = g++
OPTIMIZE = -O2
SIMD = -msse2 -mmmx -mavx2
NOSIMD = -mno-sse2 -mno-mmx

OBJECTS = yuv_MMX.o yuv_noSIMD.o yuv_SSE2_AVX.o


all: $(PROC)

yuv_MMX : yuv_mmx.cpp
	$(C) -o yuv_MMX $(OPTIMIZE) $(SIMD) yuv_mmx.cpp

yuv_noSIMD : yuv_noSIMD.cpp
	$(C) -o yuv_noSIMD $(OPTIMIZE) $(NOSIMD) yuv_noSIMD.cpp

yuv_SSE2_AVX : yuv_sse2_avx.cpp
	$(C) -o yuv_SSE2_AVX $(OPTIMIZE) $(SIMD) yuv_sse2_avx.cpp


.PHONY : clean
clean:
	-rm $(PROC) 
