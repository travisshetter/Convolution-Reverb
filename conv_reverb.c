#include <stdio.h>
#include <stdlib.h> 	/* malloc */
#include <stdbool.h>	/* true, false */
#include <string.h>		/* memset */
#include <math.h>		/* sqrt() */	
#include <sndfile.h>	/* libsndfile */
#include "convolve.h"   /* convolution code */

#define MAX_CHN		2

bool read_input(SNDFILE *sfile, SF_INFO *sfinfo, float *buf[MAX_CHN]);
bool write_output(SNDFILE *sfile, SF_INFO *sfinfo, float *buf[MAX_CHN], long frames);


int main(int argc, char *argv[])
{
	int channels;
	long iframes;
	long rframes;
	long oframes;
	float *ibuf[MAX_CHN];
	float *rbuf[MAX_CHN];
	float *obuf[MAX_CHN];
	float iss[MAX_CHN];
	float oss[MAX_CHN];
	int i; 
	char *ifilename, *rfilename, *ofilename;
	/* libsndfile data structures */
	SNDFILE *isfile, *rsfile, *osfile; 
	SF_INFO isfinfo, rsfinfo, osfinfo;
	/* convolution data structures */

	/* zero libsndfile structures */
	memset(&isfinfo, 0, sizeof(isfinfo));
  	memset(&rsfinfo, 0, sizeof(rsfinfo));
  	memset(&osfinfo, 0, sizeof(osfinfo));

	/* Parse command line and open all files */
	if ( argc != 4 ) {
		fprintf(stderr, "Usage: %s input_file.wav IR_file.wav output_file.wav\n", argv[0]);
		return -1;
	}

	/* assign arv[] to filename strings */
	ifilename = argv[1];
	rfilename = argv[2];
	ofilename = argv[3];

	/* Open Input WAV file */
	if ( (isfile = sf_open (ifilename, SFM_READ, &isfinfo)) == NULL ) {
    	fprintf (stderr, "Error: could not open wav file: %s\n", ifilename);
    	return -1;
	}

	/* Open Impulse Response WAV file */
	if ( (rsfile = sf_open (rfilename, SFM_READ, &rsfinfo)) == NULL ) {
		fprintf (stderr, "Error: could not open wav file: %s\n", rfilename);
		return -1;
	}

	/* Print Input WAV file information */
	printf("Input audio file %s:\n\tFrames: %d Channels: %d Samplerate: %d\n", 
		ifilename, (int)isfinfo.frames, isfinfo.channels, isfinfo.samplerate);

	printf("Impulse response file %s:\n\tFrames: %d Channels: %d Samplerate: %d\n", 
		rfilename, (int)rsfinfo.frames, rsfinfo.channels, rsfinfo.samplerate);

	/* If sample rates don't match, exit */
	if (isfinfo.samplerate != rsfinfo.samplerate) {
		printf("Error: audio input and impulse response sample rates must be the same.\n");
		return -1;
	} 

	/* If number of channels don't match or too many channels, exit */
	if ( isfinfo.channels != rsfinfo.channels) {
		printf("Error: audio input and impulse response number of channels must be the same.\n");
		return -1;
	} 
	/* If channels excede max channels, exit*/ 
	if ( isfinfo.channels > MAX_CHN ) {
		printf("Error: number of input channels %d greater than max channels %d.\n",
			isfinfo.channels, MAX_CHN);
		return -1;
	} 

	osfinfo.samplerate = isfinfo.samplerate;
	osfinfo.channels = isfinfo.channels;
	osfinfo.format = isfinfo.format;
	
	/* open output file */
	if ( (osfile = sf_open (ofilename, SFM_WRITE, &osfinfo)) == NULL ) {
		printf ("Error : could not open wav file : %s\n", ofilename);
		return -1;
	}

		/* initialize convolution parameters */
	channels = isfinfo.channels;
	iframes = isfinfo.frames;
    rframes = rsfinfo.frames;
	/* output length is input length + Impulse Response length -1 */
	oframes = isfinfo.frames + rsfinfo.frames - 1;

	/* Allocate buffers for each channel of 
	 * input, Impulse Response and output signals
	 */
	for (int i=0; i<channels; i++) {
		if ( (ibuf[i] = (float *)malloc(isfinfo.frames * sizeof(float))) == NULL ) {
			printf("Can't allocate memory for channel %d of input.\n", i);
			return -1;
		} 
		if ( (rbuf[i] = (float *)malloc(rsfinfo.frames * sizeof(float))) == NULL ) {
			printf("Can't allocate memory for channel %d of Impulse Response.\n", i);
			return -1;
		} 
		if ( (obuf[i] = (float *)malloc(oframes * sizeof(float))) == NULL ) {
			printf("Can't allocate memory for channel %d of output.\n", i);
			return -1;
		} 
	}
	printf("Allocated buffers\n");

	/* read interleaved data from files into de-interleaved buffers */
	/* input */
	if ( !read_input(isfile, &isfinfo, ibuf) ) {
		fprintf(stderr, "ERROR: Cannot read input file %s", ifilename);
		return -1;
	}
	/* Impulse Response*/
	if ( !read_input(rsfile, &rsfinfo, rbuf) ) {
		fprintf(stderr, "ERROR: Cannot read input file %s", rfilename);
		return -1;
	}
	printf("Read input files\n");

	printf("Convolving Audio\n");
	for (int i=0; i<channels; i++) {
		convolve (ibuf[i], rbuf[i], iframes, rframes, obuf[i]);
	}

	/* interleave output data and write output file */
	if ( !write_output(osfile, &osfinfo, obuf, oframes) ) {
		fprintf(stderr, "ERROR: Cannot write output file %s", ofilename);
		return -1;
	}

	/* Must close file; output will not be written correctly if you do not do this */
	sf_close (isfile);
	sf_close (rsfile);
	sf_close (osfile);

	/* free all buffer storage */
	printf("Freeing buffers\n");
	for (int i=0; i<MAX_CHN; i++) {
		free(ibuf[i]);
		free(rbuf[i]);
		free(obuf[i]);
	}

} 

