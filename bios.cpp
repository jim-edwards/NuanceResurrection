#include "basetypes.h"
#ifdef ENABLE_EMULATION_MESSAGEBOXES
#include <windows.h>
#endif

#include <string>
#include "byteswap.h"
#include "media.h"
#include "mpe.h"
#include "audio.h"
#include "Bios.h"
#include "dma.h"
#include "file.h"
#include "joystick.h"
#include "memory.h"
#include "mpe_alloc.h"
#include "NuonEnvironment.h"
#include "NuonMemoryMap.h"
#include "PresentationEngine.h"
#include "timer.h"
#include "video.h"

extern NuonEnvironment nuonEnv;
extern VidChannel structOverlayChannel;
extern VidChannel structMainChannel;
extern VidChannel structMainChannelPrev;
extern VidChannel structOverlayChannelPrev;

void KPrintf(MPE &mpe);

const char *BiosRoutineNames[512] = {
"CommSend",
"CommSendInfo",
"CommRecvInfo",
"CommRecvInfoQuery",
"CommSendRecv",
"CommSendRecvInfo",
"ControllerInitialize",
"ControllerExtendedInfo",
"TimeOfDay",
"DCacheSyncRegion",
"DCacheSync",
"DCacheInvalidateRegion",
"DCacheFlush",
"TimerInit",
"TimeElapsed",
"TimeToSleep",
"MPEAlloc",
"MPEAllocSpecific",
"MPEFree",
"MPEsAvailable",
"IntSetVectorX",
"IntGetVector",
"VidSync",
"VidSetup",
"VidConfig",
"VidQueryConfig",
"VidChangeBase",
"VidChangeScroll",
"VidSetCLUTRange",
"BiosInit",
"BiosExit",
"BiosReboot",
"BiosPoll",
"BiosPauseMsg",
"AudioQueryChannelMode",
"AudioSetChannelMode",
"AudioQuerySampleRate",
"AudioSetSampleRate",
"AudioReset",
"AudioMute",
"AudioSetDMABuffer",
"MemInit",
"MemAdd",
"MemAlloc",
"MemFree",
"MemLocalScratch",
"MemLoadCoffX",
"DownloadCoff",
"StreamLoadCoff",
"DMALinear",
"DMABiLinear",
"FileOpen",
"FileClose",
"FileRead",
"FileWrite",
"FileIoctl",
"FileFstat",
"FileStat",
"FileIsatty",
"FileLseek",
"FileLink",
"FileLstat",
"FileUnlink",
"NetAccept",
"NetBind",
"NetConnect",
"NetGethostname",
"NetGetpeername",
"NetGetsockname",
"NetGetsockopt",
"NetListen",
"NetRecv",
"NetRecvfrom",
"NetRecvmsg",
"NetSend",
"NetSendmsg",
"NetSendto",
"NetSethostname",
"NetSetsockopt",
"NetShutdown",
"NetSocket",
"CommSendDirect",
"comm_recv",
"comm_query",
"_serial_delay",
"_serial_read",
"_serial_write",
"_serial_write_direct",
"MediaOpen",
"MediaClose",
"MediaGetDevicesAvailable",
"MediaGetInfo",
"MediaGetStatus",
"MediaRead",
"MediaWrite",
"MediaIoctl",
"spinwait",
"CacheConfigX",
"LoadGame",
"LoadPE",
"Dma_wait",
"Dma_do",
"PatchJumptable",
"BiosResume",
"MPEStop",
"MPERun",
"MPEWait",
"MPEReadRegister",
"MPEWriteRegister",
"SetParentalControl",
"GetParentalControl",
"BiosGetInfo",
"LoadTest",
"MPELoad",
"MPEAllocThread",
"MediaInitMPE",
"MediaShutdownMPE",
"SecureForPE",
"StartImageValid",
"SetStartImage",
"GetStartImage",
"FindName",
"DeviceDetect",
"MPERunThread",
"BiosIRMask",
"DiskChange",
"DiskGetTotalSlots",
"pf_add_driver",
"VidSetBorderColor",
"DisplayBootImage",
"serial_write_cmd",
"GetMemDevice",
"WriteMemDevSector",
"ReadMemDev",
"AttachFsDevice",
"DiskEject",
"DiskRetract",
"GetSystemSettingsB",
"GetSystemSetting?",
"SetSystemSetting??",
"GetSystemSettingLe",
"LoadSystemSettings",
"StoreSystemSetting",
"mount",
"MPEStatus",
"kprintf",
"ControllerPollRate",
"VidSetOutputType",
"LoadDefaultSystemSettings",
"SetISRExitHook",
"CompatibilityMode"
};

