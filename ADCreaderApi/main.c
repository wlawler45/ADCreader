//------------------------------------------------------------------------------
// PCIe104-24DSI6LN Sample Code
//------------------------------------------------------------------------------
// Revision History:
//------------------------------------------------------------------------------
// Revision  Date        Name      Comments
// 1.0		 06/03/14	 Gary	   Initial Release
//------------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include <memory.h>
#include "CioColor.h"
#include "Tools.h"
#include "24DSI6LNeintface.h"


// Main Test Routines for the DSI
void init_verify(void);
void analog_inputs(void);
void SelfTest(void);
void DmaTest(FILE *output);
void Sample_Rate(void);
void ContinuousSaveData(void);
void MultiBoardSync(void);

DWORD WINAPI
InterruptAttachThreadDma(
    LPVOID pParam
    );


// Subs for the 24DSI
float  get_rms(void);
void   get_avg_reading(U32* chan_Values, int num_readings);
void   Scan_display(void);
void   PutCursor(U32,U32);
double calc_Parameters(U32* nVco, U32* nRef, U32* nDiv, double desired_Freq);

U32  *BuffPtr, ReadValue[512], indata, data_in;
U32 ulNumBds, ulBdNum, ulAuxBdNum, ulErr = 0;
U16 saved_x, saved_y, CurX=2, CurY=2;
char mmchar, kbchar, datebuf[16], timebuf[16], test_mode=0x00;
double vRange=0;
double fGen_nRate[100000] = {0.0};
char cBoardInfo[400];
U32 uData[262144], Data[262144];
U32 dNumChan=0;
U32 b1Ready,b2Ready,b1Done,b2Done;
double oscFreq = 0.0;

U32 *Buff1;
U32 *Buff2;
U32 *wBuff;

U8 contRunning;
volatile U8 contThreadStatus;
U32 BCR_register= 0x008310;
//
U32 PLL_register= 0x00200064;
///Rate assignment a=
U16 Rate_divisor_address=0x0010;
U32 Rate_divisor=0x00;
//U16 Buffer_address=0x0020;
U32 buffer_off_register=0x003C0FFE; ///to turn off collection
U32 buffer_on_register=0x00380FFE; ///to turn on collection
//==============================================================================
//
//==============================================================================
void main(int argc, char * argv[])
{
  //CursorVisible(FALSE);
  FILE *output;
    char *mode= "w";
    char *filename;
    filename="data.csv";
    char exitchar=NULL;
  ulNumBds = DSI6LN_FindBoards(&cBoardInfo[0], &ulErr);
  if(ulErr)
  {
   ShowAPIError(ulErr);
   do{}while( !kbhit() );         // Wait for a Key Press
   exit(0);
  }
  if(ulNumBds < 1)
  {
      printf("  ERROR: No Boards found in the system\n");
      do{}while( !kbhit() );         // Wait for a Key Press
  }
  else
  {
    ulBdNum = 1;



	DSI6LN_Get_Handle(&ulErr, ulBdNum);
    if(ulErr)
	{
     ShowAPIError(ulErr);
     do{}while( !kbhit() );         // Wait for a Key Press
     exit(0);
	}

	}
      ClrScr();
      printf("\n\n");
    indata = DSI6LN_Read_Local32(ulBdNum, &ulErr, BOARD_CONFIG);
    if(!(indata & 0x400000)){
	  printf("Contact General Standards for the correct example");
      do{}while( !kbhit() );         // Wait for a Key Press
      exit(0);
	}
	switch((indata>>17)&0x03){
		case 1: vRange = 5.00; break;
		case 2: vRange = 2.50; break;
		default: vRange = 10.0; break;
	}
    if(indata&0x10000)
	  dNumChan = 6;
	else
      dNumChan = 4;


    do
    {
      ClrScr();
      printf("\n\n");
      printf("  ====================================================\n");
      printf("   Delta Sigma (DSI) Control Code - Board # %d         \n",ulBdNum);
      printf("  ====================================================\n");
      printf("   1 - Begin Data Collection                             \n");
      printf("   X - EXIT (return to DOS)                           \n");
      mmchar = prompt_for_key("  Please Make selection: \n");

      switch(mmchar)
      {
        case '1':
            output=fopen(filename,mode);
          init_verify();
          ClrScr();
          printf("Press any button to stop data collection.");
          while( !kbhit() ){
            DmaTest(output);
          }
            fclose(output);
            break;



        case 'X':
          ClrScr();
          break;
        default:
          printf("  Invalid Selection: Press AnyKey to continue...\n");

          break;
      }
    }while(mmchar!='X');

	DSI6LN_Close_Handle(ulBdNum, &ulErr);

	CursorVisible(TRUE);
	ClrScr();

} /* end main */