bool read_input(SNDFILE *sfile, SF_INFO *sfinfo, float *buf[MAX_CHN])
{
	float *frame_buf;
	/* allocate storage for WAV file data */
	if ( (frame_buf = (float*)malloc(sfinfo->frames*sfinfo->channels*sizeof(float))) == NULL ) {
		fprintf(stderr, "Cannot malloc input storage\n");
		return false;
	}
	/* read WAV data */
	if ( sf_readf_float (sfile, frame_buf, sfinfo->frames) != sfinfo->frames ) {
		fprintf(stderr, "Cannot read imput audio\n");
		return false;
	}
	/* de-interleave into single channel buffers */
	int k=0;
	for (int i=0; i<sfinfo->frames; i++) {
		for (int j=0; j<sfinfo->channels; j++) {
		/* for each frame */
			buf[j][i] = frame_buf[k++];
		}
	}
	free(frame_buf);
	return true;
}

bool write_output(SNDFILE *sfile, SF_INFO *sfinfo, float *buf[MAX_CHN], long frames)
{
	float *frame_buf;
	/* allocate storage for WAV file data */
	if ( (frame_buf = (float*)malloc(frames*sfinfo->channels*sizeof(float))) == NULL ) {
		fprintf(stderr, "Cannot malloc output storage\n");
		return false;
	}
	/* interleave into output buffer */
	int k=0;
	for (int i=0; i<frames; i++) {
		for (int j=0; j<sfinfo->channels; j++) {
			frame_buf[k++] = buf[j][i];
		}
	}
	if ( sf_writef_float (sfile, frame_buf, frames) != frames) {
		fprintf(stderr, "Cannot write audio output\n");
		return false;
	}
	free(frame_buf);
	printf("Wrote %ld frames\n", frames);
	return true;
}