void UnimplementedFileHandler(MPE &mpe)
{
  //::MessageBox(NULL,"This BIOS Handler does nothing","Unimplemented File Routine",MB_OK);
}

void UnimplementedMediaHandler(MPE &mpe)
{
#ifdef ENABLE_EMULATION_MESSAGEBOXES
  MessageBox(NULL,"This BIOS Handler does nothing","Unimplemented Media Routine",MB_OK);
#endif
}

void UnimplementedCacheHandler(MPE &mpe)
{
  //::MessageBox(NULL,"This BIOS Handler does nothing","Unimplemented Cache Routine",MB_OK);
}

void UnimplementedCommHandler(MPE &mpe)
{
#ifdef ENABLE_EMULATION_MESSAGEBOXES
  MessageBox(NULL,"This BIOS Handler does nothing","Unimplemented Comm Routine",MB_OK);
#endif
}

void NullBiosHandler(MPE &mpe)
{
  //char msg[512];
  //sprintf(msg,"This BIOS Handler does nothing: %ld",(mpe->pcexec >> 1) & 0xFFUL);
  //::MessageBox(NULL,msg,"Unimplemented BIOS Routine",MB_OK);
}

void AssemblyBiosHandler(MPE &mpe)
{
}

void WillNotImplement(MPE &mpe)
{
  //char msg[512];
  //sprintf(msg,"This BIOS Handler does nothing: %ld",(mpe->pcexec >> 1) & 0xFFUL);
  //::MessageBox(NULL,msg,"Unimplemented BIOS Routine",MB_OK);
}

void SetISRExitHook(MPE &mpe)
{
  const uint32 newvec = SwapBytes(mpe.regs[0]);
  *((uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,ISR_EXIT_HOOK_ADDRESS)) = newvec;
}

bool InstallCommHandler(MPE &mpe, uint32 address, uint32 *handlerList, uint32 *nHandlers)
{
  uint32 numHandlers = SwapBytes(*((uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,NUM_INSTALLED_COMMRECV_HANDLERS_ADDRESS)));
  uint32 *pHandlers = handlerList;
  bool bFound = false;

  SwapScalarBytes(&address);

  uint32 i;
  for(i = 0; i < numHandlers; i++)
  {
    if(address == *pHandlers)
    {
      bFound = true;
      break;
    }
    pHandlers++;
  }

  //Didn't find it and the list is full
  if(i >= MAX_RECV_HANDLERS)
  {
    *nHandlers = MAX_RECV_HANDLERS;
    return false;
  }

  //Didn't find it in the list and there is room so install it
  if(!bFound)
  {
    *pHandlers = address;
    i++;
    *nHandlers = i;
    *((uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,NUM_INSTALLED_COMMRECV_HANDLERS_ADDRESS)) = SwapBytes(i);
    return true;
  }

  //Found it in the list so uninstall it, shifting remaining handlers by one
  while(i < (MAX_RECV_HANDLERS - 1))
  {
    *pHandlers = *(pHandlers+1);
    pHandlers++;
    i++;
  }
  *pHandlers = 0;
  
  numHandlers--;
  *nHandlers = numHandlers;
  *((uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,NUM_INSTALLED_COMMRECV_HANDLERS_ADDRESS)) = SwapBytes(numHandlers);
  return false;
}

void IntGetVector(MPE &mpe)
{
  const uint32 which = mpe.regs[0];

  if(which < 32)
  {
    const uint32* const InterruptVectors = (uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,INTERRUPT_VECTOR_ADDRESS);
    mpe.regs[0] = SwapBytes(InterruptVectors[which]);
  }
  else
    mpe.regs[0] = 0;
}

void IntSetVector(MPE &mpe)
{
  const uint32 which = mpe.regs[0];
  const uint32 newvec = mpe.regs[1];

  mpe.regs[0] = 0;

  // special handling of kIntrVideo (which==31) and kIntrAudio (which==27): handled via the trigger interrupts due to the vid timer and the audio stream callback!

  if(which < 32)
  {
    if(which == 4)
    {
      if(newvec < MPE_ADDR_SPACE_BASE)
      {
        return;
      }

      uint32 * const recvHandlers = ((uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,COMMRECV_HANDLER_LIST_ADDRESS));
      uint32 numHandlers;
      if(InstallCommHandler(mpe, newvec, recvHandlers, &numHandlers))
      {
        mpe.regs[0] = newvec;
      }
    }
    else
    {
      uint32* const InterruptVectors = (uint32 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,INTERRUPT_VECTOR_ADDRESS);
      mpe.regs[0] = InterruptVectors[which];

      if(!newvec)
      {
        //disable the interrupt
        mpe.inten1 &= (~(1UL << which));
      }
      else
      {
        //Not needed in this implementation, but needed if this IntSetVector is moved to aries assembly
        mpe.intsrc &= (~(1UL << which));
        //Enable the interrupt in case it was previously disabled
        mpe.inten1 |= (1UL << which);
      }

      InterruptVectors[which] = SwapBytes(newvec);
    }
  }
}

