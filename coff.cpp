#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "mpe.h"
#include "coff.h"
#include "NuonEnvironment.h"
#include "NuonMemoryMap.h"
#include "byteswap.h"

extern NuonEnvironment nuonEnv;

#define bswap32(n) _byteswap_ulong(n)
#define bswap16(n) _byteswap_ushort(n)

struct FILHDR
{
  uint16 f_magic;         /* magic number             */
  uint16 f_nscns;         /* number of sections       */
  uint32 f_timdat;        /* time & date stamp        */
  uint32 f_symptr;        /* file pointer to symtab   */
  uint32 f_nsyms;         /* number of symtab entries */
  uint16 f_opthdr;        /* sizeof(optional hdr)     */
  uint16 f_flags;         /* flags                    */
};

struct SCNHDR
{
  char    s_name[8];  /* section name                     */
  uint32  s_paddr;    /* physical address, aliased s_nlib */
  uint32  s_vaddr;    /* virtual address                  */
  uint32  s_size;     /* section size                     */
  uint32  s_scnptr;   /* file ptr to raw data for section */
  uint32  s_relptr;   /* file ptr to relocation           */
  uint32  s_lnnoptr;  /* file ptr to line numbers         */
  uint32  s_nreloc;   /* number of relocation entries     */
  uint32  s_nlnno;    /* number of line number entries    */
  uint32  s_flags;    /* flags                            */
};

bool MPE::LoadCoffFile(const char * const filename, bool bSetEntryPoint, int handle)
{
  if(handle == -1)
  {
    handle = open(filename, O_RDONLY);

    if(handle <= -1)
      return false;
  }

  if(handle >= 0)
  {
    const int start_offset = lseek(handle, 0, SEEK_CUR);

    FILHDR coffhdr;
    read(handle, &coffhdr, sizeof(FILHDR));
    coffhdr.f_magic = bswap16(coffhdr.f_magic);
    coffhdr.f_nscns = bswap16(coffhdr.f_nscns);
    coffhdr.f_timdat = bswap32(coffhdr.f_timdat);
    coffhdr.f_symptr = bswap32(coffhdr.f_symptr);
    coffhdr.f_nsyms = bswap32(coffhdr.f_nsyms);
    coffhdr.f_opthdr = bswap16(coffhdr.f_opthdr);
    coffhdr.f_flags = bswap16(coffhdr.f_flags);

    //expect magic number 0x01 0x20
    if(coffhdr.f_magic != 0x0120)
      return false;

    //read the entry point, which is the first four bytes of the optional header
    uint32 entryPoint;
    read(handle, &entryPoint, 4);
    entryPoint = bswap32(entryPoint);
    //skip past the remainder of the optional header
    lseek(handle, (coffhdr.f_opthdr - 4), SEEK_CUR);
    while(coffhdr.f_nscns > 0)
    {
      SCNHDR sectionhdr;
      read(handle, &sectionhdr, sizeof(SCNHDR));
      sectionhdr.s_paddr = bswap32(sectionhdr.s_paddr);
      sectionhdr.s_vaddr = bswap32(sectionhdr.s_vaddr);
      sectionhdr.s_size = bswap32(sectionhdr.s_size);
      sectionhdr.s_scnptr = bswap32(sectionhdr.s_scnptr);
      sectionhdr.s_relptr = bswap32(sectionhdr.s_relptr);
      sectionhdr.s_lnnoptr = bswap32(sectionhdr.s_lnnoptr);
      sectionhdr.s_nreloc = bswap32(sectionhdr.s_nreloc);
      sectionhdr.s_nlnno = bswap32(sectionhdr.s_nlnno);
      sectionhdr.s_flags = bswap32(sectionhdr.s_flags);

      if((sectionhdr.s_flags & 0x000000282) != 0)
      {
        //Don't skip the bss or section even though
        //it is marked as do-not-load.  VM Labs actually
        //puts code into the BSS section and later zeroes it
        //out after executing it.

        //if(strcmp(sectionhdr.s_name,"bss"))
        //{
          //Skip this section
          coffhdr.f_nscns--;
          continue;
        //}
      }

      //save position so we can go to the section data
      const long nextPos = lseek(handle, 0, SEEK_CUR);
      //start_offset may not be 0 if loading a COFF image stored inside of
      //a NUONROM-DISK image
      lseek(handle,start_offset,SEEK_SET);
      lseek(handle,sectionhdr.s_scnptr,SEEK_CUR);

      if(sectionhdr.s_paddr < MAIN_BUS_BASE)
      {
        //assume local MPE memory
        read(handle,&dtrom[sectionhdr.s_paddr & MPE_VALID_MEMORY_MASK],sectionhdr.s_size);
      }
      else if(sectionhdr.s_paddr < SYSTEM_BUS_BASE)
      {
        //main bus DRAM
        if(strcmp(sectionhdr.s_name,"PATCH") == 0)
        {
          //zero out the first 16 bytes in the PATCH section to avoid
          //BIOS patching in the C startup code

          for(int i = 0; i < 16; i++)
            nuonEnv.mainBusDRAM[(sectionhdr.s_paddr & MAIN_BUS_VALID_MEMORY_MASK) + i] = 0;

          lseek(handle,16,SEEK_CUR);
          read(handle,&(nuonEnv.mainBusDRAM[(sectionhdr.s_paddr & MAIN_BUS_VALID_MEMORY_MASK) + 16]),sectionhdr.s_size - 16);
        }
        else
        {
          read(handle,&(nuonEnv.mainBusDRAM[sectionhdr.s_paddr & MAIN_BUS_VALID_MEMORY_MASK]),sectionhdr.s_size);
        }
      }
      else
      {
        //assume system bus DRAM
        if(strcmp(sectionhdr.s_name,"PATCH") == 0)
        {
          //zero out the first 16 bytes in the PATCH section to avoid
          //BIOS patching in the C startup code

          for(int i = 0; i < 16; i++)
            nuonEnv.systemBusDRAM[(sectionhdr.s_paddr & SYSTEM_BUS_VALID_MEMORY_MASK) + i] = 0;

          lseek(handle,16,SEEK_CUR);
          read(handle,&(nuonEnv.systemBusDRAM[(sectionhdr.s_paddr & SYSTEM_BUS_VALID_MEMORY_MASK) + 16]),sectionhdr.s_size - 16);
        }
        else
        {
          read(handle,&(nuonEnv.systemBusDRAM[(sectionhdr.s_paddr & SYSTEM_BUS_VALID_MEMORY_MASK)]),sectionhdr.s_size);
        }
      }

      coffhdr.f_nscns--;
      lseek(handle,nextPos,SEEK_SET);
    }

    close(handle);

    if(bSetEntryPoint)
    {
      pcexec = entryPoint;
    }

    return true;
  }
  else
  {
    return false;
  }
}
