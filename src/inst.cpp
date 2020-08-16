#include "inst.hpp"

namespace gbaemu {
    void Instruction::setArmInstruction(ARMInstruction &armInstruction) {
        arm = armInstruction;
        isArm = true;
    }
    void Instruction::setThumbInstruction(ThumbInstruction& thumbInstruction) {
        thumb = thumbInstruction;
        isArm = false;
    }

    bool Instruction::isArmInstruction() const {
        return isArm;
    }

    Instruction Instruction::fromARM(ARMInstruction& armInst) {
        Instruction result;
        result.arm = armInst;
        result.isArm = true;
        return result;
    }

    Instruction Instruction::fromThumb(ThumbInstruction& thumbInst) {
        Instruction result;
        result.thumb = thumbInst;
        result.isArm = false;
        return result;
    }
}