void BiosExit(MPE &mpe)
{
  //const uint32 return_value = mpe.regs[0];
  mpe.Halt();
}

uint32 PatchJumptable(const uint32 vectorAddress, uint32 newEntry)
{
  uint32 oldEntryImmExt;
  uint32 oldEntryInst;
  uint32 immExt;
  uint32 inst;

  newEntry >>= 1;

  //create 64 bit JMP <newEntry>, nop instruction
  inst = 0x9220BA00UL | ((newEntry & 0x1FUL) << 16) | ((newEntry & 0x1FE0UL) >> 5);
  immExt = 0x88000000UL | ((newEntry & 0x7FFFE000UL) >> 4);
  //get the old entry stored in the BIOS vector address
  oldEntryImmExt = *((uint32 *)(&(nuonEnv.systemBusDRAM[vectorAddress - SYSTEM_BUS_BASE + 0])));
  oldEntryInst = *((uint32 *)(&(nuonEnv.systemBusDRAM[vectorAddress - SYSTEM_BUS_BASE + 4])));
  SwapScalarBytes(&inst);
  SwapScalarBytes(&immExt);
  SwapScalarBytes(&oldEntryImmExt);
  SwapScalarBytes(&oldEntryInst);
  //load the new entry into the BIOS vector
  *((uint32 *)(&(nuonEnv.systemBusDRAM[vectorAddress - SYSTEM_BUS_BASE + 0]))) = immExt;
  *((uint32 *)(&(nuonEnv.systemBusDRAM[vectorAddress - SYSTEM_BUS_BASE + 4]))) = inst;

  //extract the old BIOS function address from the previous entry's JMP instruction
  const uint32 oldAddress = (((oldEntryImmExt & 0x7FFFE00UL) << 4) | ((oldEntryInst & 0xFFUL) << 5) | ((oldEntryInst & 0x1F0000) >> 16)) << 1;

  return oldAddress;
}

void PatchJumptable(MPE &mpe)
{
  const uint32 vectorAddress = mpe.regs[0];
  const uint32 newAddress = mpe.regs[1];

  mpe.regs[0] = PatchJumptable(vectorAddress, newAddress);
}

void BiosGetInfo(MPE &mpe)
{
  mpe.regs[0] = 0x80760000;
}

