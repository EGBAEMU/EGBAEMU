#include "square.hpp"
#include "cpu/cpu.hpp"
#include "orchestrator.hpp"
#include "util.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>

namespace gbaemu::sound
{

    SquareWaveChannel::SquareWaveChannel(CPU *cpu, SoundOrchestrator *orchestrator, SoundChannel channel) : orchestrator(orchestrator), channel(channel)
    {
        cpu->state.memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                SOUND_CONTROL_REG_ADDR + sizeof(regs) * channel,
                SOUND_CONTROL_REG_ADDR + sizeof(regs) * channel + sizeof(regs) - 1,
                std::bind(&SquareWaveChannel::read8FromReg, this, std::placeholders::_1),
                std::bind(&SquareWaveChannel::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&SquareWaveChannel::read8FromReg, this, std::placeholders::_1),
                std::bind(&SquareWaveChannel::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
        reset();
    }

    void SquareWaveChannel::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);

        active = false;
        timer = 0;
        sequenceIdx = 0;

        env_active = false;
        env_value = 0;
        env_counter = 0;

        timed_active = false;
        timed_counter = 0;

        sweep_active = false;
        sweep_current = 0;
        sweep_counter = 0;
        sweep_shadow = 0;

        reg_soundLength = 0;
        reg_dutyCycle = 0;
        reg_envStepTime = 0;
        reg_envMode = false;
        reg_envInitVal = 0;
        reg_frequency = 0;
        reg_timed = false;
        reg_reset = false;
        reg_sweepShifts = 0;
        reg_sweepDirection = false;
        reg_sweepTime = 0;

    }

    uint16_t SquareWaveChannel::getCurrentVolume()
    {
        return volumeOut;
    }

    uint8_t SquareWaveChannel::read8FromReg(uint32_t offset) const
    {
        if (channel == CHAN_2)
            offset += sizeof(regs.soundCntL);

        return *(offset + reinterpret_cast<const uint8_t *>(&regs));
    }

    void SquareWaveChannel::write8ToReg(uint32_t offset, uint8_t value)
    {
        if (channel == CHAN_2) {
            // patch offsets
            switch (offset) {
                case 2:
                case 3:
                    offset = offsetof(SquareWaveRegs, _unused);
                    break;
                case 0:
                case 1:
                    offset += sizeof(regs.soundCntL);
                    break;

                default:
                    break;
            }
        }

        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        switch (offset) {
            // case offsetof(SquareWaveRegs, soundCntL) + 1:
            case offsetof(SquareWaveRegs, soundCntL): {
                const uint16_t regCntL = le(regs.soundCntL);
                // Extract the register values of reg L (Channel 2 does not support sweeps)
                reg_sweepShifts = bitGet(regCntL, SOUND_SQUARE_CHANNEL_L_SHIFTS_MASK, SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF);
                reg_sweepDirection = isBitSet<uint16_t, SOUND_SQUARE_CHANNEL_L_DIR_OFF>(regCntL);
                reg_sweepTime = bitGet(regCntL, SOUND_SQUARE_CHANNEL_L_TIME_MASK, SOUND_SQUARE_CHANNEL_L_TIME_OFF);
                break;
            }
            case offsetof(SquareWaveRegs, soundCntH_L) + 1:
            case offsetof(SquareWaveRegs, soundCntH_L): {
                const uint16_t regCntH_L = le(regs.soundCntH_L);
                // Extract the register values of reg H (L for channel 2)
                reg_soundLength = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK, SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF);
                reg_dutyCycle = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK, SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF);
                reg_envStepTime = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK, SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF);
                reg_envMode = isBitSet<uint16_t, SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF>(regCntH_L);
                reg_envInitVal = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK, SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF);
                
                std::cout << "Adjusted reg h/l is now " << std::hex << regCntH_L << " Sound length is " << std::hex << reg_soundLength << std::endl;
                break;
            }
            case offsetof(SquareWaveRegs, soundCntX_H) + 1:
            case offsetof(SquareWaveRegs, soundCntX_H): {
                const uint16_t regCntX_H = le(regs.soundCntX_H);
                // Extract the register values of reg X (H for channel 2)
                reg_frequency = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK, SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF);
                reg_timed = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK, SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF);
                reg_reset = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_RESET_MASK, SOUND_SQUARE_CHANNEL_X_RESET_OFF);
                break;
            }
        }

        onRegisterUpdated();
    }

    void SquareWaveChannel::onRegisterUpdated()
    { 
        // Only apply changes if required!
        if(!reg_reset)
            return;

        regs.soundCntX_H = le(le(regs.soundCntX_H) & ~SOUND_SQUARE_CHANNEL_X_RESET_MASK);

        /*

            Writing a value to NRx4 with bit 7 set causes the following things to occur:

                Channel is enabled (see length counter).
                If length counter is zero, it is set to 64.
                Frequency timer is reloaded with period.
                Volume envelope timer is reloaded with period.
                Channel volume is reloaded from NRx2.
                Square 1's sweep does several things (see frequency sweep).

            Note that if the channel's DAC is off, after the above actions occur the channel 
            will be immediately disabled again. 

        */

        active = true;
        timer = (2048 - reg_frequency) * 4;

        env_active = true;
        env_counter = reg_envStepTime;    
        env_value = reg_envInitVal;

        timed_active = reg_timed;
        timed_counter = reg_soundLength;
        
        LOG_SOUND(
            std::cout << "SOUND: Channel " << channel << " reset! " << std::endl;
            std::cout << "       Env active " << env_active << std::endl;
            std::cout << "       Env initial value " << env_value << std::endl;
            std::cout << "       Env counter " << env_counter << std::endl;
            std::cout << "       Timer " << timer << std::endl;
            std::cout << "       Timed " << timed_active << std::endl;
            std::cout << "       Timed counter " << static_cast<uint32_t>(reg_soundLength) << std::endl;
        );

        if (channel == CHAN_2) 
            return;

        /*
            During a trigger event, several things occur:

            - Square 1's frequency is copied to the shadow register.
            - The sweep timer is reloaded.
            - The internal enabled flag is set if either the sweep period or shift are 
              non-zero, cleared otherwise.
            - If the sweep shift is non-zero, frequency calculation and the overflow 
              check are performed immediately.

        */

        sweep_shadow = reg_frequency;
        sweep_counter = reg_sweepTime;
        sweep_active = (reg_sweepTime != 0) || (reg_sweepShifts != 0);
        
        if (reg_sweepShifts != 0)
            onCalculateFrequency(false);

    }

    void SquareWaveChannel::onCalculateFrequency(bool writeback)
    {
        /*
            Frequency calculation consists of taking the value in the frequency shadow register, 
            shifting it right by sweep shift, optionally negating the value, and summing this 
            with the frequency shadow register to produce a new frequency. What is done with this 
            new frequency depends on the context. 
        */

        uint16_t offset = sweep_shadow >> reg_sweepShifts;
        uint16_t adjusted = 0;

        // 1=Subtraction (frequency decreases) 
        if (reg_sweepDirection)
            adjusted = sweep_shadow - offset;
        else
            adjusted = sweep_shadow + offset;

        /*
            The overflow check simply calculates the new frequency and if this is greater than 
            2047, square 1 is disabled. 
        */
        if (adjusted > 2047)
            active = false;
        else {

            /*  
                If the new frequency is 2047 or less and the sweep shift is not zero, this new frequency 
                is written back to the shadow frequency and square 1's frequency in NR13 and NR14, then 
                frequency calculation and overflow check are run AGAIN immediately using this new value, 
                but this second new frequency is not written back. 

                Square 1's frequency can be modified via NR13 and NR14 while sweep is active, but the shadow 
                frequency won't be affected so the next time the sweep updates the channel's frequency this 
                modification will be lost. 
            */

            if(!writeback || (reg_sweepShifts == 0))
                return;

            sweep_shadow = adjusted;
            reg_frequency = adjusted;
            // TODO: Write back to reg

            onCalculateFrequency(false);

        }
        
    }       
    
    void SquareWaveChannel::onStepVolume()
    {
        timer -= 1;

        if (timer == 0) {

            timer = (2048 - reg_frequency) * 4;
            sequenceIdx = (sequenceIdx + 1);

            if (sequenceIdx == 8)
                sequenceIdx = 0;
        }

        /*
            When a channel is disabled, its volume unit receives 0, otherwise its volume unit 
            receives the output of the waveform generator. Other units besides the length 
            counter can enable/disable the channel as well. 
        */

        if (active)
            volumeOut = env_value;
        else
            volumeOut = 0;

        if (!((DUTY_CYCLE_LOOKUP[reg_dutyCycle] >> sequenceIdx) & 0x1))
            volumeOut = 0;
   
    }

    void SquareWaveChannel::onStepEnv()
    {

        if (!env_active)
            return;

        env_counter -= 1;

        if (env_counter != 0)
            return;

        LOG_SOUND(
            std::cout << "SOUND: Channel " << static_cast<uint32_t>(channel) << " stepping env!" << std::endl;
            std::cout << "       Active: " << static_cast<uint32_t>(env_active) << std::endl;
        );

        /*
            The volume envelope and sweep timers treat a period of 0 as 8.
        */
        if (reg_envStepTime == 0)
            env_counter = 8;
        else
            env_counter = reg_envStepTime;

        if (env_active && reg_envStepTime > 0) {
                
            LOG_SOUND(
                std::cout << "       Value was " << static_cast<uint32_t>(env_value) << std::endl;
            );

            if (reg_envMode) {
                if (env_value < 15)
                    env_value += 1;
            } else {
                if (0 < env_value)
                    env_value -= 1;
            }

            LOG_SOUND(
                std::cout << "       Value is now " << static_cast<uint32_t>(env_value) << std::endl;
            );

        }

        /*
            If this new volume within the 0 to 15 range, the volume is updated, otherwise it
            is left unchanged and no further automatic increments/decrements are made to the 
            volume until the channel is triggered again. 
        */
        if ((env_value == 15) || (env_value == 0))
            env_active = false;
    }

    void SquareWaveChannel::onStepSoundLength()
    {
        /*
            When clocked while enabled by NRx4 and the counter is not zero, it is decremented. 
            If it becomes zero, the channel is disabled. 
        */

        if (!timed_active || (timed_counter == 0))
            return;

        timed_counter -= 1;
        
        // The counter has reached 0. Disable this channel!
        if (timed_counter != 0)
            return;

        active = false;

        LOG_SOUND(
            std::cout << "SOUND: Channel " << static_cast<uint32_t>(channel) << " sound length expired!" << std::endl;
        );
    }

    void SquareWaveChannel::onStepSweep()
    {

        if (!sweep_active)
            return;

        sweep_counter -= 1;
        
        if (sweep_counter != 0)
            return;
        
        /*
            The volume envelope and sweep timers treat a period of 0 as 8.

            000: Sweep function is off
        */
        if (reg_sweepTime == 0)
            sweep_counter = 8;
        else {
            sweep_counter = reg_sweepTime;

            if (sweep_active)
                onCalculateFrequency(true);

        }
        
    }

} // namespace gbaemu
