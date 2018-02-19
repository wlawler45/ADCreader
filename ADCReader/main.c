#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <conio.h>
#include "Plx.h"
#include "PlxApi.h"
#include "PlxInit.h"
//List of variables associated with ADC chip initialization and PCI configuration registers
U16 BCR_address=0x0000;
U32 BCR_register= 0x008310;
U16 PLL_address= 0x0004;
U32 PLL_register= 0x00200064;
///Rate assignment a=
U16 Rate_divisor_address=0x0010;
U32 Rate_divisor=0x00;
U16 Buffer_address=0x0020;
U32 buffer_off_register=0x003C0FFE; ///to turn off collection
U32 buffer_on_register=0x00380FFE; ///to turn on collection
///vendor ID=10B5  Device ID=9080
///1416


U16 ChipTypeSelected;
U8  ChipRevision;
/*
Initialize the PCI chip using the PLX API, enables the interrupt as well
*/
PLX_DEVICE_OBJECT pciInit(){

    PLX_STATUS        rc;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;
    PLX_INTERRUPT     PlxInterrupt;
    //printf("*ERROR* - API failed\n");
    rc =PlxPci_DeviceOpen(&DeviceKey,&Device);
    printf("\nSelected: %04x %04x [b:%02x  s:%02x  f:%x]\n\n",DeviceKey.DeviceId, DeviceKey.VendorId,DeviceKey.bus, DeviceKey.slot, DeviceKey.function);

    PlxPci_ChipTypeGet(&Device,&ChipTypeSelected,&ChipRevision);
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));
    PlxInterrupt.LocalToPci = 1;
    rc =PlxPci_InterruptEnable(&Device,&PlxInterrupt);
}
/*
pciConfig function writes values to registers on the PCI controller to successfully initialize and configure the ADC chip and checks to make sure each configuration write finishes successfully
If config not working move reset to front and try again
*/
void pciConfig(PLX_DEVICE_OBJECT *Device){
    PLX_STATUS        rc;
    U32 regvalue;
    regvalue=PlxPci_PciRegisterReadFast(Device,PLL_address,rc);
    printf(regvalue); //code to test if registers are reading the default values correctly.
    rc =PlxPci_PciRegisterWriteFast(Device,PLL_address,PLL_register);

    if (rc != PLX_STATUS_OK)
    {
        printf("*ERROR* First write failed- \n");

        return;
    }
    rc =PlxPci_PciRegisterWriteFast(Device,Rate_divisor_address,Rate_divisor);

    if (rc != PLX_STATUS_OK)
    {
        printf("*ERROR* - Second write failed\n");

        return;
    }
    rc =PlxPci_PciRegisterWriteFast(Device,BCR_address,BCR_register);

    if (rc != PLX_STATUS_OK)
    {
        printf("*ERROR* - Third write failed\n");

        return;
    }
    rc =PlxPci_PciRegisterWriteFast(Device,Buffer_address,buffer_on_register);

    if (rc != PLX_STATUS_OK)
    {
        printf("*ERROR* - Fourth Write failed\n");

        return;
    }
    rc=PlxPci_DeviceReset(Device);
    if (rc != PLX_STATUS_OK)
    {
        printf("Reset failed\n");

        return;
    }

}
/*
Initialize DMA transfer ability to allow mass data transfer from the ADC via the PCI interface
*/
void initDMA(PLX_DEVICE_OBJECT *Device, PLX_DEVICE_KEY *DeviceKey){
    PLX_STATUS        rc;
    PLX_DRIVER_PROP   DriverProp;
    rc =PlxPci_DeviceOpen(&DeviceKey,&Device);
    printf("*ERROR* - API failed\n");
    if (rc != PLX_STATUS_OK)
    {
        exit(-1);
    }
    PlxPci_DriverProperties(&Device,&DriverProp);

    if (DriverProp.bIsServiceDriver)
        {
            exit(-1);
        }
}
/*uint32_t pciConfigReadWord (uint8_t bus,
                             uint8_t func, uint8_t offset)
 {

    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = 9080;
    uint32_t lfunc = (uint32_t)func;
    uint32_t tmp = 0;

    /* create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    /* write out the address */
   /// sysOutLong (0xCF8, address);
    /* read in the data
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register
    ///tmp = (uint32_t)((sysInLong (0xCFC) >> ((offset & 2) * 8)) & 0xffffffff);
    return (tmp);
 }
*/
                // Open device to get its properties
            // Verify driver used is PnP and not Service driver