NuonBiosHandler BiosJumpTable[256] = {
AssemblyBiosHandler, //_CommSend (0)
AssemblyBiosHandler, //_CommSendInfo (1)
AssemblyBiosHandler, //_CommRecvInfo (2)
AssemblyBiosHandler, //_CommRecvInfoQuery (3)
AssemblyBiosHandler, //_CommSendRecv (4)
AssemblyBiosHandler, //_CommSendRecvInfo (5)
ControllerInitialize, //_ControllerInitialize (6)
NullBiosHandler, //_ControllerExtendedInfo (7)
TimeOfDay, //_TimeOfDay (8)
UnimplementedCacheHandler, //_DCacheSyncRegion (9)
UnimplementedCacheHandler, //_DCacheSync (10)
UnimplementedCacheHandler, //_DCacheInvalidateRegion (11)
UnimplementedCacheHandler, //_DCacheFlush (12)
TimerInit, //_TimerInit (13)
TimeElapsed, //_TimeElapsed (14)
AssemblyBiosHandler, //_TimeToSleep (15)
MPEAlloc, //_MPEAlloc (16)
MPEAllocSpecific, //_MPEAllocSpecific (17)
MPEFree, //_MPEFree (18)
MPEsAvailable, //_MPEsAvailable (19)
IntSetVector, //_IntSetVector (20)
IntGetVector, //_IntGetVector (21)
VidSync, //_VidSync (22)
VidSetup, //_VidSetup (23)
VidConfig, //_VidConfig (24)
VidQueryConfig, //_VidQueryConfig (25)
VidChangeBase, //_VidChangeBase (26)
VidChangeScroll, //_VidChangeScroll (27)
VidSetCLUTRange, //_VidSetCLUTRange (28)
InitBios, //_BiosInit (29)
BiosExit, //_BiosExit (30)
NullBiosHandler, //_BiosReboot (31)
BiosPoll, //_BiosPoll (32)
BiosPauseMsg, //_BiosPauseMsg (33)
AudioQueryChannelMode, //_AudioQueryChannelMode (34)
AudioSetChannelMode, //_AudioSetChannelMode (35)
AudioQuerySampleRates, //_AudioQuerySampleRates (36)
AudioSetSampleRate, //_AudioSetSampleRate (37)
AudioReset, //_AudioReset (38)
AudioMute, //_AudioMute (39)
AudioSetDMABuffer, //_AudioSetDMABuffer (40)
MemInit, //_MemInit (41)
WillNotImplement, //_MemAdd (42)
MemAlloc, //_MemAlloc (43)
MemFree, //_MemFree (44)
MemLocalScratch, //_MemLocalScratch (45)
NullBiosHandler, //_MemLoadCoffX (46)
NullBiosHandler, //_DownloadCoff (47)
NullBiosHandler, //_StreamLoadCoff (48)
DMALinear, //_DMALinear (49)
DMABiLinear, //_DMABiLinear (50)
FileOpen, //_FileOpen (51)
FileClose, //_FileClose (52)
FileRead, //_FileRead (53)
FileWrite, //_FileWrite (54)
FileIoctl, //_FileIoctl (55)
FileFstat, //_FileFstat (56)
FileStat, //_FileStat (57)
FileIsatty, //_FileIsatty (58)
FileLseek, //_FileLseek (59)
FileLink, //_FileLink (60)
FileLstat, //_FileLstat (61)
FileUnlink, //_FileUnlink (62)
NullBiosHandler, //_NetAccept (63)
NullBiosHandler, //_NetBind (64)
NullBiosHandler, //_NetConnect (65)
NullBiosHandler, //_NetGethostname (66)
NullBiosHandler, //_NetGetpeername (67)
NullBiosHandler, //_NetGetsockname (68)
NullBiosHandler, //_NetGetsockopt (69)
NullBiosHandler, //_NetListen (70)
NullBiosHandler, //_NetRecv (71)
NullBiosHandler, //_NetRecvfrom (72)
NullBiosHandler, //_NetRecvmsg (73)
NullBiosHandler, //_NetSend (74)
NullBiosHandler, //_NetSendmsg (75)
NullBiosHandler, //_NetSendto (76)
NullBiosHandler, //_NetSethostname (77)
NullBiosHandler, //_NetSetsockopt (78)
NullBiosHandler, //_NetShutdown (79)
NullBiosHandler, //_NetSocket (80)
AssemblyBiosHandler, //_comm_send (CommSendDirect) (81)
AssemblyBiosHandler, //_comm_recv (82)
AssemblyBiosHandler, //_comm_query (83)
WillNotImplement, //_serial_delay (84)
WillNotImplement, //_serial_read (85)
WillNotImplement, //_serial_write (86)
WillNotImplement, //_serial_write_direct (87)
MediaOpen, //_MediaOpen (88)
MediaClose, //_MediaClose (89)
MediaGetDevicesAvailable, //_MediaGetDevicesAvailable (90)
MediaGetInfo, //_MediaGetInfo (91)
UnimplementedMediaHandler, //_MediaGetStatus (92)
MediaRead, //_MediaRead (93)
MediaWrite, //_MediaWrite (94)
MediaIoctl, //_MediaIoctl (95)
SpinWait, //_spinwait (96)
UnimplementedCacheHandler, //_CacheConfigX (97)
NullBiosHandler, //_LoadGame (98)
NullBiosHandler, //_LoadPE (99)
DMAWait, //_Dma_wait (100)
DMADo, //_Dma_do (101)
PatchJumptable, //_PatchJumptable (102)
NullBiosHandler, //_BiosResume (103)
MPEStop, //_MPEStop (104)
MPERun, //_MPERun (105)
AssemblyBiosHandler, //_MPEWait (106)
MPEReadRegister, //_MPEReadRegister (107)
MPEWriteRegister, //_MPEWriteRegister (108)
NullBiosHandler, //_SetParentalControl (109)
NullBiosHandler, //_GetParentalControl (110)
AssemblyBiosHandler, //_BiosGetInfo (111)
NullBiosHandler, //_LoadTest (112)
MPELoad, //_MPELoad (113)
NullBiosHandler, //_MPEAllocThread (114)
MediaInitMPE, //_MediaInitMPE (115)
MediaShutdownMPE, //_MediaShutdownMPE (116)
NullBiosHandler, //_SecureForPE (117)
NullBiosHandler, //_StartImageValid (118)
NullBiosHandler, //_SetStartImage (119)
NullBiosHandler, //_GetStartImage (120)
NullBiosHandler, //_FindName (121)
DeviceDetect, //_DeviceDetect (122)
MPERunThread, //_MPERunThread (123)
NullBiosHandler, //_BiosIRMask (124)
NullBiosHandler, //_DiskChange (125)
NullBiosHandler, //_DiskGetTotalSlots (126)
NullBiosHandler, //_pf_add_driver (127)
VidSetBorderColor, //_VidSetBorderColor (128)
NullBiosHandler, //_DisplayBootImage (129)
WillNotImplement, //serial_write_cmd (130)
NullBiosHandler, //_GetMemDevice (131)
NullBiosHandler, //_WriteMemDevSector (132)
NullBiosHandler, //_ReadMemDev (133)
NullBiosHandler, //_AttachFsDevice (134)
NullBiosHandler, //_DiskEject (135)
NullBiosHandler, //_DiskRetract (136)
NullBiosHandler, //_GetSystemSettingsB (137)
NullBiosHandler, //_GetSystemSetting (138)
NullBiosHandler, //_SetSystemSetting (139)
NullBiosHandler, //_GetSystemSettingLength (140)
NullBiosHandler, //_LoadSystemSettings (141)
NullBiosHandler, //_StoreSystemSetting (142)
NullBiosHandler, //_mount (143)
MPEStatus, //_MPEStatus (144)
KPrintf, //_kprintf (145)
NullBiosHandler, //_ControllerPollRate (146)
WillNotImplement, //_VidSetOutputType (147)
NullBiosHandler, //_LoadDefaultSystemSettings (148)
SetISRExitHook, //_SetISRExitHook (149)
NullBiosHandler //_CompatibilityMode (150)
};