//------------------------------------------------------------------------------
// Initialization Verification
//------------------------------------------------------------------------------
void init_verify(void)
{
  U32 ValueRead;



  DSI6LN_DisableInterrupt(ulBdNum, 0xFF,LOCAL, &ulErr);
  DSI6LN_Write_Local32(ulBdNum, &ulErr, BCR,0x0310);
    DSI6LN_Write_Local32(ulBdNum, &ulErr, RATE_CONTROL_A,0x00200064);
  DSI6LN_Write_Local32(ulBdNum, &ulErr, RATE_DIV,0x0000);
  DSI6LN_Write_Local32(ulBdNum, &ulErr, BUFFER_CONTROL,0x00380FFE);

  Busy_Signal(110);


  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, BCR);
  printf("Board Control Register     =     %04X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, RATE_CONTROL_A);
  printf("Rate Control Register A,B  = %08X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, RATE_CONTROL_B);
  printf("Rate Control Register C,D  = %08X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, RATE_ASSIGN);
  printf("Rate Assignments Register  = %08X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, RATE_DIV);
  printf("Rate Divisors              = %08X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, PLL_REF_FREQ);
  printf("PLL Reference Frequency    = %08X", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, BUFFER_CONTROL);
  printf("Buffer Control Register    = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, BOARD_CONFIG);
  printf("Board Config *Undocumented = %08X",  ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, GPS_SYNC);
  printf("GPS Synchronization        = %08X",  ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = DSI6LN_Read_Local32(ulBdNum, &ulErr, INPUT_DATA_BUFFER);
  printf("Input Data Register        = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  anykey();
}




//------------------------------------------------------------------------------
// DMA Test
//------------------------------------------------------------------------------
void DmaTest(FILE *output)
{
  GS_NOTIFY_OBJECT event[2];
  U32 ulWords;
  HANDLE myThread;
  HANDLE theHandles[MAXIMUM_WAIT_OBJECTS];
  HANDLE fbHandle;
  HANDLE sbHandle;
  DWORD EventStatus;
  int ok,i;
  U32 nVco,nRef,nDiv;
  double actualFreq;

  ClrScr();
  CurX=CurY=2;

  DSI6LN_Initialize(ulBdNum, &ulErr);

  PutCursor(CurX,19);
  printf("Initializing ...                            ");
  Busy_Signal(110);

  if(actualFreq = calc_Parameters(&nVco, &nRef, &nDiv, 200000.0)){ // 200K Sample Rate
     DSI6LN_Write_Local32(ulBdNum, &ulErr, RATE_CONTROL_A, (((nRef<<16)&0xFFFF0000)|(nVco&0xFFFF)));
     DSI6LN_Write_Local32(ulBdNum, &ulErr, RATE_DIV, (((nDiv<<8)&0xFF00)|(nDiv&0xFF)));
  }
  else{
     ClrScr();
     CurX=CurY=2;
     PutCursor(CurX,CurY++);
     printf("Error calculating parameters for frequency setup\n");
     anykey();
     return;
  }

  DSI6LN_Write_Local32(ulBdNum, &ulErr, RATE_ASSIGN, 0x0000); // All to Generator A
  //DSI6LN_Write_Local32(ulBdNum, &ulErr, BCR,0x0050);// Synchronous helps to synchronize the input data at this point

 // Sleep(500);

  // Clear, Disable the Buffer: 128K Threshold with 20 bits
  DSI6LN_Write_Local32(ulBdNum, &ulErr, BUFFER_CONTROL, 0x3DFFF8);
  fbHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (fbHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      return;
  }
  event[0].hEvent = (uint64_t)fbHandle;
  //Sleep(3000);
  DSI6LN_EnableInterrupt(ulBdNum, 0x03, LOCAL, &ulErr);
  DSI6LN_Register_Interrupt_Notify(ulBdNum, &event[0], 0x03, LOCAL, &ulErr);


  memset(Data,0,0x40000);
  memset(uData,0,0x40000);
  DSI6LN_Open_DMA_Channel(ulBdNum,0,&ulErr);

  ok = 1;

  myThread = GetCurrentThread();
  SetThreadPriority(myThread,THREAD_PRIORITY_ABOVE_NORMAL);
  DSI6LN_Write_Local32(ulBdNum, &ulErr, BUFFER_CONTROL, 0x31FFF8);

  PutCursor(CurX,19);
  for(i=0; i<500; i++){
    data_in = DSI6LN_Read_Local32(ulBdNum, &ulErr, BCR);
	if(data_in & 0x2000)
		i=500;
	Sleep(10);
  }

  printf("Acquisition Started.. Anykey to exit             ");

  do{


    EventStatus = WaitForSingleObject(fbHandle,15 * 1000); // Wait for the interrupt

	  switch(EventStatus)
	  {
		case WAIT_OBJECT_0:
		case WAIT_OBJECT_0+1: {
			ulWords = DSI6LN_DMA_ToVirtualMem(ulBdNum,0,0x1FFF8,Data,&ulErr);
			if(ulErr){
				ShowAPIError(ulErr);
				printf("\nError from Bd#%ld\n",ulBdNum);
			}

            for(int i=0;i>0x1FFF8;i++){
                fprintf(output,"/n%d",Data[i]);
            }

		}
			break;
		default:
			printf("\n\n  Interrupt did NOT Occur, No DMA Data");
		    ulWords = DSI6LN_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);
		    printf("Buffer Size = %ld             \n",ulWords);
			ok = 0;
			break;
	  }
  }while(!kbhit() && ok);

  SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
  DSI6LN_Cancel_Interrupt_Notify(ulBdNum, &event[0], &ulErr);
  CloseHandle(fbHandle);
  DSI6LN_Close_DMA_Channel(ulBdNum,0,&ulErr);
  DSI6LN_DisableInterrupt(ulBdNum, 0x03, LOCAL, &ulErr);

  if(!ok)
	  anykey();
}



// Function to write data to disk when told

DWORD WINAPI
InterruptAttachThreadDma(
    LPVOID pParam
    )
{
  FILE *myFile[1024];
  U32 wSize;
  U32 b1Written = 0;
  U32 b2Written = 0;
  U32 samplesWritten = 0;
  char fName[256];
  int filenum = 0;
  int k,l;
  int files_needed;
  double total_written;
  double bytes_needed;

  bytes_needed = *((double *)(pParam));
  if(bytes_needed < 3000000000.0)
   files_needed = 1;
  else
   files_needed = ((int)(bytes_needed / 3000000000.0)+1);
  if(files_needed >= 1024)
	  files_needed = 1024;
  for(k=0; k<files_needed; k++){
  	sprintf(fName,"DSIData%04d.txt",k);
  myFile[k] = fopen(fName,"w+b"); // Doesn't append, always overwrites
  if(myFile[k] == NULL){
	  printf("\n  Error opening file for save.. Cannot continue");
	  l=k;
	  for(l=0; l<k; l++)
	  fclose(myFile[l]);
	  contThreadStatus = 0;
	  return 0;
  }
  } // end For()

  b1Done = 1;
  b2Done = 1;
  total_written = 0.0;

  // Signal that thread is ready
    contThreadStatus = 1;

    while(contRunning){
	   if(samplesWritten > 750000000){
		 total_written += (double)samplesWritten;
   	     filenum++;
		 samplesWritten = 0;
		   }
		if(b1Ready && !b1Written){
		   b1Done = 0;
		   wSize = b1Ready;
		   samplesWritten += b1Ready;
	       memmove(wBuff,Buff1,wSize*4); // BYTES
		   b1Done = 1;
		   fwrite(wBuff,sizeof(U32),wSize,myFile[filenum]);
           printf("*");
		   b1Written = 1;
		   b2Written = 0;
		}
		if(b2Ready && !b2Written){
		   b2Done = 0;
		   wSize = b2Ready;
		   samplesWritten += b2Ready;
	       memmove(wBuff,Buff2,wSize*4); // BYTES
		   b2Done = 1;
		   fwrite(wBuff,sizeof(U32),wSize,myFile[filenum]);
           printf("+");
		   b2Written = 1;
		   b1Written = 0;
		}
	 Sleep(100);

	} // End while(contRunning)

	for(k=0; k<files_needed; k++)
	  fclose(myFile[k]);
	if(filenum == 0)
		total_written = (double)samplesWritten;
    printf("\n  %.1lf Samples successfully written ..   \n",total_written);
    contThreadStatus = 0;
 return 0;
}

//==============================================================================



//==============================================================================
void PutCursor(U32 FixedX, U32 FixedY)
{
  PositionCursor((U16) FixedX,(U16) FixedY);
}


//==============================================================================
double calc_Parameters(U32* nVco, U32* nRef, U32* nDiv, double desired_Freq){

    double div = 0.5;
    double fGen;
	double ratio;
	double actual_Freq;
	double theValue, theMin = 1000.0;
	U32 theData;
	int i;

	if((desired_Freq < 2000) || (desired_Freq > 200000))
		return 0;

	for(i=0; i<26; i++){
        fGen = 512.0*div*desired_Freq;
		if((fGen >= 25600000.0) && (fGen <= 51200000.0)){
			*nDiv = (U32)i;
		    i = 26;
		}
	    if(div == 0.5)
		   div = 1.0;
	    else
		   div += 1.0;
	}
    if(i != 27){	// Didn't find a valid fGen value
		*nVco = 30; // Should never happen, but...
		*nRef = 30;
		*nDiv = 25;
		return 0;
	}
	*nVco = 30;
    if(oscFreq == 0.0){
       theData = DSI6LN_Read_Local32(ulBdNum, &ulErr, PLL_REF_FREQ);
       oscFreq = (double) theData;
	}
    ratio = fGen / oscFreq;

	for(i=30; i<1000; i++){
	 theValue = fabs(fmod(((double)i*ratio),1.0));
	 if(theValue < theMin){
		 *nRef = i;
		 *nVco = (U32)((double)i*ratio);
		 if((double)*nVco > (1000.0-ratio))
			 i=1000;
		 theMin = theValue;
	 }
	 if(theMin == 0.0)
		 i=1000;
	}
	// Should have a value when we get to here

    if((*nVco < 30) || (*nRef < 30)){
		*nVco *= 2;
		*nRef *= 2;
	}

	actual_Freq = (double)*nVco / (double)*nRef;
	actual_Freq *= oscFreq;
	if(*nDiv)
	  actual_Freq /= ((double)*nDiv * 512.0);
    else
	  actual_Freq /= (0.5 * 512.0);

	return(actual_Freq);
}
