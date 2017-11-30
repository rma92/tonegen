#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "mmsystem.h"
#include <memory.h>

LPCSTR AppName = "TesT";

#define HARMONICS 12
#define OCTAVES 8
#define NUM_OUT_BUFFS 3
//tcc build:
// tiny_impdef C:\Windows\SysWOW64\winmm.dll
// tcc32 tonegen.c -lwinmm
//build: gcc tonegen.c -l winmm -o tonegen.exe
//for tcc you need to bring in the 
HWND hWnd_main;

HWAVEOUT ghwo;
WAVEFORMATEX wfx;
MMRESULT mmres;
UINT uDeviceID=WAVE_MAPPER;
DWORD dwCallback = (DWORD) NULL;
DWORD dwCallbackInstance = (DWORD) NULL;
WAVEHDR pwh;
MMRESULT mmres;
unsigned char *out_buf=NULL;
//unsigned char **out_bufs = NULL;
//int cur_buf = 0;
unsigned char **bufs=NULL; //array of pointers to unsigned chars for holding note buffers.
int *bufs_lengths = NULL; //array of ints to represent the length (in samples) of each buffer.
int *bufs_cur = NULL;     //array to remember where the current index of the int.
unsigned char enabled_mask[128]; //mask of signals that are enabled.

#define SPS 44100 
//44100
#define Pi 3.1416

//Clear the buffer (fill with 0s)
/*
void PurgeBuf(int buf_id)
{
  printf("purgebuf: buf_id= %d", buf_id );
  for( int cnt = 0; cnt < SPS; ++cnt )
  {
    //buf[ cnt ] = 127;
    out_bufs[0][ cnt ] = 127;
  }
}
*/
void PurgeBuf(int buf_id)
{
  printf("purgebuf: buf_id= %d", buf_id );
  for( int cnt = 0; cnt < SPS; ++cnt )
  {
    out_buf[ cnt ] = 127;
  }
}

//Add a frequency to the buffer specified in the first parameter.
void FillBufAddB(unsigned char* int_buf, float freq,char amp,float phase)
{
  register float x=freq*2*3.1416/SPS;
  for(int cnt=0;cnt<SPS;cnt++)
  {
    int_buf[cnt]+=(unsigned char)(cos(phase+cnt*x)*amp+127);
  }
}

//attempt to replicate amiga harmonics program (it works but it's slow)
//to the buffer specified in the first parameter.
//void FillBufAdd3B(unsigned char* int_buf, float freq, char amp, float phase)
//idx is the index of the buffer in the bufs array.
void FillBufAdd3B(int idx, float freq, char amp, float phase)
{
  //const int BASE_WAVEFORM_SIZE = SPS;
  //static double wave[BASE_WAVEFORM_SIZE];
  int i, j;
  float period;
  unsigned char* int_buf = bufs[idx];
  amp = amp*2;
  for( j = 1; j < OCTAVES; ++j )
  {
    //printf( "f=%f, a=%d\n",freq * j, amp / ( 1 << j ) );
    FillBufAddB( int_buf, freq * j, amp / ( 1 << j ), 0);
    if( j != 1 ) 
    {
      //printf( "f=%f, a=%d\n",freq / j, amp / ( 1 << j ) );
      FillBufAddB( int_buf, freq / j, amp / ( 1 << j ), 0);
    }
  }
  //the lowest frequency = freq / j. Get the period.
  period = SPS / (freq / j);
  printf("%d period: %d=%f\n", idx, (int)period, period);
  bufs_lengths[idx] = (int)period;
  if( ((float)period - (int)period) > 0.5 )
  {
    ++bufs_lengths[idx];
    printf("%d fixed\n", bufs_lengths[idx]);
  }
}

//attempt to replicate amiga harmonics program (it works but it's slow)
/*
void FillBufAdd3(float freq, char amp, float phase)
{
  //const int BASE_WAVEFORM_SIZE = SPS;
  //static double wave[BASE_WAVEFORM_SIZE];
  int i, j;
  amp = amp*2;
  for( j = 1; j < OCTAVES; ++j )
  {
    //printf( "f=%f, a=%d\n",freq * j, amp / ( 1 << j ) );
    FillBufAdd( freq * j, amp / ( 1 << j ), 0);
    if( j != 1 ) 
    {
      //printf( "f=%f, a=%d\n",freq / j, amp / ( 1 << j ) );
      FillBufAdd( freq / j, amp / ( 1 << j ), 0);
    }
  }
}
*/