void BiosPauseMsg(MPE &mpe)
{
  //const uint32 rval = mpe.regs[0];
  //char *msg = (char *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,mpe.regs[1]);
  //uint8 *framebuffer = (uint8 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex,mpe.regs[2]);

  //allow application to continue
  mpe.regs[0] = kPollContinue;
}

void BiosPoll(MPE &mpe)
{
  //no events
  mpe.regs[0] = 0;
}

void InitBios(MPE &mpe)
{
  bool loadStatus = nuonEnv.mpe[3].LoadCoffFile("bios.cof",false);

  if(!loadStatus)
  {
    char tmp[1024];
#ifdef ENABLE_EMULATION_MESSAGEBOXES
    GetModuleFileName(NULL, tmp, 1024);
#else
    strcpy(tmp, "./");
#endif
    string tmps(tmp);
    size_t idx = tmps.find_last_of('\\');
    if (idx != string::npos)
      tmps = tmps.substr(0, idx+1);
    loadStatus = nuonEnv.mpe[3].LoadCoffFile((tmps+"bios.cof").c_str(),false);
#ifdef ENABLE_EMULATION_MESSAGEBOXES
    if(!loadStatus)
      ::MessageBox(NULL,"Missing File!","Could not load bios.cof",MB_OK);
#endif
  }

  //Reset MPEAlloc flags to reset values
  ResetMPEFlags(mpe);

  //MEMORY MANAGEMENT INITIALIZATION
  MemInit(mpe);

  //HAL Setup
  //HalSetup();

  for(uint32 i = 0; i < 4; i++)
  {
    nuonEnv.mpe[i].WriteControlRegister(0xB0UL, INTVEC1_HANDLER_ADDRESS);
    nuonEnv.mpe[i].WriteControlRegister(0xC0UL, INTVEC2_HANDLER_ADDRESS);

    if(i == 3)
    {
      nuonEnv.mpe[i].WriteControlRegister(0x110UL, 0);
      //Commrecv needs to be enabled immediately as level2 because some programs use CommRecv and and CommRecvQuery to obtain comm packets
      //rather than installing a user comm ISR
      nuonEnv.mpe[i].WriteControlRegister(0x130UL, kIntrCommRecv);
    }
    else if(i == 0)
    {
      //Don't need to set anything for level1... InitMediaMPE will enable commrecv when minibios is loaded
      //nuonEnv.mpe[i].WriteControlRegister(0x110UL, INT_COMMRECV);
      nuonEnv.mpe[i].WriteControlRegister(0x130UL, kIntrHost);
    }
    else
    {
      nuonEnv.mpe[i].WriteControlRegister(0x110UL, 0);
      nuonEnv.mpe[i].WriteControlRegister(0x130UL, kIntrHost);
    }
  }

  //Patch the jump table for the first 151 entries
  for(uint32 i = 0; i < ((0x4B0UL >> 3) + 1); i++)
  {
    if(BiosJumpTable[i] != AssemblyBiosHandler)
    {
      PatchJumptable(SYSTEM_BUS_BASE + (i << 3UL), ROM_BIOS_BASE + (i << 1));
    }
  }

  //Fill Bios Handler entries from 151 to 255 to NullBiosHandler
  for(uint32 i = ((0x4B0UL >> 3) + 1); i <= 255; i++)
  {
    BiosJumpTable[i] = NullBiosHandler;
  }

  //DVD JUMP TABLE INITIALIZATION
  InitDVDJumpTable();

  //DEFAULT VIDCHANNEL INITIALIZATION
  memset(&structMainChannel,0,sizeof(VidChannel));
  structMainChannel.base = 0x40000000;
  structMainChannel.src_width = VIDEO_WIDTH;
  structMainChannel.src_height = VIDEO_HEIGHT;
  structMainChannel.dest_width = VIDEO_WIDTH;
  structMainChannel.dest_height = VIDEO_HEIGHT;
  structMainChannel.dmaflags = (4 << 4);

  memset(&structOverlayChannel,0,sizeof(VidChannel));
  structOverlayChannel.base = 0x40000000;
  structOverlayChannel.src_width = VIDEO_WIDTH;
  structOverlayChannel.src_height = VIDEO_HEIGHT;
  structOverlayChannel.dest_width = VIDEO_WIDTH;
  structOverlayChannel.dest_height = VIDEO_HEIGHT;
  structOverlayChannel.dmaflags = (4 << 4);
  structOverlayChannel.alpha = 0xFF;

  structMainChannelPrev.base = 0;
  structMainChannelPrev.src_width = 0;
  structOverlayChannelPrev.base = 0;
  structOverlayChannelPrev.src_width = 0;
  
  //MINIBIOS INITIALIZATION

  //Start up the minibios on MPE0
  MediaInitMPE(0);

  //TIMER INITIALIZATION
  TimerInit(0,1000*1000/200);      // triggers sys0 int at 200Hz (according to BIOS doc)
  TimerInit(1,0);
  TimerInit(2,1000*1000/VIDEO_HZ); // triggers video int at ~50 or 60Hz
}


