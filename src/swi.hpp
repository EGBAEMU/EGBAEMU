#ifndef SWI_HPP
#define SWI_HPP

#include "cpu_state.hpp"

namespace gbaemu
{
    namespace swi
    {

        typedef void SWIHandler(CPUState *);

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

        const SWIHandler *const biosCallHandler[] = {
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
            getJumpList};
    } // namespace swi
} // namespace gbaemu

#endif