int GenFreq(void)
{
  waveOutReset(ghwo);
  if(mmres != MMSYSERR_NOERROR)
    return 1;//"Cant reset device.";
  mmres=waveOutWrite(ghwo,&pwh,sizeof(WAVEHDR));
  if(mmres != MMSYSERR_NOERROR)
    return 1;//"Cant write  data to device.";
  return 0;
}

int StopGen(void)
{
  mmres=waveOutReset(ghwo);
  if(mmres != MMSYSERR_NOERROR)
    return 1;//"Cant stop device.";
  return 0; 
}

int CloseDevice(void)
{
  int i;
  /*
  for( i = 0; i < NUM_OUT_BUFFS; ++i )
  {
    free( out_bufs[i] );
  }
  */
  free( out_buf );

  mmres = waveOutReset( ghwo );
  if( mmres != MMSYSERR_NOERROR )
  {
    return 1;//can't close device.
  }
  mmres = waveOutClose( ghwo );
  if( mmres != MMSYSERR_NOERROR )
  {
    return 1;//can't close device.
  }
  return 0;
}
int PrepareDevice(void)
{
  int i;
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = 1;
  wfx.nSamplesPerSec = SPS;
  wfx.nAvgBytesPerSec = SPS;
  wfx.nBlockAlign = 1;
  wfx.wBitsPerSample = 8;
  mmres = waveOutOpen( &ghwo, uDeviceID, &wfx, dwCallback, dwCallbackInstance, CALLBACK_NULL);
  if( mmres != MMSYSERR_NOERROR)
  {
    return 1;//"Can't open device.";
  }

  /*
  out_bufs = (unsigned char **) calloc( NUM_OUT_BUFFS, sizeof( unsigned char ** ) );
  for(i = 0; i < NUM_OUT_BUFFS; ++i )
  {
    out_bufs[i] = (unsigned char *) calloc( SPS , 1);
    if( out_bufs[i] == NULL )
    {
      CloseDevice();
      printf("can't allocate memory");
      return 1;
    }
  }
  */
  out_buf = (unsigned char *) calloc( SPS , 1);

  printf("allocated!");
  //buf = (unsigned char *) calloc( SPS , 1);
  /*
  if( buf == NULL )
  {
    CloseDevice();
    printf("can't allocate memory");
    return 1;
  }
  */
  //pwh.lpData = out_bufs[i];
  pwh.lpData = out_buf;
  pwh.dwBufferLength = SPS;
  //pwh.dwFlags = 0;//WHDR_BEGINLOOP;
  pwh.dwFlags = WHDR_BEGINLOOP;
  pwh.dwLoops = 1;
  mmres = waveOutPrepareHeader( ghwo, &pwh, sizeof( pwh ) );
  //pwh.dwFlags = WHDR_PREPARED;
  pwh.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP | WHDR_PREPARED;
  //pwh.dwLoops = 0xFFFFFFFF;
  if( mmres != MMSYSERR_NOERROR )
  {
    CloseDevice();
    printf("Device prepare error");
    return 1;
  }
  return 1;
}

