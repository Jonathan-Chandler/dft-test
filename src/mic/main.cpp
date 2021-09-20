/*
 * Copyright (c) 2006, Creative Labs Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 * 	     the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * 	     and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of Creative Labs Inc. nor the names of its contributors may be used to endorse or
 * 	     promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>
#include <algorithm>
#include <stdio.h>
#include <windows.h>
#include <al.h>
#include <alc.h>

#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

#define PI_DEF              3.14159265359f
#define BUFFERSIZE				4410
#define	DFT_RES_FILE		    "dft.txt"
#define	DFT_ALL_RES_FILE		"dft_all.txt"

#define	OUTPUT_WAVE_FILE		"Capture.wav"

#pragma pack (push,1)
typedef struct
{
	char			szRIFF[4];
	long			lRIFFSize;
	char			szWave[4];
	char			szFmt[4];
	long			lFmtSize;
	WAVEFORMATEX	wfex;
	char			szData[4];
	long			lDataSize;
} WAVEHEADER;
#pragma pack (pop)

typedef struct dftRes
{
  int freq;
  double real;
  dftRes(int ind, double re) : freq(ind), real(re) {};
} dftRes_t;

bool compareDftRes(const dftRes_t &a, const dftRes_t &b)
{
    return abs(a.real) > abs(b.real);
}


#define N 256

#define FFTW_SZ   120000     // 88,880
#define FFTW_TEST 1
#define FFTW_TEST2 1

#if 1
int main()
{
	ALCdevice		*pCaptureDevice;
	const ALCchar	*szDefaultCaptureDevice;
	FILE			*pFile = 0;
	FILE			*pFile2 = 0;
	FILE			*pFile3 = 0;
	ALint			iSamplesAvailable;
	ALint			iSize;
	ALint			iDataSize = 0;
	ALchar    Buffer[BUFFERSIZE];
	WAVEHEADER		sWaveHeader;
#if FFTW_TEST
  //int fftw_samples;
  //fftw_complex fftw_in[FFTW_SZ], fftw_out[FFTW_SZ];
  fftw_complex *fftw_in = 0;
  fftw_complex *fftw_out = 0;
  fftw_plan p;
#endif

	// Get list of available Capture Devices
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		printf("\nAvailable Capture Devices are:-\n");

		while (*pDeviceList)
		{
			printf("%s\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}

#if FFTW_TEST
  if ((fftw_in = (fftw_complex*)malloc(sizeof(fftw_complex)*FFTW_SZ)) == 0)
  {
    printf("Fail to allocate fftw_in\n");
    goto cleanExit;
  }

  if ((fftw_out = (fftw_complex*)malloc(sizeof(fftw_complex)*FFTW_SZ)) == 0)
  {
    printf("Fail to allocate fftw_out\n");
    goto cleanExit;
  }
#endif

	// Get the name of the 'default' capture device
	szDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
	printf("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

	// Open the default Capture device to record a 22050Hz 16bit Mono Stream using an internal buffer
	// of BUFFERSIZE Samples (== BUFFERSIZE * 2 bytes)
	pCaptureDevice = alcCaptureOpenDevice(szDefaultCaptureDevice, 22050, AL_FORMAT_MONO16, BUFFERSIZE);
	if (pCaptureDevice)
  {
    printf("Opened capture device %s\n", szDefaultCaptureDevice);

		pFile = fopen(OUTPUT_WAVE_FILE, "wb");
    if (!pFile)
    {
      printf("Fail to open file %s\n", OUTPUT_WAVE_FILE);
      goto cleanExit;
    }

		// Prepare a WAVE file header for the captured data
		sprintf(sWaveHeader.szRIFF, "RIFF");
		sWaveHeader.lRIFFSize = 0;
		sprintf(sWaveHeader.szWave, "WAVE");
		sprintf(sWaveHeader.szFmt, "fmt ");
		sWaveHeader.lFmtSize = sizeof(WAVEFORMATEX);		
		sWaveHeader.wfex.nChannels = 1;
		sWaveHeader.wfex.wBitsPerSample = 16;
		sWaveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
		sWaveHeader.wfex.nSamplesPerSec = 22050;
		sWaveHeader.wfex.nBlockAlign = sWaveHeader.wfex.nChannels * sWaveHeader.wfex.wBitsPerSample / 8;
		sWaveHeader.wfex.nAvgBytesPerSec = sWaveHeader.wfex.nSamplesPerSec * sWaveHeader.wfex.nBlockAlign;
		sWaveHeader.wfex.cbSize = 0;
		sprintf(sWaveHeader.szData, "data");
		sWaveHeader.lDataSize = 0;

		fwrite(&sWaveHeader, sizeof(WAVEHEADER), 1, pFile);

		// Start audio capture
		alcCaptureStart(pCaptureDevice);

		// Record for two seconds or until a key is pressed
		//DWORD dwStartTime = timeGetTime();
		DWORD dwStartTime = GetTickCount();
		//while (!ALFWKeyPress() && (timeGetTime() <= (dwStartTime + 2000)))
		while (GetTickCount() <= (dwStartTime + 2000))
		{
			// Release some CPU time ...
			Sleep(1);

			// Find out how many samples have been captured
			alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &iSamplesAvailable);

			printf("Samples available : %d\r", iSamplesAvailable);

			// When we have enough data to fill our BUFFERSIZE byte buffer, grab the samples
			if (iSamplesAvailable > (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign))
			{
				// Consume Samples
				alcCaptureSamples(pCaptureDevice, Buffer, BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);

				// Write the audio data to a file
				fwrite(Buffer, BUFFERSIZE, 1, pFile);

#if FFTW_TEST2
        // copy data to fftw input buff
        for (int i = 0; i < BUFFERSIZE; i++)
        {
          if (i + iDataSize < FFTW_SZ)
          {
            fftw_in[i + iDataSize][0] = float(Buffer[i]);
            fftw_in[i + iDataSize][1] = 0;
          }
        }
        printf("\niDataSize = %d\n", iDataSize);
#endif

				// Record total amount of data recorded
				iDataSize += BUFFERSIZE;
			}
		}

		// Stop capture
		alcCaptureStop(pCaptureDevice);

#if FFTW_TEST2
    //fftw_samples = iDataSize;
#endif

		// Check if any Samples haven't been consumed yet
		alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &iSamplesAvailable);
		while (iSamplesAvailable)
		{
			if (iSamplesAvailable > (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign))
			{
				alcCaptureSamples(pCaptureDevice, Buffer, BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);
				fwrite(Buffer, BUFFERSIZE, 1, pFile);
				iSamplesAvailable -= (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);

#if 0
        for (int i = 0; i < BUFFERSIZE; i++)
        {
          fftw_in[i + iDataSize][0] = Buffer[i];
          fftw_in[i + iDataSize][1] = 0;
        }
#endif

				iDataSize += BUFFERSIZE;
			}
			else
			{
				alcCaptureSamples(pCaptureDevice, Buffer, iSamplesAvailable);
				fwrite(Buffer, iSamplesAvailable * sWaveHeader.wfex.nBlockAlign, 1, pFile);

#if 0
        for (int i = 0; i < (iSamplesAvailable * sWaveHeader.wfex.nBlockAlign); i++)
        {
          fftw_in[i + iDataSize][0] = Buffer[i];
          fftw_in[i + iDataSize][1] = 0;
        }
        printf("=%f\n",fftw_in[0][0]);
#endif

				iDataSize += iSamplesAvailable * sWaveHeader.wfex.nBlockAlign;
				iSamplesAvailable = 0;
			}
		}

		// Fill in Size information in Wave Header
		fseek(pFile, 4, SEEK_SET);
		iSize = iDataSize + sizeof(WAVEHEADER) - 8;
		fwrite(&iSize, 4, 1, pFile);
		fseek(pFile, 42, SEEK_SET);
		fwrite(&iDataSize, 4, 1, pFile);

		fclose(pFile);
    pFile = 0;

    printf("\ndata size = %d\n", iDataSize);
		printf("\nSaved captured audio data to '%s'\n", OUTPUT_WAVE_FILE);

		// Close the Capture Device
		alcCaptureCloseDevice(pCaptureDevice);
		pCaptureDevice = 0;

    for (size_t i = 0; i < 100; i++)
      printf("input: %3d %+9.5f %+9.5f I\n", i, fftw_in[i][0], fftw_in[i][1]);

#if FFTW_TEST2
    /* forward Fourier transform, save the result in 'out' */
    p = fftw_plan_dft_1d(FFTW_SZ, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

		pFile2 = fopen(DFT_RES_FILE, "w");
    if (!pFile2)
    {
      printf("Fail to open file %s\n", DFT_RES_FILE);
      goto cleanExit;
    }

		pFile3 = fopen(DFT_ALL_RES_FILE, "w");
    if (!pFile3)
    {
      printf("Fail to open file %s\n", DFT_ALL_RES_FILE);
      goto cleanExit;
    }

    for (int i = 0; i < FFTW_SZ; i++)
      fprintf(pFile3,"freq: %4d %+9.5f %+9.5f I\n", i, fftw_out[i][0], fftw_out[i][1]);

    fclose(pFile3);
    pFile3 = 0;
    
    //std::vector<fftw_complex> dft_20k;
    // low E2 82.4069 hz 
    // high E4 329.628 hz
    std::vector<dftRes_t> dft_check_range;
    for (size_t i = 25; i < 1000; i++)
    {
      dft_check_range.push_back(dftRes_t(i,fftw_out[i][0]));
    }

    //dft_20k.push_back(dftRes_t(i,fftw_out[i][0]));
    std::sort(dft_check_range.begin(), dft_check_range.end(), compareDftRes);

    for (size_t i = 0; i < dft_check_range.size(); i++)
      fprintf(pFile2,"freq: %4d %+9.5f\n", dft_check_range[i].freq, dft_check_range[i].real);

    fclose(pFile2);
    pFile2 = 0;

    fftw_destroy_plan(p);
    fftw_cleanup();
#endif
  }
  else
  {
    printf("Fail to open capture device %s\n", szDefaultCaptureDevice);
  }

  cleanExit:
		// Close the Capture Device
    if (pCaptureDevice)
      alcCaptureCloseDevice(pCaptureDevice);

    if (pFile)
      fclose(pFile);