/*
Perform DMA data transfer of data over PCI interface
*/
void PerformDma_9000(PLX_DEVICE_OBJECT *pDevice, FILE *output)
{
    U8                DmaChannel=0;
    U8               *pUserBuffer = NULL;
    U16               ChannelInput;
    U32               LocalAddress=0x00000100;
    PLX_STATUS        rc;
    PLX_DMA_PROP      DmaProp;
    PLX_INTERRUPT     PlxInterrupt;
    PLX_DMA_PARAMS    DmaParams;
    PLX_PHYSICAL_MEM  PciBuffer;
    PLX_NOTIFY_OBJECT NotifyObject;

     rc =PlxPci_NotificationCancel(pDevice,&NotifyObject);
     memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));
    DmaChannel = (U8)ChannelInput;

    // Get DMA buffer parameters
    PlxPci_CommonBufferProperties(pDevice,&PciBuffer);

    // Clear DMA structure
    memset(&DmaProp, 0, sizeof(PLX_DMA_PROP));

    // Initialize the DMA channel
    DmaProp.LocalBusWidth = 3;   // 32-bit
    DmaProp.ReadyInput    = 1;


    rc =PlxPci_DmaChannelOpen(pDevice,DmaChannel,&DmaProp);
    // Clear interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTERRUPT));

    // Setup to wait for selected DMA channel
    PlxInterrupt.DmaDone = (1 << DmaChannel);

    rc =PlxPci_NotificationRegisterFor(pDevice,&PlxInterrupt,&NotifyObject);

    // Transfer the Data
    //Cons_printf("  Perform Block DMA transfer..... ");
     if (pUserBuffer == NULL) pUserBuffer = malloc(PciBuffer.Size);
    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.PciAddr   = PciBuffer.PhysicalAddr;
    DmaParams.LocalAddr = LocalAddress;
    DmaParams.ByteCount = 0x3FF8;
    DmaParams.Direction = PLX_DMA_LOC_TO_PCI;

    rc =PlxPci_DmaTransferBlock(pDevice,DmaChannel,&DmaParams,
            0          // Don't wait for completion
            );

    if (rc == PLX_STATUS_OK)
    {
        //notification to wait for DMA transfer
        rc =PlxPci_NotificationWait(pDevice,&NotifyObject,5 * 1000);

        switch (rc)
        {
            case PLX_STATUS_OK:
                printf("Ok (DMA Int received)\n");
                break;

            case PLX_STATUS_TIMEOUT:
                printf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case PLX_STATUS_CANCELED:
                printf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                printf("*ERROR* - API failed\n");

                break;
        }
    }
    else
    {
        printf("*ERROR* - API failed\n");
    }
    rc =PlxPci_NotificationStatus(pDevice,&NotifyObject,&PlxInterrupt);

    // Allocate a buffer
    pUserBuffer = malloc(0x3FF8);

    // Clear DMA data
    memset(&DmaParams, 0, sizeof(PLX_DMA_PARAMS));

    DmaParams.UserVa    = (PLX_UINT_PTR)pUserBuffer;
    DmaParams.LocalAddr = LocalAddress;
    DmaParams.ByteCount = 0x3FF8;
    DmaParams.Direction = PLX_DMA_LOC_TO_PCI;

    rc =PlxPci_DmaTransferUserBuffer(pDevice,DmaChannel,&DmaParams,
            200         // Specify a timeout to let API perform wait
            );

        if(pUserBuffer == NULL) {
            printf("malloc of size %d failed!\n", 50);   // could also call perror here
            exit(1);   // or return an error to caller
        }
    long tempdata;
    int i;
    //loop to iterate for all allocated memory from the DMA transfer operation and writes each entry into an data field in the csv output file.
    for(i=0;i<0x3FF9;i+=4){
        tempdata=(long)((pUserBuffer[i+3]<<24)+(pUserBuffer[i+2]<<16)+(pUserBuffer[i+1]<<8)+pUserBuffer[i]);
        tempdata=tempdata&0x00FFFFFF;
        fprintf(output,"/n%d",tempdata);
    }
    free(pUserBuffer);



}
/*
Function to close PCI interface, DMA channel and also the data file
*/
void stop_sampling(PLX_DEVICE_OBJECT *pDevice,FILE *output){
    PLX_STATUS        rc;
    U8 DmaChannel=0;
    rc =PlxPci_DeviceClose(&pDevice);
    // Close DMA Channel
    printf("  Close DMA Channel.............. ");
    rc =PlxPci_DmaChannelClose(pDevice,DmaChannel);
    fclose(output);
    // Release user buffer
}

int main()
{
    FILE *output;
    char *mode= "w";
    //char *extension=".csv";
    char *filename;
    filename="data.csv";
    char exitchar=NULL;
    S16 DeviceSelected;
    PLX_DEVICE_OBJECT Device;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_STATUS        rc;
    PLX_NOTIFY_OBJECT NotifyObject;
    //("*ERROR* - API failed\n");
    //scanf("%s",filename);
    //printf("*ERROR* - API failed\n");
    Device=pciInit();


    pciConfig(&Device);
    printf("pci config finished");

    initDMA(&Device, &DeviceKey);
    //strcat(filename,extension);
    output=fopen(filename,mode);

    while(exitchar==NULL){
        //wait for PCI interface to signal that PCI memory buffer is full before starting DMA transfer
        rc =PlxPci_NotificationWait(&Device,&NotifyObject,10 * 1000);
        PerformDma_9000(&Device,output);
        //provide simple exit for program
        exitchar=getch();

    }
    //close PCI interface and DMA data channel
    stop_sampling(&Device,output);
}