// Everything below this point is solely related to the KPrintf implementation


enum NuonPrintfType {
  NPF_TYPE_NONE,
  NPF_TYPE_CHAR,
  NPF_TYPE_UCHAR,
  NPF_TYPE_SHORT,
  NPF_TYPE_USHORT,
  NPF_TYPE_INT,
  NPF_TYPE_UINT,
  NPF_TYPE_INT64,
  NPF_TYPE_UINT64,
  NPF_TYPE_DOUBLE,
  NPF_TYPE_STRING,
};


static int32 GetStackInt(const MPE& mpe, uint32& stackPtr)
{
  const int32 val = SwapBytes(*((uint32*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true))));
  stackPtr += 4;
  return val;
}

static uint32 GetStackUInt(const MPE& mpe, uint32& stackPtr)
{
  const uint32 val = SwapBytes(*((uint32*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true))));
  stackPtr += 4;
  return val;
}

static double GetStackDouble(const MPE& mpe, uint32& stackPtr)
{
  const uint8* bytes = (const uint8*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true));
  double val;
  uint8* valBytes = (uint8*)&val;
  uint32* valDwords = (uint32*)&val;
  uint32 tmp;

  memcpy(&val, bytes, sizeof(val));
  SwapScalarBytes((uint32*)&valBytes[0]);
  SwapScalarBytes((uint32*)&valBytes[4]);

  tmp = valDwords[0];
  valDwords[0] = valDwords[1];
  valDwords[1] = tmp;

  stackPtr += 8;
  return val;
}

static uint64 GetStackUInt64(const MPE& mpe, uint32& stackPtr)
{
  const uint8* bytes = (const uint8*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true));
  uint64 val;
  uint8* valBytes = (uint8*)&val;
  uint32* valDwords = (uint32*)&val;
  uint32 tmp;

  memcpy(&val, bytes, sizeof(val));
  SwapScalarBytes((uint32*)&valBytes[0]);
  SwapScalarBytes((uint32*)&valBytes[4]);

  tmp = valDwords[0];
  valDwords[0] = valDwords[1];
  valDwords[1] = tmp;

  stackPtr += 8;
  return val;
}