#if FFTW_TEST
    if (pFile2)
      fclose(pFile2);
    
    if (pFile3)
      fclose(pFile3);

    if (fftw_out)
      free(fftw_out);

    if (fftw_in)
      free(fftw_in);
#endif

    return 0;
}
#endif

#if 0
int main()
{
	ALCdevice *dev[2];
	ALCcontext *ctx;
	ALuint source, buffers[3];
	char data[5000]; 
	ALuint buf;
	ALint val;

	dev[0] = alcOpenDevice(NULL);
	ctx = alcCreateContext(dev[0], NULL);
	alcMakeContextCurrent(ctx);

	alGenSources(1, &source);
	alGenBuffers(3, buffers);

	/* Setup some initial silent data to play out of the source */
	alBufferData(buffers[0], AL_FORMAT_MONO16, data, sizeof(data), 22050);
	alBufferData(buffers[1], AL_FORMAT_MONO16, data, sizeof(data), 22050);
	alBufferData(buffers[2], AL_FORMAT_MONO16, data, sizeof(data), 22050);
	alSourceQueueBuffers(source, 3, buffers);

	/* If you don't need 3D spatialization, this should help processing time */
	alDistanceModel(AL_NONE); 

	dev[1] = alcCaptureOpenDevice(NULL, 22050, AL_FORMAT_MONO16, sizeof(data)/2);

	/* Start playback and capture, and enter the audio loop */
	alSourcePlay(source);
	alcCaptureStart(dev[1]);

	while(1) 
	{
		/* Check if any queued buffers are finished */
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
		if(val <= 0)
			continue;

		/* Check how much audio data has been captured (note that 'val' is the
		* number of frames, not bytes) */
		alcGetIntegerv(dev[1], ALC_CAPTURE_SAMPLES, 1, &val);

		/* Read the captured audio */
		alcCaptureSamples(dev[1], data, val);

		/* Pop the oldest finished buffer, fill it with the new capture data,
		then re-queue it to play on the source */
		alSourceUnqueueBuffers(source, 1, &buf);
		alBufferData(buf, AL_FORMAT_MONO16, data, val*2 /* bytes here, not
		frames */, 22050);
		alSourceQueueBuffers(source, 1, &buf);

		/* Make sure the source is still playing */
		alGetSourcei(source, AL_SOURCE_STATE, &val);
		if(val != AL_PLAYING)
		{
			alSourcePlay(source);
		}
	}

	/* Shutdown and cleanup */
	alcCaptureStop(dev[1]);
	alcCaptureCloseDevice(dev[1]);

	alSourceStop(source);
	alDeleteSources(1, &source);
	alDeleteBuffers(3, buffers);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(ctx);
	alcCloseDevice(dev[0]); 

	return 0;
}
#endif

