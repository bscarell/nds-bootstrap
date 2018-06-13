/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h> 
#include <nds/fifomessages.h>
#include "cardEngine.h"

static u32 ROM_LOCATION = 0x0C800000;
extern u32 ROM_TID;
extern u32 ROM_HEADERCRC;
extern u32 ARM9_LEN;
extern u32 romSize;
extern u32 cleanRomSize;
extern u32 consoleModel;

#define _32KB_READ_SIZE 0x8000
#define _64KB_READ_SIZE 0x10000
#define _128KB_READ_SIZE 0x20000
#define _192KB_READ_SIZE 0x30000
#define _256KB_READ_SIZE 0x40000
#define _512KB_READ_SIZE 0x80000
#define _768KB_READ_SIZE 0xC0000
#define _1MB_READ_SIZE 0x100000

#define CACHE_ADRESS_START 0x0C800000
#define retail_CACHE_ADRESS_SIZE 0x800000
#define retail_128KB_CACHE_SLOTS 0x40
#define retail_192KB_CACHE_SLOTS 0x2A
#define retail_256KB_CACHE_SLOTS 0x20
#define retail_512KB_CACHE_SLOTS 0x10
#define retail_768KB_CACHE_SLOTS 0xA
#define retail_1MB_CACHE_SLOTS 0x8
#define dev_CACHE_ADRESS_SIZE 0x1800000
#define dev_128KB_CACHE_SLOTS 0xC0
#define dev_192KB_CACHE_SLOTS 0x80
#define dev_256KB_CACHE_SLOTS 0x60
#define dev_512KB_CACHE_SLOTS 0x30
#define dev_768KB_CACHE_SLOTS 0x20
#define dev_1MB_CACHE_SLOTS 0x18

extern vu32* volatile cardStruct;
//extern vu32* volatile cacheStruct;
extern u32 sdk_version;
extern u32 needFlushDCCache;
vu32* volatile sharedAddr = (vu32*)0x027FFB08;
extern volatile int (*readCachedRef)(u32*); // this pointer is not at the end of the table but at the handler pointer corresponding to the current irq

static u32 cacheDescriptor [dev_128KB_CACHE_SLOTS] = {0xffffffff};
static u32 cacheCounter [dev_128KB_CACHE_SLOTS];
static u32 accessCounter = 0;

static u16 cacheSlots = retail_128KB_CACHE_SLOTS;

static u32 cacheReadSizeSubtract = 0;
static u32 asyncReadSizeSubtract = 0;

static u32 asyncSector = 0xFFFFFFFF;
static u32 asyncQueue [10];
static int aQHead = 0;
static int aQTail = 0;
static int aQSize = 0;
static char hexbuffer [9];

static bool flagsSet = false;
extern u32 ROMinRAM;
extern u32 enableExceptionHandler;

// 1 = start of data address, 2 = end of data address, 3 = data size, 4 = DATAEXCLUDE,
// 5 = GAME_CACHE_ADRESS_START, 6 = GAME_CACHE_SLOTS, 7 = GAME_READ_SIZE
extern u32 setDataBWlist[7];
extern u32 setDataBWlist_1[3];
extern u32 setDataBWlist_2[3];
extern u32 setDataBWlist_3[3];
extern u32 setDataBWlist_4[3];
int dataAmount = 0;

char* tohex(u32 n)
{
    unsigned size = 9;
    char *buffer = hexbuffer;
    unsigned index = size - 2;

	for (int i=0; i<size; i++) {
		buffer[i] = '0';
	}
	
    while (n > 0)
    {
        unsigned mod = n % 16;

        if (mod >= 10)
            buffer[index--] = (mod - 10) + 'A';
        else
            buffer[index--] = mod + '0';

        n /= 16;
    }
    buffer[size - 1] = '\0';
    return buffer;
}

void user_exception(void);

