#include "basetypes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include "byteswap.h"
#include "nuonrom.h"

bool MPE::LoadNuonRomFile(const char * const filename)
{
  char linebuf[64];
  int bytesRead;
  uint32 offset;

  int handle;
  if ((handle = open(filename, O_RDONLY | O_BINARY)) == 0 && handle >= 0)
  {
check_for_bles:
    bytesRead = read(handle, linebuf, 16);
    if(bytesRead == 16)
    {
      if(strncmp(linebuf,"Bles",4) == 0)
      {
        const long where = lseek(handle, 0, SEEK_CUR) - 16;
        //This is a BLES file, not a NUONROM-DISK file.
        if((linebuf[4] == 0) && linebuf[5] == 1)
        {
          //Version 1?  COFF offset value is at offset 0x52
          bytesRead = lseek(handle, 0x52 - 0x10, SEEK_CUR);
          
          read(handle, linebuf, 2);
          union {
            uint32 u32;
            struct { uint8 u8[4]; };
          } intbuf;
          intbuf.u32 = 0;
          intbuf.u8[0] = linebuf[1];
          intbuf.u8[1] = linebuf[0];
          offset = intbuf.u32;
          lseek(handle,where,SEEK_SET);
          goto load_coff_file;
        }
      }
      if(strncmp(linebuf,"NUONROM-DISK",12) == 0)
      {
        //skip to line containing "cd_app.cof"
        bytesRead = read(handle, linebuf, 48);
        if(bytesRead == 48)
        {
          bytesRead = read(handle, linebuf, 16);
          if(bytesRead == 16)
          {
            if(strncmp(linebuf,"cd_app.cof",10) == 0)
            {
              bytesRead = read(handle, &offset, 4);
              SwapScalarBytes(&offset);
              uint32 length;
              bytesRead = read(handle, &length, 4);
              SwapScalarBytes(&length);
              bytesRead = read(handle, linebuf, 8);
              lseek(handle, 0, SEEK_SET);

              if(bytesRead == 8)
              {
                //seek to the file offset point
load_coff_file:
                bytesRead = lseek(handle, offset, SEEK_CUR);
                if(bytesRead != -1)
                {
                  return LoadCoffFile(filename,true,handle);
                }
                else
                {
                  goto failure;
                }
              }
            }
            else if(strncmp(linebuf,"nuon.run",8) == 0)
            {
              bytesRead = read(handle, &offset, 4);
              SwapScalarBytes(&offset);
              uint32 length;
              bytesRead = read(handle, &length, 4);
              SwapScalarBytes(&length);
              bytesRead = read(handle, linebuf, 8);
              if(bytesRead == 8)
              {
                bytesRead = lseek(handle, offset, SEEK_SET);
                if(bytesRead != -1)
                {
                  goto check_for_bles;
                }
                else
                {
                  goto failure;
                }
              }
              else
              {
                goto failure;
              }
            }
            else
            {
              goto failure;
            }
          }
          else
          {
            goto failure;
          }
        }
        else
        {
          goto failure;
        }
      }
      else
      {
        goto failure;
      }
    }
    else
    {
      goto failure;
    }
  }

failure:
  if (handle >= 0)
    close(handle);

  return false;
}