static uint64 GetStackInt64(const MPE& mpe, uint32& stackPtr)
{
  const uint8* bytes = (const uint8*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true));
  int64 val;
  uint8* valBytes = (uint8*)&val;
  uint32* valDwords = (uint32*)&val;
  uint32 tmp;

  memcpy(&val, bytes, sizeof(val));
  SwapScalarBytes((uint32*)&valBytes[0]);
  SwapScalarBytes((uint32*)&valBytes[4]);

  tmp = valDwords[0];
  valDwords[0] = valDwords[1];
  valDwords[1] = tmp;

  stackPtr += 8;
  return val;
}

static const void* GetStackPtr(const MPE& mpe, uint32& stackPtr)
{
  const uint32 ptr = SwapBytes(*(uint32*)(nuonEnv.GetPointerToMemory(mpe.mpeIndex, stackPtr, true)));
  const void* ret = nuonEnv.GetPointerToMemory(mpe.mpeIndex, ptr, true);
  stackPtr += 4;
  return ret;
}

static const char *BuildFmtString(MPE& mpe, uint32& stackPtr, char* fmtString, const char* srcFmtString, NuonPrintfType &typeOut)
{
  char c;

  typeOut = NPF_TYPE_NONE;

  *fmtString++ = '%';

  bool doFlags = true;
  while (doFlags)
  {
    c = *srcFmtString;
    switch (c)
    {
    case '#':
    case '0':
    case '-':
    case '+':
    case ' ':
      *fmtString++ = c;
      srcFmtString++;
      break;

    default:
      doFlags = false;
      break;
    }
  }

  if (*srcFmtString == '*')
  {
    srcFmtString++;
    int fWidth = GetStackInt(mpe, stackPtr);
    fmtString += sprintf(fmtString, "%d", fWidth);
  }
  else
  {
    while (true)
    {
      c = *srcFmtString;

      if ((c >= '0') && (c <= '9')) {
        *fmtString++ = c;
        srcFmtString++;
      }
      else
      {
        break;
      }
    }
  }
  if (*srcFmtString == '.')
  {
    *fmtString++ = '.';
    srcFmtString++;
    if (*srcFmtString == '*')
    {
      srcFmtString++;
      int precision = GetStackInt(mpe, stackPtr);
      fmtString += sprintf(fmtString, "%d", precision);
    }
    else
    {
      while (true)
      {
        c = *srcFmtString;

        if ((c >= '0') && (c <= '9')) {
          *fmtString++ = c;
          srcFmtString++;
        }
        else
        {
          break;
        }
      }
    }
  }

  bool doEnding = true;
  while (doEnding)
  {
    c = *srcFmtString;

    switch (c)
    {
    case 'h':
      *fmtString++ = c;
      c = *++srcFmtString;

      if (c == 'h')
      {
        *fmtString++ = c;
        srcFmtString++;
        typeOut = NPF_TYPE_CHAR;
      }
      else
      {
        typeOut = NPF_TYPE_SHORT;
      }
      break;

    case 'l':
      // Eat a single 'l', since sizeof(long int) == sizeof(int) on Nuon.
      c = *++srcFmtString;
      if (c == 'l')
      {
        // Include a double 'l', since long long int is always 64 bits.
        *fmtString++ = c;
        *fmtString++ = c;
        srcFmtString++;
        typeOut = NPF_TYPE_INT64;
      }
      else
      {
        typeOut = NPF_TYPE_INT;
      }
      break;

    case 'z':
      // size_t is 32-bit on Nuon.
      *fmtString++ = c;
      srcFmtString++;
      typeOut = NPF_TYPE_INT;
      break;

    // XXX Don't support long doubles, intmax_t/uintmax_t, some even more arcane stuff.

    case 'd':
    case 'i':
      *fmtString++ = c;
      srcFmtString++;
      doEnding = false;
      if (typeOut == NPF_TYPE_NONE)
      {
        typeOut = NPF_TYPE_INT;
      }
      break;

    case 'o':
    case 'u':
    case 'x':
    case 'X':
      *fmtString++ = c;
      srcFmtString++;
      doEnding = false;
      switch (typeOut)
      {
      case NPF_TYPE_CHAR:
      case NPF_TYPE_SHORT:
      case NPF_TYPE_INT:
      case NPF_TYPE_INT64:
        typeOut = (NuonPrintfType)((int)typeOut + 1);
        break;

      default:
        typeOut = NPF_TYPE_UINT;
        break;
      }
      break;

    case 'e':
    case 'f':
    case 'g':
    case 'a':
      *fmtString++ = c;
      srcFmtString++;
      typeOut = NPF_TYPE_DOUBLE;
      doEnding = false;
      break;

    case 'p':
      *fmtString++ = '#';
      *fmtString++ = 'x';
      srcFmtString++;
      typeOut = NPF_TYPE_UINT;
      doEnding = false;
      break;

    case 's':
      *fmtString++ = c;
      srcFmtString++;
      typeOut = NPF_TYPE_STRING;
      doEnding = false;
      break;

    case '\0':
    default:
      /* Error condition. Invalid or unhandled conversion string */
      typeOut = NPF_TYPE_NONE;
      doEnding = false;
      break;
    }
  }

  *fmtString = '\0';

  return srcFmtString;
}