//---------------------------------------------------------------------------------
void setExceptionHandler2() {
//---------------------------------------------------------------------------------
	exceptionStack = (u32)0x23EFFFC ;
	EXCEPTION_VECTOR = enterException ;
	*exceptionC = user_exception;
}

int allocateCacheSlot() {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for(int i=0; i<cacheSlots; i++) {
		if(cacheCounter[i]<=lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int GAME_allocateCacheSlot() {
	int slot = 0;
	u32 lowerCounter = accessCounter;
	for(int i=0; i<setDataBWlist[5]; i++) {
		if(cacheCounter[i]<=lowerCounter) {
			lowerCounter = cacheCounter[i];
			slot = i;
			if(!lowerCounter) break;
		}
	}
	return slot;
}

int getSlotForSector(u32 sector) {
	for(int i=0; i<cacheSlots; i++) {
		if(cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}

int GAME_getSlotForSector(u32 sector) {
	for(int i=0; i<setDataBWlist[5]; i++) {
		if(cacheDescriptor[i]==sector) {
			return i;
		}
	}
	return -1;
}


vu8* getCacheAddress(int slot) {
	return (vu32*)(CACHE_ADRESS_START+slot*_128KB_READ_SIZE);
}

vu8* GAME_getCacheAddress(int slot) {
	return (vu32*)(setDataBWlist[4]+slot*setDataBWlist[6]);
}

void updateDescriptor(int slot, u32 sector) {
	cacheDescriptor[slot] = sector;
	cacheCounter[slot] = accessCounter;
}

void waitForArm7() {
	while(sharedAddr[3] != (vu32)0);
}

void addToAsyncQueue(sector) {
	#ifdef DEBUG
	nocashMessage("\narm9 addToAsyncQueue\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	#endif
	
	asyncQueue[aQHead] = sector;
	aQHead++;
	aQSize++;
	if(aQHead>9) {
		aQHead=0;
	}
	if(aQSize>10) {
		aQSize=10;
		aQTail++;
		if(aQTail>9) aQTail=0;
	}
}

u32 popFromAsyncQueueHead() {	
	if(aQSize>0) {
	
		aQHead--;
		if(aQHead == -1) aQHead = 4;
		aQSize--;
		
		return asyncQueue[aQHead];
	} else return 0;
}

void triggerAsyncPrefetch(sector) {	
	#ifdef DEBUG
	nocashMessage("\narm9 triggerAsyncPrefetch\n");	
	nocashMessage("\narm9 sector\n");	
	nocashMessage(tohex(sector));
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	asyncReadSizeSubtract = 0;
	if(asyncSector == 0xFFFFFFFF) {
		if (ROMinRAM == 2) {
			if (cleanRomSize > 0) {
				if (sector > cleanRomSize) {
					sector = 0;
				} else if ((sector+setDataBWlist[6]) > cleanRomSize) {
					for (u32 i = 0; i < setDataBWlist[6]; i++) {
						asyncReadSizeSubtract++;
						if (((sector+setDataBWlist[6])-asyncReadSizeSubtract) == cleanRomSize) break;
					}
				}
			}
			int slot = GAME_getSlotForSector(sector);
			// read max CACHE_READ_SIZE via the main RAM cache
			// do it only if there is no async command ongoing
			if(slot==-1) {
				addToAsyncQueue(sector);
				// send a command to the arm7 to fill the RAM cache
				u32 commandRead = 0x020ff800;		

				slot = GAME_allocateCacheSlot();
				vu8* buffer = GAME_getCacheAddress(slot);

				if(needFlushDCCache) DC_FlushRange(buffer, setDataBWlist[6]);

				cacheDescriptor[slot] = sector;
				cacheCounter[slot] = 0x0FFFFFFF ; // async marker
				asyncSector = sector;		

				// write the command
				sharedAddr[0] = buffer;
				sharedAddr[1] = setDataBWlist[6]-asyncReadSizeSubtract;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);			


				// do it asynchronously
				//waitForArm7();
			}
		} else {
			if (cleanRomSize > 0) {
				if (sector > cleanRomSize) {
					sector = 0;
				} else if ((sector+_128KB_READ_SIZE) > cleanRomSize) {
					for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
						asyncReadSizeSubtract++;
						if (((sector+_128KB_READ_SIZE)-asyncReadSizeSubtract) == cleanRomSize) break;
					}
				}
			}
			int slot = getSlotForSector(sector);
			// read max CACHE_READ_SIZE via the main RAM cache
			// do it only if there is no async command ongoing
			if(slot==-1) {
				addToAsyncQueue(sector);
				// send a command to the arm7 to fill the RAM cache
				u32 commandRead = 0x020ff800;		

				slot = allocateCacheSlot();
				vu8* buffer = getCacheAddress(slot);

				if(needFlushDCCache) DC_FlushRange(buffer, _128KB_READ_SIZE);

				cacheDescriptor[slot] = sector;
				cacheCounter[slot] = 0x0FFFFFFF ; // async marker
				asyncSector = sector;		

				// write the command
				sharedAddr[0] = buffer;
				sharedAddr[1] = _128KB_READ_SIZE-asyncReadSizeSubtract;
				sharedAddr[2] = sector;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);			


				// do it asynchronously
				//waitForArm7();
			}
		}
	}	
}

void processAsyncCommand() {
	#ifdef DEBUG
	nocashMessage("\narm9 processAsyncCommand\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if(asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			if(sharedAddr[3] == (vu32)0) {
				updateDescriptor(slot, asyncSector);
				asyncSector = 0xFFFFFFFF;
			}			
		}	
	}
}

void getAsyncSector() {
	#ifdef DEBUG
	nocashMessage("\narm9 getAsyncSector\n");	
	nocashMessage("\narm9 asyncSector\n");	
	nocashMessage(tohex(asyncSector));
	#endif
	
	if(asyncSector != 0xFFFFFFFF) {
		int slot = getSlotForSector(asyncSector);
		if(slot!=-1 && cacheCounter[slot] == 0x0FFFFFFF) {
			waitForArm7();

			updateDescriptor(slot, asyncSector);
			asyncSector = 0xFFFFFFFF;
		}	
	}	
}

int cardRead (u32* cacheStruct) {
	//nocashMessage("\narm9 cardRead\n");
	
	u8* cacheBuffer = (u8*)(cacheStruct + 8);
	u32* cachePage = cacheStruct + 2;
	u32 commandRead;
	u32 src = cardStruct[0];
	if(src==0) {
		return 0;	// If ROM read location is 0, do not proceed.
	}
	u8* dst = (u8*) (cardStruct[1]);
	u32 len = cardStruct[2];

	u32 page = (src/512)*512;
	
	if(!flagsSet) {
		if (enableExceptionHandler) {
			setExceptionHandler2();
		}

		u32 ramRomSizeLimit = 0x00800000;
		if (consoleModel > 0) {
			ramRomSizeLimit = 0x01800000;
			cacheSlots = dev_128KB_CACHE_SLOTS;
		}

		// If ROM size is 0x01800000 (0x00800000 for retail DSi) or below, then the ROM is in RAM.
		if((romSize > 0) && (romSize <= ramRomSizeLimit) && (ROM_TID != 0x45475241)
		&& (ROM_TID != 0x45525243) && (ROM_TID != 0x45425243)
		&& (romSize != (0x012C7066-0x4000-ARM9_LEN))) {
			ROM_LOCATION -= 0x4000;
			ROM_LOCATION -= ARM9_LEN;
		} else if (consoleModel > 0) {
			if((ROM_TID == 0x454B4C41) && (ROM_HEADERCRC == 0xB8C7CF56)		// Lunar Knights (U)
			|| (ROM_TID == 0x504B4C41) && (ROM_HEADERCRC == 0x5973CF56)		// Lunar Knights (E)
			|| (ROM_TID == 0x4A554343) && (ROM_HEADERCRC == 0x61DCCF56)		// Tomodachi Collection (J)
			|| (ROM_TID == 0x4A554343) && (ROM_HEADERCRC == 0x9B26CF56)) {	// Tomodachi Collection (J) (Rev 1)
				dataAmount = 1;
			}

			if(ROMinRAM==2 && setDataBWlist[3]==false) {
				ROM_LOCATION -= 0x4000;
				ROM_LOCATION -= ARM9_LEN;
			}
		}
		flagsSet = true;
	}

	#ifdef DEBUG
	// send a log command for debug purpose
	// -------------------------------------
	commandRead = 0x026ff800;

	sharedAddr[0] = dst;
	sharedAddr[1] = len;
	sharedAddr[2] = src;
	sharedAddr[3] = commandRead;

	IPC_SendSync(0xEE24);

	waitForArm7();
	// -------------------------------------*/
	#endif
	
	
	if(ROMinRAM==0) {
		u32 sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
		cacheReadSizeSubtract = 0;
		if ((cleanRomSize > 0) && ((sector+_128KB_READ_SIZE) > cleanRomSize)) {
			for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
				cacheReadSizeSubtract++;
				if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == cleanRomSize) break;
			}
		}

		accessCounter++;

		processAsyncCommand();

		if(page == src && len > _128KB_READ_SIZE && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
			getAsyncSector();

			// read directly at arm7 level
			commandRead = 0x025FFB08;

			cacheFlush();

			sharedAddr[0] = dst;
			sharedAddr[1] = len;
			sharedAddr[2] = src;
			sharedAddr[3] = commandRead;

			IPC_SendSync(0xEE24);

			waitForArm7();

		} else {
			// read via the main RAM cache
			while(len > 0) {
				int slot = getSlotForSector(sector);
				vu8* buffer = getCacheAddress(slot);
				u32 nextSector = sector+_128KB_READ_SIZE;	
				// read max CACHE_READ_SIZE via the main RAM cache
				if(slot==-1) {
					getAsyncSector();

					// send a command to the arm7 to fill the RAM cache
					commandRead = 0x025FFB08;

					slot = allocateCacheSlot();

					buffer = getCacheAddress(slot);

					if(needFlushDCCache) DC_FlushRange(buffer, _128KB_READ_SIZE);

					// write the command
					sharedAddr[0] = buffer;
					sharedAddr[1] = _128KB_READ_SIZE-cacheReadSizeSubtract;
					sharedAddr[2] = sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();

					updateDescriptor(slot, sector);	
		
					triggerAsyncPrefetch(nextSector);
				} else {
					if(cacheCounter[slot] == 0x0FFFFFFF) {
						// prefetch successfull
						getAsyncSector();
						
						triggerAsyncPrefetch(nextSector);	
					} else {
						int i;
						for(i=0; i<10; i++) {
							if(asyncQueue[i]==sector) {
								// prefetch successfull
								triggerAsyncPrefetch(nextSector);	
								break;
							}
						}
					}
					updateDescriptor(slot, sector);
				}

				u32 len2=len;
				if((src - sector) + len2 > _128KB_READ_SIZE){
					len2 = sector - src + _128KB_READ_SIZE;
				}

				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+src-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// copy directly
					fastCopy32(buffer+(src-sector),dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+page-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// read via the 512b ram cache
					fastCopy32(buffer+(page-sector), cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
					sector = (src/_128KB_READ_SIZE)*_128KB_READ_SIZE;
					cacheReadSizeSubtract = 0;
					if ((cleanRomSize > 0) && ((sector+_128KB_READ_SIZE) > cleanRomSize)) {
						for (u32 i = 0; i < _128KB_READ_SIZE; i++) {
							cacheReadSizeSubtract++;
							if (((sector+_128KB_READ_SIZE)-cacheReadSizeSubtract) == cleanRomSize) break;
						}
					}
					accessCounter++;
				}
			}
		}
	} else if (ROMinRAM==1) {
		// Prevent overwriting ROM in RAM
		while(len > 0) {
			u32 len2=len;
			if(len2 > 512) {
				len2 -= src%4;
				len2 -= len2 % 32;
			}

			if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
				#ifdef DEBUG
				// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = dst;
				sharedAddr[1] = len2;
				sharedAddr[2] = ROM_LOCATION+src;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				waitForArm7();
				// -------------------------------------*/
				#endif

				// read ROM loaded into RAM
				fastCopy32(ROM_LOCATION+src,dst,len2);

				// update cardi common
				cardStruct[0] = src + len2;
				cardStruct[1] = dst + len2;
				cardStruct[2] = len - len2;
			} else {
				#ifdef DEBUG
				// send a log command for debug purpose
				// -------------------------------------
				commandRead = 0x026ff800;

				sharedAddr[0] = page;
				sharedAddr[1] = len2;
				sharedAddr[2] = ROM_LOCATION+page;
				sharedAddr[3] = commandRead;

				IPC_SendSync(0xEE24);

				waitForArm7();
				// -------------------------------------
				#endif

				// read via the 512b ram cache
				fastCopy32(ROM_LOCATION+page, cacheBuffer, 512);
				*cachePage = page;
				(*readCachedRef)(cacheStruct);
			}
			len = cardStruct[2];
			if(len>0) {
				src = cardStruct[0];
				dst = cardStruct[1];
				page = (src/512)*512;
			}
		}
	} else if (ROMinRAM==2) {
		if(setDataBWlist[3]==true && src >= setDataBWlist[0] && src < setDataBWlist[1]) {
			u32 ROM_LOCATION2 = ROM_LOCATION-setDataBWlist[0];

			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION2+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION2+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==true && dataAmount >= 1 && src >= setDataBWlist_1[0] && src < setDataBWlist_1[1]) {
			u32 ROM_LOCATION2 = ROM_LOCATION-setDataBWlist[0]+setDataBWlist[2];

			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION2+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION2+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==true && dataAmount >= 2 && src >= setDataBWlist_2[0] && src < setDataBWlist_2[1]) {
			u32 ROM_LOCATION2 = ROM_LOCATION-setDataBWlist_2[0]+setDataBWlist[2]+setDataBWlist_1[2];

			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION2+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION2+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==true && dataAmount >= 3 && src >= setDataBWlist_3[0] && src < setDataBWlist_3[1]) {
			u32 ROM_LOCATION2 = ROM_LOCATION-setDataBWlist_3[0]+setDataBWlist[2]+setDataBWlist_1[2]+setDataBWlist_2[2];

			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION2+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION2+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==true && dataAmount == 4 && src >= setDataBWlist_4[0] && src < setDataBWlist_4[1]) {
			u32 ROM_LOCATION2 = ROM_LOCATION-setDataBWlist_4[0]+setDataBWlist[2]+setDataBWlist_1[2]+setDataBWlist_2[2]+setDataBWlist_3[2];

			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION2+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION2+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION2+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==false && src > 0 && src < setDataBWlist[0]) {
			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(setDataBWlist[3]==false && src >= setDataBWlist[1] && src < romSize) {
			while(len > 0) {
				u32 len2=len;
				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION-setDataBWlist[2]+src;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// read ROM loaded into RAM
					fastCopy32(ROM_LOCATION-setDataBWlist[2]+src,dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = ROM_LOCATION-setDataBWlist[2]+page;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------
					#endif

					// read via the 512b ram cache
					fastCopy32(ROM_LOCATION-setDataBWlist[2]+page, cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
				}
			}
		} else if(page == src && len > setDataBWlist[6] && dst < 0x02700000 && dst > 0x02000000 && ((u32)dst)%4==0) {
			accessCounter++;

			processAsyncCommand();

			getAsyncSector();

			// read directly at arm7 level
			commandRead = 0x025FFB08;

			cacheFlush();

			sharedAddr[0] = dst;
			sharedAddr[1] = len;
			sharedAddr[2] = src;
			sharedAddr[3] = commandRead;

			IPC_SendSync(0xEE24);

			waitForArm7();
		} else {
			accessCounter++;

			processAsyncCommand();

			u32 sector = (src/setDataBWlist[6])*setDataBWlist[6];
			cacheReadSizeSubtract = 0;
			if ((cleanRomSize > 0) && ((sector+setDataBWlist[6]) > cleanRomSize)) {
				for (u32 i = 0; i < setDataBWlist[6]; i++) {
					cacheReadSizeSubtract++;
					if (((sector+setDataBWlist[6])-cacheReadSizeSubtract) == cleanRomSize) break;
				}
			}

			while(len > 0) {
				int slot = GAME_getSlotForSector(sector);
				vu8* buffer = GAME_getCacheAddress(slot);
				u32 nextSector = sector+setDataBWlist[6];	
				// read max CACHE_READ_SIZE via the main RAM cache
				if(slot==-1) {
					getAsyncSector();

					// send a command to the arm7 to fill the RAM cache
					commandRead = 0x025FFB08;

					slot = GAME_allocateCacheSlot();

					buffer = GAME_getCacheAddress(slot);

					if(needFlushDCCache) DC_FlushRange(buffer, setDataBWlist[6]);

					// write the command
					sharedAddr[0] = buffer;
					sharedAddr[1] = setDataBWlist[6]-cacheReadSizeSubtract;
					sharedAddr[2] = sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();

					updateDescriptor(slot, sector);	
		
					triggerAsyncPrefetch(nextSector);
				} else {
					if(cacheCounter[slot] == 0x0FFFFFFF) {
						// prefetch successfull
						getAsyncSector();
						
						triggerAsyncPrefetch(nextSector);	
					} else {
						int i;
						for(i=0; i<10; i++) {
							if(asyncQueue[i]==sector) {
								// prefetch successfull
								triggerAsyncPrefetch(nextSector);	
								break;
							}
						}
					}
					updateDescriptor(slot, sector);
				}


				u32 len2=len;
				if((src - sector) + len2 > setDataBWlist[6]){
					len2 = sector - src + setDataBWlist[6];
				}

				if(len2 > 512) {
					len2 -= src%4;
					len2 -= len2 % 32;
				}

				if(len2 >= 512 && len2 % 32 == 0 && ((u32)dst)%4 == 0 && src%4 == 0) {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = dst;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+src-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// copy directly
					fastCopy32(buffer+(src-sector),dst,len2);

					// update cardi common
					cardStruct[0] = src + len2;
					cardStruct[1] = dst + len2;
					cardStruct[2] = len - len2;
				} else {
					#ifdef DEBUG
					// send a log command for debug purpose
					// -------------------------------------
					commandRead = 0x026ff800;

					sharedAddr[0] = page;
					sharedAddr[1] = len2;
					sharedAddr[2] = buffer+page-sector;
					sharedAddr[3] = commandRead;

					IPC_SendSync(0xEE24);

					waitForArm7();
					// -------------------------------------*/
					#endif

					// read via the 512b ram cache
					fastCopy32(buffer+(page-sector), cacheBuffer, 512);
					*cachePage = page;
					(*readCachedRef)(cacheStruct);
				}
				len = cardStruct[2];
				if(len>0) {
					src = cardStruct[0];
					dst = cardStruct[1];
					page = (src/512)*512;
					sector = (src/setDataBWlist[6])*setDataBWlist[6];
					cacheReadSizeSubtract = 0;
					if ((cleanRomSize > 0) && ((sector+setDataBWlist[6]) > cleanRomSize)) {
						for (u32 i = 0; i < setDataBWlist[6]; i++) {
							cacheReadSizeSubtract++;
							if (((sector+setDataBWlist[6])-cacheReadSizeSubtract) == cleanRomSize) break;
						}
					}
					accessCounter++;
				}
			}
		}
	}
	return 0;
}




