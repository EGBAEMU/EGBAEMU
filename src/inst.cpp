#include "inst.hpp"

namespace gbaemu
{
    uint32_t arm::shift(uint32_t value, ShiftType type, uint32_t amount) {
        switch (type) {
            case LSL:
                return value << amount;
            case LSR:
                return value >> amount;
            case ASR:
                return static_cast<uint32_t>(static_cast<int32_t>(value) >> amount);
            case ROR:
                return (value >> amount) | (value << (31 - amount));
            default:
                std::cout << "WARNING: Unknown shift type!";
                return value;
        }
    }

    void Instruction::setArmInstruction(arm::ARMInstruction &armInstruction)
    {
        arm = armInstruction;
        isArm = true;
    }
    void Instruction::setThumbInstruction(thumb::ThumbInstruction &thumbInstruction)
    {
        thumb = thumbInstruction;
        isArm = false;
    }

    bool Instruction::isArmInstruction() const
    {
        return isArm;
    }

    Instruction Instruction::fromARM(arm::ARMInstruction &armInst)
    {
        Instruction result;
        result.arm = armInst;
        result.isArm = true;
        return result;
    }

    Instruction Instruction::fromThumb(thumb::ThumbInstruction &thumbInst)
    {
        Instruction result;
        result.thumb = thumbInst;
        result.isArm = false;
        return result;
    }
} // namespace gbaemu