static void NuonSprintf(MPE &mpe, uint32 stackPtr, char* buf, size_t bufSize, const char* fmt)
{
  char* bufEnd = buf + bufSize - 1;

  for (char c = *fmt++; c && (buf < bufEnd); c = *fmt++) {
    if (c != '%')
    {
      *buf++ = c;
      continue;
    }

    c = *fmt;

    if (!c) break;

    if (c == '%')
    {
      *buf++ = '%';
      fmt++;
      continue;
    }

    char subFmtStr[128];
    NuonPrintfType type;
    fmt = BuildFmtString(mpe, stackPtr, subFmtStr, fmt, type);

    switch (type)
    {
    case NPF_TYPE_CHAR:
    case NPF_TYPE_SHORT:
    case NPF_TYPE_INT:
    {
      int32 val = GetStackInt(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    case NPF_TYPE_UCHAR:
    case NPF_TYPE_USHORT:
    case NPF_TYPE_UINT:
    {
      uint32 val = GetStackUInt(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    case NPF_TYPE_DOUBLE:
    {
      double val = GetStackDouble(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    case NPF_TYPE_STRING:
    {
      const char* val = (const char *)GetStackPtr(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    case NPF_TYPE_INT64:
    {
      int64 val = GetStackInt64(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    case NPF_TYPE_UINT64:
    {
      uint64 val = GetStackUInt64(mpe, stackPtr);
      buf += snprintf(buf, (bufEnd - buf), subFmtStr, val);
      break;
    }
    default:
      /* Unknown conversion character. Just skip it */
      stackPtr += 4; // Best guess at parameter size
      break;
    }
  }

  *buf = '\0';
}

void KPrintf(MPE &mpe)
{
  const uint32 stackPtr = mpe.regs[31];

  const uint32 pStr = SwapBytes(*((uint32 *)(nuonEnv.GetPointerToMemory(mpe.mpeIndex,stackPtr,true))));

  if(pStr)
  {
    const char* const str = (const char *)(nuonEnv.GetPointerToMemory(mpe.mpeIndex,pStr,true));

    char buf[4096];
    NuonSprintf(mpe, stackPtr + 4, buf, sizeof(buf), str);

    if (nuonEnv.debugLogFile)
    {
      fprintf(nuonEnv.debugLogFile, "%s", buf);
      fflush(nuonEnv.debugLogFile);
    }

    for (size_t i = 0; buf[i]; i++)
    {
      char c = buf[i];
      switch (c)
      {
      case '\r':
        /* Eat these */
        continue;
      case '\n':
        nuonEnv.kprintRingBuffer[nuonEnv.kprintCurrentLine][nuonEnv.kprintCurrentChar++] = '\0';
        nuonEnv.kprintCurrentLine = (nuonEnv.kprintCurrentLine + 1) % NuonEnvironment::KPRINT_RING_SIZE;
        nuonEnv.kprintCurrentChar = 0;
        continue;
      default:
        if (nuonEnv.kprintCurrentChar < NuonEnvironment::KPRINT_LINE_LENGTH)
        {
          nuonEnv.kprintRingBuffer[nuonEnv.kprintCurrentLine][nuonEnv.kprintCurrentChar++] = c;
        }
        break;
      }
    }
    nuonEnv.kprintRingBuffer[nuonEnv.kprintCurrentLine][nuonEnv.kprintCurrentChar] = '\0';
    nuonEnv.kprintUpdated = true;
  }
}

/*
  // kprintf example usage

  #include <nuon/bios.h>
  extern void kprintf(const char *fmt, ...);

  int main() {
      kprintf("Hello, world! %d %d %d\n", 1, 2, 3);
      return 0;
  }
*/