#if 0
#include"Framework.h"

#define	OUTPUT_WAVE_FILE		"Capture.wav"
#define BUFFERSIZE				4410

#pragma pack (push,1)
typedef struct
{
	char			szRIFF[4];
	long			lRIFFSize;
	char			szWave[4];
	char			szFmt[4];
	long			lFmtSize;
	WAVEFORMATEX	wfex;
	char			szData[4];
	long			lDataSize;
} WAVEHEADER;
#pragma pack (pop)

int main()
{
	ALCdevice		*pDevice;
	ALCcontext		*pContext;
	ALCdevice		*pCaptureDevice;
	const ALCchar	*szDefaultCaptureDevice;
	ALint			iSamplesAvailable;
	FILE			*pFile;
	ALchar			Buffer[BUFFERSIZE];
	WAVEHEADER		sWaveHeader;
	ALint			iDataSize = 0;
	ALint			iSize;

	// NOTE : This code does NOT setup the Wave Device's Audio Mixer to select a recording input
	// or a recording level.

	// Initialize Framework
	ALFWInit();

	ALFWprintf("Capture Application\n");

	if (!ALFWInitOpenAL())
	{
		ALFWprintf("Failed to initialize OpenAL\n");
		ALFWShutdown();
		return 0;
	}

	// Check for Capture Extension support
	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);
	if (alcIsExtensionPresent(pDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
	{
		ALFWprintf("Failed to detect Capture Extension\n");
		ALFWShutdownOpenAL();
		ALFWShutdown();
		return 0;
	}

	// Get list of available Capture Devices
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		ALFWprintf("\nAvailable Capture Devices are:-\n");

		while (*pDeviceList)
		{
			ALFWprintf("%s\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}

	// Get the name of the 'default' capture device
	szDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
	ALFWprintf("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

	// Open the default Capture device to record a 22050Hz 16bit Mono Stream using an internal buffer
	// of BUFFERSIZE Samples (== BUFFERSIZE * 2 bytes)
	pCaptureDevice = alcCaptureOpenDevice(szDefaultCaptureDevice, 22050, AL_FORMAT_MONO16, BUFFERSIZE);
	if (pCaptureDevice)
	{
		ALFWprintf("Opened '%s' Capture Device\n\n", alcGetString(pCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));

		// Create / open a file for the captured data
		pFile = fopen(OUTPUT_WAVE_FILE, "wb");

		// Prepare a WAVE file header for the captured data
		sprintf(sWaveHeader.szRIFF, "RIFF");
		sWaveHeader.lRIFFSize = 0;
		sprintf(sWaveHeader.szWave, "WAVE");
		sprintf(sWaveHeader.szFmt, "fmt ");
		sWaveHeader.lFmtSize = sizeof(WAVEFORMATEX);		
		sWaveHeader.wfex.nChannels = 1;
		sWaveHeader.wfex.wBitsPerSample = 16;
		sWaveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
		sWaveHeader.wfex.nSamplesPerSec = 22050;
		sWaveHeader.wfex.nBlockAlign = sWaveHeader.wfex.nChannels * sWaveHeader.wfex.wBitsPerSample / 8;
		sWaveHeader.wfex.nAvgBytesPerSec = sWaveHeader.wfex.nSamplesPerSec * sWaveHeader.wfex.nBlockAlign;
		sWaveHeader.wfex.cbSize = 0;
		sprintf(sWaveHeader.szData, "data");
		sWaveHeader.lDataSize = 0;

		fwrite(&sWaveHeader, sizeof(WAVEHEADER), 1, pFile);

		// Start audio capture
		alcCaptureStart(pCaptureDevice);

		// Record for two seconds or until a key is pressed
		DWORD dwStartTime = timeGetTime();
		while (!ALFWKeyPress() && (timeGetTime() <= (dwStartTime + 2000)))
		{
			// Release some CPU time ...
			Sleep(1);

			// Find out how many samples have been captured
			alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &iSamplesAvailable);

			ALFWprintf("Samples available : %d\r", iSamplesAvailable);

			// When we have enough data to fill our BUFFERSIZE byte buffer, grab the samples
			if (iSamplesAvailable > (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign))
			{
				// Consume Samples
				alcCaptureSamples(pCaptureDevice, Buffer, BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);

				// Write the audio data to a file
				fwrite(Buffer, BUFFERSIZE, 1, pFile);

				// Record total amount of data recorded
				iDataSize += BUFFERSIZE;
			}
		}

		// Stop capture
		alcCaptureStop(pCaptureDevice);

		// Check if any Samples haven't been consumed yet
		alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &iSamplesAvailable);
		while (iSamplesAvailable)
		{
			if (iSamplesAvailable > (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign))
			{
				alcCaptureSamples(pCaptureDevice, Buffer, BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);
				fwrite(Buffer, BUFFERSIZE, 1, pFile);
				iSamplesAvailable -= (BUFFERSIZE / sWaveHeader.wfex.nBlockAlign);
				iDataSize += BUFFERSIZE;
			}
			else
			{
				alcCaptureSamples(pCaptureDevice, Buffer, iSamplesAvailable);
				fwrite(Buffer, iSamplesAvailable * sWaveHeader.wfex.nBlockAlign, 1, pFile);
				iDataSize += iSamplesAvailable * sWaveHeader.wfex.nBlockAlign;
				iSamplesAvailable = 0;
			}
		}

		// Fill in Size information in Wave Header
		fseek(pFile, 4, SEEK_SET);
		iSize = iDataSize + sizeof(WAVEHEADER) - 8;
		fwrite(&iSize, 4, 1, pFile);
		fseek(pFile, 42, SEEK_SET);
		fwrite(&iDataSize, 4, 1, pFile);

		fclose(pFile);

		ALFWprintf("\nSaved captured audio data to '%s'\n", OUTPUT_WAVE_FILE);

		// Close the Capture Device
		alcCaptureCloseDevice(pCaptureDevice);
	}

	// Close down OpenAL
	ALFWShutdownOpenAL();

	// Close down the Framework
	ALFWShutdown();

	return 0;
}
#endif

#if 0
int main()
{
  fftw_complex in[N], out[N]; //, in2[N]; /* double [2] */
  fftw_plan p; //, q;
  int i;

  /* prepare a cosine wave */
  for (i = 0; i < N; i++) {
    in[i][0] = cos(10 * 2*PI_DEF*i/N);
    in[i][1] = 0;
  }

  /* forward Fourier transform, save the result in 'out' */
  p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p);

  for (i = 0; i < N/2; i++)
    printf("freq: %3d %+9.5f %+9.5f I\n", i, out[i][0], out[i][1]);
  fftw_destroy_plan(p);

#if 0
  /* backward Fourier transform, save the result in 'in2' */
  printf("\nInverse transform:\n");
  q = fftw_plan_dft_1d(N, out, in2, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_execute(q);

  /* normalize */
  for (i = 0; i < N; i++) {
    in2[i][0] *= 1./N;
    in2[i][1] *= 1./N;
  }
  for (i = 0; i < N; i++)
    printf("recover: %3d %+9.5f %+9.5f I vs. %+9.5f %+9.5f I\n",
        i, in[i][0], in[i][1], in2[i][0], in2[i][1]);

  for (i = 0; i < N; i++)
    printf("in: %3d %+9.5f %+9.5f I vs. %+9.5f %+9.5f I\n",
        i, in[i][0], in[i][1], in2[i][0], in2[i][1]);

  fftw_destroy_plan(q);
#endif

  fftw_cleanup();
  return 0;
}
#endif

