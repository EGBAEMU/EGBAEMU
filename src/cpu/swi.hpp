#ifndef SWI_HPP
#define SWI_HPP

#include "decode/inst.hpp"
#include "util.hpp"

namespace gbaemu
{
    class CPU;

    namespace swi
    {
        typedef void SWIHandler(InstructionExecutionInfo &, CPU *);

        SWIHandler softReset;
        SWIHandler registerRamReset;
        SWIHandler halt;
        SWIHandler stop;
        SWIHandler intrWait;
        SWIHandler vBlankIntrWait;
        SWIHandler div;
        SWIHandler divArm;
        SWIHandler sqrt;
        SWIHandler arcTan;
        SWIHandler arcTan2;
        SWIHandler cpuSet;
        SWIHandler cpuFastSet;
        SWIHandler biosChecksum;
        SWIHandler bgAffineSet;
        SWIHandler objAffineSet;
        SWIHandler bitUnPack;
        SWIHandler LZ77UnCompWRAM;
        SWIHandler LZ77UnCompVRAM;
        SWIHandler huffUnComp;
        SWIHandler RLUnCompWRAM;
        SWIHandler RLUnCompVRAM;
        SWIHandler diff8BitUnFilterWRAM;
        SWIHandler diff8BitUnFilterVRAM;
        SWIHandler diff16BitUnFilter;
        SWIHandler soundBiasChange;
        SWIHandler soundDriverInit;
        SWIHandler soundDriverMode;
        SWIHandler soundDriverMain;
        SWIHandler soundDriverVSync;
        SWIHandler soundChannelClear;
        SWIHandler MIDIKey2Freq;
        SWIHandler musicPlayerOpen;
        SWIHandler musicPlayerStart;
        SWIHandler musicPlayerStop;
        SWIHandler musicPlayerContinue;
        SWIHandler musicPlayerFadeOut;
        SWIHandler multiBoot;
        SWIHandler hardReset;
        SWIHandler customHalt;
        SWIHandler soundDriverVSyncOff;
        SWIHandler soundDriverVSyncOn;
        SWIHandler getJumpList;

        //Custom SWI handler
        SWIHandler changeBIOSState;

        SWIHandler callBiosCodeSWIHandler;

        SWIHandler *const biosCallHandler[] = {
            softReset,
            registerRamReset,
            halt,
            stop,
            intrWait,
            vBlankIntrWait,
            div,
            divArm,
            sqrt,
            arcTan,
            arcTan2,
            cpuSet,
            cpuFastSet,
            biosChecksum,
            bgAffineSet,
            objAffineSet,
            bitUnPack,
            LZ77UnCompWRAM,
            LZ77UnCompVRAM,
            huffUnComp,
            RLUnCompWRAM,
            RLUnCompVRAM,
            diff8BitUnFilterWRAM,
            diff8BitUnFilterVRAM,
            diff16BitUnFilter,
            soundBiasChange,
            soundDriverInit,
            soundDriverMode,
            soundDriverMain,
            soundDriverVSync,
            soundChannelClear,
            MIDIKey2Freq,
            musicPlayerOpen,
            musicPlayerStart,
            musicPlayerStop,
            musicPlayerContinue,
            musicPlayerFadeOut,
            multiBoot,
            hardReset,
            customHalt,
            soundDriverVSyncOff,
            soundDriverVSyncOn,
            getJumpList,
            //Custom SWI handler
            changeBIOSState};

        const char *const biosCallHandlerStr[] = {
            STRINGIFY(softReset),
            STRINGIFY(registerRamReset),
            STRINGIFY(halt),
            STRINGIFY(stop),
            STRINGIFY(intrWait),
            STRINGIFY(vBlankIntrWait),
            STRINGIFY(div),
            STRINGIFY(divArm),
            STRINGIFY(sqrt),
            STRINGIFY(arcTan),
            STRINGIFY(arcTan2),
            STRINGIFY(cpuSet),
            STRINGIFY(cpuFastSet),
            STRINGIFY(biosChecksum),
            STRINGIFY(bgAffineSet),
            STRINGIFY(objAffineSet),
            STRINGIFY(bitUnPack),
            STRINGIFY(LZ77UnCompWRAM),
            STRINGIFY(LZ77UnCompVRAM),
            STRINGIFY(huffUnComp),
            STRINGIFY(RLUnCompWRAM),
            STRINGIFY(RLUnCompVRAM),
            STRINGIFY(diff8BitUnFilterWRAM),
            STRINGIFY(diff8BitUnFilterVRAM),
            STRINGIFY(diff16BitUnFilter),
            STRINGIFY(soundBiasChange),
            STRINGIFY(soundDriverInit),
            STRINGIFY(soundDriverMode),
            STRINGIFY(soundDriverMain),
            STRINGIFY(soundDriverVSync),
            STRINGIFY(soundChannelClear),
            STRINGIFY(MIDIKey2Freq),
            STRINGIFY(musicPlayerOpen),
            STRINGIFY(musicPlayerStart),
            STRINGIFY(musicPlayerStop),
            STRINGIFY(musicPlayerContinue),
            STRINGIFY(musicPlayerFadeOut),
            STRINGIFY(multiBoot),
            STRINGIFY(hardReset),
            STRINGIFY(customHalt),
            STRINGIFY(soundDriverVSyncOff),
            STRINGIFY(soundDriverVSyncOn),
            STRINGIFY(getJumpList),
            //Custom SWI handler: starts at 0x2B
            STRINGIFY(changeBIOSState)};

        const char *swiToString(uint8_t index);
    } // namespace swi
} // namespace gbaemu

#endif