//Update the current playing buffer to match the current bitmask.
void UpdateNotes()
{
  int i, j, hamming_weight = 0;
  int* buf_t = calloc( SPS, sizeof(int) ); //temporary buffer to hold a sum of all signals.
  StopGen();
  //get the number of signals that are on.
  //if the signal is on, add it to buf_t.
  for( i = 0; i < 128; ++i )
  {
    if( enabled_mask[i] )
    {
      ++hamming_weight;
      for( j = 0; j < SPS; ++j )
      {
        ++bufs_cur[i];
        if( bufs_cur[i] >= bufs_lengths[i])
        {
          bufs_cur[i] = 0;
        }
        buf_t[j] += bufs[i][ bufs_cur[i] ];
      }
      SendMessage( GetDlgItem( hWnd_main, 128 | i ), WM_SETTEXT, 0, "$");
    }
    else
    {
      SendMessage( GetDlgItem( hWnd_main, 128 | i ), WM_SETTEXT, 0, " ");
    }
  }

  //divide each sample by the number of signals that is enabled.
  if( hamming_weight == 0 )
  {
    //SecureZeroMemory( out_bufs[ cur_buf ], SPS);
    SecureZeroMemory( out_buf, SPS);
  }
  else
  {
    float inverse_hamming_weight = 1.1f / hamming_weight;
    for( i = 0; i < SPS; ++i )
    {
      //printf(":%d\n", buf_t[i] );
      //out_bufs[ cur_buf ][i]  = buf_t[i] / hamming_weight;
      out_buf[i] = buf_t[i] * inverse_hamming_weight;/// hamming_weight;
    }
  }
  free( buf_t );
  GenFreq();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    //const char* labels[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "", "", "", ""};
    case WM_CREATE:
      { 
      int i,j, first_value, second_value;
      const int key_width = 48;
      const int tall_key_height = 128;
      const int short_key_height = 72;
      //const int xbase = -1 * 21 * key_width + 10;
      const int xbase = -1 * 46 * key_width + 10;
      hWnd_main = hWnd;
      bufs = (unsigned char **) calloc( 128, sizeof( unsigned char * ) );
      bufs_lengths = (int*) calloc( 128, sizeof( int ) );
      bufs_cur = (int*) calloc( 128, sizeof( int ) );
      printf("filling buffers...");
      for( i = 21; i < 88 + 21; ++i )
      { 
        int midi_note = i;
        float f = 440.0f * pow( 2.0f, ( (float)(midi_note - 69.0f) / 12.0f ) );
        bufs[i] = (unsigned char *) calloc( SPS, 1 );
        //FillBufAdd3B( bufs[i], f, 100, 0 );
        FillBufAdd3B( i, f, 100, 0 );

        if( i == 69 )
        {
          CreateWindowEx(0, "BUTTON", "A4", WS_CHILD | WS_VISIBLE, xbase + i * key_width, 24, key_width, tall_key_height, hWnd, (HMENU)( 128 + i ), NULL, NULL);
        }

        //printf("%s\n", labels[i % 12] );
        else if( i % 12 == 1 || i % 12 == 3 || i % 12 == 6 || i % 12 == 8 || i % 12 == 10 )
        { //sharp
          CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE, xbase + i * key_width, 8, key_width, short_key_height, hWnd, (HMENU)( 128 + i ), NULL, NULL);
        }
        else
        {
          CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE, xbase + i * key_width, 24, key_width, tall_key_height, hWnd, (HMENU)( 128 + i ), NULL, NULL);
        }
        //printf("%d ", i );
      }

      //setting enabled mask to 0.
      for( i = 0; i < 128; ++i )
      {
        enabled_mask[i] = 0;
      }
      printf("a");
      PrepareDevice();
      printf("b");
      //PurgeBuf(cur_buf);
      PurgeBuf(0);
      printf("c");
      GenFreq();
      printf("d");
      /*
      printf("\n\n");
      for( i = 0; i < SPS; ++i )
      {
        printf("%d ", bufs[21][i]);
      }
    
      printf("\n\n");
      for( i = 0; i < SPS; ++i )
      {
        printf("%d ", bufs[108][i]);
      }
      */
      }//WM_CREATE
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_COMMAND:
      if( LOWORD( wParam ) | 128 )
      {
        int midi_note = 127 & LOWORD( wParam );
        enabled_mask[ midi_note ] = !enabled_mask[ midi_note ];
        UpdateNotes();
      }
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
  MSG msg1;
  WNDCLASS wc1;
  HWND hWnd1;
  ZeroMemory(&wc1, sizeof wc1);
  wc1.hInstance = hInst;
  wc1.lpszClassName = AppName;
  wc1.lpfnWndProc = (WNDPROC)WndProc;
  wc1.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
  wc1.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
  wc1.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  wc1.hCursor = LoadCursor(NULL, IDC_ARROW);
  if(RegisterClass(&wc1) == FALSE) return 0;
  hWnd1 = CreateWindow(AppName, AppName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, 360, 240, 0, 0, hInst, 0);
  if(hWnd1 == NULL) return 0;
  while(GetMessage(&msg1,NULL,0,0) > 0){
    TranslateMessage(&msg1);
    DispatchMessage(&msg1);
  }
  return msg1.wParam;
}
