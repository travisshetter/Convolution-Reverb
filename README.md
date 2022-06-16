# convolution_reverb

file to file command line convolutional reverb processor written in C using the Fast Fourier transform (FFT)
Completed for Dr. Schuyler R Quackenbush's C Programming Class 
Convolution alogrorithm provided curtousy of Dr. Morwaread Farbood 

Sndfile Library used to read/write wav files 

Impulse response files collected from the Open Air Library database (https:// www.openairlib.net/) 
Sample input WAV files accessed from the FreeSound online database (https://freesound.org/).

Compiling the code: 

gcc -o conv_reverb conv_reverb.c convolve.c \
     -I/usr/local/include \
     -L/usr/local/lib -lsndfile

Usage in the command line:

./conv_reverb input_file.wav IR_file.wav output_file.wav
